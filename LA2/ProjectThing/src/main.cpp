#include <Arduino.h>
// #include <WiFi.h>
#include <driver/i2s.h>
#include <esp_task_wdt.h>
#include <Wire.h>
#include "I2SMicSampler.h"
#include "ADCSampler.h"
#include "I2SOutput.h"
#include "config.h"
#include "Application.h"
#include "SPIFFS.h"
#include "IntentProcessor.h"
#include "Speaker.h"
#include "IndicatorLight.h"
#include <SPI.h>
#include "DisplayHelper.h"
#include "NetworkHelper.h"


// MAC address etc. //////////////////////////////////////////////////////////
void sayHi();
extern char MAC_ADDRESS[];
void getMAC(char *);
char MAC_ADDRESS[13]; // MAC addresses are 12 chars, plus the NULL terminator

unsigned long startMillisHourly;
unsigned long startMillisMinutely;
unsigned long plugMillis;
unsigned long currentMillis;
unsigned long minute = 60000; // 1 minute
unsigned long clockPeriod = 5 * minute;
unsigned long weatherPeriod = 30 * minute;

bool plugOn = false;

// RCSwitch mySwitch = RCSwitch();

// i2s config for using the internal ADC
i2s_config_t adcI2SConfig = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN),
    .sample_rate = 16000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S_LSB,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = 64,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0};

// i2s config for reading from both channels of I2S
i2s_config_t i2sMemsConfigBothChannels = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = 16000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_MIC_CHANNEL,
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = 64,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0};

// i2s microphone pins
i2s_pin_config_t i2s_mic_pins = {
    .bck_io_num = I2S_MIC_SERIAL_CLOCK,
    .ws_io_num = I2S_MIC_LEFT_RIGHT_CLOCK,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_MIC_SERIAL_DATA};

// !! NOT IN USE 
// i2s speaker pins
i2s_pin_config_t i2s_speaker_pins = {
    .bck_io_num = I2S_SPEAKER_SERIAL_CLOCK,
    .ws_io_num = I2S_SPEAKER_LEFT_RIGHT_CLOCK,
    .data_out_num = I2S_SPEAKER_SERIAL_DATA,
    .data_in_num = I2S_PIN_NO_CHANGE};

// This task does all the heavy lifting for our application
void applicationTask(void *param)
{
  Application *application = static_cast<Application *>(param);

  const TickType_t xMaxBlockTime = pdMS_TO_TICKS(100);
  while (true)
  {
    // wait for some audio samples to arrive
    uint32_t ulNotificationValue = ulTaskNotifyTake(pdTRUE, xMaxBlockTime);
    if (ulNotificationValue > 0)
    {
      application->run();
    }
  }
}

void setup()
{
  Serial.begin(115200);
  delay(3000);

  Serial.println();
  Serial.println("Powering on!");

  display.begin();

  displayStartup();

  sayHi();

  Serial.printf("Total heap: %d\n", ESP.getHeapSize());
  Serial.printf("Free heap: %d\n", ESP.getFreeHeap());

  // startup SPIFFS for the wav files
  SPIFFS.begin();
  // make sure we don't get killed for our long running tasks
  esp_task_wdt_init(10, false);

  // start up the I2S input (from either an I2S microphone or Analogue microphone via the ADC)
#ifdef USE_I2S_MIC_INPUT
  // Direct i2s input from INMP441 or the SPH0645
  I2SSampler *i2s_sampler = new I2SMicSampler(i2s_mic_pins, false);
#else
  // Use the internal ADC
  I2SSampler *i2s_sampler = new ADCSampler(ADC_UNIT_1, ADC_MIC_CHANNEL);
#endif

  // !! NOT IN USE !!
  // start the i2s speaker output
  I2SOutput *i2s_output = new I2SOutput();
  i2s_output->start(I2S_NUM_1, i2s_speaker_pins);
  Speaker *speaker = new Speaker(i2s_output);

  // indicator light to show when we are listening
  IndicatorLight *indicator_light = new IndicatorLight();

  // and the intent processor
  IntentProcessor *intent_processor = new IntentProcessor(speaker, indicator_light);
  // intent_processor->addDevice("kitchen", LED_KITCHEN_PIN);
  // // bedroom works on bare feather but not unphone due to pin clash
  // intent_processor->addDevice("bedroom", LED_BEDROOM_PIN);
  // intent_processor->addDevice("table", LED_TABLE_PIN);

  // create our application
  Application *application = new Application(i2s_sampler, intent_processor, speaker, indicator_light);

  // set up the i2s sample writer task
  TaskHandle_t applicationTaskHandle;
  xTaskCreate(applicationTask, "Application Task", 8192, application, 1, &applicationTaskHandle);

  // start sampling from i2s device - use I2S_NUM_0 as that's the one that supports the internal ADC
#ifdef USE_I2S_MIC_INPUT
  i2s_sampler->start(I2S_NUM_0, i2sMemsConfigBothChannels, applicationTaskHandle);
#else
  i2s_sampler->start(I2S_NUM_0, adcI2SConfig, applicationTaskHandle);
#endif

  connectToWifi();

  printf("Starting display\n");

  displayHome(true);

  printf("display updated!\n");

  startMillisHourly = millis();
  startMillisMinutely = millis();
  plugMillis = millis();

  indicator_light->setState(PULSING);
  delay(10000);
  indicator_light->setState(OFF);

  printf("Ready to listen!\n");
}

