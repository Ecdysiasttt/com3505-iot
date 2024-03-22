// PrAndUpThing.ino
#include "WebServerPages.h"

char MAC_ADDRESS[13];

WebServer webServer(80);
String apSSID;

// IP address and port number for OTA server
#define FIRMWARE_SERVER_IP_ADDR "10.213.65.18"    // my desktop's IP address
#define FIRMWARE_SERVER_PORT    "8000"

int firmwareVersion = 12; // used to check for updates

void setup() {
  Serial.begin(115200);

  getMAC(MAC_ADDRESS);
  
  // while (!Serial); // debugging to see all serial prints 
  // delay(5000);

  Serial.println("Beginning LA1\n");
  Serial.printf("Current firmware version: %d\n\n", firmwareVersion);

  // initialise external LEDs
  for(int i = 0; i < sizeof(ledPins) / sizeof(int) ; i++) {
    pinMode(ledPins[i], OUTPUT);
  }
  turnOffAllLEDs();
  
  startAP();
  initWebServer();

  // attempt WiFi connection if previously established
  WiFi.begin();
  activateLED(connWifiLed);
  
  attemptWifiConnect();

  if (WiFi.status() == WL_CONNECTED) {
    // successful connection, will now check for OTA update
    // most of the following is lightly adapted from Ex10
    
    delay(500);

    Serial.println("Checking for available updates...");
    activateLED(otaCheckLed);

    HTTPClient http;
    int respCode;
    int highestAvailableVersion = -1;

    // read the version file from the cloud
    respCode = doCloudGet(&http, "version.txt");
    if(respCode > 0) // check response code (-ve on failure)
      highestAvailableVersion = atoi(http.getString().c_str());
    else {
      Serial.printf("Couldn't get version! rtn code: %d\n", respCode);
      activateLED(otaCheckLed);
    }

    http.end(); // free resources

    // do we know the latest version, and does the firmware need updating?
    if(respCode < 0) {
      return;
    } else if(firmwareVersion >= highestAvailableVersion) {
      Serial.println("Firmware is up to date.");
      activateLED(wifiEnabledLed);
      return;
    }

    // do a firmware update
    Serial.printf(
      "Upgrading firmware from version %d to version %d\n",
      firmwareVersion, highestAvailableVersion
    );

    // do a GET for the .bin, e.g. "23.bin" when "version.txt" contains 23
    String binName = String(highestAvailableVersion);
    binName += ".bin";
    respCode = doCloudGet(&http, binName);
    int updateLength = http.getSize();

    // possible improvement: if size is improbably big or small, refuse
    if(respCode > 0 && respCode != 404) { // check response code (-ve on failure)
      Serial.printf(".bin code/size: %d; %d\n\n", respCode, updateLength);
    } else {
      Serial.printf("Failed to get .bin! Return code: %d\n", respCode);
      http.end(); // free resources
      return;
    }

    // write the new version of the firmware to flash
    WiFiClient stream = http.getStream();
    Update.onProgress(handleOTAProgress); // print out progress
    if(Update.begin(updateLength)) {
      Serial.printf("Starting OTA update. This may take some time. DO NOT power off the device.\n");
      activateLED(updatingLed);
      Update.writeStream(stream);
      if(Update.end()) {
        Serial.printf("Update complete! Now finishing...\n");
        Serial.flush();
        if(Update.isFinished()) {
          Serial.printf("Update successfully finished! Rebooting...\n\n");
          activateLED(updatedSuccLed);
          delay(500);
          ESP.restart();
        } else {
          Serial.printf("Update didn't finish correctly...\n");
          activateLED(wifiEnabledLed);
          Serial.flush();
        }
      } else {
        Serial.printf("An update error occurred, #: %d\n" + Update.getError());
        Serial.flush();
      }
    } else {
      Serial.printf("Not enough space to start OTA update...\n");
      Serial.flush();
    }
    stream.flush();
  }
  else {
    // if autoconnect failed, disconnect to prevent weird wifi side-effects
    WiFi.disconnect();
  }
}

void loop() {
  // if connection drops, will attempt to reconnect
  if (isConnected && WiFi.status() != WL_CONNECTED){
    Serial.println("WiFi connection lost...");
    Serial.println("Attempting to reestablish...");
    attemptWifiConnect();
  }
  webServer.handleClient(); // serve pending web requests every loop
}

void startAP() {
  Serial.println("Starting AP...");
  activateLED(startAPLed);

  // append mac address to AP name
  apSSID = String("ESP32-LA1-");
  apSSID.concat(MAC_ADDRESS);

  if(! WiFi.mode(WIFI_AP_STA))
    Serial.println("Failed to set WiFi mode");
  else
    Serial.println("WiFi mode set successfully");

  if(! WiFi.softAP(apSSID, "password"))
    Serial.println("Failed to start soft AP");
  else 
    Serial.println("Started soft AP\n");
}

void initWebServer() {
  Serial.println("Starting web server...");
  activateLED(startServerLed);

  // register callbacks to handle different paths
  webServer.on("/", rootPage);                // slash
  webServer.on("/wifi", handleWifi);          // page for choosing an AP
  webServer.on("/wifichz", handleWifichz);    // landing page for AP form submit
  webServer.on("/status", handleStatus);      // status check, e.g. IP address

  webServer.begin();
  Serial.println("Web server started successfully\n");
}

// taken from Ex01
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

// following functions are taken from Ex10

// helper for downloading from cloud firmware server; for experimental
// purposes just use a hard-coded IP address and port (defined above)
int doCloudGet(HTTPClient *http, String fileName) {
  // build up URL from components; for example:
  // http://192.168.4.2:8000/Thing.bin
  String url =
    String("http://") + FIRMWARE_SERVER_IP_ADDR + ":" +
    FIRMWARE_SERVER_PORT + "/" + fileName;
  Serial.printf("getting %s\n", url.c_str());

  // make GET request and return the response code
  http->begin(url);
  http->addHeader("User-Agent", "ESP32");
  return http->GET();
}

// callback handler for tracking OTA progress ///////////////////////////////
void handleOTAProgress(size_t done, size_t total) {
  float progress = (float) done / (float) total;
  // dbf(otaDBG, "OTA written %d of %d, progress = %f\n", done, total, progress);

  int barWidth = 70;
  Serial.printf("[");
  int pos = barWidth * progress;
  for(int i = 0; i < barWidth; ++i) {
    if(i < pos)
      Serial.printf("=");
    else if(i == pos)
      Serial.printf(">");
    else
      Serial.printf(" ");
  }
  Serial.printf(
    "] %d %%%c", int(progress * 100.0), (progress == 1.0) ? '\n' : '\r'
  );
  // Serial.flush();
}