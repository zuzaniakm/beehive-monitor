#pragma once
#include "Wifi.h"
#include "Database.h"
#include "Config.h"
#include "EmailAlert.h"
#include <DHT.h>
#include <HX711.h>
#include <BluetoothSerial.h>


class BeehiveMonitor {
private:
  Wifi* _wifi;
  Database _database;
  DHT _dht;
  HX711 _scale;
  EmailAlert _emailAlert;

  unsigned _ID;
  bool _setup;
  float _lastTemperature;
  float _lastHumidity;
  float _lastWeight;

public:
  BeehiveMonitor(unsigned long channelId, const char* apiWriteKey);
  ~BeehiveMonitor();
  void setup();
  bool connectToWiFi(const char* ssid, const char* password);
  void readData();
  void sendDataToDB(float temperature, float humidity, float weight);

private:
  void setID(unsigned ID);
  void processWiFiCommand(BluetoothSerial& serialBT);
  void processIDCommand(BluetoothSerial& serialBT);
  void processReadyCommand(BluetoothSerial& serialBT);
  void processCalibrateCommand(BluetoothSerial& serialBT);
  void processEmailCommand(BluetoothSerial& serialBT);
  void printReadings(float temperature, float humidity, float weight);
  void checkReadings(float temperature, float humidity, float weight);
};

BeehiveMonitor::BeehiveMonitor(unsigned long channelId, const char* apiWriteKey)
  : _ID(0), _wifi(nullptr), _database(channelId, apiWriteKey), _dht(DHT_PIN, DHT22), _scale(),
    _emailAlert(SENDER_EMAIL_LOGIN, SENDER_EMAIL_PASSWORD, SENDER_EMAIL_LOGIN, SENDER_EMAIL_NAME), _setup(false) {
  _dht.begin();
  _scale.begin(HX711_DATA_PIN, HX711_CLOCK_PIN);
  _scale.set_offset(DEFAULT_SCALE_OFFSET);
  _scale.set_scale(DEFAULT_SCALE_CALIBRATION);
}

BeehiveMonitor::~BeehiveMonitor() {
  if (_wifi) {
    delete _wifi;
  }
}
void BeehiveMonitor::setup() {
  BluetoothSerial serialBT;
  serialBT.begin("Beehive Monitor");

  while (!_setup) {
    if (serialBT.available()) {
      String message = serialBT.readStringUntil(' ');
      if (message == "wifi") {
        processWiFiCommand(serialBT);
      } else if (message == "id") {
        processIDCommand(serialBT);
      } else if (message == "ready\n") {
        processReadyCommand(serialBT);
      } else if (message == "tare\n") {
        _scale.tare(10);
        serialBT.println(_scale.get_offset());
      } else if (message == "calibrate") {
        processCalibrateCommand(serialBT);
      } else if (message == "email") {
        processEmailCommand(serialBT);
      } else {
        serialBT.println("Unrecognized command!");
      }
    }
    delay(100);
  }
  _emailAlert.alertHumidityOutOfBounds(_ID, 99.9);
}

bool BeehiveMonitor::connectToWiFi(const char* ssid, const char* password) {
  if (_wifi) {
    _wifi->disconnect();
    delete _wifi;
    _wifi = nullptr;
  }
  _wifi = new Wifi(ssid, password);
  return _wifi->connect();
}

void BeehiveMonitor::readData() {
  float humidity = _dht.readHumidity();
  float temperature = _dht.readTemperature();
  float weight = round(_scale.get_units(10) / 100) / 10;
  weight = (weight < 0) ? 0 : weight;

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor!");
  } else {
    printReadings(temperature, humidity, weight);
    checkReadings(temperature, humidity, weight);
    sendDataToDB(temperature, humidity, weight);
  }
}

void BeehiveMonitor::sendDataToDB(float temperature, float humidity, float weight) {
  _database.prepareField(1 + _ID * 3, temperature);
  _database.prepareField(2 + _ID * 3, humidity);
  _database.prepareField(3 + _ID * 3, weight);
  _database.sendData();
}

void BeehiveMonitor::setID(unsigned ID) {
  _ID = ID % 3;
}

void BeehiveMonitor::processWiFiCommand(BluetoothSerial& serialBT) {
  if (connectToWiFi(serialBT.readStringUntil(' ').c_str(), serialBT.readStringUntil('\n').c_str())) {
    serialBT.println("WiFi connected!");
  } else {
    serialBT.println("WiFi failed to connect!");
  }
}

void BeehiveMonitor::processIDCommand(BluetoothSerial& serialBT) {
  setID(serialBT.readStringUntil('\n').toInt());
  serialBT.print("ID changed to ");
  serialBT.println(_ID);
}

void BeehiveMonitor::processReadyCommand(BluetoothSerial& serialBT) {
  if (_wifi && _wifi->getStatus() == WL_CONNECTED) {
    if (_emailAlert.isEmailSet()) {
      serialBT.println("Setup complete!");
      delay(100);
      _setup = true;
      serialBT.end();
    } else {
      serialBT.println("Email address is not set!");
    }
  } else {
    serialBT.println("WiFi connection is not set!");
  }
}

void BeehiveMonitor::processCalibrateCommand(BluetoothSerial& serialBT) {
  int units = serialBT.readStringUntil('\n').toInt();
  serialBT.print(units);
  if (units < 1000) {
    serialBT.println("Wrong value!");
    serialBT.println("Input value higher than 1000 (e.g. calibrate 1500)!");
  } else {
    _scale.calibrate_scale(units, 20);
  }
}

void BeehiveMonitor::processEmailCommand(BluetoothSerial& serialBT) {
  String email = serialBT.readStringUntil('\n');
  int length = email.length();

  if (length >= 6 && (email.charAt(length - 3) == '.' || email.charAt(length - 4) == '.')) {
    if (email.indexOf('@') != -1) {
      _emailAlert.setEmailRecipient(email);
      serialBT.print("Email address changed to ");
      serialBT.println(email);
      return;
    }
  }
  serialBT.println("Invalid email address!");
}

void BeehiveMonitor::printReadings(float temperature, float humidity, float weight) {
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.print("%");

  Serial.print("  |  ");

  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print("°C");

  Serial.print("  |  ");

  Serial.print("Weight: ");
  Serial.print(weight);
  Serial.println("KG");
}

void BeehiveMonitor::checkReadings(float temperature, float humidity, float weight) {
  if (temperature < MIN_TEMPERATURE || temperature > MAX_TEMPERATURE) {
    _emailAlert.alertTemperatureOutOfBounds(_ID, temperature);
  } else if (abs(temperature - _lastTemperature) > ALLOWED_TEMP_CHANGE && _lastTemperature != 0) {
    _emailAlert.alertTemperatureChange(_ID, temperature, _lastTemperature);
  }

  if (humidity < MIN_HUMIDITY || humidity > MAX_HUMIDITY) {
    _emailAlert.alertHumidityOutOfBounds(_ID, humidity);
  } else if (abs(humidity - _lastHumidity) > ALLOWED_HUM_CHANGE && _lastHumidity != 0) {
    _emailAlert.alertHumidityChange(_ID, humidity, _lastHumidity);
  }

  if (abs(weight - _lastWeight) > ALLOWED_WEIGHT_CHANGE && _lastWeight != 0) {
    _emailAlert.alertWeightChange(_ID, weight, _lastWeight);
  }
}