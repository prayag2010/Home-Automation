
/*
   add wifi auto reconn
   send data via serial to local SW
   add status leds
   add on-off capability for NFC

   FORMAT:

   TOPIC:
   lig/stae/n
   fan/stae/n          (HA will receive)
   lig/comm/n          (HA will publish)
   fan/comm/n

   - value of 'n' starts from 0

   PAYLOAD:
   O - on
   F - off
   S - switch
   no RGB support

   Serial data format:
   {topic}{space}{payload}
   total length of 10 + 1 + 1 = 12 characters
*/


#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <SoftwareSerial.h>

const char* ssid = "R7";
const char* password = "Internet701";
const char* mqtt_server = "m12.cloudmqtt.com";
const char* mqtt_username = "yfjefqfh";
const char* mqtt_pass = "DQ1Jsb2RgJEH";
int mqtt_port = 15267;

String ss_data = "";
boolean ss_str_sent = true;

SoftwareSerial swSer(D1, D2, false, 256);

WiFiClient espClient;
PubSubClient client(espClient);

boolean light0state = false;
boolean fan0state = false;

String lightCommTopic = "lig/comm/0";
String fanCommTopic = "fan/comm/0";

int wifiLed = D8;
int mqttLed = D7;


void setup_wifi() {

  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(wifiLed, LOW);
    return;
  }

  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  digitalWrite(wifiLed, HIGH);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  digitalWrite(wifiLed, LOW);

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  ss_data = "";
  ss_data += topic;
  ss_data += " ";
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    ss_data += (char)payload[i];
  }

  Serial.println();
  Serial.println("Prepard string for serial comm:");
  Serial.println(ss_data);
  ss_str_sent = false;
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
        digitalWrite(mqttLed, HIGH);
    setup_wifi();
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), mqtt_username, mqtt_pass)) {
      digitalWrite(mqttLed, LOW);
      Serial.println("connected");
      client.subscribe("lig/#");
      client.subscribe("fan/#");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  swSer.begin(115200);
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  pinMode(wifiLed, OUTPUT);
  pinMode(mqttLed, OUTPUT);

  Serial.println("Cloud MQTT to local Bridge");

  if (!client.connected()) {
    reconnect();
  }
}

void loop() {
  if (!ss_str_sent) {
    swSer.write(ss_data.c_str());
    ss_str_sent = true;
    Serial.println("Serial data sent");
  }

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}

