// Ex02.ino/.cpp
// blinking an external LED; reading from a switch

#include "Thing.h"

bool useInternalLED = true; // which LED to blink

// pin definitions
// changes from ESP32 Feather to ESP32S3: pin 32 becomes 6, 14 becomes 5
#ifdef ARDUINO_FEATHER_ESP32
  #define LED_PIN     32
  #define SWITCH_PIN  14
#else
  #define LED_PIN      6
  #define SWITCH_PIN   5
#endif

void macro_prints(); // print some helpful macros

void setup02() {
  setup01(); // include previous setup for serial, and for the internal LED
  Serial.printf("\nsetup02...\nESP32 MAC = %s\n", MAC_ADDRESS); // ESP's "ID"

  // set up GPIO pin for an external LED
  pinMode(LED_PIN, OUTPUT); // set up LED pin as a digital output

  // set up GPIO pin for a switch
  pinMode(SWITCH_PIN, INPUT_PULLUP); // digital in, built-in pullup resistor

  macro_prints();
}

void loop02() {
  if(digitalRead(SWITCH_PIN) == LOW) { // switch pressed
    Serial.printf("switch is pressed...\n");
    useInternalLED = ! useInternalLED;
  }

  if(useInternalLED) {
    Serial.printf("blinking internal LED...\n");
    blink(1, 500); // using the method from Ex01
  } else {
    Serial.printf("setting LED_PIN HIGH...\n");
    digitalWrite(LED_PIN, HIGH);       // on...
    delay(500); // take a rest (but: what happens if switch pressed now?!)

    Serial.printf("setting LED_PIN LOW...\n");
    digitalWrite(LED_PIN, LOW);        // off...
    delay(500); // take a rest (but: what happens if switch pressed now?!)
  }
}

// some helpful macro printouts from ../HelloWorld
void macro_prints() {
  #ifdef ESP_IDF_VERSION_MAJOR
    Serial.printf( // IDF version
      "IDF version: %d.%d.%d\n",
      ESP_IDF_VERSION_MAJOR, ESP_IDF_VERSION_MINOR, ESP_IDF_VERSION_PATCH
    );
  #endif
  #ifdef ESP_ARDUINO_VERSION_MAJOR
    Serial.printf(
      "ESP_ARDUINO_VERSION_MAJOR=%d; MINOR=%d; PATCH=%d\n",
      ESP_ARDUINO_VERSION_MAJOR, ESP_ARDUINO_VERSION_MINOR,
      ESP_ARDUINO_VERSION_PATCH
    );
  #endif
  #ifdef ARDUINO_FEATHER_ESP32
    Serial.printf("ARDUINO_FEATHER_ESP32 is defined\n");
  #endif
  #ifdef ARDUINO_ARCH_ESP32
    Serial.printf("ARDUINO_ARCH_ESP32 is defined\n");
  #endif
  #if CONFIG_IDF_TARGET_ESP32
    Serial.printf("CONFIG_IDF_TARGET_ESP32\n");
  #endif
  #if CONFIG_IDF_TARGET_ESP32S3
    Serial.printf("CONFIG_IDF_TARGET_ESP32S3\n");
  #endif
  #ifdef ESP_PLATFORM
    Serial.printf("ESP_PLATFORM is defined\n");
  #endif
  #ifdef ESP32
    Serial.printf("ESP32 is defined\n");
  #endif
  #ifdef IDF_VER
    Serial.printf("IDF_VER=%s\n", IDF_VER);
  #endif
  #ifdef ARDUINO
    Serial.printf("ARDUINO=%d\n", ARDUINO);
  #endif
  #ifdef ARDUINO_BOARD
    Serial.printf("ARDUINO_BOARD=%s\n", ARDUINO_BOARD);
  #endif
  #ifdef ARDUINO_VARIANT
    Serial.printf("ARDUINO_VARIANT=%s\n", ARDUINO_VARIANT);
  #endif
  #ifdef ARDUINO_SERIAL_PORT
    Serial.printf("ARDUINO_SERIAL_PORT=%d\n", ARDUINO_SERIAL_PORT);
  #endif
}