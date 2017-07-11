/*
 * Example code for SHT1x or SHT7x sensors demonstrating blocking calls
 * for temperature and humidity measurement in the setup routine and
 * non-blocking calls in the main loop.  The pin 13 LED is flashed as a
 * background task while temperature and humidity measurements are made.
 * Note that the status register read/write demonstration code places the
 * sensor in low resolution mode.  Delete it to stay in high res mode.
 *
 * This example contains two versions of the code: one that checks library
 * function return codes for error indications and one that does not.
 * The version with error checking may be useful in debuging possible
 * connection issues with the sensor.  A #define selects between versions.
 */

#include <SHTxx.h>

#define ENA_ERRCHK  1 // Enable error checking code

PinName dataPin =  PC_4;//12;                 // SHTxx serial data
PinName sclkPin =  PC_5;//13;                 // SHTxx serial clock
volatile uint8_t * pledPin;
const unsigned long TRHSTEP   = 750UL;  // Sensor query period
const unsigned long BLINKSTEP =  250UL;  // LED blink period

SHTxx sht = SHTxx(dataPin, sclkPin);

uint16_t rawData;
float temperature;
float humidity;
float dewpoint;

byte ledState = 0;
byte measActive = false;
byte measType = TEMP;

unsigned long trhMillis = 0;             // Time interval tracking
unsigned long blinkMillis = 0;

#ifdef ENA_ERRCHK

// This version of the code checks return codes for errors
uint8_t error = 0;

void setup() {
  byte stat;
  Serial.begin(38400);
  pledPin = HardSetPin(PA_4, DOUT_PUSH_PULL, 1); 
  delay(55);                             // Wait >= 11 ms before first cmd
// Demonstrate status register read/write
  if (error = sht.readSR(&stat))         // Read sensor status register
    logError(error);
  Serial.print("Status reg = 0x");
  Serial.println(stat, HEX);
  if (error = sht.writeSR(LOW_RES))      // Set sensor to low resolution
    logError(error);
  if (error = sht.readSR(&stat))         // Read sensor status register again
    logError(error);
  Serial.print("Status reg = 0x");
  Serial.println(stat, HEX);
// Demonstrate blocking calls
  if (error = sht.measTemp(&rawData))    // sht.meas(TEMP, &rawData, BLOCK)
    logError(error);
  temperature = sht.calcTemp(rawData);
  if (error = sht.measHumi(&rawData))    // sht.meas(HUMI, &rawData, BLOCK)
    logError(error);
  humidity = sht.calcHumi(rawData, temperature);
  dewpoint = sht.calcDewpoint(humidity, temperature);
  logData();
}

void loop() {
  unsigned long curMillis = millis();          // Get current time

  // Rapidly blink LED.  Blocking calls take too long to allow this.
  if (curMillis - blinkMillis >= BLINKSTEP) {  // Time to toggle the LED state?
    ledState ^= 1;
    *pledPin = ledState;
    blinkMillis = curMillis;
  }

  // Demonstrate non-blocking calls
  if (curMillis - trhMillis >= TRHSTEP) {      // Time for new measurements?
    measActive = true;
    measType = TEMP;
    if (error = sht.meas(TEMP, &rawData, NONBLOCK)) // Start temp measurement
      logError(error);
    trhMillis = curMillis;
  }
  if (measActive && (error = sht.measRdy())) { // Check measurement status
    if (error != S_Meas_Rdy)
      logError(error);
    if (measType == TEMP) {                    // Process temp or humi?
      measType = HUMI;
      temperature = sht.calcTemp(rawData);     // Convert raw sensor data
      if ((error = sht.meas(HUMI, &rawData, NONBLOCK))!= 0) // Start humi measurement
        logError(error);
    } else {
      measActive = false;
      humidity = sht.calcHumi(rawData, temperature); // Convert raw sensor data
      dewpoint = sht.calcDewpoint(humidity, temperature);
      logData();
    }
  }
}

#else  // If ENA_ERRCHK is not defined

// This code is the same as above but without error checking
void setup() {
  byte stat;
  byte error = 0;
  Serial.begin(38400);
  pledPin = HardSetPin(ledPin, PIN_OUTPUT, PullNone, 1);
  sht.reset();
  delay(15);                             // Wait >= 11 ms before first cmd
// Demonstrate status register read/write
  sht.readSR(&stat);                     // Read sensor status register
  Serial.print("Status reg = 0x");
  Serial.println(stat, HEX);
  sht.writeSR(LOW_RES);                  // Set sensor to low resolution
  sht.readSR(&stat);                     // Read sensor status register again
  Serial.print("Status reg = 0x");
  Serial.println(stat, HEX);
// Demonstrate blocking calls
  sht.measTemp(&rawData);                // sht.meas(TEMP, &rawData, BLOCK)
  temperature = sht.calcTemp(rawData);
  sht.measHumi(&rawData);                // sht.meas(HUMI, &rawData, BLOCK)
  humidity = sht.calcHumi(rawData, temperature);
  dewpoint = sht.calcDewpoint(humidity, temperature);
  logData();
}


void loop() {
  unsigned long curMillis = millis();          // Get current time

  // Rapidly blink LED.  Blocking calls take too long to allow this.
  if (curMillis - blinkMillis >= BLINKSTEP) {  // Time to toggle the LED state?
    ledState ^= 1;
    *pledPin = ledState;
    blinkMillis = curMillis;
  }

  // Demonstrate non-blocking calls
  if (curMillis - trhMillis >= TRHSTEP) {      // Time for new measurements?
    measActive = true;
    measType = TEMP;
    sht.meas(TEMP, &rawData, NONBLOCK);        // Start temp measurement
    trhMillis = curMillis;
  }
  if (measActive && sht.measRdy()) {           // Check measurement status
    if (measType == TEMP) {                    // Process temp or humi?
      measType = HUMI;
      temperature = sht.calcTemp(rawData);     // Convert raw sensor data
      sht.meas(HUMI, &rawData, NONBLOCK);      // Start humi measurement
    } else {
      measActive = false;
      humidity = sht.calcHumi(rawData, temperature); // Convert raw sensor data
      dewpoint = sht.calcDewpoint(humidity, temperature);
      logData();
    }
  }
}

#endif  // End of non-error-checking example

void logData() {
  Serial.print("Temperature = ");   Serial.print(temperature);
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

