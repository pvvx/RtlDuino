/* ========================================================================== */
/*  SHTxx.cpp - Library for SHT1x & SHT7x temperature and humidity sensors    */
/* ========================================================================== */


/******************************************************************************
 * Includes
 ******************************************************************************/

#include "Arduino.h"
#include "SHTxx.h"

extern "C" {
#include <bitband_io.h>
}

/******************************************************************************
 * Definitions
 ******************************************************************************/

// SHTxx command definitions:      adr command r/w
const uint8_t MEAS_TEMP   = 0x03;   // 000  0001   1
const uint8_t MEAS_HUMI   = 0x05;   // 000  0010   1
const uint8_t STAT_REG_W  = 0x06;   // 000  0011   0
const uint8_t STAT_REG_R  = 0x07;   // 000  0011   1
const uint8_t SOFT_RESET  = 0x1e;   // 000  1111   0

// Status register writable bits
const uint8_t SR_MASK     = 0x07;

// getByte flags
const bool noACK  = false;
const bool ACK    = true;

// Temperature & humidity equation constants

#ifdef FOST02 
  const float D1  = -40.0;          // for deg C @ 5V
  const float D2h =   0.04;         // for deg C, 12-bit precision
  const float D2l =   0.04;         // for deg C, 12-bit precision

  const float C1  = -4.0000;        // for FOST02 sensors
  const float C2h =  0.6480;        // for FOST02 sensors, 8-bit precision
  const float C3h = -7.2000E-4;     // for FOST02 sensors, 8-bit precision
  const float C2l =  0.6480;        // for FOST02 sensors, 8-bit precision
  const float C3l = -7.2000E-4;     // for FOST02 sensors, 8-bit precision

  const float T1  =  0.01;          // for V3 and V4 sensors
  const float T2h =  0.00008;       // for V3 and V4 sensors, 12-bit precision
  const float T2l =  0.00008;       // for V3 and V4 sensors, 12-bit precision
#else  
  const float D1  = -40.1;          // for deg C @ 5V
  const float D2h =   0.01;         // for deg C, 14-bit precision
  const float D2l =   0.04;         // for deg C, 12-bit precision

  const float C1  = -2.0468;        // for V4 sensors
  const float C2h =  0.0367;        // for V4 sensors, 12-bit precision
  const float C3h = -1.5955E-6;     // for V4 sensors, 12-bit precision
  const float C2l =  0.5872;        // for V4 sensors, 8-bit precision
  const float C3l = -4.0845E-4;     // for V4 sensors, 8-bit precision
  
  const float T1  =  0.01;          // for V3 and V4 sensors
  const float T2h =  0.00008;       // for V3 and V4 sensors, 12-bit precision
  const float T2l =  0.00128;       // for V3 and V4 sensors, 8-bit precision
#endif  


/******************************************************************************
 * Constructors
 ******************************************************************************/
 
SHTxx::SHTxx(PinName dataPin, PinName clockPin) {
  // Initialize private storage for library functions
  p_pinData = HardSetPin(dataPin, DOUT_OPEN_DRAIN, 0) + 4*32; // GPIO_PORTx_DDR
  p_ipinData = GetInPinBitBandAddr(dataPin);	// GPIO_EXT_PORTx
  p_pinClock = HardSetPin(clockPin, DOUT_PUSH_PULL, 1); // GPIO_PORTx_DR
  _presult = NULL;                  // No pending measurement
  _stat_reg = 0x00;                 // Sensor status register default state
  
  // Return sensor to default state
  resetConnection();                // Reset communication link with sensor
  putByte(SOFT_RESET);              // Send soft reset command
}


/******************************************************************************
 * User functions
 ******************************************************************************/

// All-in-one (blocking): Returns temperature, humidity, & dewpoint
uint8_t SHTxx::measure(float *temp, float *humi, float *dew) {
  uint16_t rawData;
  uint8_t error;
  if (error = measTemp(&rawData))
    return error;
  *temp = calcTemp(rawData);
  if (error = measHumi(&rawData))
    return error;
  *humi = calcHumi(rawData, *temp);
  *dew = calcDewpoint(*humi, *temp);
  return 0 ;
}

