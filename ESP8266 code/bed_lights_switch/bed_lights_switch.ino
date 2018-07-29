#include <ESP8266WiFi.h>
#include <PubSubClient.h>


const char* ssid = "R7";
const char* password = "Internet701";
const char* mqtt_server = "192.168.1.15";

int wifiLed = D8;
int mqttLed = D7;

int light1 = D5;
boolean light1state = false;

int light2 = D3;
boolean light2state = false;

int fan1 = D6;
boolean fan1state = false;

String light1StateTopic = "lig/stae/1";
String light1CommTopic = "lig/comm/1";

String light2StateTopic = "lig/stae/2";
String light2CommTopic = "lig/comm/2";

String fan1StateTopic = "fan/stae/1";
String fan1CommTopic = "fan/comm/1";


WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);

  pinMode(wifiLed, OUTPUT);
  pinMode(mqttLed, OUTPUT);
  pinMode(light1, OUTPUT);
  pinMode(light2, OUTPUT);
  pinMode(fan1, OUTPUT);

  digitalWrite(light1, LOW);
  digitalWrite(light2, LOW);
  digitalWrite(fan1, LOW);

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  if (!client.connected()) {
    reconnect();
  }

  client.publish(light1StateTopic.c_str(), "F");
  client.publish(light2StateTopic.c_str(), "F");
  client.publish(fan1StateTopic.c_str(), "F");

}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
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
    if (client.connect(clientId.c_str())) {
      digitalWrite(mqttLed, LOW);
      Serial.println("connected");
      client.subscribe(light1CommTopic.c_str());
      client.subscribe(light2CommTopic.c_str());
      client.subscribe(fan1CommTopic.c_str());
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  String strPayload = "";
  String strTopic = "";

  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  if (strcmp(topic, light1CommTopic.c_str()) == 0) {
    if (payload[0] == 'O') {
      digitalWrite(light1, HIGH);
      light1state = true;
      Serial.println("Light 1 switched on");
    }
    else if (payload[0] == 'F') {
      digitalWrite(light1, LOW);
      light1state = false;
      Serial.println("Light 1 switched off");
    }
    else if (payload[0] == 'S') {
      light1switch();
      Serial.println("Light 1 switch");
    }
    else
      Serial.println("Light 1 payload mismatch");

    if (light1state) {
      client.publish(light1StateTopic.c_str(), "O");
      Serial.println("Light 1 on confirmation Sent");
    }
    else {
      client.publish(light1StateTopic.c_str(), "F");
      Serial.println("Light 1 off confirmation sent");
    }
  }
  else if (strcmp(topic, light2CommTopic.c_str()) == 0) {
    if (payload[0] == 'O') {
      digitalWrite(light2, HIGH);
      light2state = true;
      Serial.println("Light 2 switched on");
    }
    else if (payload[0] == 'F') {
      digitalWrite(light2, LOW);
      light2state = false;
      Serial.println("Light 2 switched off");
    }
    else if (payload[0] == 'S') {
      light2switch();
      Serial.println("Light 2 switch");
    }
    else
      Serial.println("Light 2 payload mismatch");

    if (light2state) {
      client.publish(light2StateTopic.c_str(), "O");
      Serial.println("Light 2 on confirmation Sent");
    }
    else {
      client.publish(light2StateTopic.c_str(), "F");
      Serial.println("Light 2 off confirmation sent");
    }
  }
  else if (strcmp(topic, fan1CommTopic.c_str()) == 0) {
    if (payload[0] == 'O') {
      digitalWrite(fan1, HIGH);
      fan1state = true;
      Serial.println("Fan 1 switched on");
    }
    else if (payload[0] == 'F') {
      digitalWrite(fan1, LOW);
      fan1state = false;
      Serial.println("Fan 1 switched off");
    }
    else if (payload[0] == 'S') {
      fan1switch();
      Serial.println("Fan 1 switch");
    }
    else
      Serial.println("Fan 1 payload mismatch");

    if (fan1state) {
      client.publish(fan1StateTopic.c_str(), "O");
      Serial.println("Fan 1 on confirmation Sent");
    }
    else {
      client.publish(fan1StateTopic.c_str(), "F");
      Serial.println("Fan 1 off confirmation sent");
    }
  }
  else
    Serial.println("Topic Mismatch");

}


void setup_wifi()
{

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

void light1switch()
{
  light1state = !light1state;
  if (light1state) {
    digitalWrite(light1, HIGH);
    Serial.println("Light 1 switched on (switch)");
  }
  else {
    digitalWrite(light1, LOW);
    Serial.println("Light 1 switched off (switch)");
  }
}

void light2switch()
{
  light2state = !light2state;
  if (light2state) {
    digitalWrite(light2, HIGH);
    Serial.println("Light 2 switched on (switch)");
  }
  else {
    digitalWrite(light2, LOW);
    Serial.println("Light 2 switched off (switch)");
  }
}

void fan1switch()
{
  fan1state = !fan1state;
  if (fan1state) {
    digitalWrite(fan1, HIGH);
    Serial.println("Fan 1 switched on (switch)");
  }
  else {
    digitalWrite(fan1, LOW);
    Serial.println("Fan 1 switched off (switch)");
  }
}

