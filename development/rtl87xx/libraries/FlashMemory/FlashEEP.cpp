#include "FlashEEP.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "flash_api.h"
#include "flash_eep.h"

#ifdef __cplusplus
}
#endif

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
short FlashEEPClass::read(void *buf, unsigned short id, unsigned short maxsize)
{
	return flash_read_cfg(buf, id, maxsize) FLASH_EEP_ATTR; 
}

/**
 * Write ObjId
 * objsize - Object buf size (max objsize 512 bytes)
 * return - false = error 
 */
bool FlashEEPClass::write(void *buf, unsigned short id, unsigned short objsize)
{
	return flash_write_cfg(buf, id, objsize) FLASH_EEP_ATTR;
}
