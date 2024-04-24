/////////
//Erikas valiukevicius
//03/04/24
//
//code of wifi + mqtt parsing to node red which creates a site with the data on it
// 
//
////////

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include "arduino_secrets.h" // Include the file containing sensitive information
#include <PubSubClient.h> // Include the MQTT library by Nick O'Leary

// Use hardware serial for ESP32-S3 DevKit
#define GPS_SERIAL Serial2

WiFiMulti wiFiMulti; // Create an instance of WiFiMulti for handling multiple WiFi networks
WiFiClient wiFiClient; // Create a WiFi client instance for communication
PubSubClient mqttClient(wiFiClient); // Create an MQTT client instance using the WiFi client

const uint32_t connectTimeoutMs = 10000; // Timeout for WiFi connection

// MQTT broker details
const char broker[] = "b00141111.westeurope.cloudapp.azure.com"; // MQTT broker address
int port = 1883; // MQTT port
const char topic[] = "GPS"; // Topic to publish GPS data
const long interval = 8000; // Interval for reconnecting to WiFi
unsigned long previousMillis = 0;

char ssid[] = SECRET_SSID;    // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)

void setup() {
  Serial.begin(115200); // Start serial communication for debugging
  GPS_SERIAL.begin(9600, SERIAL_8N1, 18, 17); // Start serial communication for GPS module

  delay(10);
  WiFi.mode(WIFI_STA); // Set WiFi mode to Station mode

  // Add list of WiFi networks
  wiFiMulti.addAP(ssid, pass);

  Serial.println("Connecting Wifi...");
  while (wiFiMulti.run() != WL_CONNECTED) { // Wait for WiFi connection
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Connecting to broker...");
  mqttClient.setServer(broker, port); // Set MQTT broker and port
  if (!mqttClient.connect("ESP32")) { // Connect to MQTT broker
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
    if (wiFiMulti.run(connectTimeoutMs) == WL_CONNECTED) { // Check if WiFi is connected
      Serial.print("WiFi connected: ");
      Serial.print(WiFi.SSID()); // Print SSID of connected network
      Serial.print(" ");
      Serial.println(WiFi.RSSI()); // Print WiFi signal strength
    }
    else {
      Serial.println("WiFi not connected!");
    }
    previousMillis = currentMillis;
  }

  // Reconnect to MQTT if necessary
  reconnect();

  // Read GPS data and publish
  if (GPS_SERIAL.available()) { // Check if GPS data is available
    String gpsData = GPS_SERIAL.readStringUntil('\n'); // Read GPS data until newline character
    if (gpsData.startsWith("$GPGGA")) { // Check if GPS data is in GPGGA format
      // Parsing GPS data
      char* tokens[15];
      int i = 0;
      char* token = strtok((char*)gpsData.c_str(), ","); // Tokenize GPS data
      while (token != NULL) {
        tokens[i++] = token;
        token = strtok(NULL, ",");
      }

      // Ensure latitude, longitude, and altitude fields are available
      if (i >= 15 && tokens[2][0] != '\0' && tokens[4][0] != '\0' && tokens[9][0] != '\0') {
        // Extracting latitude, longitude, altitude
        float latitude = atof(tokens[2]) / 100; // Convert latitude to float
        float longitude = atof(tokens[4]) / -100; // Convert longitude to float
        float altitude = atof(tokens[9]); // Convert altitude to float

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

  delay(1000); // Delay for stability
}

void reconnect() {
  while (wiFiMulti.run() == WL_CONNECTED && !mqttClient.connected()) { // Check if WiFi is connected and MQTT is disconnected
    Serial.print("Attempting MQTT connection...");
    if (mqttClient.connect("ESP32")) { // Try to reconnect to MQTT broker
      Serial.println("connected");
    }
    else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      delay(5000); // Wait for 5 seconds before retrying
    }
  }
}
