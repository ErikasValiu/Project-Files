#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>
//#include "arduino_secrets.h" // Include the file containing sensitive information
#include <PubSubClient.h> // Include the MQTT library by Nick O'Leary

#include <DHT.h>     //DHT sensor library by Adafruit

#include <Wire.h>
#include <Adafruit_MMA8451.h>
#include <Adafruit_Sensor.h>

#define SECRET_SSID "AndroidAP60C2"
#define SECRET_PASS "elly4279"

//Pins Definitions
#define PIEZO_PIN 16 // Piezo output
#define DHTPIN  1     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22   // DHT 22 (AM2302)
#define SDA_PIN 8 //for ESP32-S3, gpio 08
#define SCL_PIN 9

DHT dht(DHTPIN, DHTTYPE);
Adafruit_MMA8451 mma = Adafruit_MMA8451();

// Use hardware serial for ESP32-S3 DevKit
#define GPS_SERIAL Serial2

WiFiMulti wiFiMulti; // Create an instance of WiFiMulti for handling multiple WiFi networks
WiFiClient wiFiClient; // Create a WiFi client instance for communication
PubSubClient mqttClient(wiFiClient); // Create an MQTT client instance using the WiFi client

const uint32_t connectTimeoutMs = 10000; // Timeout for WiFi connection

// MQTT broker details
const char broker[] = "b00141111.westeurope.cloudapp.azure.com"; // MQTT broker address
int port = 1883; // MQTT port
const char topicLatitude[] = "GPS/latitude"; // Topic to publish latitude
const char topicLongitude[] = "GPS/longitude"; // Topic to publish longitude
const char topicShock[] = "PIEZO/shock";
const char topicTemp[] = "DHT/temperature";
const char topicHumid[] = "DHT/humidity";
const char topicDirect[] = "ACCEL/orientation_rotation";
const char topicSpeed[] = "ACCEL/speed";

const long interval = 10000; // Interval for reconnecting to WiFi
unsigned long previousMillis = 0;

char ssid[] = SECRET_SSID;    // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)

void setup() {
  Serial.begin(115200); // Start serial communication for debugging
  GPS_SERIAL.begin(9600, SERIAL_8N1, 18, 17); // Start serial communication for GPS module

  dht.begin();
  Wire.begin(SDA_PIN, SCL_PIN);
  Serial.println("Adafruit MMA8451 test!");
  if (! mma.begin()) {
    Serial.println("Couldnt start the accelerometer sensor");
    while (1);
  }
  Serial.println("MMA8451 found!");
  mma.setRange(MMA8451_RANGE_2_G);
  Serial.print("Range = "); Serial.print(2 << mma.getRange());  
  Serial.println("G");

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

        // Publish latitude and longitude separately to MQTT, altitude is not publishing to broker
        mqttClient.publish(topicLatitude, String(latitude, 6).c_str());
        mqttClient.publish(topicLongitude, String(longitude, 6).c_str());
        

        // Print parsed GPS data
        Serial.print("Latitude: ");
        Serial.println(latitude, 6);
        Serial.print("Longitude: ");
        Serial.println(longitude, 6);
        Serial.print("Altitude: ");
        Serial.println(altitude);
      }
    }
  }

  // Read Piezo ADC value in, and convert it to a voltage
  int piezoADC = analogRead(PIEZO_PIN);
  float piezoV = map(piezoADC, 0, 4096, 0, 100.0);
  mqttClient.publish(topicShock, String(piezoV, 2).c_str());
  Serial.print("The Shock pulse is: "); // Print the shock voltage.
  Serial.println(piezoV);

  // Reading temperature or humidity takes about 250 milliseconds!
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  mqttClient.publish(topicTemp, String(temperature, 2).c_str());
  mqttClient.publish(topicHumid, String(humidity, 1).c_str());
  // Check if any reads failed and exit early (to try again).
  if (isnan(temperature) || isnan(humidity))
    Serial.println("Failed to read data from DHT sensor!");
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print(" °C\t");
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");

  /* Get a new Acc. sensor event */ 
  sensors_event_t event; 
  mma.getEvent(&event);
  float x_acceleration, y_acceleration, x_velocity, y_velocity;
  x_acceleration = event.acceleration.x;
  y_acceleration = event.acceleration.y;
  // Integrate acceleration along X and Y axes to obtain velocity
  // Integration step size (time interval) dt
  float dt = 0.15; // Example time interval in seconds
  x_velocity += x_acceleration * dt;
  y_velocity += y_acceleration * dt;
  const float threshold = 4.9;
  // Check device orientation using switch cases
  String orientation;
  if (y_acceleration > threshold) {
    orientation = "moving forward";
  } else if (y_acceleration < -threshold) {
    orientation = "moving backward";
  } else {
    orientation = "stationary";
  }
  // Print device movement status and speed
  Serial.print("Device is ");
  Serial.print(orientation);
  Serial.print(" with a speed of "); 
  if(orientation == "stationary") y_velocity = 0;
  Serial.print(y_velocity);
  Serial.println(" m/s");
  // Check device rotation using switch cases
  String rotation;
  if (x_acceleration > threshold) {
    rotation = "going toward right";
  } else if (x_acceleration < -threshold) {
    rotation = "going toward left";
  } else {
    rotation = "not rotating";
  }
  // Print device rotation status
  Serial.print("Device is ");
  Serial.print(rotation);
  Serial.print(" with a speed of "); 
  if(rotation == "not rotating") x_velocity = 0;
  Serial.print(x_velocity);
  Serial.println(" m/s");

  mqttClient.publish(topicDirect, (orientation + ", " + rotation).c_str());
  mqttClient.publish(topicSpeed, (String(x_velocity) + ", " + String(y_velocity)).c_str());

  delay(500); // Delay for stability
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
