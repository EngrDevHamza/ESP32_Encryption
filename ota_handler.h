#include <ArduinoOTA.h>

void setupOTA() {
  ArduinoOTA
    .onStart([]() { Serial.println("Start OTA"); })
    .onEnd([]() { Serial.println("\nEnd OTA"); })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
    });
  ArduinoOTA.begin();
}