// Initiate measurement.  If blocking, wait for result
uint8_t SHTxx::meas(uint8_t cmd, uint16_t *result, bool block) {
  uint8_t error, i;
#ifdef CRC_ENA
  _crc = bitrev(_stat_reg & SR_MASK);  // Initialize CRC calculation
#endif
  startTransmission();
  if (cmd == TEMP)
    cmd = MEAS_TEMP;
  else
    cmd = MEAS_HUMI;
  if (error = putByte(cmd))
    return error;
#ifdef CRC_ENA
  calcCRC(cmd, &_crc);              // Include command byte in CRC calculation
#endif
  // If non-blocking, save pointer to result and return
  if (!block) {
    _presult = result;
    return 0;
  }
  // Otherwise, wait for measurement to complete with 720ms timeout
  i = 240;
  while (*p_ipinData) {
    i--;
    if (i == 0)
      return S_Err_TO;              // Error: Timeout
    delay(3);
  }
  error = getResult(result);
  return error;
}

// Check if non-blocking measurement has completed
// Non-zero return indicates complete (with or without error)
uint8_t SHTxx::measRdy(void) {
  uint8_t error = 0;
  if (_presult == NULL)             // Already done?
    return S_Meas_Rdy;
  if (*p_ipinData != 0)   // Measurement ready yet?
    return 0;
  error = getResult(_presult);
  _presult = NULL;
  if (error)
    return error;                   // Only possible error is S_Err_CRC
  return S_Meas_Rdy;
}

// Get measurement result from sensor (plus CRC, if enabled)
uint8_t SHTxx::getResult(uint16_t *result) {
  uint8_t val;
#ifdef CRC_ENA
  val = getByte(ACK);
  calcCRC(val, &_crc);
  *result = val;
  val = getByte(ACK);
  calcCRC(val, &_crc);
  *result = (*result << 8) | val;
  val = getByte(noACK);
  val = bitrev(val);
  if (val != _crc) {
    *result = 0xFFFF;
    return S_Err_CRC;
  }
#else
  *result = getByte(ACK);
  *result = (*result << 8) | getByte(noACK);
#endif
  return 0;
}

// Write status register
uint8_t SHTxx::writeSR(uint8_t value) {
  uint8_t error;
  value &= SR_MASK;                 // Mask off unwritable bits
  _stat_reg = value;                // Save local copy
  startTransmission();
  if (error = putByte(STAT_REG_W))
    return error;
  return putByte(value);
}

// Read status register
uint8_t SHTxx::readSR(uint8_t *result) {
  uint8_t val;
  uint8_t error = 0;
#ifdef CRC_ENA
  _crc = bitrev(_stat_reg & SR_MASK);  // Initialize CRC calculation
#endif
  startTransmission();
  if (error = putByte(STAT_REG_R)) {
    *result = 0xFF;
    return error;
  }
#ifdef CRC_ENA
  calcCRC(STAT_REG_R, &_crc);       // Include command byte in CRC calculation
  *result = getByte(ACK);
  calcCRC(*result, &_crc);
  val = getByte(noACK);
  val = bitrev(val);
  if (val != _crc) {
    *result = 0xFF;
    error = S_Err_CRC;
  }
#else
  *result = getByte(noACK);
#endif
  return error;
}

// Public reset function
// Note: Soft reset returns sensor status register to default values
uint8_t SHTxx::reset(void) {
  _stat_reg = 0x00;                 // Sensor status register default state
  resetConnection();                // Reset communication link with sensor
  return putByte(SOFT_RESET);       // Send soft reset command & return status
}


/******************************************************************************
 * SHTxx data communication
 ******************************************************************************/

// Write byte to sensor and check for acknowledge
uint8_t SHTxx::putByte(uint8_t value) {
  uint8_t mask, i;
  uint8_t error = 0;
  mask = 0x80;                      // Bit mask to transmit MSB first
  for (i = 8; i > 0; i--) {
    *p_pinData = (value & mask) == 0;
    PULSE_SHORT;
    *p_pinClock = 1;  				// Generate clock pulse
    PULSE_LONG;
    *p_pinClock = 0;
    PULSE_SHORT;
    mask >>= 1;                     // Shift mask for next data bit
  }
  *p_pinData = 0;			        // Return data line to input mode
  PULSE_SHORT;
  *p_pinClock = 1;    				// Clock #9 for ACK
  PULSE_LONG;
  if (*p_ipinData)        // Verify ACK ('0') received from sensor
    error = S_Err_NoACK;
  PULSE_SHORT;
  *p_pinClock = 0;     // Finish with clock in low state
  PULSE_SHORT;
  return error;
}

