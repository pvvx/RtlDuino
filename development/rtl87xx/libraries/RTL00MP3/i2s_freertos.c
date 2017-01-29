/******************************************************************************
 *
 * FileName: i2s_freertos.c
 *
 * Description: I2S output routines for a FreeRTOS system.
 *
 * Modification history:
 *     2015/10, RTL8710 kissste, pvvx
*******************************************************************************/

/*
How does this work? Basically, to get sound, you need to:
- Connect an I2S codec to the I2S pins on the RTL.
- Start up a thread that's going to do the sound output
- Call I2sInit()
- Call I2sSetRate() with the sample rate you want.
- Generate sound and call i2sPushSample() with 32-bit samples.
The 32bit samples basically are 2 16-bit signed values (the analog values for
the left and right channel) concatenated as (Rout<<16)+Lout

I2sPushSample will block when you're sending data too quickly, so you can just
generate and push data as fast as you can and I2sPushSample will regulate the
speed.
*/

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "playerconfig.h"

#include "i2s_api.h"
#include "i2s_freertos.h"

#define USE_RTL_I2S_API  0 // speed

PI2S_OBJS pi2s[MAX_I2S_OBJS]; // I2S0, I2S1

// i2s interrupt callback
static void i2s_test_tx_complete(void *data, char *pbuf)
{
#if I2S_DEBUG_LEVEL > 1
    i2s_t *i2s_obj = (i2s_t *)data;
	int idx = i2s_obj->InitDat.I2SIdx;
	int reg = HAL_I2S_READ32(idx, REG_I2S_TX_PAGE0_OWN);
	reg |= HAL_I2S_READ32(idx, REG_I2S_TX_PAGE1_OWN);
	reg |= HAL_I2S_READ32(idx, REG_I2S_TX_PAGE2_OWN);
	reg |= HAL_I2S_READ32(idx, REG_I2S_TX_PAGE3_OWN);
	if(!(reg & BIT_PAGE_I2S_OWN_BIT)) pi2s[idx]->underrunCnt++;
#endif
}

void i2sClose(int mask) {
	int i;
	for(i = 0; i < MAX_I2S_OBJS; i++) {
		if(mask & (1 << i)) {
			if(pi2s[i] != NULL)	{
				if(pi2s[i]->i2s_obj.InitDat.I2SEn != I2S_DISABLE) {
					i2s_disable(&pi2s[i]->i2s_obj); // HalI2SDisable(&pi2s[i]->i2s_obj.I2SAdapter);
					i2s_deinit(&pi2s[i]->i2s_obj); // HalI2SDeInit(&pi2s[i]->i2s_obj.I2SAdapter);
#if I2S_DEBUG_LEVEL > 0
					DBG_8195A("I2S%d: i2s_disable (%d)\n", i, pi2s[i]->i2s_obj.InitDat.I2SEn);
#endif
				}
				if(pi2s[i]->i2s_obj.InitDat.I2STxData != NULL) {
					vPortFree(pi2s[i]->i2s_obj.InitDat.I2STxData);
					pi2s[i]->i2s_obj.InitDat.I2STxData = NULL;
				}
				vPortFree(pi2s[i]);
				pi2s[i] = NULL;
				if(i==0) HalPinCtrlRtl8195A(JTAG, 0, 1);
				DBG_8195A("I2S%d: Closed.\n", i);
			}
		}
	}
}

