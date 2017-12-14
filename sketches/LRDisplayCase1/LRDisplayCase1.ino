/*
 * *******************************************************
 *       Woods WiON and ECOPlug Device Deployment
 *             Software Revision: 1.62
 *             
 *            LIVING ROOM DISPLAYCASE 1
 * *******************************************************
 * 
 * COMPILE OPTIONS:
 *   WiON or ECOPlug
 *     Board:   Generic ESP8266 Module
 *     Memory:  1M (no SPIFFS)
 *     ALL OTHER SETTINGS DEFAULT
 * 
 * *******************************************************
 * 
 * Device: Woods WiOn | Model 50050
 * Description:
 *    Indoor Wi-Fi outlet. 125v. 15A General, 8.3A Ballast
 * 
 * Enabled Hardware Capabilities:
 *    LED Indicator
 *    Relay
 *    Bypass Button
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
#include <PubSubClient.h>   // https://github.com/knolleary/pubsubclient

// Libraries containing sensitive network config parameters
#include <Skynet-WiFi-Connect.h>
#include <Skynet-WiFi-OTA.h>
#include <Skynet-Infra.h>
#include <Skynet-MQTT.h>

//#define DEBUG
#define MQTT_VERSION MQTT_VERSION_3_1_1

// ESP8266: SSID and password of desired network
const char*       AP_SSID           = WIFI_SSID;
const char*       AP_PASSWORD       = WIFI_PASSWORD;

// Device network details: IP address, IP gateway, subnet, dns
// NOTE: WiFi does not support configurable MAC
const IPAddress   IP                (10,0,10,3);
const IPAddress   IP_GATEWAY        NET_GATEWAY;
const IPAddress   IP_SUBNET         NET_SUBNET;
const IPAddress   IP_DNS            NET_DNS;

// MQTT: client ID, broker IP, port, username & password
const char*       MQTT_CLIENT_ID    = "MQTT-LRDISPLAYCASE1";
const char*       MQTT_SERVER_IP    = MQTT_SERV_IP;
const uint16_t    MQTT_SERVER_PORT  = MQTT_SERV_PORT;
const char*       MQTT_USERNAME     = MQTT_USER_INT;
const char*       MQTT_PASSWORD     = MQTT_PASS_INT;

// OTA Information
const char*       OTA_SERVER_IP     = OTA_SERV_IP;
const uint8_t     OTA_PORT          = OTA_SERV_PORT;

// MQTT: topics
const char*       TOPIC_RELAY_STATUS         = "firstfloor/lrdisplaycase1/status";
const char*       TOPIC_RELAY_COMMAND        = "firstfloor/lrdisplaycase1/switch";

// MQTT: Expected Payloads
// Pump : "ON"/"OFF"
const char*       PAYLOAD_ON      = "ON";
const char*       PAYLOAD_OFF     = "OFF";

// Track current status of the devices
boolean           g_relay_status             = false;
uint8_t           g_bypass_activated         = 0;

// Buffer configuration used to send/receive data with MQTT
const uint8_t     MSG_BUFFER_SIZE   = 20;
char              g_msg_buffer[MSG_BUFFER_SIZE];

// Pin configuration
const uint8_t     RELAY_PIN       = 15; // LOW is off
const uint8_t     BYPASS_PIN      = 13; // HIGH is unpressed
const uint8_t     LED_PIN         = 2;  // HIGH is off

// WiFi and MQTT client object initialization
WiFiClient        g_wifiClient;
PubSubClient      g_mqttClient(g_wifiClient);

// Other event handling
const uint8_t       DEBOUNCE_DELAY           = 200; // Delay in MS is .2 sec
unsigned long       DEBOUNCE_TIME            = 0;
const uint8_t       WIFI_LED_BLINK_DELAY     = 150; // Delay in MS is .15 sec
const unsigned long UPDATE_DELAY             = 300000; // Delay in MS is 5 mins
unsigned long       UPDATE_TIME              = 0;


//############################################################################
//
//                          CONTROL AND STATUS 
//
//############################################################################

/*
 * Publishes the status of the relay
 *   INPUT:  NA
 *   RETURN: NA
 */
