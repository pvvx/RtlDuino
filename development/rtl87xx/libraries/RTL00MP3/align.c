
#include "rtl_common.h"
/*
char unalChar(const char *adr) {
	return (*((unsigned int *)((unsigned int)adr & (~3))))>>(((unsigned int)adr & 3) << 3);
}
*/

short unalShort(const short *adr) {
	int *p=(int *)((int)adr&(~3));
	int v=*p;
	int w=((int)adr&3);
	if (w==0) return v; else return (v>>16);
}
