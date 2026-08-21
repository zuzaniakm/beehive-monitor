#include "BeehiveMonitor.h"
#include "Config.h"

BeehiveMonitor bhm(THINGSPEAK_CHANNEL_ID, "THINGSPEAK_WRITE_API_KEY");

void setup() {
  Serial.begin(115200);
  Serial.print("\nStarted...");
  delay(1000);

  bool reading = bhm.readSettings();
  //if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0 || !reading) {
      bhm.setup();
  //}
  
  delay(100);
  bhm.readData();
  delay(100);

  pinMode(32, INPUT_PULLDOWN);
  delay(100);
  unsigned scanDelay = bhm.getDelay();
  esp_sleep_enable_timer_wakeup(((scanDelay * 60) - 3) * 1e6);
  //esp_sleep_enable_timer_wakeup(5 * 60 * 1e6);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_32, 1); 
  Serial.println("Going to sleep now");
  Serial.flush();
  delay(1000);
  esp_deep_sleep_start();
}

void loop() {
}