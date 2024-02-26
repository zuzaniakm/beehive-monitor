#pragma once
#include <WiFi.h>

class Wifi {
  private:
    const char* _ssid;
    const char* _password;

  public: 
    Wifi(const char* ssid, const char* password) : _ssid(ssid), _password(password) {}
    bool connect();
    void disconnect();
    int getStatus();
};

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
  WiFi.disconnect();
}

int Wifi::getStatus() {
  return WiFi.status();
}