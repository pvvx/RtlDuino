#ifndef _SDFATFS_H_
#define _SDFATFS_H_

#if defined(ARDUINO_AMEBA)
// #error "Ameba Boadr do not support RtlDuino!"
#endif

#include "SdFatFile.h"

#define MAX_FILENAME_LEN 256

/* File attribute bits for directory entry */

#define	ATTR_RDO	0x01	/* Read only */
#define	ATTR_HID	0x02	/* Hidden */
#define	ATTR_SYS	0x04	/* System */
#define	ATTR_VOL	0x08	/* Volume label */
#define ATTR_LFN	0x0F	/* LFN entry */
#define ATTR_DIR	0x10	/* Directory */
#define ATTR_ARC	0x20	/* Archive */
#define ATTR_MASK	0x3F	/* Mask of defined bits */

/** 
 * @class SdFatFs SdFatFs.h 
 * @brief SD FAT File system
 */
class SdFatFs
{
public:

    /**
     * @brief Constructor of SdFatFs
     */
    SdFatFs();

    /**
     * @brief Destructor of SdFatFs
     */
    ~SdFatFs();

    /**
     * @brief Initialize SD FAT File System
     */
    int begin();

    /**
     * @brief Deinitialize SD FAT File System
     */
    int end();
#if defined(BOARD_RTL8710) 
	void WP_Off();
	void WP_On();
	void InsertSD();
	void RemoveSD();
#endif
    /**
     * @brief Get root path of the SD FAT File System
     *
     * The logical volum character is start from '0', so the root path would like "0:/"
     *
     * @return root path
     */
    char *getRootPath();

    /**
     * @brief List items under specific folder
     *
     * List items under specific folder and store the result in the buffer that user specified.
     * Each item are seperated by '\0'.
     *
     * @param[in] path The absolute directory path to be listed
     * @param[in] result_buf The buffer to be stored results
     * @param[in] bufsize The size of result_buf. If results exceed this size, then the results larger than this size would be discard.
     * @return Return 0 if success. Return negativate value for a failure.
     */
    int readDir(char *path, char *result_buf, unsigned int bufsize);

    /**
     * @brief Create folder
     *
     * @param[in] absolute_path The absolute directory path to be created.
     * @return Return 0 if success. Return negativate value for a failure.
     */
    int mkdir(char *absolute_path);

    /**
     * @brief Remove folder or file
     *
     * @param[in] absolute_path The absolute directory or file path to be deleted.
     * @return Return 0 if success. Return negativate value for a failure.
     */
    int rm(char *absolute_path);

    /**
     * @brief Check if a specific path is a directory
     *
     * @param[in] absolute_path The absolute path to be queried
     * @return Return 1 if it is directory. And 0 vise versa.
     */
    unsigned char isDir(char *absolute_path);

    /**
     * @brief Check if a specific path is a file
     *
     * @param[in] absolute_path The absolute path to be queried
     * @return Return 1 if it is file. And 0 vise versa.
     */
    unsigned char isFile(char *absolute_path);

    /**
     * @brief Get last modified time for a file or directory
     *
     * @param[in] absolute_path The absolute path to be queried
     * @param[in] year
     * @param[in] month
     * @param[in] date
     * @param[in] hour
     * @param[in] minute
     * @param[in] second field "second" contains no valid information in current version
     * @return Return 0 if success. Return negativate value for a failure.
     */
    int getLastModTime(char *absolute_path, uint16_t *year, uint16_t *month, uint16_t *date, uint16_t *hour, uint16_t *minute, uint16_t *second);

    /* get file size and arttribute */
    int getFsize(char *absolute_path, uint32_t *size, unsigned char *attr = NULL);

    /* SD CSD size 16 bytes */
    char getCSD(unsigned char * csd_data); 
    
    /*  Get volume label 
    * @param[in] absolute_path The absolute path to be changed
    * @param[in] bufname  Pointer to a buffer to return the volume label 
    * @param[in] svn Pointer to a variable to return the volume serial number 
    * @return Return 0 if success. Return negativate value for a failure.
    */
    
    int getLabel(char *absolute_path, char *bufname, uint32_t *svn);


    /**
     * @brief Set last modified time for a file or directory
     *
     * Ameba don't have built-in RTC. So we manually change file/directory last modified time.
     *
     * @param[in] absolute_path The absolute path to be changed
     * @param[in] year
     * @param[in] month
     * @param[in] date
     * @param[in] hour
     * @param[in] minute
     * @param[in] second field "second" contains no valid information in current version
     * @return Return 0 if success. Return negativate value for a failure.
     */
    int setLastModTime(char *absolute_path, uint16_t year, uint16_t month, uint16_t date, uint16_t hour, uint16_t minute, uint16_t second);

    /**
     * @brief Open a file
     *
     * @param[in] absolute_path the path to a file
     * @return The file object which is instance of SdFatFile
     */
    SdFatFile open(char *absolute_path);

    /**
     * @brief Return current status of SD
     *
     * @return Return 1 if ready to use. Return 0 if status is inactivate or abnormal.
     */
    int status();

private:
    void *m_fs;
    char logical_drv[4];
    int drv_num;

    int getAttribute(char *absolute_path, unsigned char *attr);
};

#endif
