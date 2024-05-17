// #include <Arduino.h>
#include "DisplayHelper.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "config.h"

// #include <string>

const int displayEndX = 250;
const int displayEndY = 122;

// NTP time info
const char* ntpServer = "pool.ntp.org";   // server
const long gmtOffset_sec = 0;             // diff in seconds from current tz to gmt
const int daylightOffset_sec = 3600;      // 3600s = 1h diff for dst

struct tm timeinfo;

int timeSec;
int timeMin;

bool startup = true;
int minPrev = 0;

char timeString[6];
char dayOfWeek[11];
char dayAndMonth[50];

String jsonResponse;
String openWeatherMapApiKey = "ae313677cd16b69cb29462fdf1e76991";
String location = "Sheffield,GB";

char weatherDesc[50];
// char tempDisplay[6];

JsonObject weather_0;
int weather_0_id;

JsonObject sys;
int sys_type;

const char* weather_0_main;
const char* weather_0_icon;

JsonObject mainBlock;
float main_temp;

const char* sys_country;
const char* name;

ThinkInk_213_Mono_B72 display(EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY);

// Allows for the printing of notifications to the user, i.e. booting, updating, etc
//      ____________________________
//      |                          |
//      |          Hello!          |   // header
//      |     Getting ready...     |   // subtext
//      |__________________________|
//
// header should be no more than 10 chars long
// subtext should be no more than 20 chars long
void displayCentredAlertWithSubtext(char *header, char *subtext) {
  display.clearBuffer();

  int headerSize = 4;     // 24x32
  int subtextSize = 2;    // 12x16

  int headerYStart = (displayEndY - getTextY(headerSize, true) - getTextY(subtextSize, true)) / 2;

  drawText(
    (displayEndX - getTextX(header, headerSize, true)) / 2,
    headerYStart,
    header, BLACK, headerSize
  );
  drawText(
    (displayEndX - getTextX(subtext, subtextSize, true)) / 2,
    (headerYStart + getTextY(headerSize, true)),
    subtext, BLACK, subtextSize
  );

  display.display();
}

void displayStartup() {
  displayCentredAlertWithSubtext("Hello!", "Getting ready...");
}

void displayUpdating() {
  displayCentredAlertWithSubtext("Updating!", "Do not power off...");
}

// default home screen. Displays location, time, and weather
// performs a full refresh.
void displayHome(bool fullRefresh) {

  // if full refresh, call openweathermap api
  if (fullRefresh) {
    Serial.println("Full refresh.");

    getWeather();
    Serial.println("Weather updated.");
  }

  getTime();
  Serial.println("Time updated.");

  drawHome(fullRefresh);
}

char weatherTempBuff[6];
char weatherTypeBuff[31];
char locationBuff[50];

// Query OpenWeatherMap API to get latest weather
void getWeather() {
  Serial.println("\n========================");
  Serial.println("   Getting Weather...");
  Serial.println("========================");

  String serverPath = 
    "http://api.openweathermap.org/data/2.5/weather?q=" + location + "&APPID=" + openWeatherMapApiKey + "&units=metric";    // gets temp in celsius instead of kelvin

  jsonResponse = httpGETRequest(serverPath.c_str());
  Serial.println(jsonResponse);
  
  // Serial.println("\n\n\n");

  deserializeWeather(jsonResponse);

  weatherTempBuff[0] = '\0';
  weatherDesc[0] = '\0';
  locationBuff[0] = '\0';

  dtostrf(main_temp, 1, 0, weatherTempBuff); // e.g. 23 or -14

  // strcat(weatherDesc, weatherTempBuff);        // e.g: 23
  // strcat(weatherDesc, "(char)247C, ");                  //     °C, 

  strcat(weatherDesc, weather_0_main);  // e.g. Scattered clouds

  // serialise location

  strcat(locationBuff, name);
  strcat(locationBuff, ", ");
  strcat(locationBuff, sys_country);

  Serial.printf("\nWeather obtained:\n");
  Serial.printf("%s°C - %s in %s.", weatherTempBuff, weatherDesc, locationBuff);
}

