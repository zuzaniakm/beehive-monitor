#pragma once
#include "Wifi.h"
#include "Database.h"
#include "Config.h"
#include "EmailAlert.h"
#include "SDCardController.h"
#include <DHT.h>
#include <HX711.h>
#include <BluetoothSerial.h>
#include <Preferences.h>
#include <time.h>


class BeehiveMonitor {
  private:
    Wifi _wifi;
    Database _database;
    DHT _dht;
    HX711 _scale;
    EmailAlert _emailAlert;
    Preferences _settings;
    SDCardController _SD;

    unsigned _boot;
    unsigned _ID;
    unsigned _delay;
    bool _setup;
    float _lastTemperature;
    float _lastHumidity;
    float _lastWeight;

  public:
    BeehiveMonitor(unsigned long channelId, const char* apiWriteKey);
    unsigned getDelay();
    void setup();
    bool readSettings();
    bool connectToWiFi(String ssid, String password);
    void readData();

  private:
    void setID(unsigned ID);
    void setDelay(unsigned delay);
    void processWiFiCommand(BluetoothSerial& serialBT);
    void processIDCommand(BluetoothSerial& serialBT);
    void processReadyCommand(BluetoothSerial& serialBT);
    void processTareCommand(BluetoothSerial& serialBT);
    void processOffsetCommand(BluetoothSerial& serialBT);
    void processCalibrateCommand(BluetoothSerial& serialBT);
    void processEmailCommand(BluetoothSerial& serialBT);
    void processDelayCommand(BluetoothSerial& serialBT);
    void processClearCommand(BluetoothSerial& serialBT);
    void processScaleCommand(BluetoothSerial& serialBT);
    void printReadings(float temperature, float humidity, float weight);
    void checkReadings(float temperature, float humidity, float weight);
    void sendDataToDB(float temperature, float humidity, float weight);
    void saveDataToSD(float temperature, float humidity, float weight);
    void syncTime();
};

BeehiveMonitor::BeehiveMonitor(unsigned long channelId, const char* apiWriteKey)
  : _ID(0), _delay(SCAN_DELAY), _database(channelId, apiWriteKey), _dht(DHT_PIN, DHT11), _scale(),
    _emailAlert(SENDER_EMAIL_LOGIN, SENDER_EMAIL_PASSWORD, SENDER_EMAIL_LOGIN, SENDER_EMAIL_NAME), _setup(false) {
  _dht.begin();
  _scale.begin(HX711_DATA_PIN, HX711_CLOCK_PIN);
  _scale.set_offset(DEFAULT_SCALE_OFFSET);
  _scale.set_scale(DEFAULT_SCALE_CALIBRATION);
}

unsigned BeehiveMonitor::getDelay() {
  return _delay;
}

void BeehiveMonitor::setDelay(unsigned delay) {
  _delay = delay;
}

void BeehiveMonitor::setID(unsigned ID) {
  _ID = ID % 3;
}

void BeehiveMonitor::setup() {
  BluetoothSerial serialBT;
  serialBT.begin("Beehive Monitor");
  _settings.begin("settings", false);
  unsigned long startTime = millis();

  while (!_setup) {
    if (millis() - startTime >= 120000) {
      break;
    }

    if (serialBT.available()) {
      startTime = millis();
      String message = serialBT.readStringUntil(' ');
      if (message == "wifi") {
        processWiFiCommand(serialBT);
      } else if (message == "id") {
        processIDCommand(serialBT);
      } else if (message == "tare\n") {
        processTareCommand(serialBT);
      } else if (message == "offset") {
        processOffsetCommand(serialBT);
      } else if (message == "calibrate") {
        processCalibrateCommand(serialBT);
      } else if (message == "email") {
        processEmailCommand(serialBT);
      } else if (message == "delay") {
        processDelayCommand(serialBT);
      } else if (message == "clear\n") {
        processClearCommand(serialBT);
      } else if (message == "ready\n") {
        processReadyCommand(serialBT);
      } else if (message == "scale") {
        processScaleCommand(serialBT);
      } else {
        serialBT.println("Unrecognized command!");
      }
    }
    delay(100);
  }

  Serial.flush();
  delay(250);
  _wifi.turnOff();
  serialBT.end();
  _settings.end();
  delay(250);

  if (!SD.begin()) {
    Serial.println("Card Mount Failed");
    return;
  }

  if (!SD.exists("/data.csv")) {
    _SD.writeFile(SD, "/data.csv", "created_at,field1,field2,field3\n");
  }
}

