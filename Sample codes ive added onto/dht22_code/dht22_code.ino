////////
// Erikas Valiukevicius
// 20/3/24
//accelorometer
////////
#include <DHT.h>     //DHT sensor library by Adafruit

#define DHTPIN  1     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22   // DHT 22 (AM2302)

DHT dht(DHTPIN, DHTTYPE);

float temperature = 0;
float humidity = 0;

void setup() {
  Serial.begin(9600);
  dht.begin();
}

void loop() {
  // Wait a few seconds between measurements.
  delay(2000);

  // Reading temperature or humidity takes about 250 milliseconds!
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();

  // Check if any reads failed and exit early (to try again).
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read data from DHT sensor!");
//    return;
  }
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print(" °C\t");

  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");
}