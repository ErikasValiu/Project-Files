/////////
//Erikas valiukevicius
//03/04/24
//Quoted sample "gps software serial parsing"
//code of wifi + mqtt
// trying to get the correct message to print to nodered 
//
////////

#include <Arduino.h> 
#include <WiFi.h>
#include <WiFiMulti.h>
#include "arduino_secrets.h"
#include <PubSubClient.h> //by Nick O'Leary
#include <TinyGPS++.h>

#define GPS_BAUDRATE 9600  // The default baudrate of NEO-6M is 9600



WiFiMulti wiFiMulti;
WiFiClient wiFiClient;
PubSubClient mqttClient(wiFiClient);
const uint32_t connectTimeoutMs = 10000;
TinyGPSPlus gps;
//Following is from MQTT sender simple example


const char broker[] = "b00141111.westeurope.cloudapp.azure.com";
int        port     = 1883;
const char topic[]  = "GPS";

const long interval = 8000;
unsigned long previousMillis = 0;

int count = 0;

///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)



// WiFi connect timeout per AP. Increase when connecting takes longer.

void setup(){
  Serial.begin(9600);
    Serial.begin(GPS_BAUDRATE);
  delay(10);
  WiFi.mode(WIFI_STA);
  
  // Add list of wifi networks
  wiFiMulti.addAP(ssid, pass);
  //wifiMulti.addAP("ssid_from_AP_2", "your_password_for_AP_2");
  //wifiMulti.addAP("ssid_from_AP_3", "your_password_for_AP_3");

  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0) {
      Serial.println("no networks found");
  } 
  else {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i) {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");
      delay(10);
    }
  }

  // Connect to Wi-Fi using wifiMulti (connects to the SSID with strongest connection)
  Serial.println("Connecting Wifi...");
  while(wiFiMulti.run()!=WL_CONNECTED){
    Serial.print(".");
  }
 
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Connecting to broker...");
  mqttClient.setServer(broker,1883);
  if (!mqttClient.connect("ESP32")){
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.state());

    while (1);

  }  
  else{
    Serial.println("Connected to broker");
  }

}
void reconnect() {
  // Loop until we're reconnected
  while (wiFiMulti.run()==WL_CONNECTED && !mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqttClient.connect("ESP32")) {
      Serial.println("connected");}
    else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void loop(){
  unsigned long currentMillis = millis();

  //if the connection to the stongest hotstop is lost, it will connect to the next network on the list
  if (currentMillis - previousMillis >=interval){
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

  reconnect();
  if (Serial2.available() > 0) {
    if (gps.encode(Serial2.read())) {
      if (gps.location.isValid()) {
        Serial.print(F("- latitude: "));
        Serial.println(gps.location.lat());

        Serial.print(F("- longitude: "));
        Serial.println(gps.location.lng());

        Serial.print(F("- altitude: "));
        if (gps.altitude.isValid())
          Serial.println(gps.altitude.meters());
        else
          Serial.println(F("INVALID"));
      } else {
        Serial.println(F("- location: INVALID"));
      }

      Serial.print(F("- speed: "));
      if (gps.speed.isValid()) {
        Serial.print(gps.speed.kmph());
        Serial.println(F(" km/h"));
      } else {
        Serial.println(F("INVALID"));
      }

      Serial.print(F("- GPS date&time: "));
      if (gps.date.isValid() && gps.time.isValid()) {
        Serial.print(gps.date.year());
        Serial.print(F("-"));
        Serial.print(gps.date.month());
        Serial.print(F("-"));
        Serial.print(gps.date.day());
        Serial.print(F(" "));
        Serial.print(gps.time.hour());
        Serial.print(F(":"));
        Serial.print(gps.time.minute());
        Serial.print(F(":"));
        Serial.println(gps.time.second());
      } else {
        Serial.println(F("INVALID"));
      }

      Serial.println();
    }
  }

  if (millis() > 5000 && gps.charsProcessed() < 10)
  delay(1000);
    Serial.println(F("No GPS data received: check wiring"));

  mqttClient.beginPublish(topic,1,false);
  mqttClient.write(gps.time.second());
  mqttClient.endPublish();

  delay(1000);

}