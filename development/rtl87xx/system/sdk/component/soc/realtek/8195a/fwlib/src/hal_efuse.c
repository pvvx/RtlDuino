/*
* Disassemble hal_efuse.o pvvx 10.2016
*/
#include "rtl8195a.h"
#ifdef CONFIG_EFUSE_EN
#include "hal_efuse.h"

//#define	NO_ROM_API

#define EFUSE_WRITE_ENABLE 0

#define EFUSE_SECTION_SIZE (1<<7) // 128 bytes
#define EFUSE_BUF_MAX_LEN (1<<5) // 32 bytes
#define OTP_START_ADDR EFUSE_SECTION_SIZE
#define OTP_BUF_MAX_LEN (1<<5) // 32 bytes
#define	EFUSE_SECTION_CODE 11

#ifdef NO_ROM_API
//====================================================== Start libs ROM efuse
//----- HalEFUSEPowerSwitch8195AROM addr 0x6561
_LONG_CALL_ROM_ int HalEFUSEPowerSwitch8195AROM(IN unsigned char bWrite, IN unsigned char PwrState, IN unsigned char L25OutVoltage) {
	  if (PwrState == 1) {
		  HAL_WRITE32(SYSTEM_CTRL_BASE, REG_SYS_EEPROM_CTRL0, (HAL_READ32(SYSTEM_CTRL_BASE, REG_SYS_EEPROM_CTRL0) & 0xFFFFFF) | 0x69000000); // EFUSE_UNLOCK
		  if (!(HAL_READ32(SYSTEM_CTRL_BASE, REG_SYS_FUNC_EN) & BIT_SYS_FEN_EELDR))	// REG_SYS_FUNC_EN BIT_SYS_FEN_EELDR ?
			  HAL_WRITE32(SYSTEM_CTRL_BASE, REG_SYS_FUNC_EN, HAL_READ32(SYSTEM_CTRL_BASE, REG_SYS_FUNC_EN) | BIT_SYS_FEN_EELDR);
		  if (!(HAL_READ32(SYSTEM_CTRL_BASE, REG_SYS_CLK_CTRL0) & BIT_SYSON_CK_EELDR_EN))	// REG_SYS_CLK_CTRL0 BIT_SYSON_CK_EELDR_EN ?
			  HAL_WRITE32(SYSTEM_CTRL_BASE, REG_SYS_CLK_CTRL0, HAL_READ32(SYSTEM_CTRL_BASE, REG_SYS_CLK_CTRL0) | BIT_SYSON_CK_EELDR_EN);
		  if (!(HAL_READ32(SYSTEM_CTRL_BASE, REG_SYS_CLK_CTRL1) & BIT_PESOC_EELDR_CK_SEL)) // REG_SYS_CLK_CTRL1 BIT_PESOC_EELDR_CK_SEL ?
			  HAL_WRITE32(SYSTEM_CTRL_BASE, REG_SYS_CLK_CTRL1, HAL_READ32(SYSTEM_CTRL_BASE, REG_SYS_CLK_CTRL1) | BIT_PESOC_EELDR_CK_SEL);
		  if (bWrite == 1)
			  HAL_WRITE32(SYSTEM_CTRL_BASE, REG_SYS_REGU_CTRL0, (HAL_READ32(SYSTEM_CTRL_BASE, REG_SYS_REGU_CTRL0) & 0xFFFFF0FF) | BIT_SYS_REGU_LDO25E_EN | BIT_SYS_REGU_LDO25E_ADJ(L25OutVoltage));
	  }
	  else
	  {
		  HAL_WRITE32(SYSTEM_CTRL_BASE, REG_SYS_EEPROM_CTRL0, HAL_READ32(SYSTEM_CTRL_BASE, REG_SYS_EEPROM_CTRL0) & 0xFFFFFF); // EFUSE_UNLOCK
		  if ( bWrite == 1 )
			  HAL_WRITE32(SYSTEM_CTRL_BASE, REG_SYS_REGU_CTRL0, (HAL_READ32(SYSTEM_CTRL_BASE, REG_SYS_REGU_CTRL0) & (~BIT_SYS_REGU_LDO25E_EN)));
	  }
	  return bWrite;
}

