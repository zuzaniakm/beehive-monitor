#pragma once
#include <ThingSpeak.h>
#include <WiFi.h>

class Database {
  private:
    unsigned long _channelId;
    const char* _apiWriteKey;
    WiFiClient _client;

  public:
    Database(unsigned long channelId, const char* apiWriteKey) : _channelId(channelId), _apiWriteKey(apiWriteKey) {
        ThingSpeak.begin(_client);
    }
    void prepareField(unsigned field, float data);
    void sendData();
};

void Database::prepareField(unsigned field, float data) {
  ThingSpeak.setField(field, data);
}

void Database::sendData() {
  Serial.println("Uploading...");
  ThingSpeak.writeFields(_channelId, _apiWriteKey);
  delay(1000);
}