void publishRelayStatus() {
  if (g_relay_status) {
    if (g_mqttClient.publish(TOPIC_RELAY_STATUS, PAYLOAD_ON, true)) {
      #ifdef DEBUG
        Serial.print(F("INFO:  MQTT message publish succeeded.\n       Topic: "));
        Serial.println(TOPIC_RELAY_STATUS);
        Serial.print(F("       Payload: "));
        Serial.println(PAYLOAD_ON);
      #endif
    } else {
      #ifdef DEBUG
            Serial.println(F("ERROR: MQTT message publish failed, either connection lost, or message too large"));
      #endif
    }
  } else {
    if (g_mqttClient.publish(TOPIC_RELAY_STATUS, PAYLOAD_OFF, true)) {
      #ifdef DEBUG
        Serial.print(F("INFO:  MQTT message publish succeeded.\n       Topic: "));
        Serial.println(TOPIC_RELAY_STATUS);
        Serial.print(F("       Payload: "));
        Serial.println(PAYLOAD_OFF);
      #endif
    } else {
      #ifdef DEBUG
        Serial.println(F("ERROR: MQTT message publish failed, either connection lost, or message too large"));
      #endif
    }
  }
}

/*
 * Sets the WiFi LED Status
 *   INPUT:  8-bit unsigned integer 'count' is the number of times that the LED should blink
 *           8-bit unsigned integer 'on_time' is the duration the LED should be turned on
 *           8-bit unsigned integer 'off_time' is the duration the LED should be off for
 *   RETURN: NA
 */
void setLED(uint8_t count, uint16_t on_time, uint16_t off_time) {
  // Cycle LED the appropriate number of times.
  for(int i = 0; i < count; i++) {
    digitalWrite(LED_PIN, LOW);
    delay(on_time);
    digitalWrite(LED_PIN, HIGH);
    delay(off_time);
  }
  return;
}

/*
 * Switches the state of the relay
 *   INPUT:  NA
 *   RETURN: NA
 */
