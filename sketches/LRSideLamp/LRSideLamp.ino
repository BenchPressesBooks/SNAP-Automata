/*
 * Configuration for Home Assistant:
 *   light:
 *   # Interior: Living Room Side Lamp
 *   - platform: mqtt
 *     name: 'Living Room Side Lamp'
 *     state_topic: 'firstfloor/lrsidelamp/status'
 *     command_topic: 'firstfloor/lrsidelamp/switch'
 *     optimistic: false
 */

#include <ESP8266WiFi.h>    // https://github.com/esp8266/Arduino (GNUv2.1 licence)
#include <PubSubClient.h>   // https://github.com/knolleary/pubsubclient (no licence)
#include <ArduinoOTA.h>


#define DEBUG
#define MQTT_VERSION MQTT_VERSION_3_1_1

// ESP8266: SSID and password of desired network
const char*       AP_SSID           = "<REDACTED>";
const char*       AP_PASSWORD       = "<REDACTED>";

// Device network details: IP address, IP gateway, subnet, dns.
// NOTE: WiFi does not support configurable MAC
const IPAddress   IP                (<REDACTED>);
const IPAddress   IP_GATEWAY        (<REDACTED>);
const IPAddress   IP_SUBNET         (<REDACTED>);
const IPAddress   IP_DNS            (<REDACTED>);

// MQTT: client ID, broker IP, port, username & password
const char*       MQTT_CLIENT_ID    = "<REDACTED>";
const char*       MQTT_SERVER_IP    = "<REDACTED>";
const uint16_t    MQTT_SERVER_PORT  = 1883;
const char*       MQTT_USERNAME     = "<REDACTED>";
const char*       MQTT_PASSWORD     = "<REDACTED>";

// OTA: hostname & password
const char*       OTA_HOSTNAME      = MQTT_CLIENT_ID;
const char*       OTA_PASSWORD      = "<REDACTED>";
const uint16_t    OTA_PORT          = 8266;

// MQTT: topics
// Interior: Living Room Side Lamp
const char*       TOPIC_RELAY_STATUS             = "firstfloor/lrsidelamp/status";
const char*       TOPIC_RELAY_COMMAND            = "firstfloor/lrsidelamp/switch";

// MQTT: Expected Payloads
// Lamps : "ON"/"OFF"
const char*       PAYLOAD_ON        = "ON";
const char*       PAYLOAD_OFF       = "OFF";

// Track current status of the devices
boolean           g_relay_status                 = false;
uint8_t           g_bypass_activated             = 0;
boolean           g_bypass_hold                  = false;
boolean           g_bypass_type                  = true; // True = switch; False = button

// Buffer configuration used to send/receive data with MQTT
const uint8_t     MSG_BUFFER_SIZE   = 20;
char              g_msg_buffer[MSG_BUFFER_SIZE];

// Pin configuration
const uint8_t     RELAY_PIN         = D1;
const uint8_t     BYPASS_PIN        = D2;

// WiFi and MQTT client object initialization
WiFiClient        g_wifiClient;
PubSubClient      g_mqttClient(g_wifiClient);

// Other event handling
const uint8_t     DEBOUNCE_DELAY    = 200; // Delay in MS
long              DEBOUNCE_TIME     = 0;

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
  delay(10);
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
    Serial.print(F("       MAC Address: "));
    Serial.println(WiFi.macAddress());
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
  if (String(TOPIC_RELAY_COMMAND).equals(p_topic) && !g_bypass_hold) {
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
  } else if (g_bypass_hold) {
    publishRelayStatus();
    #ifdef DEBUG
      Serial.println(F("INFO:  Bypass set, aborting requested action."));
    #endif
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
    } else {
      #ifdef DEBUG
        Serial.print(F("ERROR: The connection failed with the MQTT broker.\n       Status: "));
        Serial.println(g_mqttClient.state());
      #endif
      
      // Wait before retrying instead of clobbering the server.
      delay(1000);
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
    Serial.println(F("\nINFO:  Wireless network initializing..."));
  #endif

  // Connect to network.
  setupWifi();
  
  // Set IO pins for use.
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BYPASS_PIN, INPUT);

  // Configure and connect to MQTT broker.
  g_mqttClient.setServer(MQTT_SERVER_IP, MQTT_SERVER_PORT);
  g_mqttClient.setCallback(callback);

  // Configure and initialize OTA listener.
  ArduinoOTA.setHostname(OTA_HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.setPort(OTA_PORT);
  
  #ifdef DEBUG
    ArduinoOTA.onStart([]() {
      Serial.println(F("INFO:  OTA session started."));
    });
    ArduinoOTA.onEnd([]() {
      Serial.println(F("INFO:  OTA session ended."));
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("INFO:  OTA rogress is %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
      Serial.printf("ERROR: [%u] ", error);
      if (error == OTA_AUTH_ERROR) {
        Serial.println("Auth Failed");
      } else if (error == OTA_BEGIN_ERROR) {
        Serial.println("Begin Failed");
      } else if (error == OTA_CONNECT_ERROR) {
        Serial.println("Connect Failed");
      } else if (error == OTA_RECEIVE_ERROR) {
        Serial.println("Receive Failed");
      } else if (error == OTA_END_ERROR) {
        Serial.println("End Failed");
      }
    });
  #endif

  ArduinoOTA.begin();
}

/*
 * System run
 */
void loop() {
  // Ensure connection to MQTT broker is active, if not fix it.
  if (!g_mqttClient.connected()) {
    reconnect();
  }

  // Listen for messages from the broker.
  g_mqttClient.loop();

  // Listen for OTA messages
  ArduinoOTA.handle();

  // Prevent bouncing when performing actions following a read.
  if (millis() - DEBOUNCE_TIME >= DEBOUNCE_DELAY) {
    if(g_bypass_activated != digitalRead(BYPASS_PIN)) {
      //Read in the new state change.
      g_bypass_activated = digitalRead(BYPASS_PIN);
      
      // Perform the requested action
      if (g_relay_status == false && g_bypass_activated == HIGH) {
        #ifdef DEBUG
          Serial.println("INFO:  Bypass state switched from OFF to ON.");
        #endif
        g_relay_status = true;
        // Hold state if switch; prevents MQTT from overriding physical user.
        if(g_bypass_type) {
          g_bypass_hold = true;
        }
        setRelayStatus();
        publishRelayStatus();
      } else if (g_relay_status == true && g_bypass_activated == LOW) {
        #ifdef DEBUG
          Serial.println("INFO:  Bypass state switched from ON to OFF");
        #endif
        g_relay_status = false;
        // Release state if switch; prevents MQTT from overriding physical user.
        if(g_bypass_type) {
          g_bypass_hold = false;
        }
        setRelayStatus();
        publishRelayStatus();
      } else {
        #ifdef DEBUG
          Serial.println("INFO:  Requested bypass state matches current state.");
        #endif
        // Publish status anyway to prevent client hangs.
        publishRelayStatus();
      }
    }
    // Set the last time of button press.
    DEBOUNCE_TIME = millis();
  }
}
