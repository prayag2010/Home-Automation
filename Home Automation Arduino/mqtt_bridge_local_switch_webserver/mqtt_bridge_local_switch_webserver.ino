
/*
   add wifi auto reconn
   send data via serial to local SW
   add status leds
   add on-off capability for NFC
   add 2nd light


   PIN CONFIG:

   D1, D2 : Software Serial
   Light 0 : D5
   Fan 0 : D6
   Button : D0
   WiFi Led : D8 NAHHAHAHAH D3 IT IS
   MQTT Led : D7
   IR Led : D3 NAHHAHAHAH D8 IT IS
   Off Button : D0

   FORMAT:

   TOPIC:
   lig/stae/n
   fan/stae/n          (HA will receive)
   lig/comm/n          (HA will publish)
   fan/comm/n
   ac/command          (HA will send IR commands correnponding to input numbers)

   - value of 'n' starts from 0

   PAYLOAD:
   O - on
   F - off
   S - switch
   no RGB support

   Serial data format:
   {topic}{space}{payload}
   total length of 10 + 1 + 1 = 12 characters

   AC IR raw data
   KELVINATOR encoding

   AC config 0:
   22C, Full swing, Turbo

   AC config 1:
   22C Front swing Turbo

   AC config 2:
   22C Front swing fan 2

   AC config 3:
   22C Full swing fan 3

   AC config 4:
   22C Full swing fan 2

   Separate AC off command

*/

#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#ifndef UNIT_TEST
#include <Arduino.h>
#endif
#include <IRremoteESP8266.h>
#include <IRsend.h>

#include "ac_remote_raw_data.h"

#define IR_LED D8

SoftwareSerial swSer(D1, D2, false, 256);

WiFiClient espClient;
ESP8266WebServer server(80);
PubSubClient client(espClient);

IRsend irsend(IR_LED);

const char* ssid = "R7";
const char* password = "Internet701";
const char* mqtt_server = "192.168.1.15";
int mqtt_port = 1883;

String serialInput = "";
boolean ssInAddressed = true;

String light0CommTopic = "lig/comm/0";
String fan0CommTopic = "fan/comm/0";

String light0StateTopic = "lig/stae/0";
String fan0StateTopic = "fan/stae/0";

//On other devices
String light1CommTopic = "lig/comm/1";
String light2CommTopic = "lig/comm/2";
String fan1CommTopic = "fan/comm/1";

String irCommandTopic = "ac/command";
char irCommand = '0';
boolean sendIrCommand = false;

int light0 = D5;
int fan0 = D6;

int wifiLed = D3;
int mqttLed = D7;

int offButton = D0;
boolean prevButState = false;
boolean curButState = false;

boolean light0state = false;
boolean fan0state = false;


void setup() {
  Serial.begin(115200);
  swSer.begin(115200);

  irsend.begin();

  Serial.println("Local client + switch");

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  pinMode(light0, OUTPUT);
  pinMode(fan0, OUTPUT);

  digitalWrite(light0, HIGH);
  digitalWrite(fan0, HIGH);

  pinMode(wifiLed, OUTPUT);
  pinMode(mqttLed, OUTPUT);

  pinMode(offButton, INPUT);

  if (!client.connected()) {
    reconnect();
  }

  client.publish(light0StateTopic.c_str(), "F");
  client.publish(fan0StateTopic.c_str(), "F");

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  client.loop();
  server.on("/", handleRoot);
  server.on("/light0", handleLight0);
  server.on("/fan0", handleFan0);
  server.on("/light1", handleLight1);
  server.on("/light2", handleLight2);
  server.on("/fan1", handleFan1);
  server.onNotFound(handleNotFound);
  client.loop();

  server.begin();
  client.loop();

  Serial.println("HTTP server started");
}

