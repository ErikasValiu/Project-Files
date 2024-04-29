////////
// Erikas Valiukevicius
// 20/3/24
//accelorometer
////////
#include <Wire.h>
#include <Adafruit_MMA8451.h>
#include <Adafruit_Sensor.h>

Adafruit_MMA8451 mma = Adafruit_MMA8451();

#define SDA_PIN 8 //for ESP32-S3, gpio 08
#define SCL_PIN 9

// Global variables to store accelerometer data
float x_acceleration, y_acceleration;
float x_velocity = 0.0;
float y_velocity = 0.0;

const float threshold = 4.9;

void setup(void) {
  Serial.begin(9600);
  
  Wire.begin(SDA_PIN, SCL_PIN);
  Serial.println("Adafruit MMA8451 test!");
  

  if (! mma.begin()) {
    Serial.println("Couldnt start");
    while (1);
  }
  Serial.println("MMA8451 found!");
  
  mma.setRange(MMA8451_RANGE_2_G);
  
  Serial.print("Range = "); Serial.print(2 << mma.getRange());  
  Serial.println("G");
  
}

void loop() {
  /* Get a new sensor event */ 
  sensors_event_t event; 
  mma.getEvent(&event);

  x_acceleration = event.acceleration.x;
  y_acceleration = event.acceleration.y;

  // Integrate acceleration along X and Y axes to obtain velocity
  // Integration step size (time interval) dt
  float dt = 0.15; // Example time interval in seconds
  x_velocity += x_acceleration * dt;
  y_velocity += y_acceleration * dt;

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
  Serial.println(rotation);
  Serial.print(" with a speed of "); 
  if(rotation == "not rotating") x_velocity = 0;
  Serial.print(x_velocity);
  Serial.println(" m/s");

  delay(500);
}