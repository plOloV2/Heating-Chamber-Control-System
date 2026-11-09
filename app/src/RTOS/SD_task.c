/**
 * @file SD_task.c
 * @brief Implementation of SD RTOS task controlling SDIO interface (SD Card).
 *
 * @details Implementation of SD card resposible task. Tihs task First reads magic num from
 * from screen blob in SD_card/screen/screeen_flash.bin and hands it to UI task for screen
 * flash evaluation. Later logs baking data onto sd card every 5sec.
 *
 * @author Aleksander Dziągwa
 * @date 27.10.2026
 */

#include "system_state/error.h"
#include "RTOS/SD_task.h"
#include "mem_location.h"
#include "stm32f4xx.h"
#include "string.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "EVE4_Riverdi_lib/screen_flash_map.h"

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
#define SD_BUFFER_SIZE 4096U
static DMA_BUFFER uint8_t SD_buffer[SD_BUFFER_SIZE];

/** Semaphores for task comunication */
extern SemaphoreHandle_t sd_ready;
extern SemaphoreHandle_t ui_ready;

/** Some variables used to comunicate with UI task */
extern volatile bool mag_num;
const void* data_buffer = SD_buffer;

/** Timeout waiting for UI task response */
#define UI_INIT_TIMEOUT_MS  2000U
#define UI_CHUNK_TIMEOUT_MS 1000U

static bool load_screen_magic_num(void);
static bool read_whole_screen_blob(void);


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

    /** Load magic number (screen/screen_flash.bin) into SD_buffer */
    if(!load_screen_magic_num())
    {
        SET_BIT(SystemStatus, SD_CARD_READ_ERROR);
        f_unmount("0:");
        vTaskSuspend(NULL);
    }

    /** Notify UI task that magic number is in data_buffer */
    (void)xSemaphoreGive(sd_ready);

    /** Wait for UI task to compare magic number and set mag_num */
    if(xSemaphoreTake(ui_ready, pdMS_TO_TICKS(UI_INIT_TIMEOUT_MS)) == pdTRUE)
    {
        /** If magic number mismatched (mag_num == false), stream whole screen_flash.bin */
        if(!mag_num)
        {
            if(!read_whole_screen_blob())
            {
                SET_BIT(SystemStatus, SD_CARD_READ_ERROR);
                f_unmount("0:");
                vTaskSuspend(NULL);
            }

        }
        
    }

    
    for(;;)
    {
        // Logging to be added in future
        vTaskSuspend(NULL);
    }

}


/**
 * @brief Loads magic numner from screen/screen_flash.bin into data buffer.
 * 
 * @returns TRUE if load succeded, FALSE on error.
 */
static bool load_screen_magic_num(void)
{
    /** Get magic num location and size */
    const FSIZE_t target_offset = (FSIZE_t)flash_asset_list[magic_val_bin].offset;
    const UINT target_size      = (UINT)flash_asset_list[magic_val_bin].size;

    /** Open screen's binary blob */
    FFcode = f_open(&file, "0:/screen/screen_flash.bin", FA_READ);
    if(FFcode != FR_OK)
    {
        return false;
    }

    /** Go to magic num offset */
    FFcode = f_lseek(&file, target_offset);
    if((FFcode != FR_OK) || (f_tell(&file) != target_offset))
    {
        f_close(&file);
        return false;
    }

    /** Read magic num into SD_buffer */
    FFcode = f_read(&file, SD_buffer, target_size, &bytesRead);
    f_close(&file);

    return ((FFcode == FR_OK) && (bytesRead == target_size));
}


/**
 * @brief Reads whole screen/screen_flash.bin into data buffer. Synchronized with UI task.
 * 
 * @returns TRUE if load succeded, FALSE on error.
 */
static bool read_whole_screen_blob(void)
{
    /** Open screen's binary blob from the beginning */
    FFcode = f_open(&file, "0:/screen/screen_flash.bin", FA_READ);
    if(FFcode != FR_OK)
    {
        return false;
    }

    while(!f_eof(&file))
    {
        /** Abort immediately if UI task failed */
        if(READ_BIT(SystemStatus, LCD_INIT_ERROR) != 0U)
        {
            f_close(&file);
            return false;
        }

        FFcode = f_read(&file, SD_buffer, SD_BUFFER_SIZE, &bytesRead);
        if((FFcode != FR_OK) || (bytesRead == 0U))
        {
            f_close(&file);
            return false;
        }

        /** Pad final partial 4 KiB sector with 0xFF */
        if(bytesRead < SD_BUFFER_SIZE)
        {
            memset(&SD_buffer[bytesRead], 0xFF, SD_BUFFER_SIZE - bytesRead);
        }

        /** Signal UI task that 4 KiB chunk is ready in data_buffer */
        (void)xSemaphoreGive(sd_ready);

        /** Wait up to 1s for UI task to copy data_buffer to EVE RAM_G */
        if(xSemaphoreTake(ui_ready, pdMS_TO_TICKS(UI_CHUNK_TIMEOUT_MS)) != pdTRUE)
        {
            f_close(&file);
            return false;
        }
        
    }

    f_close(&file);
    return true;
}
