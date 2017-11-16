#include "SdFatFs.h"
#include "Arduino.h"
#include "pinmap.h"

#ifdef __cplusplus
extern "C" {

#include "gpio_api.h"
#include "sdio_host.h"
#include <disk_if/inc/sdcard.h>

}
#endif

#define TEST_SIZE	(512)

char WRBuf[TEST_SIZE];
char RDBuf[TEST_SIZE];

int sdioInitErr = FR_OK;

#if defined(BOARD_RTL8710)

extern "C" unsigned short GPIOState[];
extern "C" void HalPinCtrlRtl8195A(int,int,int);

void SdFatFs::WP_Off() {
		pin_mode(PA_7, PullDown);
}

void SdFatFs::WP_On() {
		pin_mode(PA_7, PullDown);
}

void SdFatFs::InsertSD() {
		pin_mode(PA_6, PullDown);
		delay(2);
}

void SdFatFs::RemoveSD()
{
		pin_mode(PA_6, PullUp);
}
#endif

SdFatFs::SdFatFs() {
    m_fs = NULL;
    drv_num = -1;
    logical_drv[0] = '0';
    logical_drv[1] = ':';
    logical_drv[2] = '/';
    logical_drv[3] = '\0';

    if(sdio_sd_init() != 0){
    	printf("SDIO host init fail.\n");
    	sdioInitErr = FR_DISK_ERR;
    }
    else {
	    if(sdio_sd_status() >=0) {
	    	uint32_t i = sdio_sd_getCapacity();
			printf("\nSD Capacity: %d sectors (%d GB | %d MB | %d KB)\n", i,
				i >> 21, i >> 11, i >> 1);
		}
    }
}

SdFatFs::~SdFatFs() {
	sdio_sd_deinit();
}

int SdFatFs::begin() {

//	pin_mode(PA_6, PullDown);

    FRESULT ret = FR_OK;
    do {
        m_fs = (FATFS *) malloc (sizeof(FATFS));
        if (m_fs == NULL) {
            ret = FR_INT_ERR;
            break;
        }

        if(sdioInitErr == FR_DISK_ERR)
            break;

        drv_num = FATFS_RegisterDiskDriver(&SD_disk_Driver);
        if (drv_num < 0) {
            printf("Rigester disk driver to FATFS fail.\n");
            ret = FR_DISK_ERR;
            break;
        }

        logical_drv[0] += drv_num;

        ret = f_mount((FATFS *)m_fs, logical_drv, 1);
        if( ret != FR_OK ){
            printf("FATFS mount logical drive fail:%d\n",ret);
            break;
        }

    } while (0);

    if (ret != FR_OK) {
        drv_num = -1;
    }

    return (-(int)ret);
}

int SdFatFs::end() {
    FRESULT ret = FR_OK;

    ret = f_mount(NULL, logical_drv, 1);
    if( ret != FR_OK ) {
        printf("FATFS unmount logical drive fail.\n");
    }

    if( FATFS_UnRegisterDiskDriver(drv_num) ) {
        ret = FR_DISK_ERR;
        printf("Unregister disk driver from FATFS fail.\n");
    }

    if (m_fs != NULL) {
        free(m_fs);
        m_fs = NULL;
    }

    drv_num = -1;

	pin_mode(PA_6, PullUp);

    return -ret;
}

char *SdFatFs::getRootPath() {
    if (drv_num < 0) {
        return NULL;
    } else {
        return logical_drv;
    }
}

