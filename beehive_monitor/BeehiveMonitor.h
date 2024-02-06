#pragma once
#include "Wifi.h"
#include "Database.h"
#include <DHT.h>
#include <BluetoothSerial.h>
#include <String.h>

class BeehiveMonitor
{
  private:
    unsigned _ID = 0;
    Wifi* _wifi;
    Database _database;
    DHT _dht;
    bool _setup;
  public:
    BeehiveMonitor(unsigned long channelId, const char* apiWriteKey);
    void setup();
    void setID(unsigned ID);
    bool connectToWiFi(const char* ssid, const char* password);
    void readData();
    void sendDataToDB(float temperature, float humidity, float weight);
};

BeehiveMonitor::BeehiveMonitor(unsigned long channelId, const char* apiWriteKey) : _database(channelId, apiWriteKey), _dht(5, DHT22), _setup(false)
{
  _dht.begin();
}

void BeehiveMonitor::setup()
{
  BluetoothSerial serialBT;
  serialBT.begin("Beehive Monitor"); 
  while (!_setup)
  {
    if (serialBT.available()) 
    {
      String message = serialBT.readStringUntil(' ');
      if (message == "wifi")
      {
        if (connectToWiFi(serialBT.readStringUntil(' ').c_str(), serialBT.readStringUntil('\n').c_str()))
        {
          serialBT.println("WiFi connected!");
        }
        else
        {
          serialBT.println("WiFi failed to connect!");
        }
      }
      else if (message == "id")
      { 
        setID(serialBT.readStringUntil('\n').toInt());
        serialBT.print("ID changed to ");
        serialBT.println(_ID);
      }
      else if (message == "ok\n")
      { 
        if (_wifi->getStatus() == WL_CONNECTED)
        {
          serialBT.println("Setup complete!");
          delay(100);
          _setup = true;
          serialBT.end();
        }
        else 
        {
          serialBT.println("Setup is not complete!");
        }
      }
      else 
      {
        serialBT.println("Unrecognized command!");
      }
    }
    delay(100);
  }
}

void BeehiveMonitor::setID(unsigned ID)
{
  _ID = ID % 3;
}

bool BeehiveMonitor::connectToWiFi(const char* ssid, const char* password)
{ 
  _wifi->disconnect();
  delete _wifi;
  _wifi = nullptr;
  _wifi = new Wifi(ssid, password);
  return _wifi->connect();
}

void BeehiveMonitor::readData()
{
  float humidity  = _dht.readHumidity();
  float temperature = _dht.readTemperature();

  if ( isnan(temperature) || isnan(humidity)) 
  {
    Serial.println("Failed to read from DHT sensor!");
  } 
  else 
  {
    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.print("%");

    Serial.print("  |  ");

    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println("°C");

    sendDataToDB(temperature, humidity, 0);
  }
}

void BeehiveMonitor::sendDataToDB(float temperature, float humidity, float weight)
{
  _database.prepareField(1 + _ID * 3, temperature);
  _database.prepareField(2 + _ID * 3, humidity);
  _database.prepareField(3 + _ID * 3, weight);
  _database.sendData();
}