bool BeehiveMonitor::readSettings() {
  _settings.begin("settings", true);
  delay(250);

  String ssid = _settings.getString("ssid", "");
  String password = _settings.getString("password", "");
  String email = _settings.getString("email", "");
  _ID = _settings.getUInt("id", 0);
  _delay = _settings.getUInt("delay", SCAN_DELAY);

  _lastTemperature = _settings.getFloat("lastTemperature", 0);
  _lastHumidity = _settings.getFloat("lastHumidity", 0);
  _lastWeight = _settings.getFloat("lastWeight", 0);

  _scale.set_offset(_settings.getLong("offset", DEFAULT_SCALE_OFFSET));
  _scale.set_scale(_settings.getFloat("calibration", DEFAULT_SCALE_CALIBRATION));
  _settings.end();

  if (ssid == "" || email == "") {
    Serial.println("Missing SSID or email address!");
    return false;
  } else {
    _wifi.setCredentials(ssid, password);
    _emailAlert.setEmailRecipient(email);
    return true;
  }
}

bool BeehiveMonitor::connectToWiFi(String ssid, String password) {
  _wifi.setCredentials(ssid, password);
  return _wifi.connect();
}

void BeehiveMonitor::processWiFiCommand(BluetoothSerial& serialBT) {
  String ssid = serialBT.readStringUntil(' ');
  String password = serialBT.readStringUntil('\n');
  if (connectToWiFi(ssid, password)) {
    _settings.putString("ssid", ssid);
    _settings.putString("password", password);
    serialBT.println("WiFi connected!");
  } else {
    serialBT.println("WiFi failed to connect!");
  }
}

void BeehiveMonitor::processIDCommand(BluetoothSerial& serialBT) {
  setID(serialBT.readStringUntil('\n').toInt());
  serialBT.print("ID changed to ");
  serialBT.println(_ID);
  _settings.putUInt("id", _ID);
}

void BeehiveMonitor::processReadyCommand(BluetoothSerial& serialBT) {
  if (_wifi.getStatus() == WL_CONNECTED) {
    if (_emailAlert.isEmailSet()) {
      serialBT.println("Setup complete!");
      _setup = true;
    } else {
      serialBT.println("Email address is not set!");
    }
  } else {
    serialBT.println("WiFi connection is not set!");
  }
}

void BeehiveMonitor::processTareCommand(BluetoothSerial& serialBT) {
  _scale.tare(10);
  Serial.println("Tare complete");
  long offset = _scale.get_offset();
  Serial.println(offset);
  _settings.putLong("offset", offset);
  Serial.println("Offset written");
  serialBT.println("Offset: " + String(offset));
}

void BeehiveMonitor::processOffsetCommand(BluetoothSerial& serialBT) {
  long offset = serialBT.readStringUntil('\n').toInt();
  _scale.set_offset(offset);
  _settings.putLong("offset", offset);
}

void BeehiveMonitor::processCalibrateCommand(BluetoothSerial& serialBT) {
  long units = serialBT.readStringUntil('\n').toInt();
  if (units < 1000) {
    serialBT.println("Wrong value!");
    serialBT.println("Input value higher than 1000 (e.g. calibrate 1500)!");
  } else {
    _scale.calibrate_scale(units, 20);
    serialBT.println("Calibration factor set to " + String(_scale.get_scale()));
    _settings.putFloat("calibration", _scale.get_scale());
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
      _settings.putString("email", email);
      return;
    }
  }
  serialBT.println("Invalid email address!");
}

void BeehiveMonitor::processDelayCommand(BluetoothSerial& serialBT) {
  int delay = serialBT.readStringUntil('\n').toInt();
  if (delay >= 15) {
    setDelay(delay);
    serialBT.print("Delay changed to ");
    serialBT.print(_delay);
    serialBT.println(" min");
    _settings.putUInt("delay", _delay);
  } else {
    serialBT.println("Minimal delay is 15 minutes!");
  }
}