void loop() {

  prevButState = curButState;
  curButState = digitalRead(offButton);

  if (!prevButState && curButState) {
    Serial.println("Rising Edge detected");
    Serial.println("All devices turned off");
    delay(250);
    fan0state = false;
    light0state = false;
    digitalWrite(light0, HIGH);
    digitalWrite(fan0, HIGH);
    client.publish(light0StateTopic.c_str(), "F");
    client.publish(fan0StateTopic.c_str(), "F");

    //EDIT
    client.publish(light1CommTopic.c_str(), "F");
    client.publish(light2CommTopic.c_str(), "F");
    client.publish(fan1CommTopic.c_str(), "F");
  }

  while (swSer.available() > 0) {
    serialInput += (char) swSer.read();
    yield();
    ssInAddressed = false;
  }

  if (!ssInAddressed) {
    ssInAddressed = true;

    String topic = "";
    String payload = "";

    Serial.println("--------------------------------------------------------");
    Serial.println("Serial RX data is:");
    Serial.println(serialInput);

    const char *ptr = strchr(serialInput.c_str(), ' ');
    int index;
    if (ptr) {
      index = ptr - serialInput.c_str();
      Serial.print("Blank is at index ");
      Serial.println(index);
    }

    for (int i = 0; i < index; i++) {
      topic += serialInput[i];
    }

    payload += serialInput[index + 1];

    Serial.print("Topic received through serial is: ");
    Serial.println(topic);
    Serial.print("Payload received through serial is: ");
    Serial.println(payload);

    client.publish(topic.c_str(), payload.c_str());
    Serial.println("Data received through Serial published (blindly)");

    if (strcmp(light0StateTopic.c_str(), topic.c_str()) == 0) {
      if (payload[0] == 'O') {
        digitalWrite(light0, LOW);
        Serial.println("light 0 on");
        light0state = true;
      }
      else if (payload[0] == 'F') {
        digitalWrite(light0, HIGH);
        Serial.println("light 0 off");
        light0state = false;
      }
      else if (payload[0] == 'S') {
        switchLight0();
        Serial.println("Light 0 switched through Serial");
      }
      else
        Serial.println("Payload mismatch at serial receive");
    }
    else if (strcmp(fan0StateTopic.c_str(), topic.c_str()) == 0) {
      if (payload[0] == 'O') {
        digitalWrite(fan0, LOW);
        Serial.println("fan 0 on");
        fan0state = true;
      }
      else if (payload[0] == 'F') {
        digitalWrite(fan0, HIGH);
        Serial.println("fan 0 off");
        fan0state = false;
      }
      else if (payload[0] == 'S') {
        switchFan0();
        Serial.println("Fan 0 switched through Serial");
      }
      else
        Serial.println("Payload mismatch at serial receive");
    }
    else
      Serial.println("State topic mismatch at serial receive");

    Serial.println("--------------------------------------------------------");
    serialInput = "";
  }

  if (sendIrCommand) {
    sendIrCommand = false;
    if (irCommand == '0') {
      irsend.sendRaw(config0, 379, 38);
      Serial.println("config 0 sent");
    }
    else if (irCommand == '1') {
      irsend.sendRaw(config1, 379, 38);
      Serial.println("config 1 sent");
    }
    else if (irCommand == '2') {
      irsend.sendRaw(config2, 379, 38);
      Serial.println("config 2 sent");
    }
    else if (irCommand == '3') {
      irsend.sendRaw(config3, 379, 38);
      Serial.println("config 3 sent");
    }
    else if (irCommand == '4') {
      irsend.sendRaw(config4, 379, 38);
      Serial.println("config 4 sent");
    }
    else if (irCommand == 'O') {
      irsend.sendRaw(acOffRaw, 379, 38);
      Serial.println("AC off command sent");
    }
    else
      Serial.println("IR command payload mismatch");
  }

  if (!client.connected()) {
    reconnect();
  }

  client.loop();
  server.handleClient();
  client.loop();
}

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

  String strPayload = "";
  String strTopic = "";

  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    //    strPayload += String((char)payload[i]);
    //    strTopic += String(topic[i]);
  }

  char temp = payload[0];

  Serial.println();

  if (strcmp(light0CommTopic.c_str(), topic) == 0) {
    if (payload[0] == 'O') {
      digitalWrite(light0, LOW);
      light0state = true;
      Serial.println("light 0 on");
      client.publish(light0StateTopic.c_str(), "O");
      Serial.println("Lights on confirmation sent");
    }
    else if (payload[0] == 'F') {
      digitalWrite(light0, HIGH);
      light0state = false;
      Serial.println("light 0 off");
      client.publish(light0StateTopic.c_str(), "F");
      Serial.println("Lights off confirmation sent");
    }
    else
      Serial.println("Payload mismatch at MQTT receive");
  }
  else if (strcmp(fan0CommTopic.c_str(), topic) == 0) {
    if (payload[0] == 'O') {
      digitalWrite(fan0, LOW);
      fan0state = true;
      Serial.println("fan 0 on");
      client.publish(fan0StateTopic.c_str(), "O");
      Serial.println("Fan on confirmation sent");
    }
    else if (payload[0] == 'F') {
      digitalWrite(fan0, HIGH);
      fan0state = false;
      Serial.println("fan 0 off");
      client.publish(fan0StateTopic.c_str(), "F");
      Serial.println("Fan off confirmation sent");
    }
    else
      Serial.println("Payload mismatch at MQTT receive");
  }
  else if (strcmp(irCommandTopic.c_str(), topic) == 0) {
    Serial.println("IR command topic received");
    Serial.println("Received command is:");
    Serial.println(payload[0]);

    irCommand = payload[0];
    sendIrCommand = true;
  }
  else
    Serial.println("Topic mismatch at MQTT receive");


  Serial.println("--------------------------------------------------------");
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
      client.subscribe(light0CommTopic.c_str());
      client.subscribe(fan0CommTopic.c_str());
      client.subscribe(irCommandTopic.c_str());
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void handleRoot()
{
  server.send(200, "text/plain", "hello from esp8266!");
}

