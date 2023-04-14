#include "DHT.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define DHTPIN D6
#define SOILPIN A0
#define RELAYPIN D1

#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

int threshold = 500;

const char* ssid = "SSID";
const char* password = "Password ";
const char* mqtt_server = "ip";
const int mqtt_port = 1883;
const char* mqtt_pub_topic = "intellify/pub";
const char* mqtt_sub_topic = "intellify/sub";


WiFiClient espClient;
PubSubClient client(espClient);

void callback(char* topic, byte* payload, unsigned int length) {
  String payload_string;
  for (int i = 0; i < length; i++) {
    payload_string += (char)payload[i];
  }
  threshold = payload_string.toInt();
}


void reconnect() {
  while (!client.connected()) {
    Serial.println("Connecting to MQTT");
    if (client.connect("ESP8266Client")) {
      Serial.println("Connected to MQTT ");
      if (client.subscribe(mqtt_sub_topic, 0)) {
        Serial.println("subscribe successfully!");
      } else {
        Serial.println("Failed to Subscribe!");
      }
    } else {
      Serial.print("Failed  ");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

void setup() {

  Serial.begin(9600);

  dht.begin();
  pinMode(SOILPIN, INPUT);
  pinMode(RELAYPIN, OUTPUT);

  Serial.println("Connecting ");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("");

  Serial.println("WiFi connected!");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }

  client.loop();


  float h = dht.readHumidity();

  float t = dht.readTemperature();

  int soil_moist = analogRead(SOILPIN);

  if (isnan(h) || isnan(t)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  if (soil_moist >= threshold) {
    digitalWrite(RELAYPIN, HIGH); // turn on the relay
    Serial.println("Soil moisture is above threshold, turning on relay.");
  } else {
    digitalWrite(RELAYPIN, LOW); // turn off the relay
    Serial.println("Soil moisture is below threshold, turning off relay.");
  }

  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.print(t);
  Serial.print(F("°C "));

  String msg = "{\"temp\":" + String(t) + ", \"hum\":" + String(h) + ", \"threshold\":" + threshold + " , \"soil\": " + soil_moist + "}";
  client.publish(mqtt_pub_topic, msg.c_str());
  Serial.println("sent");

  delay(2000);
}