// Read byte from sensor and send acknowledge if "ack" is true
uint8_t SHTxx::getByte(bool ack) {
  uint8_t i;
  uint8_t result = 0;
  for (i = 8; i > 0; i--) {
    PULSE_SHORT;
    result <<= 1;                   // Shift received bits towards MSB
    *p_pinClock = 1;  				// Generate clock pulse
    PULSE_LONG;
    result |= *p_ipinData;  // Merge next bit into LSB position
    *p_pinClock = 0;
    PULSE_SHORT;
  }
  *p_pinData = ack;	// Assert ACK ('0') if ack == 1
  PULSE_SHORT;
  *p_pinClock = 1;			// Clock #9 for ACK / noACK
  PULSE_LONG;
  *p_pinClock = 0;			// Finish with clock in low state
  PULSE_SHORT;
  *p_pinData = 0;     	// Return data line to input mode
  return result;
}


/******************************************************************************
 * SHTxx signaling
 ******************************************************************************/

// Generate SHTxx-specific transmission start sequence
// This is where SHTxx does not conform to the I2C standard and is
// the main reason why the AVR TWI hardware support can not be used.
//       _____         ________
// DATA:      |_______|
//           ___     ___
// SCK : ___|   |___|   |______
void SHTxx::startTransmission(void) {
  *p_pinData = 0;    // Set data register high before turning on
  PULSE_SHORT;
  *p_pinClock = 1;
  PULSE_SHORT;
  *p_pinData = 1;
  PULSE_SHORT;
  *p_pinClock = 0;
  PULSE_SHORT;
  *p_pinClock = 1;
  PULSE_SHORT;
  *p_pinData = 0;    // Set data register high before turning on
  PULSE_SHORT;
  *p_pinClock = 0;
  PULSE_SHORT;
}

// Communication link reset
// At least 9 SCK cycles with DATA=1, followed by transmission start sequence
//      ______________________________________________________         ________
// DATA:                                                      |_______|
//          _    _    _    _    _    _    _    _    _        ___     ___
// SCK : __| |__| |__| |__| |__| |__| |__| |__| |__| |______|   |___|   |______
void SHTxx::resetConnection(void) {
  uint8_t i;
  *p_pinData = 0;    // Set data register high before turning on
  PULSE_LONG;
  for (i = 0; i < 9; i++) {
    *p_pinClock = 1;
    PULSE_LONG;
    *p_pinClock = 0;
    PULSE_LONG;
  }
  startTransmission();
}


/******************************************************************************
 * Helper Functions
 ******************************************************************************/

// Calculates temperature in degrees C from raw sensor data
float SHTxx::calcTemp(uint16_t rawData) {
#ifdef DEFAULT_RES
  return D1 + D2h * (float) rawData;
#else
  if (_stat_reg & LOW_RES)
    return D1 + D2l * (float) rawData;
  else
    return D1 + D2h * (float) rawData;
#endif
}

// Calculates relative humidity from raw sensor data
//   (with temperature compensation)
float SHTxx::calcHumi(uint16_t rawData, float temp) {
  float humi;
#ifdef DEFAULT_RES
  humi = C1 + C2h * rawData + C3h * rawData * rawData;
  humi = (temp - 25.0) * (T1 + T2h * rawData) + humi;
#else
  if (_stat_reg & LOW_RES) {
    humi = C1 + C2l * rawData + C3l * rawData * rawData;
    humi = (temp - 25.0) * (T1 + T2l * rawData) + humi;
  } else {
    humi = C1 + C2h * rawData + C3h * rawData * rawData;
    humi = (temp - 25.0) * (T1 + T2h * rawData) + humi;
  }
#endif  
  if (humi > 100.0) humi = 100.0;
  if (humi < 0.1) humi = 0.1;
  return humi;
}

// Calculates dew point in degrees C
float SHTxx::calcDewpoint(float humi, float temp) {
  float k;
  k = log(humi/100) + (17.62 * temp) / (243.12 + temp);
  return 243.12 * k / (17.62 - k);
}

#ifdef CRC_ENA
// Calculate CRC for a single byte
void SHTxx::calcCRC(uint8_t value, uint8_t *crc) {
  const uint8_t POLY = 0x31;   // Polynomial: x**8 + x**5 + x**4 + 1
  uint8_t i;
  *crc ^= value;
  for (i = 8; i > 0; i--) {
    if (*crc & 0x80)
      *crc = (*crc << 1) ^ POLY;
    else
      *crc = (*crc << 1);
  }
}

// Bit-reverse a byte (for CRC calculations)
uint8_t SHTxx::bitrev(uint8_t value) {
  uint8_t i;
  uint8_t result = 0;
  for (i = 8; i > 0; i--) {
    result = (result << 1) | (value & 0x01);
    value >>= 1;
  }
  return result;
}
#endif
