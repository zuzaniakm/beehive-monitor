#include "Wifi.h"
#include "Database.h"
#include "BeehiveMonitor.h"

BeehiveMonitor bhm(THINGSPEAK_CHANNEL_ID, "THINGSPEAK_WRITE_API_KEY");

void setup() {
  Serial.begin(9600);
  Serial.print("\nStarted...");
  bhm.setup();
  delay(200);
}

void loop() {
  bhm.readData();
  delay(10000);
}