void handleLight0()
{
  switchLight0();
  if (light0state) {
    server.send(200, "text/plain", "Light 0 turned off");
    Serial.println("light0 turned off via local webserver");
  }
  else {
    server.send(200, "text/plain", "Light 0 turned on");
    Serial.println("light0 turned on via local webserver");
  }
}

void handleLight1()
{
  client.publish(light1CommTopic.c_str(), "S");
  Serial.println("Light 1 switch command published");
  server.send(200, "text/plain", "Light 1 Switched State");
}

void handleLight2()
{
  client.publish(light2CommTopic.c_str(), "S");
  Serial.println("Light 2 switch command published");
  server.send(200, "text/plain", "Light 2 Switched State");
}

void handleFan1()
{
  client.publish(fan1CommTopic.c_str(), "S");
  Serial.println("Fan 1 switch command published");
  server.send(200, "text/plain", "Fan 1 Switched State");
}

void handleFan0()
{
  switchFan0();
  if (fan0state) {
    server.send(200, "text/plain", "Fan 0 turned off");
    Serial.println("fan0 turned off via local webserver");
  }
  else {
    server.send(200, "text/plain", "Fan 0 turned on");
    Serial.println("fan0 turned on via local webserver");
  }
}

void switchFan0()
{
  if (fan0state) {
    fan0state = !fan0state;
    digitalWrite(fan0, HIGH);
    client.publish(fan0StateTopic.c_str(), "F");
  }
  else {
    fan0state = !fan0state;
    digitalWrite(fan0, LOW);
    client.publish(fan0StateTopic.c_str(), "O");
  }
}

void switchLight0()
{
  if (light0state) {
    light0state = !light0state;
    digitalWrite(light0, HIGH);
    client.publish(light0StateTopic.c_str(), "F");
  }
  else {
    light0state = !light0state;
    digitalWrite(light0, LOW);
    client.publish(light0StateTopic.c_str(), "O");
  }
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

