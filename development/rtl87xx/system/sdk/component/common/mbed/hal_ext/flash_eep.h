/******************************************************************************
 * FileName: flash_eep.h
 * Description: FLASH
 * Alternate SDK 
 * Author: PV`
 * (c) PV` 2015
*******************************************************************************/
#ifndef __FLASH_EEP_H_
#define __FLASH_EEP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <basic_types.h>
#include <FreeRTOS.h>
#include <queue.h>

//-----------------------------------------------------------------------------
#ifndef FLASH_SECTOR_SIZE
	#define FLASH_SECTOR_SIZE 		4096
#endif
#define FLASH_SECTORS 			256	// 1 Mbytes
#define FLASH_CHIP_SIZE			(FLASH_SECTORS * FLASH_SECTOR_SIZE)
#define FMEMORY_SCFG_BANK_SIZE	FLASH_SECTOR_SIZE // размер сектора, 4096 bytes
#define FMEMORY_SCFG_BANKS 		2 // кол-во секторов для работы 2...
#define FMEMORY_SCFG_BASE_ADDR	(FLASH_CHIP_SIZE - (FMEMORY_SCFG_BANKS*FMEMORY_SCFG_BANK_SIZE)) // = 0xFE000
//-----------------------------------------------------------------------------
#define FLASH_EEP_ATTR
//-----------------------------------------------------------------------------
enum eFMEMORY_ERRORS {
	FMEM_NOT_FOUND = -1,	//  -1 - не найден
	FMEM_FLASH_ERR = -2,	//  -2 - flash rd/wr/erase error
	FMEM_ERROR = -3,		//  -3 - error
	FMEM_OVR_ERR = -4,		//  -4 - переполнение FMEMORY_SCFG_BANK_SIZE
	FMEM_MEM_ERR = -5		//  -5 - heap alloc error
};
//-----------------------------------------------------------------------------
// extern QueueHandle_t flash_mutex;
signed short flash_read_cfg(void *ptr, unsigned short id, unsigned short maxsize) FLASH_EEP_ATTR; // возврат: размер объекта последнего сохранения, -1 - не найден, -2 - error
bool flash_write_cfg(void *ptr, unsigned short id, unsigned short size) FLASH_EEP_ATTR;
//-----------------------------------------------------------------------------
#ifndef USE_FLASH_EEP
#define USE_FLASH_EEP 1
#endif

#ifdef __cplusplus
}
#endif


#endif /* __FLASH_EEP_H_ */
