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
    unsigned sendData(int data, unsigned field);
};

unsigned Database::sendData(int data, unsigned int field)
{
  return ThingSpeak.writeField(_channelId, field, data, _apiWriteKey);
}