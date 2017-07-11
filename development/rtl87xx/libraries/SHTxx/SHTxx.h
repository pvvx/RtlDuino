/* ========================================================================== */
/*  Sensirion.h - Library for Sensirion SHT1x & SHT7x family temperature      */
/*    and humidity sensors                                                    */
/*  Created by Markus Schatzl, November 28, 2008                              */
/*  Released into the public domain                                           */
/*                                                                            */
/*  Revised (v1.1) by Carl Jackson, August 4, 2010                            */
/*  Rewritten (v2.0) by Carl Jackson, December 10, 2010                       */
/*    See README.txt file for details                                         */
/* ========================================================================== */


#ifndef SHTxx_h
#define SHTxx_h

extern "C" {
#include <bitband_io.h>
}

#include <stdint.h>

// Enable CRC checking
#define CRC_ENA

// Clock pulse timing macros
// Lengthening these may assist communication over long wires
#define PULSE_LONG  delayMicroseconds(4)
#define PULSE_SHORT delayMicroseconds(3)

// Useful macros
#define measTemp(result)  meas(TEMP, result, BLOCK)
#define measHumi(result)  meas(HUMI, result, BLOCK)

// User constants
const uint8_t TEMP     =     0;
const uint8_t HUMI     =     1;
const bool    BLOCK    =  true;
const bool    NONBLOCK = false;

// Status register bit definitions
const uint8_t LOW_RES  =  0x01;  // 12-bit Temp / 8-bit RH (vs. 14 / 12)
const uint8_t NORELOAD =  0x02;  // No reload of calibrarion data
const uint8_t HEAT_ON  =  0x04;  // Built-in heater on
const uint8_t BATT_LOW =  0x40;  // VDD < 2.47V

// Function return code definitions
const uint8_t S_Err_NoACK  = 1;  // ACK expected but not received
const uint8_t S_Err_CRC    = 2;  // CRC failure
const uint8_t S_Err_TO     = 3;  // Timeout
const uint8_t S_Meas_Rdy   = 4;  // Measurement ready

class SHTxx
{
  private:
    volatile uint8_t *p_pinData;	
    volatile uint8_t *p_ipinData;
    volatile uint8_t *p_pinClock;
    uint16_t *_presult;
    uint8_t _stat_reg;
#ifdef CRC_ENA
    uint8_t _crc;
#endif
    uint8_t getResult(uint16_t *result);
    uint8_t putByte(uint8_t value);
    uint8_t getByte(bool ack);
    void startTransmission(void);
#ifdef CRC_ENA
    void calcCRC(uint8_t value, uint8_t *crc);
    uint8_t bitrev(uint8_t value);
#endif

  public:
    void resetConnection(void);
    SHTxx(PinName dataPin, PinName clockPin);
    uint8_t measure(float *temp, float *humi, float *dew);   
    uint8_t meas(uint8_t cmd, uint16_t *result, bool block);
    uint8_t measRdy(void);
    uint8_t writeSR(uint8_t value);
    uint8_t readSR(uint8_t *result);
    uint8_t reset(void);
    float calcTemp(uint16_t rawData);
    float calcHumi(uint16_t rawData, float temp);
    float calcDewpoint(float humi, float temp);
};

#endif  // #ifndef SHTxx_h
