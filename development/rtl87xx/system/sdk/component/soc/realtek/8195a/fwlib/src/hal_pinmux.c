/*
 * hal_pinmux.c
 *
 *  Created on: 08/10/2016
 *      Author: pvvx
*/
#include "rtl8195a.h"

//-------------------------------------------------------------------------
// Function declarations
/*
u8 GpioFunctionChk(IN u32  chip_pin, IN u8 Operation);
u32 GpioIcFunChk(IN u32 chip_pin, IN u8 Operation);
u8 FunctionChk(IN u32 Function, IN u32 PinLocation);
u8 RTL8710afFunChk(IN u32 Function, IN u32 PinLocation);
void HalJtagPinOff();
*/
// extern _LONG_CALL_ u8 HalPinCtrlRtl8195A(IN u32  Function, IN u32  PinLocation, IN BOOL   Operation);
// extern HALEFUSEOneByteReadRAM();
//-------------------------------------------------------------------------
// Data declarations
extern u16 GPIOState[];
#define REG_EFUSE_0xF8 0xF8 // [0xF8] = 0xFC RTL8710AF

#define RTL8710_DEF_PIN_ON 0

//----- HalJtagPinOff
void HalJtagPinOff(void)
{
	HalPinCtrlRtl8195A(JTAG, 0, 0);
}

#if RTL8710_DEF_PIN_ON

//----- GpioIcFunChk
u8 GpioIcFunChk(IN u32 chip_pin, IN u8 Operation)
{
  u8 tst, result;
  HALEFUSEOneByteReadRAM(HAL_READ32(SYSTEM_CTRL_BASE, REG_SYS_EFUSE_CTRL), REG_EFUSE_0xF8, &tst, L25EOUTVOLTAGE);

  tst += 8; // tst = 0xfc+8 = 0x04
  if ( tst > 7 ) result = 0;
  else {
	  tst = 1 << tst;  // v6 = 0x10
	  if (tst & 0xEF) result = 1;
	  else {
		  result = tst & 0x10;
	      if(tst & 0x10) { // RTL8710AF ?
	          if (chip_pin - 1 <= 2) result = 0; // PA_1, PA_2, PA_3
	          else {
	            result = chip_pin - PC_5;  // PC_5
	            if (chip_pin != PC_5)
	              result = 1;
	          }
	      }
	  }
  }
  return result;
}

#endif // RTL8710_DEF_PIN_ON

//----- GpioFunctionChk
u8 GpioFunctionChk(IN u32 chip_pin, IN u8 Operation)
{
  u8 result;
  u16 tst;

#if RTL8710_DEF_PIN_ON
  result = GpioIcFunChk(chip_pin, Operation);
#else
  result = 1;
#endif
  if(result) {
    result = 1;
    tst = 1 << (chip_pin & 0xF);
    if (!Operation) {
      tst = GPIOState[chip_pin >> 4] & (~tst);
      GPIOState[chip_pin >> 4] = tst;
      return result;
    }
    if (!(GPIOState[chip_pin >> 4] & tst)) {
      tst |= GPIOState[chip_pin >> 4];
      GPIOState[chip_pin >> 4] = tst;
      return result;
    }
    result = 0;
  }
  return result;
}

#if RTL8710_DEF_PIN_ON
//----- RTL8710afFunChk
u8 RTL8710afFunChk(IN u32 Function, IN u32  PinLocation)
{
  u8 result;
  if (Function == SPI0_MCS) //  SPI0_MCS
    return PinLocation - 1 + (PinLocation - 1 <= 0) - (PinLocation - 1);
  if (Function > I2C0) {
    if (Function == I2S1) goto LABEL_15;
    if(Function > I2S1) {
    	if(Function == JTAG || Function == LOG_UART) return 1;
    }
    else if(Function == I2C3) goto LABEL_15;
    return 0;
  }
  if(Function == UART2) goto LABEL_15;
  if(Function == SPI0)
    return PinLocation - 1 + (PinLocation - 1 <= 0) - (PinLocation - 1);
  if (Function != UART0) return 0;
LABEL_15:
  result = 1 - PinLocation;
  if (PinLocation > 1) result = 0;
  return result;
}
#endif // RTL8710_DEF_PIN_ON

//----- FunctionChk
u8 FunctionChk( IN u32  Function, IN u32 PinLocation)
{
#if RTL8710_DEF_PIN_ON
  u8 result, tst;
  HALEFUSEOneByteReadRAM(HAL_READ32(SYSTEM_CTRL_BASE, REG_SYS_EFUSE_CTRL), REG_EFUSE_0xF8, &tst, L25EOUTVOLTAGE);
  tst += 8; // tst = 0xfc+8 = 0x04
  if ( tst > 7 ) result = 0;
  else {
	tst = 1 << tst;  // v6 = 0x10
    if (tst & 0xEF) result = 1;
    else {
      result = tst & 0x10;
      if (tst & 0x10)
        result = RTL8710afFunChk(Function, PinLocation);
    }
  }
  return result;
#else
  return 1;
#endif // RTL8710_DEF_PIN_ON
}