//----- HALEFUSEOneByteReadROM addr 0x6561
_LONG_CALL_ROM_ int HALEFUSEOneByteReadROM(IN unsigned int CtrlSetting, IN unsigned short Addr, OUT unsigned char *Data, IN unsigned char L25OutVoltage)
{
int i = 0, result = 0;
	if ( (Addr <= 0xFF) || ((CtrlSetting & 0xFFFF) == 0x26AE) ) {
		HalEFUSEPowerSwitch8195AROM(1, 1, L25OutVoltage);

		HAL_WRITE32(SYSTEM_CTRL_BASE, REG_SYS_EFUSE_TEST, HAL_READ32(SYSTEM_CTRL_BASE, REG_SYS_EFUSE_TEST) & (~BIT_SYS_EF_FORCE_PGMEN));
		HAL_WRITE32(SYSTEM_CTRL_BASE, REG_SYS_EFUSE_CTRL,
				(CtrlSetting & (~(BIT_SYS_EF_RWFLAG | (BIT_MASK_SYS_EF_ADDR << BIT_SHIFT_SYS_EF_ADDR)	| (BIT_MASK_SYS_EF_DATA << BIT_SHIFT_SYS_EF_DATA))))
				| BIT_SYS_EF_ADDR(Addr));
		while(1) {
			if(HAL_READ32(SYSTEM_CTRL_BASE, REG_SYS_EFUSE_CTRL) & BIT_SYS_EF_RWFLAG) {
				*Data = HAL_READ32(SYSTEM_CTRL_BASE, REG_SYS_EFUSE_CTRL);
				result = 1;
				break;
			}
			HalDelayUs(1000);
			if (i++ >= 100) {
				*Data = -1;
				break;
			};
		};
		HalEFUSEPowerSwitch8195AROM(1, 0, L25OutVoltage);
	}
	else *Data = -1;
	return result;
}

//----- HALEFUSEOneByteWriteROM addr 0x6699
_LONG_CALL_ROM_ int HALEFUSEOneByteWriteROM(IN unsigned int CtrlSetting, IN unsigned short Addr, IN unsigned char Data, IN unsigned char L25OutVoltage)
{
int i = 0, result = 0;
	if ( (Addr <= 0xFF) || ((CtrlSetting & 0xFFFF) == 0x26AE) ) {
		HalEFUSEPowerSwitch8195AROM(1, 1, L25OutVoltage);
		HAL_WRITE32(SYSTEM_CTRL_BASE, REG_SYS_EFUSE_TEST, HAL_READ32(SYSTEM_CTRL_BASE, REG_SYS_EFUSE_TEST) | BIT_SYS_EF_FORCE_PGMEN);
		HAL_WRITE32(SYSTEM_CTRL_BASE, REG_SYS_EFUSE_CTRL, Data | BIT_SYS_EF_RWFLAG | BIT_SYS_EF_ADDR(Addr) | BIT_SYS_EF_DATA(Data) |
				(CtrlSetting & (~(BIT_SYS_EF_RWFLAG | (BIT_MASK_SYS_EF_ADDR << BIT_SHIFT_SYS_EF_ADDR) | (BIT_MASK_SYS_EF_DATA << BIT_SHIFT_SYS_EF_DATA)))));
		while(1) {
			HalDelayUs(1000);
			if(HAL_READ32(SYSTEM_CTRL_BASE, REG_SYS_EFUSE_CTRL) & BIT_SYS_EF_RWFLAG) break;
			if (i++ >= 100) {
				result = 1;
				break;
			};
		};
		HalEFUSEPowerSwitch8195AROM(1, 0, L25OutVoltage);
	}
	return result;
}
//====================================================== End libs ROM efuse
#endif

