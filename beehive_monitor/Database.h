#pragma once

#include <ThingSpeak.h>
#include <WiFi.h>

class Database
{
  private:
    unsigned long _channelId;
    const char* _apiWriteKey;
    WiFiClient _client;

  public:
    Database(unsigned long channelId, const char* apiWriteKey) : _channelId(channelId), _apiWriteKey(apiWriteKey) 
    {
        ThingSpeak.begin(_client);
    }
    void prepareField(unsigned field, const char* data);
    unsigned sendData();
};

void Database::prepareField(unsigned field, const char* data)
{
  ThingSpeak.setField(field, data);
}

unsigned Database::sendData()
{
  return ThingSpeak.writeFields(_channelId, _apiWriteKey);
}