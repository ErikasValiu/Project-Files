#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include "arduino_secrets.h"
#include <PubSubClient.h> //by Nick O'Leary

// Use hardware serial for ESP32-S3 DevKit
#define GPS_SERIAL Serial2

WiFiMulti wiFiMulti;
WiFiClient wiFiClient;
PubSubClient mqttClient(wiFiClient);
const uint32_t connectTimeoutMs = 10000;

const char broker[] = "b00141111.westeurope.cloudapp.azure.com";
int port = 1883; // MQTT port
const char topic[] = "GPS";
const long interval = 8000;
unsigned long previousMillis = 0;

char ssid[] = SECRET_SSID;    // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)

void setup() {
  Serial.begin(115200);
  GPS_SERIAL.begin(9600, SERIAL_8N1, 18, 17); // RX, TX pins for ESP32-S3 DevKit

  delay(10);
  WiFi.mode(WIFI_STA);

  // Add list of WiFi networks
  wiFiMulti.addAP(ssid, pass);

  Serial.println("Connecting Wifi...");
  while (wiFiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Connecting to broker...");
  mqttClient.setServer(broker, port);
  if (!mqttClient.connect("ESP32")) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.state());
    while (1);
  }
  else {
    Serial.println("Connected to broker");
  }
}

void loop() {
  unsigned long currentMillis = millis();

  // Reconnect to WiFi if necessary
  if (currentMillis - previousMillis >= interval) {
    if (wiFiMulti.run(connectTimeoutMs) == WL_CONNECTED) {
      Serial.print("WiFi connected: ");
      Serial.print(WiFi.SSID());
      Serial.print(" ");
      Serial.println(WiFi.RSSI());
    }
    else {
      Serial.println("WiFi not connected!");
    }
    previousMillis = currentMillis;
  }

  // Reconnect to MQTT if necessary
  reconnect();

  // Read GPS data and publish
  if (GPS_SERIAL.available()) {
    String gpsData = GPS_SERIAL.readStringUntil('\n');
    if (gpsData.startsWith("$GPGGA")) {
      // Parsing GPS data
      char* tokens[15];
      int i = 0;
      char* token = strtok((char*)gpsData.c_str(), ",");
      while (token != NULL) {
        tokens[i++] = token;
        token = strtok(NULL, ",");
      }

      // Ensure latitude, longitude, and altitude fields are available
      if (i >= 15 && tokens[2][0] != '\0' && tokens[4][0] != '\0' && tokens[9][0] != '\0') {
        // Extracting latitude, longitude, altitude
        float latitude = atof(tokens[2]) / 100; // Convert to float directly
        float longitude = atof(tokens[4]) / 100;
        float altitude = atof(tokens[9]);

        // Concatenate latitude, longitude, and altitude into a single payload
        String payload = String(latitude, 6) + "," + String(longitude, 6) + "," + String(altitude);

        // Print parsed GPS data
        Serial.print("Latitude: ");
        Serial.println(latitude, 6);
        Serial.print("Longitude: ");
        Serial.println(longitude, 6);
        Serial.print("Altitude: ");
        Serial.println(altitude);

        // Publish parsed GPS data to MQTT
        mqttClient.publish(topic, payload.c_str());
      }
    }
  }

  delay(1000);
}

void reconnect() {
  while (wiFiMulti.run() == WL_CONNECTED && !mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (mqttClient.connect("ESP32")) {
      Serial.println("connected");
    }
    else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}
