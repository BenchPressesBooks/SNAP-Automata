/*
  Configuration for Home Assistant:
    mqtt:
      broker: <REDACTED>
      port: 1883
      client_id: 'ha'
      username: 'ha'
      password: '<REDACTED>'
      
    light:
    # Zone 1 Front Path Light
    - platform: mqtt
      name: 'Zone 1 Front Path Light'
      state_topic: 'exterior/plfront/status'
      command_topic: 'exterior/plfront/switch'
      optimistic: false

    # Zone 2 Side Path Light
    - platform: mqtt
      name: 'Zone 2 Side Path Light'
      state_topic: 'exterior/plside/status'
      command_topic: 'exterior/plside/switch'
      optimistic: false

    # Zone 3 Rear Pond Lights
    - platform: mqtt
      name: 'Zone 3 Rear Pond Lights'
      state_topic: 'exterior/plrear/status'
      command_topic: 'exterior/plrear/switch'
      optimistic: false
*/

#include <EthernetV2_0.h>   // https://github.com/Seeed-Studio/Ethernet_Shield_W5200 (For the Seeedstudio W5200 ethernet shield.)
#include <PubSubClient.h>   // https://github.com/knolleary/pubsubclient (no licence)

#define DEBUG
#define MQTT_VERSION MQTT_VERSION_3_1_1

// MQTT: client ID, broker IP address, port, username & password
const char*       MQTT_CLIENT_ID    = "MQTT-PATHLIGHTCONTROL";
const char*       MQTT_SERVER_IP    = "<REDACTED>";
const uint16_t    MQTT_SERVER_PORT  = 1883;
const char*       MQTT_USERNAME     = "exterior";
const char*       MQTT_PASSWORD     = "<REDACTED>";

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

// MQTT: payloads
// Zones 1 , 2 & 3: "ON"/"OFF"
const char*       PAYLOAD_ON            = "ON";
const char*       PAYLOAD_OFF           = "OFF";

boolean           g_zone1_status       = false;
boolean           g_zone2_status       = false;
boolean           g_zone3_status       = false;

// buffer used to send/receive data with MQTT
const uint8_t     MSG_BUFFER_SIZE       = 20;
char              g_msg_buffer[MSG_BUFFER_SIZE];

const uint8_t     ZONE1_PIN            = 7;
const uint8_t     ZONE2_PIN            = 6;
const uint8_t     ZONE3_PIN            = 5;

// Fixed IP address: IP address, IP gateway, subnet, dns
const IPAddress   IP                (<REDACTED>);
const IPAddress   IP_GATEWAY        (<REDACTED>);
const IPAddress   IP_SUBNET         (<REDACTED>);
const IPAddress   IP_DNS            (<REDACTED>);
byte mac[] = {<REDACTED>};

// Ethernet and MQTT client setup
EthernetClient  g_ethernetClient;
PubSubClient      g_mqttClient(g_ethernetClient);

///////////////////////////////////////////////////////////////////////////
//
// Zone 1 - Front
//
///////////////////////////////////////////////////////////////////////////

/*
  Function called to publish the status of zone 1
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
  Function called to switch the status of zone 1
*/
void setZone1Status() {
  if (g_zone1_status) {
    digitalWrite(ZONE1_PIN, LOW);
  } else {
    digitalWrite(ZONE1_PIN, HIGH);
  }
}

///////////////////////////////////////////////////////////////////////////
//
// Zone 2 - Side
//
///////////////////////////////////////////////////////////////////////////

/*
  Function called to publish the status of zone 2
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
  Function called to switch the status of zone 2
*/
void setZone2Status() {
  if (g_zone2_status) {
    digitalWrite(ZONE2_PIN, LOW);
  } else {
    digitalWrite(ZONE2_PIN, HIGH);
  }
}

///////////////////////////////////////////////////////////////////////////
//
// Zone 3 - Rear
//
///////////////////////////////////////////////////////////////////////////

/*
  Function called to publish the status of zone 3
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
  Function called to switch the status of zone 3
*/
void setZone3Status() {
  if (g_zone3_status) {
    digitalWrite(ZONE3_PIN, LOW);
  } else {
    digitalWrite(ZONE3_PIN, HIGH);
  }
}


///////////////////////////////////////////////////////////////////////////
//
// Ethernet
//
///////////////////////////////////////////////////////////////////////////

/*
   Function called to setup the connection to the ethernet network
*/
void setupEthernet() {
  delay(10);
  // attempt to connect to the network
  Ethernet.begin(mac, IP, IP_DNS, IP_GATEWAY, IP_SUBNET);

  /*while (!g_ethernetClient) {
    delay(500);
    #ifdef DEBUG
      Serial.println("Not connected... waiting 500ms.");
    #endif
  }*/

#ifdef DEBUG
  Serial.println(F("INFO: Client is now connected to the network"));
  Serial.print(F("INFO: IP address: "));
  Serial.println(Ethernet.localIP());
#endif
}

///////////////////////////////////////////////////////////////////////////
//
// MQTT
//
///////////////////////////////////////////////////////////////////////////

