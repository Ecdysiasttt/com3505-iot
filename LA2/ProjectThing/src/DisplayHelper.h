#ifndef DISPLAYHELPER_H
#define DISPLAYHELPER_H

// #include <Arduino.h>
#include "Adafruit_ThinkInk.h"
#include "time.h"

// open weather API information
// String openWeatherMapApiKey = "ae313677cd16b69cb29462fdf1e76991";
// String location = "Sheffield,GB";
// #define DATA_SOURCE_URL = "http://api.openweathermap.org/data/2.5/weather?q=";

extern ThinkInk_213_Mono_B72 display;

void displayHome(bool fullRefresh);

void displayStartup();
void displayUpdating();

void getWeather();
void deserializeWeather(String weatherJson);

void getTime();
char *getDaySuffix(int day);

void drawHome(bool fullRefresh);
void drawText(int16_t x, int16_t y, const char *text, uint16_t color, uint8_t size);
void displayCentredAlertWithSubtext(char *header, char *subtext);
int getTextX(char *text, int size, bool margin);
int getTextY(int size, bool margin);

// void displayTime();
// void printTime();
// void drawTime();



String httpGETRequest(const char* serverName);

int adjustPeriod(int initialPeriod, bool isHour);

#endif