int SdFatFs::readDir(const char *path, char *result_buf, unsigned int bufsize) {
    FRESULT ret = FR_OK;
    FILINFO fno;
    DIR dir;

    char *fn;
    unsigned int fnlen;
    int bufidx = 0;

#if _USE_LFN
    char lfn[_MAX_LFN + 1];
    fno.lfname = lfn;
    fno.lfsize = sizeof(lfn);
#endif

    do {
        if (drv_num < 0) {
            ret = FR_DISK_ERR;
            break;
        }

        ret = f_opendir(&dir, path);
        if (ret != FR_OK) {
            break;
        }

        memset(result_buf, 0, bufsize);

        while (1) {
            ret = f_readdir(&dir, &fno);
            if (ret != FR_OK || fno.fname[0] == 0) {
                break;
            }

#if _USE_LFN
            if (*fno.lfname)
            {
                fn = fno.lfname;
                fnlen = fno.lfsize;
            }
            else
#endif
            {
                fn = fno.fname;
                fnlen = fno.fsize;
            }

            bufidx += sprintf(result_buf + bufidx, "%s", fn);
            bufidx++;
/*            if (fno.fattrib & AM_DIR) {
            } else {
                if (bufidx + fnlen + 1 < bufsize) {
                    bufidx += sprintf(result_buf + bufidx, "%s", fn);
                    bufidx++;
                }
            }
*/
        }
    } while (0);

    return -ret;

}

int SdFatFs::mkdir(const char *absolute_path) {
    FRESULT ret = FR_OK;

    do {
        if (drv_num < 0) {
            ret = FR_DISK_ERR;
            break;
        }

        ret = f_mkdir(absolute_path);
        if (ret != FR_OK) {
            break;
        }
    } while (0);

    return ret;
}

int SdFatFs::rm(const char *absolute_path) {
    FRESULT ret = FR_OK;

    do {
        if (drv_num < 0) {
            ret = FR_DISK_ERR;
            break;
        }

        ret = f_unlink(absolute_path);
        if (ret != FR_OK) {
            break;
        }
    } while (0);

    return ret;
}

unsigned char SdFatFs::isDir(const char *absolute_path) {
    unsigned char attr;
    if ( getAttribute(absolute_path, &attr) >= 0) {
        if (attr & AM_DIR) {
            return 1;
        }
    }
    return 0;
}

unsigned char SdFatFs::isFile(const char *absolute_path) {
    unsigned char attr;
    if ( getAttribute(absolute_path, &attr) >= 0) {
        if (attr & AM_ARC) {
            return 1;
        }
    }
    return 0;
}

SdFatFile SdFatFs::open(const char *absolute_path, const char* mode) {
    FRESULT ret = FR_OK;
    SdFatFile file;
		unsigned char seekEnd = 0;
		unsigned char flags = FA_READ;

			do{
				if (strcmp(mode,"r") == 0 ){
					flags = FA_READ;
					break;
				}
				if (strcmp(mode,"r+") == 0 ){
					flags = FA_READ | FA_WRITE;
					break;
				}
				if (strcmp(mode,"w") == 0 ){
					flags = FA_CREATE_ALWAYS | FA_WRITE;
					break;
				}
				if (strcmp(mode,"w+") == 0 ){
					flags = FA_CREATE_ALWAYS | FA_WRITE | FA_READ;
					break;
				}
				if (strcmp(mode,"a") == 0 ){
					flags = FA_OPEN_ALWAYS | FA_WRITE;
					seekEnd = 1;
					break;
				}
				if (strcmp(mode,"a+") == 0 ){
					flags = FA_OPEN_ALWAYS | FA_WRITE | FA_READ;
					seekEnd = 1;
					break;
				}
		}while(0);

    do {
        if (drv_num < 0) {
            ret = FR_DISK_ERR;
            break;
        }

        file.m_file = (FIL *)malloc (sizeof(FIL));
        if (file.m_file == NULL) {
            ret = FR_INT_ERR;
            break;
        }

        ret = f_open((FIL *)file.m_file, absolute_path, flags);

        if (ret != FR_OK) {
            printf("open file (%s) fail. (ret=%d)\n", absolute_path, ret);
            break;
        }
				if(seekEnd){
					file.seek( file.size() - 1);
				}
    } while (0);

    if (ret != FR_OK) {
        if (file.m_file != NULL) {
            free(file.m_file);
            file.m_file = NULL;
        }
    }

    return file;
}

