#pragma once
#include <WiFi.h>

class Wifi {
  private:
    String _ssid;
    String _password;

  public: 
    Wifi() = default;
    int getStatus();
    void setCredentials(String ssid, String password);
    bool connect();
    void disconnect();
    void turnOn();
    void turnOff();
};

int Wifi::getStatus() {
  return WiFi.status();
}

void Wifi::setCredentials(String ssid, String password) {
  _ssid = ssid;
  _password = password;
}

bool Wifi::connect() {
  WiFi.begin(_ssid, _password);
  for (unsigned i = 1; i <= 20; i++) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("WiFi connected!");
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());
      return true;
    } else {
      delay(500);
      Serial.print(".");
    }
  }
  Serial.println("WiFi failed to connect!");
  WiFi.disconnect();
  return false;
}

void Wifi::disconnect() {
  WiFi.disconnect(true);
}

void Wifi::turnOff() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

void Wifi::turnOn() {
  WiFi.enableSTA(true);
  WiFi.mode(WIFI_STA);
  delay(100);
}