/*
 * *******************************************************
 *        SNAP Automata Custom Device Deployment
 *               Software Revision: 2.91
 *             
 *          GARAGE MONITOR AND ACCESS SYSTEM
 * *******************************************************
 * 
 * COMPILE OPTIONS:
 *   SNAP Automata
 *     Board:   WeMos D1 R2 & mini
 *     Memory:  4M (1. SPIFFS)
 *     ALL OTHER SETTINGS DEFAULT
 * 
 * *******************************************************
 * 
 * Device: SNAP Automata | Model: WeMos D1 Mini Pro
 * Description:
 *    
 * 
 * Enabled Hardware Capabilities:
 *    Relay
 *    Magswitch Position Sensor
 *    Reset Switch
 *    Notifier LED for Garage Door and Device Status
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

// Device network details: IP address, IP gateway, subnet, dns.
// NOTE: WiFi does not support configurable MAC
const IPAddress   IP                (10,0,10,6);
const IPAddress   IP_GATEWAY        NET_GATEWAY;
const IPAddress   IP_SUBNET         NET_SUBNET;
const IPAddress   IP_DNS            NET_DNS;

// MQTT: client ID, broker IP, port, username & password
const char*       MQTT_CLIENT_ID    = "MQTT-GARAGEDOOR";
const char*       MQTT_SERVER_IP    = MQTT_SERV_IP;
const uint16_t    MQTT_SERVER_PORT  = MQTT_SERV_PORT;
const char*       MQTT_USERNAME     = MQTT_USER_INT;
const char*       MQTT_PASSWORD     = MQTT_PASS_INT;

// OTA Information
const char*       OTA_SERVER_IP     = OTA_SERV_IP;
const uint8_t     OTA_PORT          = OTA_SERV_PORT;

// MQTT: topics
// Interior: Living Room Side Lamp
const char*       TOPIC_MAGSWITCH_STATUS         = "basement/garagedoor/status";
const char*       TOPIC_RELAY_COMMAND            = "basement/garagedoor/switch";

// MQTT: Expected Payload
// Garage Door Action : "TRIGGER"
const char*       PAYLOAD_OPEN      = "ON";
const char*       PAYLOAD_CLOSED    = "OFF";

// Track current status of the devices
boolean           g_magswitch_status             = false;

// Buffer configuration used to send/receive data with MQTT
const uint8_t     MSG_BUFFER_SIZE   = 20;
char              g_msg_buffer[MSG_BUFFER_SIZE];

// Pin configuration
const uint8_t     RELAY_PIN         = D1;
const uint8_t     MAGSWITCH_PIN     = D2;
const uint8_t     NOTIFIER_LED_PIN  = D3;

// WiFi and MQTT client object initialization
WiFiClient        g_wifiClient;
PubSubClient      g_mqttClient(g_wifiClient);

// Other event handling
const uint16_t      SWITCH_CHECK_DELAY           = 2000; // Delay in MS is 2 sec
unsigned long       SWITCH_STATE_TIME            = 0;
const uint8_t       LED_BLINK_DELAY              = 150; // Delay in MS is .15 sec
const uint16_t      NOTIFIER_LED_DELAY           = 5000; // Delay in MS is 5 sec
unsigned long       NOTIFIER_LED_TIME            = 0;
const uint8_t       RELAY_HOLD_TIME              = 200; // Delay in MS is .2 sec
const unsigned long UPDATE_DELAY                 = 300000; // Delay in MS is 5 mins
unsigned long       UPDATE_TIME                  = 0;


//############################################################################
//
//                          CONTROL AND STATUS 
//
//############################################################################

/*
 * Publishes the status of the mag switch
 *   INPUT:  NA
 *   RETURN: NA
 */
void publishMagswitchStatus() {
  if (g_magswitch_status) {
    if (g_mqttClient.publish(TOPIC_MAGSWITCH_STATUS, PAYLOAD_OPEN, true)) {
      #ifdef DEBUG
        Serial.print(F("INFO:  MQTT message publish succeeded.\n       Topic: "));
        Serial.println(TOPIC_MAGSWITCH_STATUS);
        Serial.print(F("       Payload: "));
        Serial.println(PAYLOAD_OPEN);
      #endif
    } else {
      #ifdef DEBUG
            Serial.println(F("ERROR: MQTT message publish failed, either connection lost, or message too large"));
      #endif
    }
  } else {
    if (g_mqttClient.publish(TOPIC_MAGSWITCH_STATUS, PAYLOAD_CLOSED, true)) {
      #ifdef DEBUG
        Serial.print(F("INFO:  MQTT message publish succeeded.\n       Topic: "));
        Serial.println(TOPIC_MAGSWITCH_STATUS);
        Serial.print(F("       Payload: "));
        Serial.println(PAYLOAD_CLOSED);
      #endif
    } else {
      #ifdef DEBUG
        Serial.println(F("ERROR: MQTT message publish failed, either connection lost, or message too large"));
      #endif
    }
  }
}