//----- HALOTPOneByteReadRAM
int HALOTPOneByteReadRAM(IN unsigned int CtrlSetting, IN unsigned short Addr, OUT  unsigned char *Data, IN unsigned char L25OutVoltage)
{
	int result;
	  if ( (unsigned int)(Addr - EFUSE_SECTION_SIZE) > OTP_BUF_MAX_LEN - 1 )
	    result = 1;
	  else
	    result = HALEFUSEOneByteReadROM(CtrlSetting, Addr, Data, L25OutVoltage);
	return result;
}

//----- HALOTPOneByteWriteRAM
int HALOTPOneByteWriteRAM(IN unsigned int CtrlSetting, IN unsigned short Addr, IN unsigned char Data, IN unsigned char L25OutVoltage)
{
#if EFUSE_WRITE_ENABLE
	int result;
	  if ( (unsigned int)(Addr - EFUSE_SECTION_SIZE) > OTP_BUF_MAX_LEN - 1 )
	    result = 1;
	  else
	    result = HALEFUSEOneByteWriteROM(CtrlSetting, Addr, Data, L25OutVoltage);
	return result;
#else
  return 1;
#endif
}

//----- HALEFUSEOneByteReadRAM
int HALEFUSEOneByteReadRAM(IN unsigned int CtrlSetting, IN unsigned short Addr, IN unsigned char *Data, IN unsigned char L25OutVoltage)
{
	int result;

	if ( (unsigned int)(Addr - 160) > 0x33 )
	{
		result = HALEFUSEOneByteReadROM(CtrlSetting, Addr, Data, L25OutVoltage);
	}
	else
	{
		*Data = -1;
		result = 1;
	}
	return result;
}

//----- HALEFUSEOneByteWriteRAM
int HALEFUSEOneByteWriteRAM(IN unsigned int CtrlSetting, IN unsigned short Addr, IN unsigned char Data, IN unsigned char L25OutVoltage)
{
#if EFUSE_WRITE_ENABLE
	int result;
	if ( (unsigned int)(Addr - 127) <= 0x54 )
	    result = 1;
	else {
		result = HALEFUSEOneByteWriteROM(CtrlSetting, Addr, Data, L25OutVoltage);
	}
	return result;
#else
  return 1;
#endif
}

//----- ReadEfuseContant
void ReadEfuseContant(IN unsigned char UserCode, IN unsigned char *pContant)
{
  unsigned int i, offset, bcnt, eFuse_Addr = 0;
  unsigned char DataTemp0;
  unsigned char DataTemp1;
  unsigned char * pbuf = pContant;

  do {
    HALEFUSEOneByteReadRAM(HAL_READ32(SYSTEM_CTRL_BASE, REG_SYS_EFUSE_CTRL), eFuse_Addr, &DataTemp0, L25EOUTVOLTAGE);
    if (DataTemp0 == 0x0FF)  break;
    if ((DataTemp0 & 0x0F) == 0x0F) {
      HALEFUSEOneByteReadRAM(HAL_READ32(SYSTEM_CTRL_BASE, REG_SYS_EFUSE_CTRL), ++eFuse_Addr, &DataTemp1, L25EOUTVOLTAGE);
      offset = ((DataTemp1 & 0x0F0) | (DataTemp0 >> 4)) >> 1;
      bcnt = (~DataTemp1) & 0x0F;
      if (((UserCode + EFUSE_SECTION_CODE) << 2) > offset || offset >= ((UserCode + EFUSE_SECTION_CODE + 1) << 2))  {
        while(bcnt) {
          if (bcnt & 1) eFuse_Addr += 2;
          bcnt >>= 1;
        }
      }
      else {
        int base = (offset - ((EFUSE_SECTION_CODE + UserCode) << 2)) << 3;
        i = 0;
        while(bcnt) {
          if (bcnt & 1) {
            HALEFUSEOneByteReadRAM(HAL_READ32(SYSTEM_CTRL_BASE, REG_SYS_EFUSE_CTRL), ++eFuse_Addr, &pbuf[base + i], L25EOUTVOLTAGE);
            HALEFUSEOneByteReadRAM(HAL_READ32(SYSTEM_CTRL_BASE, REG_SYS_EFUSE_CTRL), ++eFuse_Addr, &pbuf[base + i + 1], L25EOUTVOLTAGE);
          }
          bcnt >>= 1;
          i += 2;
        }
      }
    }
    else for(i = (~DataTemp0) & 0x0F; i; i >>= 1) if (i & 1) eFuse_Addr += 2;
    eFuse_Addr++;
  }
  while(eFuse_Addr < EFUSE_SECTION_SIZE - 1);
}

