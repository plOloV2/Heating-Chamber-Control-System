/**
 * @file SD_task.c
 * @brief Implementation of SD RTOS task controlling SDIO interface (SD Card).
 *
 * @details
 *
 * @author Aleksander Dziągwa
 * @date 27.10.2026
 */

#include "system_state/error.h"
#include "RTOS/SD_task.h"
#include "mem_location.h"
#include "stm32f4xx.h"
#include "FreeRTOS.h"
#include "task.h"

#include "diskio.h"
#include "ff.h"

/** Disk id for FatFs */
#define DISK_NUM 0U

/** FatFs objects */
static FATFS    fs;
static FIL      file;
static FRESULT  FFcode;
static UINT     bytesRead;

/** Buffer for file read */
#define SD_BUFFER_SIZE 512U
static DMA_BUFFER uint8_t SD_buffer[SD_BUFFER_SIZE];

static bool load_screen_magic_num(void);

void vSDTask(void *pvParameters){

    (void)pvParameters;

    /** Initialize SD card */
    DSTATUS SD_Card_init = disk_initialize(DISK_NUM);
    if(SD_Card_init != 0)
    {
        SET_BIT(SystemStatus, SD_CARD_INIT_ERROR);
        vTaskSuspend(NULL);
    }

    /** Mount volume */
    FFcode = f_mount(&fs, "0:", 1);
    if(FFcode != FR_OK)
    {
        SET_BIT(SystemStatus, SD_CARD_INIT_ERROR);
        vTaskSuspend(NULL);
    }

    /** Load magic number (screen/screen_flash.bin) for screen binary blob into buffer */
    if(!load_screen_magic_num())
    {
        SET_BIT(SystemStatus, SD_CARD_READ_ERROR);
        f_unmount("0:");
        vTaskSuspend(NULL);
    }



    for(;;)
    {

        vTaskSuspend(NULL);

    }

}


static bool load_screen_magic_num(void)
{

    FFcode = f_open(&file, "0:/screen/screen_flash.map", FA_READ);
    if(FFcode != FR_OK)
    {
        return false;
    }

    FSIZE_t fileSize = f_size(&file);
    
    FFcode = f_read(&file, SD_buffer, fileSize, &bytesRead);
    f_close(&file);
    return (FFcode == FR_OK);

}
