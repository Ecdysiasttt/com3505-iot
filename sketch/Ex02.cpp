#include "Thing.h"

#ifdef ARDUINO_FEATHER_ESP32
  #define LED_PIN     32
  #define SWITCH_PIN  14
#else
  #define LED_PIN      6
  #define SWITCH_PIN   5
#endif

void setup02() {
  setup01();

  pinMode(LED_PIN, OUTPUT);
  pinMode(SWITCH_PIN, INPUT_PULLUP);

}

void loop02() {
  if (digitalRead(SWITCH_PIN) == LOW) {
    Serial.printf("Switch pressed.\n");
    
    Serial.printf("Setting LED_PIN high\n");
    digitalWrite(LED_PIN, HIGH);
    delay(100);

    Serial.printf("Setting LED_PIN low\n");
    digitalWrite(LED_PIN, LOW);
    delay(100);
  }
}

