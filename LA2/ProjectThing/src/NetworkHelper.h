#ifndef NETWORKHELPER_H
#define NETWORKHELPER_H

// #include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h> // ESP32 library for making HTTP requests
#include <Update.h>     // OTA update library
#include "DisplayHelper.h"
#include "Speaker.h"

// network credentials
#define WIFI_SSID "ASK4 Wireless"
#define WIFI_PASSWORD ""
#define HOST_NAME "ESP32-S2-MINI"

// IP address and port number for OTA server
#define FIRMWARE_SERVER_IP_ADDR "10.213.65.18"    // my desktop's IP address
#define FIRMWARE_SERVER_PORT    "8000"

const int firmware_version = 11; // used to check for updates

void connectToWifi();
void attemptWifiConnect();
void doCloudGet();
void handleOTAProgress();
void performOTAUpdate(bool fromIntent);

void startAP();
void initWebServer();

void rootPage();
void handleWifi();
void handleWifichz();
void handleStatus();



#endif