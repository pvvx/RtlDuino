/******************************************************************************
 *
 * FileName: spiram_fifo.c
 * RTL8710: (c) kissste, pvvx
 *
*******************************************************************************/
#include "rtl_common.h"
#include "diag.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"

#include "spiram_fifo.h"
#include "playerconfig.h"


typedef struct _sBUF_FIFO_ {
	xSemaphoreHandle mux;
	xSemaphoreHandle semCanRead;
	xSemaphoreHandle semCanWrite;
	int fifoRpos, fifoWpos, fifoFill, fifoSize;
	long fifoOvfCnt, fifoUdrCnt;
	unsigned char * buf;
} BUF_FIFO, * PBUF_FIFO;

PBUF_FIFO pbuf_fifo;

#define FIFO_REZSIZE 2048

void RamFifoClose(void) {
	if(pbuf_fifo != NULL) {
		if(pbuf_fifo->mux != NULL) vSemaphoreDelete(pbuf_fifo->mux); // xSemaphoreTake(mux, portMAX_DELAY);
		if(pbuf_fifo->semCanRead != NULL) vSemaphoreDelete(pbuf_fifo->semCanRead);
		if(pbuf_fifo->semCanWrite != NULL) vSemaphoreDelete(pbuf_fifo->semCanWrite);
		if(pbuf_fifo->buf != NULL) {
			if((int)pbuf_fifo->buf >= 0x1FFF0000) tcm_heap_freemem(pbuf_fifo->buf, pbuf_fifo->fifoSize);
			else vPortFree(pbuf_fifo->buf);
		}
		pbuf_fifo = NULL;
		DBG_8195A("FIFO: Closed.\n");
	}
}

static int RamFifoAlloc(int size) {
	pbuf_fifo = (PBUF_FIFO) pvPortMalloc(sizeof(BUF_FIFO));
	if(pbuf_fifo == NULL) return 0;
	pbuf_fifo->mux = NULL;
	pbuf_fifo->semCanRead = NULL;
	pbuf_fifo->semCanWrite = NULL;
	pbuf_fifo->fifoSize = 0;
	if(tcm_heap_freeSpace() > size + 4*1024) pbuf_fifo->buf = tcm_heap_allocmem(size);
	else pbuf_fifo->buf = pvPortMalloc(size);
	if(pbuf_fifo->buf == NULL) return 0;
	pbuf_fifo->fifoSize = size;
	vSemaphoreCreateBinary(pbuf_fifo->semCanRead);
	if(pbuf_fifo->semCanRead == NULL) return 0;
	vSemaphoreCreateBinary(pbuf_fifo->semCanWrite);
	if(pbuf_fifo->semCanWrite == NULL) return 0;
	pbuf_fifo->mux = xSemaphoreCreateMutex();
	if(pbuf_fifo->mux == NULL) return 0;
	return 1;
}

//Initialize the FIFO
int RamFifoInit(int size) {
	if(size < 2*FIFO_REZSIZE) {
		DBG_8195A("FIFO: Buffer size < %d?", 2*FIFO_REZSIZE);
		return 0;
	}
	if(pbuf_fifo == NULL) {
		if (!RamFifoAlloc(size)) {
			RamFifoClose();
			DBG_8195A("FIFO: Low Heap!\n");
			return 0;
		}
	}
	xSemaphoreTake(pbuf_fifo->mux, portMAX_DELAY);
	pbuf_fifo->fifoRpos = 0;
	pbuf_fifo->fifoWpos = 0;
	pbuf_fifo->fifoFill = 0;
	pbuf_fifo->fifoOvfCnt = 0;
	pbuf_fifo->fifoUdrCnt = 0;
	if (pbuf_fifo->fifoSize != size) {
		vPortFree(pbuf_fifo->buf);
		pbuf_fifo->buf = pvPortMalloc(size);
		if(pbuf_fifo->buf == NULL) {
			pbuf_fifo->fifoSize = 0;
			xSemaphoreGive(pbuf_fifo->mux);
			DBG_8195A("FIFO: Low Heap!\n");
			return 0;
		}
		pbuf_fifo->fifoSize = size;
	}
	DBG_8195A("FIFO: Alloc %d bytes at %p\n", pbuf_fifo->fifoSize, pbuf_fifo->buf);
	xSemaphoreGive(pbuf_fifo->mux);
	return 1;
}