/*
   Function called when a MQTT message arrived
   @param p_topic   The topic of the MQTT message
   @param p_payload The payload of the MQTT message
   @param p_length  The length of the payload
*/
void callback(char* p_topic, byte* p_payload, unsigned int p_length) {
#ifdef DEBUG
  Serial.println(F("INFO: A new MQTT message arrived"));
  Serial.print(F("INFO: Topic: "));
  Serial.println(p_topic);
  Serial.print(F("INFO: Payload: "));
  for (int i = 0; i < p_length; i++) {
    Serial.print((char)p_payload[i]);
  }
  Serial.println();
  Serial.print(F("INFO: Length: "));
  Serial.println(p_length);
#endif
  // handle the MQTT topic of the received message
  if (String(TOPIC_ZONE1_COMMAND).equals(p_topic)) {
    // concat the payload into a string
    String payload;
    for (uint8_t i = 0; i < p_length; i++) {
      payload.concat((char)p_payload[i]);
    }    
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
    // concat the payload into a string
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
    // concat the payload into a string
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
  } else {
    // do nothing.....
#ifdef DEBUG
    Serial.println(F("INFO: The received MQTT message was not used"));
#endif
  }
}

/*
   Function called to reconnect the client to the MQTT broker and publish/subscribe to/from some MQTT topics
*/
void reconnect() {
  while (!g_mqttClient.connected()) {
    if (g_mqttClient.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
#ifdef DEBUG
      Serial.println(F("INFO: The client is successfully connected to the MQTT broker"));
#endif

      // subscribe to the Zone 1 command topic
      if (g_mqttClient.subscribe(TOPIC_ZONE1_COMMAND)) {
#ifdef DEBUG
        Serial.print(F("INFO: Sending the MQTT subscribe succeeded. Topic: "));
        Serial.println(TOPIC_ZONE1_COMMAND);
#endif
      } else {
#ifdef DEBUG
        Serial.print(F("ERROR: Sending the MQTT subscribe failed. Topic: "));
        Serial.println(TOPIC_ZONE1_COMMAND);
#endif
      }

      // subscribe to the Zone 2 command topic
      if (g_mqttClient.subscribe(TOPIC_ZONE2_COMMAND)) {
#ifdef DEBUG
        Serial.print(F("INFO: Sending the MQTT subscribe succeeded. Topic: "));
        Serial.println(TOPIC_ZONE2_COMMAND);
#endif
      } else {
#ifdef DEBUG
        Serial.print(F("ERROR: Sending the MQTT subscribe failed. Topic: "));
        Serial.println(TOPIC_ZONE2_COMMAND);
#endif
      }

      // subscribe to the Zone 3 command topic
      if (g_mqttClient.subscribe(TOPIC_ZONE3_COMMAND)) {
#ifdef DEBUG
        Serial.print(F("INFO: Sending the MQTT subscribe succeeded. Topic: "));
        Serial.println(TOPIC_ZONE3_COMMAND);
#endif
      } else {
#ifdef DEBUG
        Serial.print(F("ERROR: Sending the MQTT subscribe failed. Topic: "));
        Serial.println(TOPIC_ZONE3_COMMAND);
#endif
      }

      // set the initial status of zones 1 , 2 & 3
      setZone1Status();
      setZone2Status();
      setZone3Status();

      // publish the initial status of zones 1 , 2 & 3
      publishZone1Status();
      publishZone2Status();
      publishZone3Status();
    } else {
#ifdef DEBUG
      Serial.println(F("ERROR: The connection failed with the MQTT broker"));
      Serial.print("ERROR: rc: ");
      Serial.println(g_mqttClient.state());
      // wait 5 seconds before retrying
      delay(5000);
#endif
    }
  }
}

///////////////////////////////////////////////////////////////////////////
//
// SETUP and LOOP
//
///////////////////////////////////////////////////////////////////////////

/*
   Function called once to initialize the board
*/
void setup() {
#ifdef DEBUG
  Serial.begin(115200);
  Serial.println(F("\nINFO: The ethernet module is starting..."));
#endif

  // init the zone pins as output
  pinMode(ZONE1_PIN, OUTPUT);
  pinMode(ZONE2_PIN, OUTPUT);
  pinMode(ZONE3_PIN, OUTPUT);

  // set the zone pins high to turn off relays
  digitalWrite(ZONE1_PIN, HIGH);
  digitalWrite(ZONE2_PIN, HIGH);
  digitalWrite(ZONE3_PIN, HIGH);

  // Disable SD Card
  pinMode(4,OUTPUT);
  digitalWrite(4,HIGH);

  setupEthernet();

  // set the MQTT broker IP address and port
  g_mqttClient.setServer(MQTT_SERVER_IP, MQTT_SERVER_PORT);
  // set the MQTT callback function
  g_mqttClient.setCallback(callback);
}

/*
   Function called infinitely after the setup function
*/
void loop() {
  // keep the MQTT client connected to the broker
  if (!g_mqttClient.connected()) {
    reconnect();
  }
  g_mqttClient.loop();
  
  // a delay is necessary if the debug is disabled
  // without a delay, there is some latency to switch on/off a light
  yield();
  //delay(100); 
}
