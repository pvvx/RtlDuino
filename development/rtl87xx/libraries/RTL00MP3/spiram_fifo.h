#ifndef _SPIRAM_FIFO_H_
#define _SPIRAM_FIFO_H_
/*
 * RTL8710: (c) kissste, pvvx
 */

int  RamFifoInit(int size);
void  RamFifoRead(char *buff, int len);
void  RamFifoWrite(char *buff, int len);
int  RamFifoFill();
int  RamFifoFree();
long  RamGetOverrunCt();
long  RamGetUnderrunCt();

#endif