// Read bytes from the FIFO
void RamFifoRead(char *buff, int len) {
	while (len>0) {
		int n = len;
//		if (n > FIFO_REZSIZE) n = FIFO_REZSIZE;			//don't read more than SPIREADSIZE
		if (n > (pbuf_fifo->fifoSize - pbuf_fifo->fifoRpos)) n = pbuf_fifo->fifoSize - pbuf_fifo->fifoRpos; //don't read past end of buffer
		xSemaphoreTake(pbuf_fifo->mux, portMAX_DELAY);
		if (pbuf_fifo->fifoFill < n) {
			// DBG_8195A("FIFO empty.\n");
			//Drat, not enough data in FIFO. Wait till there's some written and try again.
			pbuf_fifo->fifoUdrCnt++;
			xSemaphoreGive(pbuf_fifo->mux);
			if (pbuf_fifo->fifoFill < pbuf_fifo->fifoSize - FIFO_REZSIZE) xSemaphoreTake(pbuf_fifo->semCanRead, portMAX_DELAY);
		} else {
			//Read the data.
			memcpy(buff, &pbuf_fifo->buf[pbuf_fifo->fifoRpos], n);
			buff += n;
			len -= n;
			pbuf_fifo->fifoFill -= n;
			pbuf_fifo->fifoRpos += n;
			if (pbuf_fifo->fifoRpos >= pbuf_fifo->fifoSize) pbuf_fifo->fifoRpos = 0;
			xSemaphoreGive(pbuf_fifo->mux);
			xSemaphoreGive(pbuf_fifo->semCanWrite); //Indicate writer thread there's some free room in the fifo
		}
	}
}

//Write bytes to the FIFO
void RamFifoWrite(char *buff, int len) {
	while (len > 0) {
		int n = len;
//		if (n > FIFO_REZSIZE) n = FIFO_REZSIZE;		//don't read more than SPIREADSIZE
		if (n > (pbuf_fifo->fifoSize - pbuf_fifo->fifoWpos)) n = pbuf_fifo->fifoSize - pbuf_fifo->fifoWpos; //don't read past end of buffer

		xSemaphoreTake(pbuf_fifo->mux, portMAX_DELAY);
		if ((pbuf_fifo->fifoSize - pbuf_fifo->fifoFill) < n) {
			// DBG_8195A("FIFO full.\n");
			//Drat, not enough free room in FIFO. Wait till there's some read and try again.
			pbuf_fifo->fifoOvfCnt++;
			xSemaphoreGive(pbuf_fifo->mux);
			xSemaphoreTake(pbuf_fifo->semCanWrite, portMAX_DELAY);
		} else {
			// Write the data.
			memcpy(&pbuf_fifo->buf[pbuf_fifo->fifoWpos], buff, n);
			buff += n;
			len -= n;
			pbuf_fifo->fifoFill += n;
			pbuf_fifo->fifoWpos += n;
			if (pbuf_fifo->fifoWpos >= pbuf_fifo->fifoSize) pbuf_fifo->fifoWpos = 0;
			xSemaphoreGive(pbuf_fifo->mux);
			xSemaphoreGive(pbuf_fifo->semCanRead); //Tell reader thread there's some data in the fifo.
		}
	}
}

//Get amount of bytes in use
int RamFifoFill() {
	xSemaphoreTake(pbuf_fifo->mux, portMAX_DELAY);
	int ret = pbuf_fifo->fifoFill;
	xSemaphoreGive(pbuf_fifo->mux);
	return ret;
}

int RamFifoFree() {
	return (pbuf_fifo->fifoSize - RamFifoFill());
}

int RamFifoLen() {
	return pbuf_fifo->fifoSize;
}

long RamGetOverrunCt() {
	xSemaphoreTake(pbuf_fifo->mux, portMAX_DELAY);
	long ret = pbuf_fifo->fifoOvfCnt;
	xSemaphoreGive(pbuf_fifo->mux);
	return ret;
}

long RamGetUnderrunCt() {
	xSemaphoreTake(pbuf_fifo->mux, portMAX_DELAY);
	long ret = pbuf_fifo->fifoUdrCnt;
	xSemaphoreGive(pbuf_fifo->mux);
	return ret;
}

