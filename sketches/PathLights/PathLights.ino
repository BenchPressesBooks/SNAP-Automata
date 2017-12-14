/*
 * *******************************************************
 *   Arduino Uno and SeeedStudio W5200 Device Deployment
 *               Software Revision: 2.19
 *             
 *           EXTERIOR PATH LIGHT CONTROLLER
 * *******************************************************
 * 
 * COMPILE OPTIONS:
 *   Arduino Uno Based Device
 *     Board:   Arduino/Genuino Uno
 *     ALL OTHER SETTINGS DEFAULT
 * 
 * *******************************************************
 * 
 * Device: Arduino Uno with SeeedStudio W5200 Shield
 * Description:
 *    Indoor/Outdoor enclosed relay and output system.
 *    Emulates photo eye sensor on outdoor transformer
 *    to trigger landscape lighting. Relays used to
 *    trip 12vAC photo eye input. Transformers must be
 *    set to "Automatic - Daylight" option to work.
 * 
 * Enabled Hardware Capabilities:
 *    Four Channel Relay Selector
 *    Three Bypass Switches
 *    Reset Button
 * 
 * Copyright 2017 Braden Licastro
 * www.bradenlicastro.com
 * www.github.com/benchpressesbooks
 * 
*/

// Required libraries
#include <EthernetV2_0.h>   // https://github.com/Seeed-Studio/Ethernet_Shield_W5200 (For the Seeedstudio W5200 ethernet shield)
#include <PubSubClient.h>   // https://github.com/knolleary/pubsubclient

// Libraries containing sensitive network config parameters
#include <Skynet-Infra.h>
#include <Skynet-MQTT.h>

//#define DEBUG
#define MQTT_VERSION MQTT_VERSION_3_1_1

// Device network details: IP address, IP gateway, subnet, dns.
const IPAddress   IP                (10, 0, 10, 2);
byte mac[] = {0xDE, 0x05, 0x97, 0x2C, 0x61, 0xB3};
const IPAddress   IP_GATEWAY        NET_GATEWAY;
const IPAddress   IP_SUBNET         NET_SUBNET;
const IPAddress   IP_DNS            NET_DNS;

// MQTT: client ID, broker IP address, port, username & password
const char*       MQTT_CLIENT_ID    = "MQTT-PATHLIGHTCONTROL";
const char*       MQTT_SERVER_IP    = MQTT_SERV_IP;
const uint16_t    MQTT_SERVER_PORT  = MQTT_SERV_PORT;
const char*       MQTT_USERNAME     = MQTT_USER_EXT;
const char*       MQTT_PASSWORD     = MQTT_PASS_EXT;

// MQTT: topics
// Zone 1
const char*       TOPIC_ZONE1_STATUS             = "exterior/plfront/status";
const char*       TOPIC_ZONE1_COMMAND            = "exterior/plfront/switch";
// Zone 2
const char*       TOPIC_ZONE2_STATUS             = "exterior/plside/status";
const char*       TOPIC_ZONE2_COMMAND            = "exterior/plside/switch";
// Zone 3
const char*       TOPIC_ZONE3_STATUS             = "exterior/plrear/status";
const char*       TOPIC_ZONE3_COMMAND            = "exterior/plrear/switch";
// Zone 4
const char*       TOPIC_ZONE4_STATUS             = "exterior/unused/status";
const char*       TOPIC_ZONE4_COMMAND            = "exterior/unused/switch";

// MQTT: Expected Payloads
// Zones 1 , 2, 3 & 4: "ON"/"OFF"
const char*       PAYLOAD_ON            = "ON";
const char*       PAYLOAD_OFF           = "OFF";

// Track current status of the devices
boolean           g_zone1_status             = false;
int               g_zone1_bypass_activated   = 0;
boolean           g_zone2_status             = false;
int               g_zone2_bypass_activated   = 0;
boolean           g_zone3_status             = false;
int               g_zone3_bypass_activated   = 0;
boolean           g_zone4_status             = false;

// Buffer configuarion used to send/receive data with MQTT
const uint8_t     MSG_BUFFER_SIZE       = 20;
char              g_msg_buffer[MSG_BUFFER_SIZE];