//----- ReadEfuseContant1
void ReadEfuseContant1(OUT unsigned char *pContant)
{
	ReadEfuseContant(0, pContant);
}

//----- ReadEfuseContant2
void ReadEfuseContant2(OUT unsigned char *pContant)
{
	ReadEfuseContant(1, pContant);
}

//----- ReadEfuseContant3
void ReadEfuseContant3(OUT unsigned char *pContant)
{
	ReadEfuseContant(2, pContant);
}

//----- GetRemainingEfuseLength
int GetRemainingEfuseLength(void)
{
  unsigned int i, eFuse_Addr = 0;
  unsigned char DataTemp0;
  do
  {
	  HALEFUSEOneByteReadRAM(HAL_READ32(SYSTEM_CTRL_BASE, REG_SYS_EFUSE_CTRL), eFuse_Addr, &DataTemp0, L25EOUTVOLTAGE);
	  if(DataTemp0 == 0x0FF)  break;
	  if((DataTemp0 & 0x0F) == 0x0F)
		  HALEFUSEOneByteReadRAM(HAL_READ32(SYSTEM_CTRL_BASE, REG_SYS_EFUSE_CTRL), ++eFuse_Addr, &DataTemp0, L25EOUTVOLTAGE);
	  for (i = (~DataTemp0) & 0x0F; i; i >>= 1 ) if (i & 1) eFuse_Addr += 2;
	  eFuse_Addr++;
  }
  while(eFuse_Addr < EFUSE_SECTION_SIZE - 1);
  return (EFUSE_SECTION_SIZE - 1 - eFuse_Addr);
}

//----- WriteEfuseContant
int WriteEfuseContant(IN unsigned char UserCode, IN unsigned char CodeWordNum, IN unsigned char WordEnable, IN unsigned char *pContant)
{
#if EFUSE_WRITE_ENABLE
  int result = 0;
  unsigned int i, j, eFuse_Addr; // r4@3
  unsigned char DataTemp0; 
  unsigned int bmask = WordEnable & 0xF;
  
  if (bmask) {
	  eFuse_Addr = 0;
	  do { // eFuse_Addr = 128 - _GetRemainingEfuseLength
		    HALEFUSEOneByteReadRAM(HAL_READ32(SYSTEM_CTRL_BASE, REG_SYS_EFUSE_CTRL), eFuse_Addr, &DataTemp0, L25EOUTVOLTAGE);
		  if (DataTemp0 == 0x0ff) break;
		  if ((DataTemp0 & 0x0F) == 0x0F) 
			 HALEFUSEOneByteReadRAM(HAL_READ32(SYSTEM_CTRL_BASE, REG_SYS_EFUSE_CTRL), ++eFuse_Addr, &DataTemp0, L25EOUTVOLTAGE);
		  for (i = (~DataTemp0) & 0x0F; i; i >>= 1) if (i & 1) eFuse_Addr += 2;
		  eFuse_Addr++;
	  }
	  while (eFuse_Addr <= EFUSE_SECTION_SIZE - 2);
	  
	  j = 0;
	  do
	  {
	    if (bmask & 1) j += 2;
	    bmask >>= 1;
	  }
	  while (bmask);
	  if ((eFuse_Addr + j) <= EFUSE_SECTION_SIZE - 4)
	  {
	    HALEFUSEOneByteWriteRAM(HAL_READ32(SYSTEM_CTRL_BASE, REG_SYS_EFUSE_CTRL), eFuse_Addr, (((UserCode + EFUSE_SECTION_CODE) << 7) | 0x0F) + ((CodeWordNum & 3) << 5), L25EOUTVOLTAGE);
	    HALEFUSEOneByteWriteRAM(HAL_READ32(SYSTEM_CTRL_BASE, REG_SYS_EFUSE_CTRL), eFuse_Addr + 1, (((UserCode + EFUSE_SECTION_CODE) << 3) & 0xF0) | ((~bmask) & 0xF), L25EOUTVOLTAGE);
	    i = 0;
	    while (i < j)
	    {
	      HALEFUSEOneByteWriteRAM(HAL_READ32(SYSTEM_CTRL_BASE, REG_SYS_EFUSE_CTRL), eFuse_Addr + 2 + i, pContant[i], L25EOUTVOLTAGE);
	      i++;
	    }
	    result = 1;
	  }
  }
  return result;
#else  
  return 1;
#endif
}

