#include "NetworkHelper.h"

// char apSSID[23];
WebServer webServer(80);
const char apSSID[10] = "Marvin-AP";

void connectToWifi() {
  WiFi.setHostname(HOST_NAME);
  WiFi.mode(WIFI_STA);

  WiFi.begin();

  attemptWifiConnect();

  if (WiFi.status() != WL_CONNECTED) {
    WiFi.disconnect();
    startAP();
    initWebServer();
    while (WiFi.status() != WL_CONNECTED) {
      // hold here until connected, handling web requests
      webServer.handleClient(); // serve pending web requests every loop
    }
    // successful connection, will now check for OTA update
    // most of the following is lightly adapted from Ex10
  }
  // else {
  //   // if autoconnect failed, disconnect to prevent weird wifi side-effects
  // }
  displayCentredAlertWithSubtext("Connected!", "Continuing setup...");
  performOTAUpdate();
}

// will attempt to connect to WiFi and set correct LED status
void attemptWifiConnect() {
  Serial.print("Attempting to connect to WiFi...");

  // makes 10 attempts
  uint16_t connectionTries = 0;
  while(WiFi.status() != WL_CONNECTED && connectionTries < 50) {
    if (connectionTries % 5 == 0)
      Serial.print(".");
    connectionTries++;
    delay(100);
  }

  // isConnected is used to check for WiFi drop in loop()
  if (WiFi.status() == WL_CONNECTED)
    Serial.printf("\n\nConnection to %s successful!\n\n", WiFi.SSID());
  else
    Serial.print("\nConnection failed.\n");
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

void performOTAUpdate() {
  delay(500);

  Serial.println("===============================");
  Serial.println("    Checking for updates...");
  Serial.println("===============================");

  Serial.printf("\nCurrent firmware version: %i\n", firmware_version);
  Serial.println("Checking for available updates...\n");

  HTTPClient http;
  int respCode;
  int highestAvailableVersion = -1;

  // read the version file from the cloud
  respCode = doCloudGet(&http, "version.txt");
  if(respCode > 0) // check response code (-ve on failure)
    highestAvailableVersion = atoi(http.getString().c_str());
  else {
    Serial.printf("\n Error - Couldn't get version! rtn code: %d\n", respCode);
  }

  http.end(); // free resources

  // do we know the latest version, and does the firmware need updating?
  if(respCode < 0) {
    return;
  } else if(firmware_version >= highestAvailableVersion) {
    Serial.println("\n Firmware is up to date.");
    return;
  }

  // do a firmware update
  Serial.printf(
    "\nUpgrading to firmware version %d\n\n",
    highestAvailableVersion
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
  // doesn't work for some reason :/
  // Update.onProgress(handleOTAProgress); // print out progress
  if(Update.begin(updateLength)) {
    displayUpdating();
    Serial.printf("Starting OTA update. This may take some time. DO NOT power off the device.\n");
    Update.writeStream(stream);
    if(Update.end()) {
      Serial.printf("Update complete! Now finishing...\n");
      Serial.flush();
      if(Update.isFinished()) {
        Serial.printf("Update successfully finished! Rebooting...\n\n");
        delay(500);
        ESP.restart();
      } else {
        Serial.printf("Update didn't finish correctly...\n");
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

void startAP() {
  displayCentredAlertWithSubtext("WiFi Error", "Connect to Marvin-AP");
  Serial.println("\nStarting AP...");

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

  // register callbacks to handle different paths
  webServer.on("/", rootPage);                // slash
  webServer.on("/wifi", handleWifi);          // page for choosing an AP
  webServer.on("/wifichz", handleWifichz);    // landing page for AP form submit
  webServer.on("/status", handleStatus);      // status check, e.g. IP address

  webServer.begin();
  Serial.println("Web server started successfully\n");
}

void accessPointForm(String& f);
String ip2string(IPAddress address);

// taken from Ex07
typedef struct { int position; const char *replacement; } replacement_t;
void getHtml(String& html, const char *[], int, replacement_t [], int);
// getting the length of an array in C can be complex...
// https://stackoverflow.com/questions/37538/how-do-i-determine-the-size-of-my-array-in-c
#define ALEN(a) ((int) (sizeof(a) / sizeof(a[0]))) // only in definition scope!
#define GET_HTML(strout, boiler, repls) \
  getHtml(strout, boiler, ALEN(boiler), repls, ALEN(repls));

// taken from Ex07
void getHtml( // turn array of strings & set of replacements into a String
  String& html, const char *boiler[], int boilerLen,
  replacement_t repls[], int replsLen
) {
  for(int i = 0, j = 0; i < boilerLen; i++) {
    if(j < replsLen && repls[j].position == i)
      html.concat(repls[j++].replacement);
    else
      html.concat(boiler[i]);
  }
}

// taken from Ex09
const char *templatePage[] = {
  "<html><head><title>",                                                //  0
  "default title",                                                      //  1
  "</title>\n",                                                         //  2
  "<meta charset='utf-8'>",                                             //  3
  "<meta name='viewport' content='width=device-width, initial-scale=1.0'>\n"
  "<style>body{background:#FFF; color: #000; font-family: sans-serif;", //  4
  "font-size: 150%;}</style>\n",                                        //  5
  "</head><body>\n",                                                    //  6
  "<h2>Marvin config</h2>\n",                              //  7
  "<!-- page payload goes here... -->\n",                               //  8
  "<!-- ...and/or here... -->\n",                                       //  9
  "\n<p><a href='/'>Home</a>&nbsp;&nbsp;&nbsp;</p>\n",                  // 10
  "</body></html>\n\n",                                                 // 11
};


// taken from Ex09
void rootPage() {
  Serial.println("Serving /...");

  replacement_t repls[] = { // the elements to replace in the boilerplate
    {  1, "Select WiFi Network" },
    {  8, "" },
    {  9, "<p>Choose a <a href=\"wifi\">wifi access point</a>.</p>" },
    { 10, "<p>Check <a href='/status'>wifi status</a>.</p>" },
  };
  String htmlPage = ""; // a String to hold the resultant page
  GET_HTML(htmlPage, templatePage, repls); // GET_HTML sneakily added to Ex07
  webServer.send(200, "text/html", htmlPage);
}


// taken from Ex09
void handleWifi() {
  Serial.println("Serving /wifi...");

  String form = ""; // a form for choosing an access point and entering key
  accessPointForm(form);
  replacement_t repls[] = { // the elements to replace in the boilerplate
    { 1, "Choose WiFi connection" },
    { 7, "<h2>Network configuration</h2>\n" },
    { 8, "" },
    { 9, form.c_str() },
  };
  String htmlPage = ""; // a String to hold the resultant page
  GET_HTML(htmlPage, templatePage, repls); // GET_HTML sneakily added to Ex07

  webServer.send(200, "text/html", htmlPage);
}

// adapted from Ex09
// modifications include:
//  - modifying page shown to user based on connection result
//  - 10 connection attempts before timeout and wifi disconnects
//    to prevent strange wifi side-effects
//  - activating LEDs at correct points to meet LA1 rubric
void handleWifichz() {
  Serial.println("Serving /wifichz...");

  String title = "<h2>Joining wifi network...</h2>";
  String message = "<p>Check <a href='/status'>wifi status</a>.</p>";

  String ssid = "";
  String key = "";
  for(uint8_t i = 0; i < webServer.args(); i++ ) {
    if(webServer.argName(i) == "ssid")
      ssid = webServer.arg(i);
    else if(webServer.argName(i) == "key")
      key = webServer.arg(i);
  }

  if(ssid == "") {
    message = "<h2>Ooops, no SSID...?</h2>\n<p>Looks like a bug :-(</p>";
  } else {
    char ssidchars[ssid.length()+1];
    char keychars[key.length()+1];
    ssid.toCharArray(ssidchars, ssid.length()+1);
    key.toCharArray(keychars, key.length()+1);
    WiFi.begin(ssidchars, keychars);

    attemptWifiConnect();

    if (WiFi.status() == WL_CONNECTED) {
      title = "<h2>Successfully connected!</h2>";
    }
    else {
      title = "<h2>Connection failed</h2>";
      message = "<p><a href='/wifi'>Check available networks</a> and try again.</p>";

      WiFi.disconnect(); // discconect from wifi to stop weird connection issues
    }
  }


  replacement_t repls[] = { // the elements to replace in the template
    { 1, "Joining Network..." },
    { 7, title.c_str() },
    { 8, "" },
    { 9, message.c_str() },
  };
  String htmlPage = "";     // a String to hold the resultant page
  GET_HTML(htmlPage, templatePage, repls);

  webServer.send(200, "text/html", htmlPage);
}

// taken from Ex09
void handleStatus() {
  Serial.println("Serving /status...");
  // isWifiConnecting = false;

  String s = "";
  s += "<ul>\n";
  s += "\n<li>SSID: ";
  s += WiFi.SSID();
  s += "</li>";
  s += "\n<li>Status: ";
  switch(WiFi.status()) {
    case WL_IDLE_STATUS:
      s += "WL_IDLE_STATUS</li>"; break;
    case WL_NO_SSID_AVAIL:
      s += "WL_NO_SSID_AVAIL</li>"; break;
    case WL_SCAN_COMPLETED:
      s += "WL_SCAN_COMPLETED</li>"; break;
    case WL_CONNECTED:
      s += "WL_CONNECTED</li>"; break;
    case WL_CONNECT_FAILED:
      s += "WL_CONNECT_FAILED</li>"; break;
    case WL_CONNECTION_LOST:
      s += "WL_CONNECTION_LOST</li>"; break;
    case WL_DISCONNECTED:
      s += "WL_DISCONNECTED</li>"; break;
    default:
      s += "unknown</li>";
  }

  s += "\n<li>Local IP: ";     s += ip2string(WiFi.localIP());
  s += "</li>\n";
  s += "\n<li>Soft AP IP: ";   s += ip2string(WiFi.softAPIP());
  s += "</li>\n";
  s += "\n<li>AP SSID name: "; s += apSSID;
  s += "</li>\n";

  s += "</ul></p>";

  replacement_t repls[] = { // the elements to replace in the boilerplate
    { 1, "Connection Status" },
    { 7, "<h2>Status</h2>\n" },
    { 8, "" },
    { 9, s.c_str() },
  };
  String htmlPage = ""; // a String to hold the resultant page
  GET_HTML(htmlPage, templatePage, repls); // GET_HTML sneakily added to Ex07

  webServer.send(200, "text/html", htmlPage);
}

// taken from Ex09
void accessPointForm(String& f) { // utility to create a form for choosing AP
  const char *checked = " checked";
  int n = WiFi.scanNetworks();
  Serial.println("Scan done: ");

  if(n == 0) {
    Serial.println("No networks found");
    f += "No wifi access points found :-( ";
    f += "<a href='/'>Back</a><br/><a href='/wifi'>Try again?</a></p>\n";
  } else {
    Serial.printf("%d networks found\n", n);
    f += "<p>Wifi access points available:</p>\n"
         "<p><form method='POST' action='wifichz'> ";
    for(int i = 0; i < n; ++i) {
      f.concat("<input type='radio' name='ssid' value='");
      f.concat(WiFi.SSID(i));
      f.concat("'");
      f.concat(checked);
      f.concat(">");
      f.concat(WiFi.SSID(i));
      f.concat(" (");
      f.concat(WiFi.RSSI(i));
      f.concat(" dBm)");
      f.concat("<br/>\n");
      checked = "";
    }
      f += "<br/>Pass key: <input type='textarea' name='key'><br/><br/> ";
      f += "<input type='submit' value='Submit'></form></p>";
  }
}

// taken from Ex09
String ip2string(IPAddress address) { // utility for printing IP addresses
  return
    String(address[0]) + "." + String(address[1]) + "." +
    String(address[2]) + "." + String(address[3]);
}