// Pin Configuration
// W5200 also uses pins D10, D11, D12, D13
const uint8_t     ZONE1_PIN            = 8;
const uint8_t     ZONE2_PIN            = 7;
const uint8_t     ZONE3_PIN            = 6;
const uint8_t     ZONE4_PIN            = 5;
const uint8_t     SDCARD_PIN           = 4;
const uint8_t     BYPASS_ZONE1_PIN     = 3;
const uint8_t     BYPASS_ZONE2_PIN     = 2;
const uint8_t     BYPASS_ZONE3_PIN     = 9;

// Ethernet and MQTT client object initialization
EthernetClient    g_ethernetClient;
PubSubClient      g_mqttClient(g_ethernetClient);

// Other event handling
const uint8_t     DEBOUNCE_DELAY    = 200; // Delay in MS
long              DEBOUNCE_TIME     = 0;


//############################################################################
//
//                          Zone 1 - Front
//
//############################################################################

/*
 * Publishes the status of zone 1
 *   INPUT:  NA
 *   RETURN: NA
 */
void publishZone1Status() {
  if (g_zone1_status) {
    if (g_mqttClient.publish(TOPIC_ZONE1_STATUS, PAYLOAD_ON, true)) {
#ifdef DEBUG
      Serial.print(F("INFO: MQTT message publish succeeded. Topic: "));
      Serial.print(TOPIC_ZONE1_STATUS);
      Serial.print(F(". Payload: "));
      Serial.println(PAYLOAD_ON);
#endif
    } else {
#ifdef DEBUG
      Serial.println(F("ERROR: MQTT message publish failed, either connection lost, or message too large"));
#endif
    }
  } else {
    if (g_mqttClient.publish(TOPIC_ZONE1_STATUS, PAYLOAD_OFF, true)) {
#ifdef DEBUG
      Serial.print(F("INFO: MQTT message publish succeeded. Topic: "));
      Serial.print(TOPIC_ZONE1_STATUS);
      Serial.print(F(". Payload: "));
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
 * Switches the state of the zone 1 relay
 *   INPUT:  NA
 *   RETURN: NA
 */
void setZone1Status() {
  if (g_zone1_status) {
    digitalWrite(ZONE1_PIN, LOW);
  } else {
    digitalWrite(ZONE1_PIN, HIGH);
  }
}

//############################################################################
//
//                            Zone 2 - Side
//
//############################################################################

/*
 * Publishes the status of zone 2
 *   INPUT:  NA
 *   RETURN: NA
 */
void publishZone2Status() {
  if (g_zone2_status) {
    if (g_mqttClient.publish(TOPIC_ZONE2_STATUS, PAYLOAD_ON, true)) {
#ifdef DEBUG
      Serial.print(F("INFO: MQTT message publish succeeded. Topic: "));
      Serial.print(TOPIC_ZONE2_STATUS);
      Serial.print(F(". Payload: "));
      Serial.println(PAYLOAD_ON);
#endif
    } else {
#ifdef DEBUG
      Serial.println(F("ERROR: MQTT message publish failed, either connection lost, or message too large"));
#endif
    }
  } else {
    if (g_mqttClient.publish(TOPIC_ZONE2_STATUS, PAYLOAD_OFF, true)) {
#ifdef DEBUG
      Serial.print(F("INFO: MQTT message publish succeeded. Topic: "));
      Serial.print(TOPIC_ZONE2_STATUS);
      Serial.print(F(". Payload: "));
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
 * Switches the state of the zone 2 relay
 *   INPUT:  NA
 *   RETURN: NA
 */
void setZone2Status() {
  if (g_zone2_status) {
    digitalWrite(ZONE2_PIN, LOW);
  } else {
    digitalWrite(ZONE2_PIN, HIGH);
  }
}

//############################################################################
//
//                              Zone 3 - Rear
//
//############################################################################

/*
 * Publishes the status of zone 3
 *   INPUT:  NA
 *   RETURN: NA
 */
void publishZone3Status() {
  if (g_zone3_status) {
    if (g_mqttClient.publish(TOPIC_ZONE3_STATUS, PAYLOAD_ON, true)) {
#ifdef DEBUG
      Serial.print(F("INFO: MQTT message publish succeeded. Topic: "));
      Serial.print(TOPIC_ZONE3_STATUS);
      Serial.print(F(". Payload: "));
      Serial.println(PAYLOAD_ON);
#endif
    } else {
#ifdef DEBUG
      Serial.println(F("ERROR: MQTT message publish failed, either connection lost, or message too large"));
#endif
    }
  } else {
    if (g_mqttClient.publish(TOPIC_ZONE3_STATUS, PAYLOAD_OFF, true)) {
#ifdef DEBUG
      Serial.print(F("INFO: MQTT message publish succeeded. Topic: "));
      Serial.print(TOPIC_ZONE3_STATUS);
      Serial.print(F(". Payload: "));
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
 * Switches the state of the zone 3 relay
 *   INPUT:  NA
 *   RETURN: NA
 */
void setZone3Status() {
  if (g_zone3_status) {
    digitalWrite(ZONE3_PIN, LOW);
  } else {
    digitalWrite(ZONE3_PIN, HIGH);
  }
}

//############################################################################
//
//                           Zone 4 - UNUSED
//
//############################################################################

/*
 * Publishes the status of zone 4
 *   INPUT:  NA
 *   RETURN: NA
 */
void publishZone4Status() {
  if (g_zone4_status) {
    if (g_mqttClient.publish(TOPIC_ZONE4_STATUS, PAYLOAD_ON, true)) {
#ifdef DEBUG
      Serial.print(F("INFO: MQTT message publish succeeded. Topic: "));
      Serial.print(TOPIC_ZONE4_STATUS);
      Serial.print(F(". Payload: "));
      Serial.println(PAYLOAD_ON);
#endif
    } else {
#ifdef DEBUG
      Serial.println(F("ERROR: MQTT message publish failed, either connection lost, or message too large"));
#endif
    }
  } else {
    if (g_mqttClient.publish(TOPIC_ZONE4_STATUS, PAYLOAD_OFF, true)) {
#ifdef DEBUG
      Serial.print(F("INFO: MQTT message publish succeeded. Topic: "));
      Serial.print(TOPIC_ZONE4_STATUS);
      Serial.print(F(". Payload: "));
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
 * Switches the state of the zone 4 relay
 *   INPUT:  NA
 *   RETURN: NA
 */
void setZone4Status() {
  if (g_zone4_status) {
    digitalWrite(ZONE4_PIN, LOW);
  } else {
    digitalWrite(ZONE4_PIN, HIGH);
  }
}


//############################################################################
//
//                                Ethernet
//
//############################################################################

/*
 *  Set up the connection to the ethernet network
 *   INPUT:  NA
 *   RETURN: NA
 */
void setupEthernet() {
  #ifdef DEBUG
    Serial.println(F("\nINFO:  Ethernet network initializing..."));
  #endif
  
  // Attempt to connect to the network
  Ethernet.begin(mac, IP, IP_DNS, IP_GATEWAY, IP_SUBNET);

  #ifdef DEBUG
    Serial.println(F("INFO:  Client is now connected to the network!"));
    Serial.print(F("       IP address:  "));
    Serial.println(Ethernet.localIP());
  #endif
}

//############################################################################
//
//                                  MQTT
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
  if (String(TOPIC_ZONE1_COMMAND).equals(p_topic)) {
    // Type juggle the payload from char array to string.
    String payload;
    for (uint8_t i = 0; i < p_length; i++) {
      payload.concat((char)p_payload[i]);
    }

    // Perform payload action IF manual bypass isn't triggered for the given event.
    if (payload.equals(String(PAYLOAD_ON))) {
      g_zone1_status = true;
      setZone1Status();
      publishZone1Status();
    } else if (payload.equals(String(PAYLOAD_OFF))) {
      g_zone1_status = false;
      setZone1Status();
      publishZone1Status();
    } else {
      #ifdef DEBUG
        Serial.println(F("ERROR: The payload of the MQTT message is not valid"));
      #endif
    }    
  } else if (String(TOPIC_ZONE2_COMMAND).equals(p_topic)) {
    // Type juggle the payload from char array to string.
    String payload;
    for (uint8_t i = 0; i < p_length; i++) {
      payload.concat((char)p_payload[i]);
    }
    if (payload.equals(String(PAYLOAD_ON))) {
      g_zone2_status = true;
      setZone2Status();
      publishZone2Status();
    } else if (payload.equals(String(PAYLOAD_OFF))) {
      g_zone2_status = false;
      setZone2Status();
      publishZone2Status();
    } else {
      #ifdef DEBUG
        Serial.println(F("ERROR: The payload of the MQTT message is not valid"));
      #endif
    }
  } else if (String(TOPIC_ZONE3_COMMAND).equals(p_topic)) {
    // Type juggle the payload from char array to string.
    String payload;
    for (uint8_t i = 0; i < p_length; i++) {
      payload.concat((char)p_payload[i]);
    }
    if (payload.equals(String(PAYLOAD_ON))) {
      g_zone3_status = true;
      setZone3Status();
      publishZone3Status();
    } else if (payload.equals(String(PAYLOAD_OFF))) {
      g_zone3_status = false;
      setZone3Status();
      publishZone3Status();
    } else {
      #ifdef DEBUG
        Serial.println(F("ERROR: The payload of the MQTT message is not valid"));
      #endif
    }
  } else if (String(TOPIC_ZONE4_COMMAND).equals(p_topic)) {
    // Type juggle the payload from char array to string.
    String payload;
    for (uint8_t i = 0; i < p_length; i++) {
      payload.concat((char)p_payload[i]);
    }
    if (payload.equals(String(PAYLOAD_ON))) {
      g_zone4_status = true;
      setZone4Status();
      publishZone4Status();
    } else if (payload.equals(String(PAYLOAD_OFF))) {
      g_zone4_status = false;
      setZone4Status();
      publishZone4Status();
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

      // Subscribe to the Zone 1 command topic
      if (g_mqttClient.subscribe(TOPIC_ZONE1_COMMAND)) {
        #ifdef DEBUG
          Serial.print(F("INFO:  Sending the MQTT subscribe succeeded.\n       Topic: "));
          Serial.println(TOPIC_ZONE1_COMMAND);
        #endif
      } else {
        #ifdef DEBUG
          Serial.print(F("ERROR: Sending the MQTT subscribe failed.\n       Topic: "));
          Serial.println(TOPIC_ZONE1_COMMAND);
        #endif
      }

      // Subscribe to the Zone 2 command topic
      if (g_mqttClient.subscribe(TOPIC_ZONE2_COMMAND)) {
        #ifdef DEBUG
          Serial.print(F("INFO:  Sending the MQTT subscribe succeeded.\n       Topic: "));
          Serial.println(TOPIC_ZONE2_COMMAND);
        #endif
      } else {
        #ifdef DEBUG
          Serial.print(F("ERROR: Sending the MQTT subscribe failed.\n       Topic: "));
          Serial.println(TOPIC_ZONE2_COMMAND);
        #endif
      }

      // Subscribe to the Zone 3 command topic
      if (g_mqttClient.subscribe(TOPIC_ZONE3_COMMAND)) {
        #ifdef DEBUG
          Serial.print(F("INFO:  Sending the MQTT subscribe succeeded.\n       Topic: "));
          Serial.println(TOPIC_ZONE3_COMMAND);
        #endif
      } else {
        #ifdef DEBUG
          Serial.print(F("ERROR: Sending the MQTT subscribe failed.\n       Topic: "));
          Serial.println(TOPIC_ZONE3_COMMAND);
        #endif
      }

      // Subscribe to the Zone 4 command topic
      if (g_mqttClient.subscribe(TOPIC_ZONE4_COMMAND)) {
        #ifdef DEBUG
          Serial.print(F("INFO:  Sending the MQTT subscribe succeeded.\n       Topic: "));
          Serial.println(TOPIC_ZONE4_COMMAND);
        #endif
      } else {
        #ifdef DEBUG
          Serial.print(F("ERROR: Sending the MQTT subscribe failed.\n       Topic: "));
          Serial.println(TOPIC_ZONE4_COMMAND);
        #endif
      }

      // Set the relays to match the values that we have stored for zones 1, 2, 3 & 4
      setZone1Status();
      setZone2Status();
      setZone3Status();
      setZone4Status();

      // Publish the status of the relays to bring everything back into sync.
      publishZone1Status();
      publishZone2Status();
      publishZone3Status();
      publishZone4Status();
    } else {
      #ifdef DEBUG
        Serial.print(F("ERROR: The connection failed with the MQTT broker.\n       Status: "));
        Serial.println(g_mqttClient.state());
      #endif
      
      // Wait before retrying instead of clobbering the server.
      delay(3000);
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

  // Disable SD Card or Ethernet breaks
  pinMode(4,OUTPUT);
  digitalWrite(4,HIGH);

  // Connect to network.
  setupEthernet();
  
  // Set IO pins for use.
  pinMode(ZONE1_PIN, OUTPUT);
  pinMode(ZONE2_PIN, OUTPUT);
  pinMode(ZONE3_PIN, OUTPUT);
  pinMode(ZONE4_PIN, OUTPUT);
  pinMode(BYPASS_ZONE1_PIN, INPUT);
  pinMode(BYPASS_ZONE2_PIN, INPUT);
  pinMode(BYPASS_ZONE3_PIN, INPUT);

  // Set the zone pins high to turn off relays
  digitalWrite(ZONE1_PIN, HIGH);
  digitalWrite(ZONE2_PIN, HIGH);
  digitalWrite(ZONE3_PIN, HIGH);
  digitalWrite(ZONE4_PIN, HIGH);

  // Configure and connect to MQTT broker.
  g_mqttClient.setServer(MQTT_SERVER_IP, MQTT_SERVER_PORT);
  g_mqttClient.setCallback(callback);
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

  // Listen for bypass calls
  g_zone1_bypass_activated = digitalRead(BYPASS_ZONE1_PIN);
  g_zone2_bypass_activated = digitalRead(BYPASS_ZONE2_PIN);
  g_zone3_bypass_activated = digitalRead(BYPASS_ZONE3_PIN);

  if (millis() - DEBOUNCE_TIME >= DEBOUNCE_DELAY) {
    // Prevent bouncing when performing actions following a read.
    if (g_zone1_bypass_activated == HIGH) {
      if (g_zone1_status == false) {
        #ifdef DEBUG
          Serial.println("INFO:  Bypass 1 state switched from OFF to ON.");
        #endif
        g_zone1_status = true;
        setZone1Status();
        publishZone1Status();
      } else if (g_zone1_status == true) {
        #ifdef DEBUG
          Serial.println("INFO:  Bypass 1 state switched from ON to OFF");
        #endif
        g_zone1_status = false;
        setZone1Status();
        publishZone1Status();
      } else {
        #ifdef DEBUG
          Serial.println("INFO:  Requested bypass 1 state matches current state.");
        #endif
      }
    } else if (g_zone2_bypass_activated == HIGH) {
      if (g_zone2_status == false) {
        #ifdef DEBUG
          Serial.println("INFO:  Bypass 2 state switched from OFF to ON.");
        #endif
        g_zone2_status = true;
        setZone2Status();
        publishZone2Status();
      } else if (g_zone2_status == true) {
        #ifdef DEBUG
          Serial.println("INFO:  Bypass 2 state switched from ON to OFF");
        #endif
        g_zone2_status = false;
        setZone2Status();
        publishZone2Status();
      } else {
        #ifdef DEBUG
          Serial.println("INFO:  Requested bypass 2 state matches current state.");
        #endif
      }
    } else if (g_zone3_bypass_activated == HIGH) {
      if (g_zone3_status == false) {
        #ifdef DEBUG
          Serial.println("INFO:  Bypass 3 state switched from OFF to ON.");
        #endif
        g_zone3_status = true;
        setZone3Status();
        publishZone3Status();
      } else if (g_zone3_status == true) {
        #ifdef DEBUG
          Serial.println("INFO:  Bypass 3 state switched from ON to OFF");
        #endif
        g_zone3_status = false;
        setZone3Status();
        publishZone3Status();
      } else {
        #ifdef DEBUG
          Serial.println("INFO:  Requested bypass 3 state matches current state.");
        #endif
      }
    }
    // Set the last time of button press.
    DEBOUNCE_TIME = millis();
  }
}