//Initialize I2S subsystem for DMA circular buffer use
int i2sInit(int mask, int bufsize, int word_len) { // word_len = WL_16b or WL_24b
#if I2S_DEBUG_LEVEL > 2
	DBG_ERR_MSG_ON(_DBG_I2S_ | _DBG_GDMA_);
	DBG_INFO_MSG_ON(_DBG_I2S_ | _DBG_GDMA_);
	DBG_WARN_MSG_ON(_DBG_I2S_ | _DBG_GDMA_);
#endif
	if(bufsize < I2S_DMA_PAGE_SIZE_MS_96K*2) {
		DBG_8195A("I2S: Min buffer %d bytes!\n", I2S_DMA_PAGE_SIZE_MS_96K*2);
		return 0;
	}
	int page_size = bufsize * sizeof(u32);
	if(word_len != WL_16b) page_size <<= 1; //bufsize *2;
	int i;
	for(i = 0; i < MAX_I2S_OBJS; i++) {
		if (mask & (1 << i)) {
			if(pi2s[i] != NULL) i2sClose(1 << i);
			PI2S_OBJS pi2s_new = pvPortMalloc(sizeof(I2S_OBJS));
			if(pi2s_new == NULL) {
		        DBG_8195A("I2S%d: Not heap buffer %d bytes!\n", i, sizeof(i2s_t) + page_size * I2S_DMA_PAGE_NUM);
				return 0;
			}
			rtl_memset(pi2s_new, 0, sizeof(i2s_t));
			u8 * i2s_tx_buf = (u8 *) pvPortMalloc(page_size * I2S_DMA_PAGE_NUM);
		    if (i2s_tx_buf == NULL) {
		    	vPortFree(pi2s_new);
		        DBG_8195A("I2S%d: Not heap buffer %d bytes!\n", i, sizeof(i2s_t) + page_size * I2S_DMA_PAGE_NUM);
		    	return 0;
		    }
		    pi2s[i] = pi2s_new;
#if I2S_DEBUG_LEVEL > 1
		    pi2s_new->underrunCnt = 0;
#endif
		    pi2s[i]->sampl_err = 0;
		    pi2s_new->currDMABuffPos = 0;
		    pi2s_new->currDMABuff = NULL;

		    i2s_t * pi2s_obj = &pi2s_new->i2s_obj;

		    pi2s_obj->channel_num = CH_STEREO;
		    pi2s_obj->sampling_rate = SR_96KHZ;
		    pi2s_obj->word_length = word_len;
		    pi2s_obj->direction = I2S_DIR_TX; //consider switching to TX only
			if(i == 0) {
				HalPinCtrlRtl8195A(JTAG, 0, 0);
				i2s_init(pi2s_obj, I2S0_SCLK_PIN, I2S0_WS_PIN, I2S0_SD_PIN);
			}
			else i2s_init(pi2s_obj, I2S1_SCLK_PIN, I2S1_WS_PIN, I2S1_SD_PIN);
			i2s_set_param(pi2s_obj, pi2s_obj->channel_num, pi2s_obj->sampling_rate, pi2s_obj->word_length);
		    i2s_set_dma_buffer(pi2s_obj, i2s_tx_buf, NULL, I2S_DMA_PAGE_NUM, page_size);
		    i2s_tx_irq_handler(pi2s_obj, i2s_test_tx_complete, (uint32_t)pi2s_obj);
	//    	i2s_rx_irq_handler(pi2s_obj, (i == 0)? (i2s_irq_handler)i2s1_test_rx_complete : (i2s_irq_handler)i2s2_test_rx_complete, i); // TX only!
		    i2s_enable(pi2s_obj);
		    DBG_8195A("I2S%d: Alloc DMA buf %d bytes (%d x %d samples %d bits)\n", i, page_size * I2S_DMA_PAGE_NUM, I2S_DMA_PAGE_NUM, bufsize, (word_len == WL_16b)? 32 : 96);
		}
	}
}

//Set the I2S sample rate, in HZ
char i2sSetRate(int mask, int rate) {

	int sample_rate;
	char result = 1;
#if defined(OVERSAMPLES) && defined(PWM_HACK96BIT)
	rate <<= 1;
	while (rate <= 48000) {
		rate <<= 1;
		result++;
	}
#endif
	if (rate>=96000) sample_rate = SR_96KHZ;
	else if (rate>=88200) sample_rate = SR_88p2KHZ;
    else if (rate>=48000) sample_rate = SR_48KHZ;
    else if (rate>=44100) sample_rate = SR_44p1KHZ;
    else if (rate>=32000) sample_rate = SR_32KHZ;
	else if (rate>=24000) sample_rate = SR_24KHZ;
	else if (rate>=22050) sample_rate = SR_22p05KHZ;
	else if (rate>=16000) sample_rate = SR_16KHZ;
	else if (rate>=11020) sample_rate = SR_11p02KHZ;
	else if (rate>= 8000) sample_rate = SR_8KHZ;
	else sample_rate = SR_7p35KHZ;
	int i;
	for(i = 0; i < MAX_I2S_OBJS; i++) {
		if (mask & (1 << i)) {
			i2s_t * pi2s_obj = &pi2s[i]->i2s_obj;
			pi2s[i]->sampl_err = 0;
			pi2s_obj->sampling_rate = sample_rate;
#if USE_RTL_I2S_API
			i2s_set_param(pi2s_obj, pi2s_obj->channel_num, pi2s_obj->sampling_rate, pi2s_obj->word_length);
#else
			pi2s_obj->I2SAdapter.pInitDat->I2SRate = sample_rate;
			HalI2SSetRate(pi2s_obj->I2SAdapter.pInitDat);
#endif
		}
	}
	DBG_8195A("I2S: Set Sample Rate %d (x%d)\n", rate, result);
	return result;
}

