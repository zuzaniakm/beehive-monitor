#pragma once
#include <WiFi.h>

class Wifi
{
  private:
    char* _ssid;
    char* _password;

  public: 
    Wifi() : _ssid(nullptr), _password(nullptr) {}
    void setCredentials(char* ssid, char* password);
    void connect();
    void disconnect();
};

void Wifi::setCredentials(char* ssid, char* password)
{
  _ssid = ssid;
  _password = password;
}

void Wifi::connect()
{
  WiFi.begin(_ssid, _password);
  for (unsigned i = 1; i <= 20; i++)
  {
    if (WiFi.status() != WL_CONNECTED) 
    {
        delay(500);
        Serial.print(".");
    }
    else
    {
      Serial.println("WiFi connected!");
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());
      return;
    }
  }
  Serial.println("WiFi failed to connect!");
  WiFi.disconnect();
}

void Wifi::disconnect()
{
  WiFi.disconnect();
}