void loop()
{
  // every 5 minutes, get the time
  currentMillis = millis();
  if (currentMillis - startMillisMinutely >= clockPeriod) {
    displayHome(false);
    startMillisMinutely = millis();
    clockPeriod = adjustPeriod(clockPeriod, false);
  }

  // every 30 minutes, get the weather
  if (currentMillis - startMillisHourly >= weatherPeriod) {
    displayHome(true);
    startMillisHourly = millis();
    weatherPeriod = adjustPeriod(weatherPeriod, true);
  }
}

void getMAC(char *buf) { // the MAC is 6 bytes, so needs careful conversion...
  uint64_t mac = ESP.getEfuseMac(); // ...to string (high 2, low 4):
  char rev[13];
  sprintf(rev, "%04X%08X", (uint16_t) (mac >> 32), (uint32_t) mac);

  // the byte order in the ESP has to be reversed relative to normal Arduino
  for(int i=0, j=11; i<=10; i+=2, j-=2) {
    buf[i] = rev[j - 1];
    buf[i + 1] = rev[j];
  }
  buf[12] = '\0';
}

void sayHi() {
  printf("\nHi there.\n");
  getMAC(MAC_ADDRESS);            // store the MAC address as a chip identifier
  Serial.printf("ESP32 MAC = %s\n", MAC_ADDRESS); // print the ESP's "ID"

  #ifdef UNPHONE
    printf("UNPHONE is defined\n");
  #endif
  #ifdef UNPHONE_SPIN
    printf("UNPHONE_SPIN=%d\n", UNPHONE_SPIN);
  #endif
  #ifdef ARDUINO_FEATHER_ESP32
    printf("ARDUINO_FEATHER_ESP32 is defined\n");
  #endif

  #ifdef ESP_IDF_VERSION_MAJOR
    printf( // IDF version
      "IDF version: %d.%d.%d\n",
      ESP_IDF_VERSION_MAJOR, ESP_IDF_VERSION_MINOR, ESP_IDF_VERSION_PATCH
    );
  #endif
  #ifdef ESP_ARDUINO_VERSION_MAJOR
    printf(
      "ESP_ARDUINO_VERSION_MAJOR=%d; MINOR=%d; PATCH=%d\n",
      ESP_ARDUINO_VERSION_MAJOR, ESP_ARDUINO_VERSION_MINOR,
      ESP_ARDUINO_VERSION_PATCH
    );
  #endif

  #ifdef ARDUINO_ARCH_ESP32
    printf("ARDUINO_ARCH_ESP32 is defined\n");
  #endif
  #ifdef ESP_PLATFORM
    printf("ESP_PLATFORM is defined\n");
  #endif
  #ifdef ESP32
    printf("ESP32 is defined\n");
  #endif
  #ifdef IDF_VER
    printf("IDF_VER=%s\n", IDF_VER);
  #endif
  #ifdef ARDUINO
    printf("ARDUINO=%d\n", ARDUINO);
  #endif
  #ifdef ARDUINO_BOARD
    printf("ARDUINO_BOARD=%s\n", ARDUINO_BOARD);
  #endif
  #ifdef ARDUINO_VARIANT
    printf("ARDUINO_VARIANT=%s\n", ARDUINO_VARIANT);
  #endif
  #ifdef ARDUINO_SERIAL_PORT
    printf("ARDUINO_SERIAL_PORT=%d\n", ARDUINO_SERIAL_PORT);
  #endif

  #ifdef ARDUINO_IDE_BUILD
    printf("ARDUINO_IDE_BUILD is defined\n");
  #else
    printf("no definition of ARDUINO_IDE_BUILD\n");
  #endif

  printf("Marvin mic and speaker pins:\n");
  printf("I2S_MIC_SERIAL_CLOCK=%d\n",           I2S_MIC_SERIAL_CLOCK);
  printf("I2S_MIC_LEFT_RIGHT_CLOCK=%d\n",       I2S_MIC_LEFT_RIGHT_CLOCK);
  printf("I2S_MIC_SERIAL_DATA=%d\n",            I2S_MIC_SERIAL_DATA);
  printf("I2S_SPEAKER_SERIAL_CLOCK=%d\n",       I2S_SPEAKER_SERIAL_CLOCK);
  printf("I2S_SPEAKER_LEFT_RIGHT_CLOCK=%d\n",   I2S_SPEAKER_LEFT_RIGHT_CLOCK);
  printf("I2S_SPEAKER_SERIAL_DATA=%d\n",        I2S_SPEAKER_SERIAL_DATA);
  printf("E-INK EPD_DC=%d\n", EPD_DC);
  printf("E-INK EPD_CS=%d\n", EPD_CS);
  printf("E-INK SRAM_CS=%d\n", SRAM_CS);
  // printf("=%d\n", );
  // printf("LED_KITCHEN_PIN=%d\n",                LED_KITCHEN_PIN);
  // printf("LED_BEDROOM_PIN=%d\n",                LED_BEDROOM_PIN);
  // printf("LED_TABLE_PIN=%d\n",                  LED_TABLE_PIN);
}
