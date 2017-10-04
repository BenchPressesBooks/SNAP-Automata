/*
  Configuration for Home Assistant:
    mqtt:
      broker: <REDACTED>
      port: 1883
      client_id: 'ha'
      username: 'ha'
      password: '<REDACTED>'
      
    light:
    # Interior: Living Room Display Case
    - platform: mqtt
      name: 'Living Room Display Case'
      state_topic: 'firstfloor/lrdisplaycase1/status'
      command_topic: 'firstfloor/lrdisplaycase1/switch'
      optimistic: false

  Sources:
    - MQTT: File > Examples > PubSubClient > mqtt_esp8266
    - TLS:  https://io.adafruit.com/blog/security/2016/07/05/adafruit-io-security-esp8266/
    - OTA:  File > Examples > ArduinoOTA > BasicOTA
*/

#include <ESP8266WiFi.h>    // https://github.com/esp8266/Arduino (GNUv2.1 licence)
#include <PubSubClient.h>   // https://github.com/knolleary/pubsubclient (no licence)

#define DEBUG
#define MQTT_VERSION MQTT_VERSION_3_1_1

// Wi-Fi: Access Point SSID and password
const char*       AP_SSID           = "<REDACTED>";
const char*       AP_PASSWORD       = "<REDACTED>";

// MQTT: client ID, broker IP address, port, username & password
const char*       MQTT_CLIENT_ID    = "MQTT-LRDC1";
const char*       MQTT_SERVER_IP    = "<REDACTED>";
const uint16_t    MQTT_SERVER_PORT  = 1883;
const char*       MQTT_USERNAME     = "interior";
const char*       MQTT_PASSWORD     = "<REDACTED>";

// MQTT: topics

// Living Room Side Lamp
const char*       TOPIC_RELAY_STATUS             = "firstfloor/lrdisplaycase1/status";
const char*       TOPIC_RELAY_COMMAND            = "firstfloor/lrdisplaycase1/switch";

// MQTT: payloads
// Lamps : "ON"/"OFF"
const char*       PAYLOAD_ON            = "ON";
const char*       PAYLOAD_OFF           = "OFF";

boolean           g_relay_status       = false;

// buffer used to send/receive data with MQTT
const uint8_t     MSG_BUFFER_SIZE       = 20;
char              g_msg_buffer[MSG_BUFFER_SIZE];

const uint8_t     RELAY_PIN       = D1;

// Fixed IP address: IP address, IP gateway, subnet, dns
const IPAddress   IP                (<REDACTED>);
const IPAddress   IP_GATEWAY        (<REDACTED>);
const IPAddress   IP_SUBNET         (<REDACTED>);
const IPAddress   IP_DNS            (<REDACTED>);

// WiFi and MQTT client setup
WiFiClient  g_wifiClient;
PubSubClient      g_mqttClient(g_wifiClient);

///////////////////////////////////////////////////////////////////////////
//
// RELAY
//
///////////////////////////////////////////////////////////////////////////

/*
  Function called to publish the status of the relay
*/
void publishRelayStatus() {
  if (g_relay_status) {
    if (g_mqttClient.publish(TOPIC_RELAY_STATUS, PAYLOAD_ON, true)) {
#ifdef DEBUG
      Serial.print(F("INFO: MQTT message publish succeeded. Topic: "));
      Serial.print(TOPIC_RELAY_STATUS);
      Serial.print(F(". Payload: "));
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
      Serial.print(F("INFO: MQTT message publish succeeded. Topic: "));
      Serial.print(TOPIC_RELAY_STATUS);
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
  Function called to switch the status of the relay
*/
void setRelayStatus() {
  if (g_relay_status) {
    digitalWrite(RELAY_PIN, HIGH);
  } else {
    digitalWrite(RELAY_PIN, LOW);
  }
}

///////////////////////////////////////////////////////////////////////////
//
// WIFI
//
///////////////////////////////////////////////////////////////////////////

/*
   Function called to setup the connection to the Wi-Fi Access Point
*/
void setupWifi() {
  delay(10);
  // attempt to connect to the Wi-Fi AP
  WiFi.mode(WIFI_STA);
  WiFi.begin(AP_SSID, AP_PASSWORD);
  // define the fixed IP address
  WiFi.config(IP, IP_GATEWAY, IP_SUBNET, IP_DNS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

#ifdef DEBUG
  Serial.println(F("INFO: Client is now connected to the Wi-Fi AP"));
  Serial.print(F("INFO: IP address: "));
  Serial.println(WiFi.localIP());
  Serial.print("INFO:MAC Address:");
  Serial.println(WiFi.macAddress());
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
  if (String(TOPIC_RELAY_COMMAND).equals(p_topic)) {
    // concat the payload into a string
    String payload;
    for (uint8_t i = 0; i < p_length; i++) {
      payload.concat((char)p_payload[i]);
    }
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

      // subscribe to the relay command topic
      if (g_mqttClient.subscribe(TOPIC_RELAY_COMMAND)) {
#ifdef DEBUG
        Serial.print(F("INFO: Sending the MQTT subscribe succeeded. Topic: "));
        Serial.println(TOPIC_RELAY_COMMAND);
#endif
      } else {
#ifdef DEBUG
        Serial.print(F("ERROR: Sending the MQTT subscribe failed. Topic: "));
        Serial.println(TOPIC_RELAY_COMMAND);
#endif
      }

      // set the initial status of the relay
      setRelayStatus();

      // publish the initial status of the relay
      publishRelayStatus();
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
  Serial.println(F("\nINFO: The Wi-Fi module is starting..."));
#endif

  // init the relay pin as output
  pinMode(RELAY_PIN,         OUTPUT);

  setupWifi();

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
  // without a delay, there is some latency to switch on/off a relay
  yield();
  //delay(100);
}