void setRelayStatus() {
  if (g_relay_status) {
    digitalWrite(RELAY_PIN, HIGH);
  } else {
    digitalWrite(RELAY_PIN, LOW);
  }
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
  WiFi.config(IP, IP_GATEWAY, IP_SUBNET, IP_DNS); // MAC address configuration only supported for ethernet devices.

  // Hold until wireless connection is up and running.
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  #ifdef DEBUG
    Serial.println(F("INFO:  Client is now connected to the wireless network!"));
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

  // Make the server side call and hold onto the HTTP return status
  t_httpUpdate_return state = ESPhttpUpdate.update(OTA_SERVER_IP, OTA_PORT, "/updater.php");

  // Do something based on the response from the server
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
//                                MQTT
//
//############################################################################

/*
 * Function called when a MQTT message arrives
 *   INPUT:  Charanter array 'p_topic' is the topic of the MQTT message
 *           Byte array 'p_payload' is the payload of the MQTT message
 *           Integer 'p_length' is the expectedlength of the payload
 *   RETURN: NA
 */
void callback(char* p_topic, byte* p_payload, unsigned int p_length) {
  #ifdef DEBUG
    Serial.println(F("INFO:  A new MQTT message has arrived"));
    Serial.print(F("       Topic:   "));
    Serial.println(p_topic);
    Serial.print(F("       Payload: "));
    for (int i = 0; i < p_length; i++) {
      Serial.print((char)p_payload[i]);
    }
    Serial.println();
    Serial.print(F("       Length:  "));
    Serial.println(p_length);
  #endif
  
  // Act on the contents of the payload.
  if (String(TOPIC_RELAY_COMMAND).equals(p_topic)) {
    // Type juggle the payload from char array to string.
    String payload;
    for (uint8_t i = 0; i < p_length; i++) {
      payload.concat((char)p_payload[i]);
    }

    // Perform payload action.
    if (payload.equals(String(PAYLOAD_ON))) {
      g_relay_status = true;
      setRelayStatus();
      publishRelayStatus();
    } else if (payload.equals(String(PAYLOAD_OFF))) {
      g_relay_status = false;
      setRelayStatus();
      publishRelayStatus();
    } else {
      #ifdef DEBUG
        Serial.println(F("ERROR: The payload of the MQTT message is not valid"));
      #endif
    }
  } else {
    #ifdef DEBUG
      Serial.println(F("INFO:  The received MQTT message was for a topic we did not subscribe to and was not used."));
    #endif
  }
}

/*
 * Function called to reconnect the client to the MQTT broker and publish/subscribe to/from some MQTT topics
 *   INPUT:  NA
 *   Return: NA
 */
void reconnect() {
  while (!g_mqttClient.connected()) {
    if (g_mqttClient.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
      #ifdef DEBUG
        Serial.println(F("INFO:  The client is successfully connected to the MQTT broker"));
      #endif

      // Subscribe to the command topic.
      if (g_mqttClient.subscribe(TOPIC_RELAY_COMMAND)) {
        #ifdef DEBUG
          Serial.print(F("INFO:  Sending the MQTT subscribe succeeded.\n       Topic: "));
          Serial.println(TOPIC_RELAY_COMMAND);
        #endif
      } else {
        #ifdef DEBUG
          Serial.print(F("ERROR: Sending the MQTT subscribe failed.\n       Topic: "));
          Serial.println(TOPIC_RELAY_COMMAND);
        #endif
      }

      // Set the relay to match the value that we have stored.
      setRelayStatus();

      // Publish the status of the relay to bring everything back into sync.
      publishRelayStatus();

      // Indicate connnection to MQTT server via the WiFi LED.
      digitalWrite(LED_PIN, LOW);
    } else {
      #ifdef DEBUG
        Serial.print(F("ERROR: The connection failed with the MQTT broker.\n       Status: "));
        Serial.println(g_mqttClient.state());
      #endif
      
      // Wait before retrying instead of clobbering the server and indicate failure with LED.
      setLED(10, WIFI_LED_BLINK_DELAY, WIFI_LED_BLINK_DELAY);
    }
  }
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
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BYPASS_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);

  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(LED_PIN, HIGH);

  // Configure and connect to MQTT broker.
  g_mqttClient.setServer(MQTT_SERVER_IP, MQTT_SERVER_PORT);
  g_mqttClient.setCallback(callback);
}

/*
 * System run
 */
void loop() {
  // Ensure connection to wireless network is active, if not fix it.
  if (WiFi.status() != WL_CONNECTED) {
    setupWifi();
  }

  // Ensure connection to MQTT broker is active, if not fix it.
  if (!g_mqttClient.connected()) {
    reconnect();
  }

  // Listen for messages from the broker.
  g_mqttClient.loop();

  // Prevent bouncing when performing actions following a read.
  if (millis() - DEBOUNCE_TIME >= DEBOUNCE_DELAY) {
    if(g_bypass_activated != !digitalRead(BYPASS_PIN)) {
      // Read in the new state change.
      g_bypass_activated = !digitalRead(BYPASS_PIN);
      
      // Perform the requested action
      if (g_bypass_activated == HIGH) {
        if (g_relay_status == false) {
          #ifdef DEBUG
            Serial.println("INFO:  Bypass state switched from OFF to ON.");
          #endif
          g_relay_status = true;
          setRelayStatus();
          publishRelayStatus();
        } else if (g_relay_status == true) {
          #ifdef DEBUG
            Serial.println("INFO:  Bypass state switched from ON to OFF");
          #endif
          g_relay_status = false;
          setRelayStatus();
          publishRelayStatus();
        } else {
          #ifdef DEBUG
            Serial.println("INFO:  Requested bypass state matches current state.");
          #endif
        }
      }
    }
    // Set the last time of button press.
    DEBOUNCE_TIME = millis();
  }

  // Check for code updates at a throttled pace.
  if (millis() - UPDATE_TIME >= UPDATE_DELAY) {
    // Check for update.
    checkForUpdates();

    // Set the last time of update check.
    UPDATE_TIME = millis();
  }
}
