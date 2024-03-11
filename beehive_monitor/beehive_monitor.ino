#include "BeehiveMonitor.h"
#include "Config.h"

BeehiveMonitor bhm(THINGSPEAK_CHANNEL_ID, "THINGSPEAK_WRITE_API_KEY");

void setup() {
  Serial.begin(115200);
  Serial.print("\nStarted...");
  bhm.setup();
  bhm.readData(true);
}

void loop() {
  unsigned scans = bhm.getDelay() / SCAN_DELAY;
  for (unsigned i = 1; i <= scans; i++) {
    esp_sleep_enable_timer_wakeup(SCAN_DELAY * 60 * 1e6);
    esp_light_sleep_start();
    delay(1000);
    Serial.print(i == scans);
    bhm.readData(i == scans);
    delay(1000);
  }
}