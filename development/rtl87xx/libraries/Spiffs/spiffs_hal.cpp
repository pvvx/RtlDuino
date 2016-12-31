/*
 * spiffs_hal.cpp
 * RTL871x pvvx
 */

#include <Arduino.h>
#include <stdlib.h>
//#include <algorithm>
#include "spiffs/spiffs.h"
#include "debug.h"
#include "flash_api.h"

extern "C" {
#include "device_lock.h"
//extern flash_t flashobj;
}

int32_t spiffs_hal_read(uint32_t addr, uint32_t size, uint8_t *dst) {
	device_mutex_lock(RT_DEV_LOCK_FLASH);
	flash_burst_read(&flashobj, addr, size, dst);
	device_mutex_unlock(RT_DEV_LOCK_FLASH);    
	return SPIFFS_OK;
}

int32_t spiffs_hal_write(uint32_t addr, uint32_t size, uint8_t *src) {
	device_mutex_lock(RT_DEV_LOCK_FLASH);
	flash_burst_write(&flashobj, addr, size, src);
	device_mutex_unlock(RT_DEV_LOCK_FLASH);    
	return SPIFFS_OK;
}

int32_t spiffs_hal_erase(uint32_t addr, uint32_t size) {
    if ((size & (FLASH_SECTOR_SIZE - 1)) != 0 ||
        (addr & (FLASH_SECTOR_SIZE - 1)) != 0) {
        return SPIFFS_ERR_INTERNAL;
    }
    const uint32_t sector = addr / FLASH_SECTOR_SIZE;
    const uint32_t sectorCount = size / FLASH_SECTOR_SIZE;
    if(sectorCount) {
		device_mutex_lock(RT_DEV_LOCK_FLASH);
	    for (uint32_t i = 0; i < sectorCount; ++i) {
			flash_erase_sector(&flashobj, (sector + i)*FLASH_SECTOR_SIZE);
	    }
		device_mutex_unlock(RT_DEV_LOCK_FLASH);    
    }
    return SPIFFS_OK;
}
