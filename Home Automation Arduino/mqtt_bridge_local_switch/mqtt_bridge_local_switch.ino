
/*
   add wifi auto reconn
   send data via i2c to local SW
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
   no RGB support

   Serial data format:
   {topic}{space}{payload}
   total length of 10 + 1 + 1 = 12 characters
*/

#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

SoftwareSerial swSer(D1, D2, false, 256);

WiFiClient espClient;
PubSubClient client(espClient);

const char* ssid = "R7";
const char* password = "Internet701";
const char* mqtt_server = "192.168.1.15";
int mqtt_port = 1883;

String serialInput = "";
boolean ssInAddressed = true;

String lightCommTopic = "lig/comm/0";
String fanCommTopic = "fan/comm/0";

String lightStateTopic = "lig/stae/0";
String fanStateTopic = "fan/stae/0";

int light0 = D5;
int fan0 = D6;

void setup() {
  Serial.begin(115200);           // start serial for output
  swSer.begin(115200);
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  pinMode(light0, OUTPUT);
  pinMode(fan0, OUTPUT);
}

void loop() {
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

    if (strcmp(lightStateTopic.c_str(), topic.c_str()) == 0) {
      if (payload[0] == 'O') {
        digitalWrite(light0, HIGH);
        Serial.println("light 0 on");
      }
      else if (payload[0] == 'F') {
        digitalWrite(light0, LOW);
        Serial.println("light 0 off");
      }
      else
        Serial.println("Payload mismatch");
    }
    else if (strcmp(fanStateTopic.c_str(), topic.c_str()) == 0) {
      if (payload[0] == 'O') {
        digitalWrite(fan0, HIGH);
        Serial.println("fan 0 on");
      }
      else if (payload[0] == 'F') {
        digitalWrite(fan0, LOW);
        Serial.println("fan 0 off");
      }
      else
        Serial.println("Payload mismatch at serial receive");
    }
    else
      Serial.println("State topic mismatch at serial receive");

    Serial.println("--------------------------------------------------------");
    serialInput = "";
  }

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}

void setup_wifi() {

  if (WiFi.status() == WL_CONNECTED)
    return;

  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

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

  if (strcmp(lightCommTopic.c_str(), topic) == 0) {
    if (payload[0] == 'O') {
      digitalWrite(light0, HIGH);
      Serial.println("light 0 on");
      client.publish(lightStateTopic.c_str(),"O");
      Serial.println("Lights on confirmation sent");
    }
    else if (payload[0] == 'F') {
      digitalWrite(light0, LOW);
      Serial.println("light 0 off");
      client.publish(lightStateTopic.c_str(),"F");
      Serial.println("Lights off confirmation sent");
    }
    else
      Serial.println("Payload mismatch at MQTT receive");
  }
  else if (strcmp(fanCommTopic.c_str(), topic) == 0) {
    if (payload[0] == 'O') {
      digitalWrite(fan0, HIGH);
      Serial.println("fan 0 on");
      client.publish(fanStateTopic.c_str(),"O");
      Serial.println("Fan on confirmation sent");
    }
    else if (payload[0] == 'F') {
      digitalWrite(fan0, LOW);
      Serial.println("fan 0 off");
      client.publish(fanStateTopic.c_str(),"F");
      Serial.println("Fan off confirmation sent");
    }
    else
      Serial.println("Payload mismatch at MQTT receive");
  }
  else
    Serial.println("Topic mismatch at MQTT receive");


  Serial.println("--------------------------------------------------------");
}

void reconnect() {

  // Loop until we're reconnected
  while (!client.connected()) {
    setup_wifi();
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe("lig/comm/#");
      client.subscribe("fan/comm/#");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