// Call an NTP server to get current time
void getTime() {
  Serial.println("\n========================");
  Serial.println("    Getting time...");
  Serial.println("========================");

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);


  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time\n");
    return;
  }

  timeSec = timeinfo.tm_sec;
  timeMin = timeinfo.tm_min;

  // stringify date to display
  char dateOfMonth[3];
  char suffix[4] = "";
  char month[5];

  dayAndMonth[0] = '\0'; // set initial char to null to prevent silly concats
  dayOfWeek[0] = '\0';

  strftime(dayOfWeek, sizeof(dayOfWeek), "%A", &timeinfo);

  strftime(dateOfMonth, sizeof(dateOfMonth), "%e", &timeinfo);
  strcat(suffix, getDaySuffix(timeinfo.tm_mday));
  strcat(dateOfMonth, suffix);

  strftime(month, sizeof(month), "%b", &timeinfo);

  // strcat(dateString, dayOfWeek);
  strcat(dayAndMonth, dateOfMonth);
  strcat(dayAndMonth, month);

  // results in, for instance:
  // Saturday, 21st Dec

  Serial.printf("Date obtained: %s, %s\n", dayOfWeek, dayAndMonth);
  
  // stringify time to display 
  strftime(timeString, sizeof(timeString), "%H:%M", &timeinfo);
  Serial.printf("Time obtained: %s\n\n", timeString);
}

char *getDaySuffix(int day) {
  switch (day) {
    case 1:
    case 21:
    case 31:
      return "st ";
    case 2:
    case 22:
      return "nd ";
    case 3:
    case 23:
      return "rd ";
    default:
      return "th ";
  }
}


// void drawHome(bool fullRefresh) {
//   display.clearBuffer();

//   drawText(0, 0, timeString, BLACK, 3);
//   if(fullRefresh) {
//     drawText(0, 26, dateString, BLACK, 2);
//     drawText(0, 86, locationBuff, BLACK, 2);
//     drawText(0, 104, weatherDisplay, BLACK, 2);
//     display.display(false);
//   } else {
//     display.displayPartial();
//   }
// }

// !!! DISPLAY IS 250x122

void drawHome(bool fullRefresh) {
  display.clearBuffer();
  
  int timeSize = 4;
  int dateSize = 2;
  int tempSize = 4;
  int degreeSize = 2;
  int locSize = 2;
  int weatherDescSize = 2;

  // int degPos = getTextLength(weatherTempBuff, 3);

  drawText(
    0,
    0,
    timeString, BLACK, timeSize);
  drawText(
    (displayEndX - getTextX(dayOfWeek, dateSize, true)),
    0,
    dayOfWeek, BLACK, dateSize);
  drawText(
    (displayEndX - getTextX(dayAndMonth, dateSize, true)),
    (0 + getTextY(dateSize, true)),
    dayAndMonth, BLACK, dateSize);
  drawText(
    0,
    (displayEndY - getTextY(tempSize, true)),
    weatherTempBuff, BLACK, tempSize);
  drawText(
    getTextX(weatherTempBuff, tempSize, true),
    (displayEndY - getTextY(tempSize, true)), // 
    "o", BLACK, degreeSize);
  drawText(
    getTextX(weatherTempBuff, tempSize, true) + getTextX("o", degreeSize, true),
    (displayEndY - getTextY(tempSize, true)), // 
    "C", BLACK, tempSize);
  drawText(
    (displayEndX - getTextX(weatherDesc, weatherDescSize, true)),
    (displayEndY - getTextY(locSize, true) - getTextY(weatherDescSize, true)), // 
    weatherDesc, BLACK, weatherDescSize);
  drawText(
    (displayEndX - getTextX(locationBuff, locSize, true)),
    (displayEndY - getTextY(locSize, true)),
    locationBuff, BLACK, locSize);

  display.display();
}