/*
 * Sets the indicator LED Status
 *   INPUT:  8-bit unsigned integer 'count' is the number of times that the LED should blink
 *           8-bit unsigned integer 'on_time' is the duration the LED should be turned on
 *           8-bit unsigned integer 'off_time' is the duration the LED should be off for
 *   RETURN: NA
 */
void setNotifierLED(uint8_t count, uint16_t on_time, uint16_t off_time) {
  // Cycle LED the appropriate number of times.
  for(int i = 0; i < count; i++) {
    digitalWrite(NOTIFIER_LED_PIN, HIGH);
    #ifdef DEBUG
      digitalWrite(LED_BUILTIN, LOW);
    #endif
    delay(on_time);
    digitalWrite(NOTIFIER_LED_PIN, LOW);
    #ifdef DEBUG
      digitalWrite(LED_BUILTIN, HIGH);
    #endif
    delay(off_time);
  }
  return;
}

/*
 * Toggles the state of the relay
 *   INPUT:  NA
 *   RETURN: NA
 */
void toggleRelay() {
  // Use relay to simulate someone pressing a physical button. IE use a delay.  
  digitalWrite(RELAY_PIN, HIGH);
  delay(RELAY_HOLD_TIME);
  digitalWrite(RELAY_PIN, LOW);

  // Even though we don't care about the relay status, publish anyway to prevent devices from hanging.
  publishMagswitchStatus();
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
  
  // Act on the contents of the payload IF manual bypass isn't triggered for the given event.
  if (String(TOPIC_RELAY_COMMAND).equals(p_topic)) {
    // Type juggle the payload from char array to string.
    String payload;
    for (uint8_t i = 0; i < p_length; i++) {
      payload.concat((char)p_payload[i]);
    }

    // Perform payload action.
    if (payload.equals(String(PAYLOAD_OPEN)) || payload.equals(String(PAYLOAD_CLOSED))) {
      toggleRelay();
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

      // Publish the status of the magswitch to bring everything back into sync.
      publishMagswitchStatus();

      // Indicate connection to MQTT server via the notification LED.
      setNotifierLED(1,NOTIFIER_LED_DELAY,0);
    } else {
      #ifdef DEBUG
        Serial.print(F("ERROR: The connection failed with the MQTT broker.\n       Status: "));
        Serial.println(g_mqttClient.state());
      #endif
      
      // Wait before retrying instead of clobbering the server and indicate failure with LED.
      setNotifierLED(10, LED_BLINK_DELAY, LED_BLINK_DELAY);
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
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
  #endif

  // Connect to network.
  setupWifi();
  
  // Set IO pins for use.
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(MAGSWITCH_PIN, INPUT);
  pinMode(NOTIFIER_LED_PIN, OUTPUT);

  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(NOTIFIER_LED_PIN, LOW);

  // Configure and connect to MQTT broker.
  g_mqttClient.setServer(MQTT_SERVER_IP, MQTT_SERVER_PORT);
  g_mqttClient.setCallback(callback);
}

/*
 * System run
 */
void loop() {
  // Ensure we are connected to the wireless network, if not fix it.
  if (WiFi.status() != WL_CONNECTED) {
    setupWifi();
  } 

  // Ensure connection to MQTT broker is active, if not fix it.
  if (!g_mqttClient.connected()) {
    reconnect();
  }

  // Listen for messages from the broker.
  g_mqttClient.loop();

  // Check for magswitch status change after delay.
  if (millis() - SWITCH_STATE_TIME >= SWITCH_CHECK_DELAY) {
    if(g_magswitch_status == false && digitalRead(MAGSWITCH_PIN) == LOW) {
      // Read in the new state change.
      g_magswitch_status = !digitalRead(MAGSWITCH_PIN);
      #ifdef DEBUG
        Serial.println("INFO:  Garage door state switched from CLOSED to OPENED.");
      #endif
      publishMagswitchStatus();
    } else if (g_magswitch_status == true && digitalRead(MAGSWITCH_PIN) == HIGH) {
      // Read in the new state change.
      g_magswitch_status = !digitalRead(MAGSWITCH_PIN);
      #ifdef DEBUG
        Serial.println("INFO:  Garage door state switched from OPENED to CLOSED.");
      #endif
      publishMagswitchStatus();
    }
    // Set the last time of door state change.
    SWITCH_STATE_TIME = millis();
  }

  // Send visual warning.
  if(g_magswitch_status == true && millis() - NOTIFIER_LED_TIME >= NOTIFIER_LED_DELAY) {
    // Trigger LED
    setNotifierLED(3, LED_BLINK_DELAY, (LED_BLINK_DELAY*2));
    // Update the last time the LED was triggered.
    NOTIFIER_LED_TIME = millis();
  }

  // Check for code updates at a throttled pace.
  if (millis() - UPDATE_TIME >= UPDATE_DELAY) {
    // Check for update.
    checkForUpdates();

    // Set the last time of update check.
    UPDATE_TIME = millis();
  }
}
