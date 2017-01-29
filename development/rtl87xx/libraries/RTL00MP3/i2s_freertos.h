#ifndef _I2S_FREERTOS_H_
#define _I2S_FREERTOS_H_
/*
 * RTL8710: (c) kissste, pvvx
 */

#include "i2s_api.h"
#include "playerconfig.h"

#define I2S_DMA_PAGE_WAIT_MS_MIN 	4 // 8 // min 2 ms (CPU CLK 166), min 4 ms (CPU CLK 83),
#define I2S_DMA_PAGE_SIZE_MS_96K	(96000/1000) // in sizeof(u32)
#define I2S_DMA_PAGE_NUM    		4   // Valid number is 2~4

#define I2S0_SCLK_PIN            PE_1 // PD_1
#define I2S0_WS_PIN              PE_0 // PD_0
#define I2S0_SD_PIN              PE_2 // PD_2

#define I2S1_SCLK_PIN            PC_1
#define I2S1_WS_PIN              PC_0
#define I2S1_SD_PIN              PC_2

#define I2S_DEBUG_LEVEL 0

typedef struct _I2S_OBJS_ {
	i2s_t i2s_obj;
	u32 *currDMABuff;	// Current DMA buffer we're writing to
	u32 currDMABuffPos;	// Current position in that DMA buffer
	s32 sampl_err;
#if I2S_DEBUG_LEVEL > 1
	u32 underrunCnt;	// DMA underrun counter
#endif
}I2S_OBJS, *PI2S_OBJS;

#define MAX_I2S_OBJS 2
#define I2S0_OBJSN 0
#define I2S1_OBJSN 1

//extern PI2S_OBJS pi2s[MAX_I2S_OBJS]; // I2S0, I2S1

int i2sInit(int mask, int bufsize, int word_len); // word_len = WL_16b or WL_24b
void i2sClose(int mask);
char i2sSetRate(int mask, int rate);
u32 i2sPushPWMSamples(u32 sample);

#if I2S_DEBUG_LEVEL > 1
long i2sGetUnderrunCnt(int num);
#endif

#endif