int getTextX(char *text, int size, bool margin) {
  int defaultX = 6;

  int textLength = size*defaultX*strlen(text);
  if (margin)
    textLength += 2;

  return textLength;
}

int getTextY(int size, bool margin) {
  int defaultY = 8;

  int textHeight = size*defaultY;
  
  if (margin)
    textHeight += 2;

  return textHeight;
}


void drawText(int16_t x, int16_t y, const char *text, uint16_t color, uint8_t size) {
  display.setCursor(x, y);
  display.setTextColor(color);
  display.setTextWrap(true);
  display.setTextSize(size);
  display.print(text);
}

String httpGETRequest(const char* serverName) {
  WiFiClient client;
  HTTPClient http;
    
  // Your Domain name with URL path or IP address with path
  http.begin(client, serverName);
  
  // Send HTTP POST request
  int httpResponseCode = http.GET();
  
  String payload = "{}"; 
  
  if (httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

  return payload;
}

// Adjusts the period in which the ESP queries the NTP server
// prevents the clock display being potentially a minute behind
int adjustPeriod(int initialPeriod, bool isHour) {

  Serial.println("\nAdjusting time periods");
  Serial.printf("Starting period: %i\n", initialPeriod);

  if (isHour) {
    if (timeMin != 0) {
      int newPeriod = (30 * 60000) - (60000 * timeMin);
      Serial.printf("Adjusted hour time period to %d\n\n", newPeriod);
      return newPeriod;
    }
    return (30 * 60000);
  } else {
    if (timeSec != 0) {
      int newPeriod = (5 * 60000) - (1000 * timeSec);
      Serial.printf("Adjusted minute time period to %d\n\n", newPeriod);
      return newPeriod;
    }
    return (5 * 60000);
  }
}


// Takes the JSON response from OpenWeatherMap API and deserializes it
// for use in displaying the weather
void deserializeWeather(String weatherJson) {
  JsonDocument doc;

  DeserializationError error = deserializeJson(doc, weatherJson);

  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }

  // coord_lon = doc["coord"]["lon"]; // -1.4659
  // coord_lat = doc["coord"]["lat"]; // 53.383

  weather_0 = doc["weather"][0];
  // weather_0_id = weather_0["id"]; // 801
  weather_0_main = doc["weather"][0]["main"]; // "Clouds"
  // weather_0_description = weather_0["description"]; // "few clouds"
  weather_0_icon = weather_0["icon"]; // "02d"

  // base = doc["base"]; // "stations"

  mainBlock = doc["main"];
  main_temp = mainBlock["temp"]; // 285.49
  // main_feels_like = mainBlock["feels_like"]; // 284.32
  // main_temp_min = mainBlock["temp_min"]; // 285.22
  // main_temp_max = mainBlock["temp_max"]; // 286.43
  // main_pressure = mainBlock["pressure"]; // 994
  // main_humidity = mainBlock["humidity"]; // 59

  // visibility = doc["visibility"]; // 10000

  // wind = doc["wind"];
  // wind_speed = wind["speed"]; // 0.45
  // wind_deg = wind["deg"]; // 68
  // wind_gust = wind["gust"]; // 2.24
  // clouds_all = doc["clouds"]["all"]; // 22

  // dt = doc["dt"]; // 1711821033

  sys = doc["sys"];
  // sys_type = sys["type"]; // 2
  // sys_id = sys["id"]; // 38363
  sys_country = sys["country"]; // "GB"
  // sys_sunrise = sys["sunrise"]; // 1711777392
  // sys_sunset = sys["sunset"]; // 1711823817

  // timezone = doc["timezone"]; // 0
  // id = doc["id"]; // 2638077
  name = doc["name"]; // "Sheffield"
  // cod = doc["cod"]; // 200
}
