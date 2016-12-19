#ifndef _FLASH_EEPMEMORY_H_
#define _FLASH_EEPMEMORY_H_

/** 
 * @class FlashEEPClass FlashEEP.h 
 * Used 2 sectors flash (faddr: 0xFE000...0xFFFFF)
 * Protect when power is turned off during recording
 * The maximum sum total of objects to 4 kbytes
 * RTL871x pvvx 19-12-2016 unlicense http://unlicense.org/
 */

// ObjID reserved for the 'AT' and other applications (!):
#define FEEP_ID_WIFI_CFG 0x5730 //5730..5739 id:'0W'..'9W', type: struct wlan_fast_reconnect
#define FEEP_ID_UART_CFG 0x5530 //5530..5539 id:'0U'..'9U', type: struct UART_LOG_CONF
#define FEEP_ID_LWIP_CFG 0x4C30 //4C30..4C39 id:'0L'..'9L', type: struct atcmd_lwip_conf
#define FEEP_ID_DHCP_CFG 0x4430 //4430..4439 id:'0D'..'9D', type: struct dhcp_cfg

/* include "atcmd_wifi.h"

struct atcmd_wifi_conf{
	struct wlan_fast_reconnect reconn[ATCMD_WIFI_CONN_STORE_MAX_NUM];
	int32_t auto_enable;
	rtw_wifi_setting_t setting;
	int32_t reconn_num;
	int32_t reconn_last_index;	
};

typedef struct _UART_LOG_CONF_{
	u32 BaudRate;
	u8 DataBits;
	u8 StopBits;
	u8 Parity;
	u8 FlowControl;
}UART_LOG_CONF, *PUART_LOG_CONF;

typedef struct _sdhcp_cfg {
	  unsigned char  mode; // =0 dhcp off, =1 - dhcp on, =2 Static ip, =3 - auto
	  unsigned int ip;
	  unsigned int mask;
	  unsigned int gw;
}dhcp_cfg;
*/

class FlashEEPClass
{
public:
    /**
     * Read ObjId
     * sizebuf - Object buf size
     * return - read Object size
	 *   -1 - obj not found
	 *   -2 - flash rd/wr/erase error
	 *   -3 - error
	 *   -4 - overflow FMEMORY_SCFG_BANK_SIZE
	 *   -5 - heap alloc error
     */
    short read(void *buf, unsigned short ObjId, unsigned short maxsize);

    /**
     * Write ObjId
     * objsize - Object buf size (max objsize 512 bytes)
     * return - false = error 
     *
     */
    bool write(void *buf, unsigned short ObjId, unsigned short objsize);
  
};

#endif // _FLASH_EEPMEMORY_H_
