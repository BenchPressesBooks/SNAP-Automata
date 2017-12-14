/*
 * *******************************************************
 *      Standardized Device Deployment Baseline <S2B>
 *               Software Revision: 2.74
 * *******************************************************
 * 
 * COMPILE SELECTION:
 *   WiON or ECOPlug (Uncomment BUILTIN_LED line for this board below)
 *     Board:       Generic ESP8266 Module
 *     Flash Size:  1M (no SPIFFS)
 *     ALL OTHER SETTINGS DEFAULT
 *
 *   WeMos D1 Mini PRO
 *     Board:   WeMos D1 R2 & mini
 *     Flash Size:  4M (1M SPIFFS)
 *     ALL OTHER SETTINGS DEFAULT
 *
 * *******************************************************
 *   
 * This code allows for rapid debugging and deploymemt of
 * ESP8266 devices.
 * 
 * On boot, the device will print out its MAC address before
 * attempting to initialize a network connection. The device
 * can be immediately deployed with this code still running.
 * Once programming has been completed, the update server
 * should be pointed to the latest compiled binary. The client
 * device will automatically download the binary, install
 * and reboot.
 * 
 * Features:
 *  Serial DEBUGGER will print out connection information
 *  needed to configure the device for use on a network.
 *  
 *  Onboard LED NOT flashing: The wireless network connection
 *  hasn't been established or device has hung.
 *  
 *  Onboard LED IS flashing: The device is network connected
 *  and operational. Update checks will occur at the set
 *  time intervals as expected.
 * 
 * *******************************************************
 * 
 * Copyright 2017 Braden Licastro
 * www.bradenlicastro.com
 * www.github.com/benchpressesbooks
 * 
*/

// Required libraries
#include <ESP8266WiFi.h>    // https://github.com/esp8266/Arduino
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

// Libraries containing sensitive network config parameters
#include <Skynet-WiFi-Connect.h>
#include <Skynet-WiFi-OTA.h>

#define DEBUG

// ESP8266: SSID and password of desired network
const char*       AP_SSID           = WIFI_SSID;
const char*       AP_PASSWORD       = WIFI_PASSWORD;

// OTA Information
const char*       OTA_SERVER_IP     = OTA_SERV_IP;
const uint8_t     OTA_PORT          = OTA_SERV_PORT;

// WiFi and MQTT client object initialization.
WiFiClient        g_wifiClient;

// LED Pin Override (Only necessary for indicated devices below)
//#define BUILTIN_LED 2 // WiOn Outlet w. Generic ESP8266

// Other event handling.
const unsigned long LED_BLINK_DELAY   = 5000; // Delay in MS is 5 seconds
const unsigned long UPDATE_DELAY      = 300000; // Delay in MS is 5 mins
unsigned long       UPDATE_TIME       = 0;


//############################################################################
//
//                          CONTROL AND STATUS 
//
//############################################################################

/*
 * Sets the LED Status
 *   INPUT:  8-bit unsigned integer 'count' is the number of times that the LED should blink
 *           8-bit unsigned integer 'on_time' is the duration the LED should be turned on
 *           8-bit unsigned integer 'off_time' is the duration the LED should be off for
 *   RETURN: NA
 */
void setLED(uint8_t count, uint16_t on_time, uint16_t off_time) {
  // Cycle LED the appropriate number of times.
  for(int i = 0; i < count; i++) {
    digitalWrite(BUILTIN_LED, LOW);
    delay(on_time);
    digitalWrite(BUILTIN_LED, HIGH);
    delay(off_time);
  }
  return;
}


//############################################################################
//
//                          WIFI CONFIGURATION
//
//############################################################################

/*
 * Set up the connection to the Access Point
 *   INPUT:  NA
 *   RETURN: NA
*/
void setupWifi() {
  #ifdef DEBUG
    Serial.println(F("\nINFO:  Wireless network initializing..."));
    Serial.print(F("       MAC Address: "));
    Serial.println(WiFi.macAddress());
  #endif
  
  WiFi.mode(WIFI_STA); // Station mode. This is a client.
  WiFi.begin(AP_SSID, AP_PASSWORD);

  // Hold until wireless connection is up and running.
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  #ifdef DEBUG
    Serial.println(F("\nINFO:  Client is now connected to the wireless network!"));
    Serial.print(F("       IP address:  "));
    Serial.println(WiFi.localIP());
  #endif
}


//############################################################################
//
//                              OTA Updates
//
//############################################################################

/*
 * Function called to check for updates from the binary server
 *   INPUT:  NA
 *   Return: NA
 */
void checkForUpdates() {
  #ifdef DEBUG
    Serial.println(F("INFO:  OTA update check beginning..."));
  #endif

  // Make the server side call and hold onto the HTTP return status.
  t_httpUpdate_return state = ESPhttpUpdate.update(OTA_SERVER_IP, OTA_PORT, "/updater.php");

  // Do something based on the response from the server.
  #ifdef DEBUG
    switch (state) {
      case HTTP_UPDATE_FAILED:
        Serial.println(F("ERROR: OTA update failed."));
        break;
      case HTTP_UPDATE_NO_UPDATES:
        Serial.println(F("INFO:  OTA update not available. Already running latest version."));
        break;
      case HTTP_UPDATE_OK:
        Serial.println(F("INFO:  OTA update succeeded."));
        break;
    }
  #endif
}

//############################################################################
//
//                            SETUP and LOOP
//
//############################################################################

/*
 * System initialization
 */
void setup() {
  #ifdef DEBUG
    Serial.begin(115200);
  #endif

  // Connect to network.
  setupWifi();
  
  // Set IO pins for use.
  pinMode(BUILTIN_LED, OUTPUT);

  #ifdef DEBUG
    Serial.println("\nINFO: Boot complete! Beginning heartbeat.");
  #endif

}

/*
 * System run
 */
void loop() {
  // Ensure connection to wireless network is active, if not fix it.
  if (WiFi.status() != WL_CONNECTED) {
    setupWifi();
  }

  // Heartbeat using internal LED for state indication.
  setLED(5, LED_BLINK_DELAY, LED_BLINK_DELAY);
  
  // Check for code updates at a throttled pace.
  if (millis() - UPDATE_TIME >= UPDATE_DELAY) {
    //Check for update.
    checkForUpdates();

    // Set the last time of update check.
    UPDATE_TIME = millis();
  }
}
