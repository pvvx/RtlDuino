/* Copyright (C) 2015 Kristian Sloth Lauszus. All rights reserved.

 Based on the code by Randy Mackay. DIYDrones.com

 This code is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This code is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this code.  If not, see <http://www.gnu.org/licenses/>.

 Contact information
 -------------------

 Kristian Sloth Lauszus
 Web      :  http://www.lauszus.com
 e-mail   :  lauszus@gmail.com
*/

#include <SPI.h>

void UserPreInit(void)
{
  Init_CPU_CLK_UART(0, 38400); // 166.7 MHz
  // 0 - 166666666 Hz, 1 - 83333333 Hz, 2 - 41666666 Hz, 3 - 20833333 Hz, 4 - 10416666 Hz, 5 - 4000000 Hz
  // 6 - 200000000 Hz, 7 - 10000000 Hz, 8 - 50000000 Hz, 9 - 25000000 Hz, 10 - 12500000 Hz, 11 - 4000000 Hz
}

SPISettings spiSettings(2000000, MSBFIRST, SPI_MODE3); // 2 MHz, mode 3

// Register Map for the ADNS3080 Optical OpticalFlow Sensor
#define ADNS3080_PRODUCT_ID            0x00
#define ADNS3080_REVISION_ID           0x01
#define ADNS3080_MOTION                0x02
#define ADNS3080_DELTA_X               0x03
#define ADNS3080_DELTA_Y               0x04
#define ADNS3080_SQUAL                 0x05
#define ADNS3080_CONFIGURATION_BITS    0x0A
#define ADNS3080_MOTION_CLEAR          0x12
#define ADNS3080_FRAME_CAPTURE         0x13
#define ADNS3080_SROM_ID               0x1f
#define ADNS3080_Observation           0x3D
#define ADNS3080_MOTION_BURST          0x50

// ADNS3080 hardware config
#define ADNS3080_PIXELS_X              30
#define ADNS3080_PIXELS_Y              30

// Id returned by ADNS3080_PRODUCT_ID register
#define ADNS3080_PRODUCT_ID_VALUE      0x17

static const uint8_t RESET_PIN = 12; // PC_4
static const uint8_t SS_PIN = 3; // PA_3

static int32_t x, y;

void setup() {
  uint8_t buf[0x40];
  SPI.begin();
  // Set reset pin as output
  pinMode(RESET_PIN, OUTPUT);
  while (1) {
    reset();
    for (uint8_t addr = 0; addr <= 0x40; addr++) buf[addr] = spiRead(addr);
    if (buf[0] == ADNS3080_PRODUCT_ID_VALUE) {
      printf("ADNS-3080 found (id = 0x%02x, rev = 0x%02x, fw = 0x%02x)\n", buf[ADNS3080_PRODUCT_ID], buf[ADNS3080_REVISION_ID], buf[ADNS3080_SROM_ID]);
      break;
    }
    else {
      printf("Could not find ADNS-3080: (id = %02x)\n", buf[ADNS3080_PRODUCT_ID]);
      delay(1500);
    }
  }
  printf("Config reg = 0x%02x\n", buf[ADNS3080_CONFIGURATION_BITS]);
  spiWrite(ADNS3080_CONFIGURATION_BITS, buf[ADNS3080_CONFIGURATION_BITS] | 0x10); // Set resolution to 1600 counts per inch
}

void loop() {
#if 0
  updateSensor();
#else
  printf("image data --------------\n");
  printPixelData();
  printf("-------------------------\n");
  delay(1500);
#endif
}

void printPixelData(void) {
  bool isFirstPixel = true;

  // Write to frame capture register to force capture of frame
  spiWrite(ADNS3080_FRAME_CAPTURE, 0x83);

  // Wait 3 frame periods + 10 nanoseconds for frame to be captured
  delay(5); // Minimum frame speed is 2000 frames/second so 1 frame = 500 nano seconds. So 500 x 3 + 10 = 1510

  // Display the pixel data
  for (uint8_t i = 0; i < ADNS3080_PIXELS_Y; i++) {
    for (uint8_t j = 0; j < ADNS3080_PIXELS_X; j++) {
      uint8_t regValue = spiRead(ADNS3080_FRAME_CAPTURE);
      if (isFirstPixel && !(regValue & 0x40)) {
        printf("Failed to find first pixel\n");
        goto reset;
      }
      isFirstPixel = false;
      uint8_t pixelValue = regValue << 2; // Only lower 6 bits have data
#if 1
      if (j < ADNS3080_PIXELS_X - 1 )  printf("%u,", pixelValue);
      else printf("%u\n", pixelValue);
#else
      if (j < ADNS3080_PIXELS_X - 1 )  printf("%02x,", pixelValue);
      else printf("%02x\n", pixelValue);
#endif
    }
  }
reset:
  reset(); // Hardware reset to restore sensor to normal operation
}

void updateSensor(void) {
  // Read sensor
  uint8_t buf[4];
  spiRead(ADNS3080_MOTION_BURST, buf, 4);
  uint8_t motion = buf[0];
  //Serial.print(motion & 0x01); // Resolution

  if (motion & 0x10) // Check if we've had an overflow
    printf("ADNS-3080 overflow\n");
  else if (motion & 0x80) {
    int8_t dx = buf[1];
    int8_t dy = buf[2];
    uint8_t surfaceQuality = buf[3];
    x += dx;
    y += dy;
    // Print values
    printf("%d, %d\t%d, %d\t%d\n", x, dx, y, dy, surfaceQuality);
  }
#if 0
  else
    printf("0x%02x\n", motion);
#endif
  delay(10);
}

void reset(void) {
  digitalWrite(RESET_PIN, HIGH); // Set high
  delayMicroseconds(10);
  digitalWrite(RESET_PIN, LOW); // Set low
  delay(2); //    delayMicroseconds(1500); // Wait for sensor to get ready
}

// Will cause the Delta_X, Delta_Y, and internal motion registers to be cleared
void clearMotion() {
  spiWrite(ADNS3080_MOTION_CLEAR, 0xFF); // Writing anything to this register will clear the sensor's motion registers
  x = y = 0;
}

void spiWrite(uint8_t reg, uint8_t data) {
  spiWrite(reg, &data, 1);
}

void spiWrite(uint8_t reg, uint8_t *data, uint8_t length) {
  SPI.beginTransaction(SS_PIN, spiSettings);

  SPI.transfer(reg | 0x80, SPI_CONTINUE); // Indicate write operation
  delayMicroseconds(75); // Wait minimum 75 us in case writing to Motion or Motion_Burst registers
  SPI.transfer(data, length); // Write data

  SPI.endTransaction();
}

uint8_t spiRead(uint8_t reg) {
  uint8_t buf;
  spiRead(reg, &buf, 1);
  return buf;
}

void spiRead(uint8_t reg, uint8_t *data, uint8_t length) {
  SPI.beginTransaction(SS_PIN, spiSettings);
  SPI.transfer(reg, SPI_CONTINUE); // Send register address // , SPI_CONTINUE
  delayMicroseconds(75); // Wait minimum 75 us in case writing to Motion or Motion_Burst registers
  memset(data, 0, length); // Make sure data buffer is 0
  SPI.transfer(data, length); // Write data
  SPI.endTransaction();
}