#if defined(PWM_HACK96BIT)
//This routine pushes a single, 32-bit sample to the I2S buffers. Call this at (on average) 
//at least the current sample rate. You can also call it quicker: it will suspend the calling
//thread if the buffer is full and resume when there's room again.
u32 i2sPushPWMSamples(u32 sample) {
	int i;
	for(i = 0; i < MAX_I2S_OBJS; i++) {
		PI2S_OBJS pi2s_cur = pi2s[i];
		PHAL_I2S_ADAPTER I2SAdapter = &pi2s_cur->i2s_obj.I2SAdapter;
		while(pi2s_cur->currDMABuff == NULL){
#if USE_RTL_I2S_API
			pi2s_cur->currDMABuff = i2s_get_tx_page(&pi2s_cur->i2s_obj);
			if(pi2s_cur->currDMABuff == NULL) vTaskDelay(I2S_DMA_PAGE_WAIT_MS_MIN);
#else
			u8 page_idx = HalI2SGetTxPage((VOID*)I2SAdapter->pInitDat);
			if(page_idx < I2S_DMA_PAGE_NUM)	pi2s_cur->currDMABuff = ((u32 *)I2SAdapter->TxPageList[page_idx]);
			else vTaskDelay(I2S_DMA_PAGE_WAIT_MS_MIN);
#endif
			pi2s_cur->currDMABuffPos = 0;
		}
		u32 *p = &pi2s_cur->currDMABuff[pi2s_cur->currDMABuffPos];
		if(i) sample >>= 16;
		s32 smp = (s16)sample + 0x8000 + pi2s_cur->sampl_err;
		if (smp > 0xffff) smp = 0xffff;
		else if (smp < 0) smp = 0;
		u8 x = smp/(u16)(0x10000/97);
		pi2s_cur->sampl_err = smp - x * (u16)(0x10000/97);
		if(x < 24) {
			*p++ = (1 << x) -1;
			*p++ = 0;
			*p++ = 0;
			*p = 0;
		}
		else if (x < 48) {
			*p++ = 0xFFFFFFFF;
			*p++ = (1 << (x - 24)) -1;
			*p++ = 0;
			*p = 0;
		}
		else if (x < 72) {
			*p++ = 0xFFFFFFFF;
			*p++ = 0xFFFFFFFF;
			*p++ = (1 << (x - 48)) -1;
			*p = 0;
		}
		else if (x < 96) {
			*p++ = 0xFFFFFFFF;
			*p++ = 0xFFFFFFFF;
			*p++ = 0xFFFFFFFF;
			*p = (1 << (x - 72)) -1;
		}
		else {
			*p++ = 0xFFFFFFFF;
			*p++ = 0xFFFFFFFF;
			*p++ = 0xFFFFFFFF;
			*p = 0xFFFFFFFF;
		}
		pi2s_cur->currDMABuffPos += 4;
	}
	portENTER_CRITICAL();
	for(i = 0; i < MAX_I2S_OBJS; i++) {
		PI2S_OBJS pi2s_cur = pi2s[i];
		if (pi2s_cur->currDMABuffPos > pi2s_cur->i2s_obj.InitDat.I2SPageSize) {
#if USE_RTL_I2S_API
			i2s_send_page(&pi2s_cur->i2s_obj, pi2s_cur->currDMABuff);
#else
			PHAL_I2S_ADAPTER I2SAdapter = &pi2s_cur->i2s_obj.I2SAdapter;
			int n;
			for (n = 0; n < I2S_DMA_PAGE_NUM; n++) {
				if (I2SAdapter->TxPageList[n] == pi2s_cur->currDMABuff) {
					HalI2SPageSend(I2SAdapter->pInitDat, n);
					HAL_I2S_WRITE32(i, REG_I2S_TX_PAGE0_OWN + 4 * n, BIT_PAGE_I2S_OWN_BIT);
					break;  // break the for loop
				}
			}
#endif
			pi2s_cur->currDMABuff = NULL;
		}
	}
	portEXIT_CRITICAL();
}
#endif

#if I2S_DEBUG_LEVEL > 1
long i2s1GetUnderrunCnt(int num) {
	return pi2s[num]->underrunCnt;
}
#endif
