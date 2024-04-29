////////
// Erikas Valiukevicius
// 20/3/24
//shock sensor code
////////
const int PIEZO_PIN = 16; // Piezo output

void setup() 
{
  Serial.begin(9600);
}

void loop() 
{
  // Read Piezo ADC value in, and convert it to a voltage
  int piezoADC = analogRead(PIEZO_PIN);
  //float piezoV = piezoADC / 1023.0 * 5.0;
  float piezoV = map(piezoADC, 0, 4096, 0, 5.0);
  Serial.println(piezoV); // Print the voltage.
  delay(250);
}