void BeehiveMonitor::processClearCommand(BluetoothSerial& serialBT) {
  _wifi.disconnect();
  _emailAlert.resetEmail();
  _settings.clear();
}

void BeehiveMonitor::processScaleCommand(BluetoothSerial& serialBT) {
  float calibration = serialBT.readStringUntil('\n').toFloat();
  if (calibration != 0) {
    _scale.set_scale(calibration);
    _settings.putFloat("calibration", calibration);
    serialBT.println("Calibration factor set to " + String(calibration));
  } else {
    serialBT.println("Wrong calibration factor!");
  }
}

void BeehiveMonitor::readData() {
  float temperature = _dht.readTemperature();
  float humidity = _dht.readHumidity();

  _scale.power_up();
  float weight = round(_scale.get_units(25) / 100.0) / 10;
  _scale.power_down();
  weight = (weight < 0) ? 0 : weight;

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor!");
  } else {
    printReadings(temperature, humidity, weight);
    checkReadings(temperature, humidity, weight);
    sendDataToDB(temperature, humidity, weight);
    saveDataToSD(temperature, humidity, weight);
    _wifi.turnOff();
  }
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
  unsigned alertsCount = 0;

  //Temperature check
  if (temperature < MIN_TEMPERATURE || temperature > MAX_TEMPERATURE) {
    ++alertsCount;
    _emailAlert.alertTemperatureOutOfBounds(_ID, temperature);
  } else if (abs(temperature - _lastTemperature) > ALLOWED_TEMP_CHANGE && _lastTemperature != 0) {
    ++alertsCount;
    _emailAlert.alertTemperatureChange(_ID, temperature, _lastTemperature);
  }

  //Humidity check
  if (humidity < MIN_HUMIDITY || humidity > MAX_HUMIDITY) {
    ++alertsCount;
    _emailAlert.alertHumidityOutOfBounds(_ID, humidity);
  } else if (abs(humidity - _lastHumidity) > ALLOWED_HUM_CHANGE && _lastHumidity != 0) {
    ++alertsCount;
    _emailAlert.alertHumidityChange(_ID, humidity, _lastHumidity);
  }

  //Weigh check
  if (abs(weight - _lastWeight) > ALLOWED_WEIGHT_CHANGE && _lastWeight != 0) {
    ++alertsCount;
    _emailAlert.alertWeightChange(_ID, weight, _lastWeight);
  }

  if (alertsCount > 0) {
    if (alertsCount > 1) {
      _emailAlert.alertMultiple(_ID, temperature, _lastTemperature, humidity, _lastHumidity, weight, _lastWeight);
    }
    if (_wifi.checkConnection()) {
      _emailAlert.sendAlert();
    }
  }

  _settings.begin("settings", false);
  delay(50);
  _settings.putFloat("lastTemperature", temperature);
  _settings.putFloat("lastHumidity", humidity);
  _settings.putFloat("lastWeight", weight);
  _settings.end();
}

void BeehiveMonitor::sendDataToDB(float temperature, float humidity, float weight) {
  if (_wifi.checkConnection()) {
    _database.prepareField(1 + _ID * 3, temperature);
    _database.prepareField(2 + _ID * 3, humidity);
    _database.prepareField(3 + _ID * 3, weight);
    _database.sendData();
  }
}

void BeehiveMonitor::saveDataToSD(float temperature, float humidity, float weight) {
  if (!SD.begin()) {
    Serial.println("Card Mount Failed");
    return;
  } else {
    syncTime();
    time_t timeStamp = time(nullptr);
    String text = String(timeStamp) + ",";
    text += temperature;
    text += ",";
    text += humidity;
    text += ",";
    text += weight;
    text += "\n";

    Serial.println(text);
    _SD.appendFile(SD, "/data.csv", text.c_str());
    delay(10);
    SD.end();
  }
}

void BeehiveMonitor::syncTime() {
  if (_wifi.getStatus() == WL_CONNECTED && time(nullptr) < 17000000) {
    configTime(TIMEZONE * 3600, DAYSAVETIME * 3600, "time.nist.gov", "0.pool.ntp.org", "1.pool.ntp.org");
    struct tm tmstruct;
    getLocalTime(&tmstruct, 5000);
  }
}