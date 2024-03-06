#include "Thing.h"

// LEDs
int ledPins[] = {
   5, // was 14
   6, // was 32
   9, // was 15
  10, // was 33
  11, // was 27
  12, // was 21
  18, // A0, was 26
  17, // A1, was 25
   8, // A5, was  4
};

void setup03() {
  setup01();

  for (int pin : ledPins)
  {
    pinMode(pin, OUTPUT);
  }
  
  // pinMode(ledPins[], OUTPUT);
  // pinMode(SWITCH_PIN, INPUT_PULLUP);

}

void loop03() {
  Serial.printf("Starting loop\n");

  for (int pin : ledPins)
  {
    digitalWrite(pin, HIGH);
    delay(100);
    digitalWrite(pin, LOW);
  }

  Serial.printf("Loop complete\n");

  // if (digitalRead(SWITCH_PIN) == LOW) {
    // Serial.printf("Switch pressed.\n");
    
  //   Serial.printf("Setting LED_PIN high\n");
  //   digitalWrite(LED_PIN, HIGH);
  //   delay(100);

  //   Serial.printf("Setting LED_PIN low\n");
  //   digitalWrite(LED_PIN, LOW);
  //   delay(100);
  // }

}

