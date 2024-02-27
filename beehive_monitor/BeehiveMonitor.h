#pragma once
#include "Wifi.h"
#include "Database.h"
#include <DHT.h>
#include <HX711.h>
#include <BluetoothSerial.h>

class BeehiveMonitor {
private:
  unsigned _ID;
  Wifi* _wifi;
  Database _database;
  DHT _dht;
  HX711 _scale;
  bool _setup;

public:
  BeehiveMonitor(unsigned long channelId, const char* apiWriteKey);
  ~BeehiveMonitor();
  void setup();
  void processWiFiCommand(BluetoothSerial& serialBT);
  void processIDCommand(BluetoothSerial& serialBT);
  void processReadyCommand(BluetoothSerial& serialBT);
  void processCalibrateCommand(BluetoothSerial& serialBT);
  void setID(unsigned ID);
  bool connectToWiFi(const char* ssid, const char* password);
  void readData();
  void sendDataToDB(float temperature, float humidity, float weight);
};

BeehiveMonitor::BeehiveMonitor(unsigned long channelId, const char* apiWriteKey)
  : _ID(0), _wifi(nullptr), _database(channelId, apiWriteKey), _dht(5, DHT22), _scale(), _setup(false) {
  _dht.begin();
  _scale.begin(19, 18);
  _scale.set_offset(10000);
  _scale.set_scale(21);
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
      } else {
        serialBT.println("Unrecognized command!");
      }
    }
    delay(100);
  }
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
    serialBT.println("Setup complete!");
    delay(100);
    _setup = true;
    serialBT.end();
  } else {
    serialBT.println("Setup is not complete!");
  }
}

void BeehiveMonitor::processCalibrateCommand(BluetoothSerial& serialBT) {
  int units = serialBT.readStringUntil('\n').toInt();
  serialBT.print(units);
  if (units < 1000) {
    serialBT.println("Wrong value!");
    serialBT.println("Input value higher than 1000 (e.g. calibrate 1500)!");
  } else {
    _scale.calibrate_scale(units, 25);
  }
}

void BeehiveMonitor::setID(unsigned ID) {
  _ID = ID % 3;
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
  float weight = _scale.get_units(10) / 1000;

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor!");
  } else {
    if (weight < 0.05) {
      weight = 0.0;
    }
    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.print("%");

    Serial.print("  |  ");

    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println("°C");

     Serial.print("  |  ");

    Serial.print("Weight: ");
    Serial.print(weight);
    Serial.println("KG");


    sendDataToDB(temperature, humidity, weight);
  }
}

void BeehiveMonitor::sendDataToDB(float temperature, float humidity, float weight) {
  _database.prepareField(1 + _ID * 3, temperature);
  _database.prepareField(2 + _ID * 3, humidity);
  _database.prepareField(3 + _ID * 3, weight);
  _database.sendData();
}