//----- WriteEfuseContant1
int WriteEfuseContant1(IN unsigned char CodeWordNum, IN unsigned char WordEnable, IN unsigned char *pContant)
{
#if EFUSE_WRITE_ENABLE
  return WriteEfuseContant(0, CodeWordNum, WordEnable, pContant);
#else  
  return 1;
#endif
}

//----- WriteEfuseContant2
int WriteEfuseContant2(IN unsigned char CodeWordNum, IN unsigned char WordEnable, IN unsigned char *pContant)
{
#if EFUSE_WRITE_ENABLE
  return WriteEfuseContant(1, CodeWordNum, WordEnable, pContant);
#else  
  return 1;
#endif
}

//----- WriteEfuseContant2
int WriteEfuseContant3(IN unsigned char CodeWordNum, IN unsigned char WordEnable, IN unsigned char *pContant)
{
#if EFUSE_WRITE_ENABLE
  return WriteEfuseContant(2, CodeWordNum, WordEnable, pContant);
#else  
  return 1;
#endif
}

//----- ReadEOTPContant
void ReadEOTPContant(IN unsigned char *pContant)
{
	int i;
	for(i = 0; i < OTP_BUF_MAX_LEN; i++ )
	    HALOTPOneByteReadRAM(HAL_READ32(SYSTEM_CTRL_BASE, REG_SYS_EFUSE_CTRL), i+EFUSE_SECTION_SIZE, &pContant[i], L25EOUTVOLTAGE);
}

//----- WriteEOTPContant
void WriteEOTPContant(IN unsigned char *pContant)
{
#if EFUSE_WRITE_ENABLE
	int i;
	unsigned char DataTemp0;
	for(i = 0; i < OTP_BUF_MAX_LEN; i++ ) {
		HALOTPOneByteReadRAM(HAL_READ32(SYSTEM_CTRL_BASE, REG_SYS_EFUSE_CTRL), i+EFUSE_SECTION_SIZE, &DataTemp0, L25EOUTVOLTAGE);
		if (DataTemp0 == 0xFF)
			HALOTPOneByteWriteRAM(HAL_READ32(SYSTEM_CTRL_BASE, REG_SYS_EFUSE_CTRL), i+EFUSE_SECTION_SIZE, pContant[i], L25EOUTVOLTAGE);
	}
#endif	
}

//----- HALJtagOff
void HALJtagOff(void)
{
#if EFUSE_WRITE_ENABLE
	HALEFUSEOneByteWriteROM(HAL_READ32(SYSTEM_CTRL_BASE, REG_SYS_EFUSE_CTRL), 211, 0xFE, L25EOUTVOLTAGE);
#endif	
}

#endif //CONFIG_EFUSE_EN
