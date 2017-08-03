
#include <Wire.h>

void UserPreInit(void)
{
  // 0 - 166666666 Hz, 1 - 83333333 Hz, 2 - 41666666 Hz, 3 - 20833333 Hz, 4 - 10416666 Hz, 5 - 4000000 Hz
  // 6 - 200000000 Hz, 7 - 10000000 Hz, 8 - 50000000 Hz, 9 - 25000000 Hz, 10 - 12500000 Hz, 11 - 4000000 Hz
  // Init_CPU_CLK_UART(5, 38400); // 4 MHz
}

#define MLX90614_I2CADDR 0x5A
// RAM
#define MLX90614_RAWIR1 0x04
#define MLX90614_RAWIR2 0x05
#define MLX90614_TA 0x06
#define MLX90614_TOBJ1 0x07
#define MLX90614_TOBJ2 0x08
// EEPROM
#define MLX90614_TOMAX 0x20
#define MLX90614_TOMIN 0x21
#define MLX90614_PWMCTRL 0x22
#define MLX90614_TARANGE 0x23
#define MLX90614_EMISS 0x24
#define MLX90614_CONFIG 0x25
#define MLX90614_ADDR 0x0E
#define MLX90614_ID1 0x3C
#define MLX90614_ID2 0x3D
#define MLX90614_ID3 0x3E
#define MLX90614_ID4 0x3F

uint16_t MLX90614_read16(uint8_t a) {
  uint16_t ret = 0;
  do {
    Wire.beginTransmission(MLX90614_I2CADDR); // start transmission to device
    Wire.write(a); // sends register address to read from
    Wire.endTransmission(false); // end transmission
    Wire.requestFrom(MLX90614_I2CADDR, (uint8_t)3);// send data n-bytes read
    ret = Wire.read(); // receive DATA
    ret |= Wire.read() << 8; // receive DATA

    uint8_t pec = Wire.read();
    Wire.endTransmission(); // end transmission
    if (ret == 0xFFFF) {
      Wire.beginTransmission(0xFF); // start transmission to device
      Wire.endTransmission(); // end transmission
      delay(250);
    }
  } while (ret == 0xFFFF);
  return ret;
}

float MLX90614_readTemp(uint8_t reg) {
  float tt = MLX90614_read16(reg);
  tt *= 0.02;
  tt  -= 273.15;
  return tt;
}

void setup() {
  Wire.begin();
  Wire.setClock(50000); // x2 = 100 kHz
  printf("\nObject / Ambient Temp:\r\n");
}

void loop() {
//  printf("\r%04x %0.2f / %0.2f C\t", MLX90614_read16(MLX90614_RAWIR1), MLX90614_readTemp(MLX90614_TOBJ1), MLX90614_readTemp(MLX90614_TA));
  printf("\r%0.2f\t%0.2f C\t", MLX90614_readTemp(MLX90614_TOBJ1), MLX90614_readTemp(MLX90614_TA));
  delay(100);
}