int SdFatFs::status() {
    return sdio_sd_status() == 4;
}

int SdFatFs::getLastModTime(const char *absolute_path, uint16_t *year, uint16_t *month, uint16_t *date, uint16_t *hour, uint16_t *minute, uint16_t *second) {
    FRESULT ret = FR_OK;

    FILINFO fno;
#if _USE_LFN
    char lfn[_MAX_LFN + 1];
    fno.lfname = lfn;
    fno.lfsize = sizeof(lfn);
#endif

    do {
        if (drv_num < 0) {
            ret = FR_DISK_ERR;
            break;
        }

        ret = f_stat(absolute_path, &fno);
        if (ret != FR_OK) {
            break;
        }

        *year   = (fno.fdate >> 9) + 1980;
        *month  = (fno.fdate >> 5) & 0x0F;
        *date   = (fno.fdate & 0x1F);
        *hour   = (fno.ftime >> 11);
        *minute = (fno.ftime >> 5) & 0x3F;
        *second = 0;

    } while (0);

    return -ret;
}

int SdFatFs::setLastModTime(const char *absolute_path, uint16_t year, uint16_t month, uint16_t date, uint16_t hour, uint16_t minute, uint16_t second) {
    FRESULT ret = FR_OK;
    FILINFO fno;
#if _USE_LFN
    char lfn[_MAX_LFN + 1];
    fno.lfname = lfn;
    fno.lfsize = sizeof(lfn);
#endif

    int scan_count = 0;

    do {
        if (drv_num < 0) {
            ret = FR_DISK_ERR;
            break;
        }

        fno.fdate = 0x0000 | ((year - 1980) <<  9) | ((month  & 0x0F) << 5) | (date & 0x1F);
        fno.ftime = 0x0000 | ((hour & 0x1F) << 11) | ((minute & 0x3F) << 5);
        ret = f_utime(absolute_path, &fno);
        if (ret != FR_OK) {
            break;
        }
    } while (0);

    return -ret;
}

int SdFatFs::getAttribute(const char *absolute_path, unsigned char *attr) {
    FRESULT ret = FR_OK;
    FILINFO fno;
#if _USE_LFN
    char lfn[_MAX_LFN + 1];
    fno.lfname = lfn;
    fno.lfsize = sizeof(lfn);
#endif

    do {
        if (drv_num < 0) {
            ret = FR_DISK_ERR;
            break;
        }

        ret = f_stat(absolute_path, &fno);
        if (ret != FR_OK) {
            break;
        }

        *attr = fno.fattrib;
    } while (0);

    return -ret;
}

int SdFatFs::getFsize(const char *absolute_path, uint32_t *size, unsigned char *attr) {
    FRESULT ret = FR_OK;
    FILINFO fno;
#if _USE_LFN
    char lfn[_MAX_LFN + 1];
    fno.lfname = lfn;
    fno.lfsize = sizeof(lfn);
#endif
    do {
        if (drv_num < 0) {
            ret = FR_DISK_ERR;
            break;
        }

        ret = f_stat(absolute_path, &fno);
        if (ret != FR_OK) {
            break;
        }
        *size = fno.fsize;
        if(attr) *attr = fno.fattrib;
    } while (0);
    return -ret;
}

int SdFatFs::getLabel(const char *absolute_path, char *bufname, uint32_t *svn) {
    FRESULT ret = FR_OK;

    do {
        if (drv_num < 0) {
            ret = FR_DISK_ERR;
            break;
        }
        ret = f_getlabel(absolute_path, bufname, svn);
        if (ret != FR_OK) {
            break;
        }
    } while (0);
    return -ret;
}

char SdFatFs::getCSD(unsigned char * csd_data) {
	return sdio_sd_getCSD(csd_data);
}
