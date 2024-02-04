#pragma once
#include <WiFi.h>

class Wifi
{
  private:
    const char* _ssid;
    const char* _password;

  public: 
    Wifi(const char* ssid, const char* password) : _ssid(ssid), _password(password) {}
    void connect();
    void disconnect();
};

void Wifi::connect()
{
  Serial.println(_ssid);
  Serial.println(_password);
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

