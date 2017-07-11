/*
   Query a SHTxx/FOST02x temperature and humidity sensor
*/

#include <SHTxx.h>

PinName dataPin =  PC_4;//12;                 // SHTxx serial data
PinName sclkPin =  PC_5;//13;                 // SHTxx serial clock

float temperature;
float humidity;
float dewpoint;

SHTxx sht = SHTxx(dataPin, sclkPin);

void setup()
{
/*  
  Serial.begin(38400);
  byte stat;
  sht.writeSR(0);                  // Set sensor to low resolution
  sht.readSR(&stat);                     // Read sensor status register again
  Serial.print("Status reg = 0x");
  Serial.println(stat, HEX);
*/
}

uint8_t error = 0;
uint16_t rawData;
byte measType = TEMP;
unsigned long trhMillis = 0;             // Time interval tracking

void loop() {
  unsigned long curMillis = millis();          // Get current time
  if (error) {
    if (error = sht.reset())
      logError(error);
  }
  else {
    error = sht.measRdy();
    if (error == S_Meas_Rdy) switch (measType)  {
        case TEMP:
          measType = HUMI;
          if ((error = sht.meas(HUMI, &rawData, NONBLOCK)) != 0) // Start humi measurement
            logError(error);
          temperature = sht.calcTemp(rawData);     // Convert raw sensor data
          break;
        case HUMI:
          measType = TEMP;
          if (error = sht.meas(TEMP, &rawData, NONBLOCK)) // Start temp measurement
            logError(error);

          humidity = sht.calcHumi(rawData, temperature); // Convert raw sensor data
          dewpoint = sht.calcDewpoint(humidity, temperature);
          logData(curMillis - trhMillis);
          trhMillis = curMillis;
          break;
      }
  }
}

void logData(unsigned long cycletime) {
  Serial.print(cycletime, DEC);
  Serial.print(" ms, Temperature = ");   Serial.print(temperature);
  Serial.print(" C, Humidity = ");  Serial.print(humidity);
  Serial.print(" %, Dewpoint = ");  Serial.print(dewpoint);
  Serial.println(" C");
}

// The following code is only used with error checking enabled
void logError(byte error) {
  switch (error) {
    case S_Err_NoACK:
      Serial.println("Error: No response (ACK) received from sensor!");
      break;
    case S_Err_CRC:
      Serial.println("Error: CRC mismatch!");
      break;
    case S_Err_TO:
      Serial.println("Error: Measurement timeout!");
      break;
    default:
      Serial.println("Unknown error received!");
      break;
  }
}



