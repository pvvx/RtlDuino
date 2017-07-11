/*
 * Query a SHT10 temperature and humidity sensor
 *
 * A simple example that queries the sensor every 5 seconds
 * and communicates the result over a serial connection.
 * Error handling is omitted in this example.
 */

#include <SHTxx.h>

PinName dataPin =  PC_4;//12;                 // SHTxx serial data
PinName sclkPin =  PC_5;//13;                 // SHTxx serial clock

float temperature;
float humidity;
float dewpoint;

SHTxx tempSensor = SHTxx(dataPin, sclkPin);

void setup()
{
  Serial.begin(38400);
}

void loop()
{
  tempSensor.measure(&temperature, &humidity, &dewpoint);

  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print(" C, Humidity: ");
  Serial.print(humidity);
  Serial.print(" %, Dewpoint: ");
  Serial.print(dewpoint);
  Serial.println(" C");
  
  delay(1000);  
}
