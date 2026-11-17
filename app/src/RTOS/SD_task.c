/**
 * @file SD_task.c
 * @brief Implementation of SD RTOS task controlling SDIO interface (SD Card).
 *
 * @details Implementation of SD card resposible task. Tihs task First reads magic num from
 *          from screen blob in SD_card/screen/screeen_flash.bin and hands it to UI task for screen
 *          flash evaluation. Later logs baking data onto sd card every 5sec.
 *
 * @author Aleksander Dziągwa
 * @date 27.10.2026
 */

#include "stm32f4xx.h"
#include "string.h"
#include "stdio.h"

#include "drivers/RTC_operations.h"
#include "system_state/error.h"
#include "RTOS/temp_task.h"
#include "furnace_struck.h"
#include "RTOS/SD_task.h"
#include "mem_location.h"

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

/** Max line width in log file */
#define MAX_LINE_WIDTH 512U

static bool load_screen_magic_num(void);
static bool read_whole_screen_blob(void);
static bool creat_new_log(void);
static bool write_log_data(void);


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

    /** Wait for oven controll task to wake up after the baking has staeted */
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    if(!creat_new_log())
    {
        SET_BIT(SystemStatus, SD_CARD_WRITE_ERROR);
        f_unmount("0:");
        vTaskSuspend(NULL);
    }

    /** Immediately write first data */
    if(!write_log_data())
    {
        SET_BIT(SystemStatus, SD_CARD_WRITE_ERROR);
        f_unmount("0:");
        vTaskSuspend(NULL);
    }
    
    
    for(;;)
    {
        /** Oven controll task wakes this task up every 5s */
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        if(!write_log_data())
        {
            SET_BIT(SystemStatus, SD_CARD_WRITE_ERROR);
            f_unmount("0:");
            vTaskSuspend(NULL);
        }

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

/**
 * @brief Creates /logs directory if it doesn't exist and creates log_time_date.csv file inside it.
 * 
 * @returns TRUE if creation was succeful, FALSE on error.
 */
static bool creat_new_log(void)
{
    /** Try to create /logs directory */
    FFcode = f_mkdir("0:/logs");
    if(FFcode != FR_OK && FFcode != FR_EXIST)
    {
        return false;
    }

    /** Read curent date and time */
    uint32_t date = read_date();
    uint32_t time = read_time();

    uint8_t day   = RTC_UNPACK_DAY(date);
    uint8_t month = RTC_UNPACK_MONTH(date);
    uint16_t year = RTC_UNPACK_YEAR(date);

    uint8_t hour  = RTC_UNPACK_HOUR(time);
    uint8_t min   = RTC_UNPACK_MIN(time);
    uint8_t sec   = RTC_UNPACK_SEC(time);

    /** Create name for new file with date and time in it */
    char log_file_name[64];
    snprintf(log_file_name, sizeof(log_file_name), "0:/logs/log_%02u-%02u-%02u_%02u-%02u-%04u.csv", hour, min, sec, day, month, year);

    /** Try to create new file */
    FFcode = f_open(&file, log_file_name, FA_CREATE_NEW | FA_WRITE);
    if(FFcode != FR_OK)
    {
        return false;
    }

    const char header[] = {
        "stage_num;stage_time_min;stage_time_sec;stage_target_temp;stage_target_temp_grow;stage_fan_on;target_temp_now;temp1;temp2;temp3;temp4;temp_avg;heat_on;fan_on;\n"
    };

    /** Write csv header into file */
    UINT bytes_writen = 0;
    const UINT size_bytes = sizeof(header) - 1U;

    /** Copy data into SD_buffer */
    memcpy(SD_buffer, header, size_bytes);

    FFcode = f_write(&file, &SD_buffer, size_bytes, &bytes_writen);
    if(FFcode != FR_OK || bytes_writen != size_bytes)
    {
        f_close(&file);
        return false;
    }

    /** Push all data into file */
    FFcode = f_sync(&file);
    if(FFcode != FR_OK)
    {
        f_close(&file);
        return false;
    }

    return true;

}

/** External data definitions */
extern bool fan_on;
extern bool heater_on;
extern float target_temp;

/**
 * @brief Writes baking data into file created by creat_new_log.
 * 
 * @returns TRUE if write was succeful, FALSE on error.
 */
static bool write_log_data(void)
{
    /** Close file when baking has ended and suspend execution */
    if(Furnace_state.stage_ID == Furnace_state.stages_num)
    {
        f_close(&file);
        f_unmount("0:");
        vTaskSuspend(NULL);
    }

    char data[MAX_LINE_WIDTH];
    const int len = snprintf(data, sizeof(data), "%u;%u;%u;%u;%.1f;%s;%.2f;%.2f;%.2f;%.2f;%.2f;%.2f;%s;%s;\n",
                            Furnace_state.stage_ID,
                            Furnace_state.minutes_passed,
                            Furnace_state.seconds_passed,
                            Stages[Furnace_state.stage_ID].stage_required_temp,
                            ((float)Stages[Furnace_state.stage_ID].temp_grow / 10.0f),
                            (Stages[Furnace_state.stage_ID].FAN_ON? "ON" : "OFF"),
                            target_temp,
                            TempReal[0],
                            TempReal[1],
                            TempReal[2],
                            TempReal[3],
                            Furnace_state.average_temp,
                            (heater_on? "ON" : "OFF"),
                            (fan_on? "ON" : "OFF")
                        );

    /** Check lenght of created line */
    if(len <= 0 || (UINT) len >= MAX_LINE_WIDTH)
    {
        f_close(&file);
        return false;
    }
    
    UINT bytes_writen = 0;
    const UINT size_bytes = len;

    /** Copy data into SD_buffer */
    memcpy(SD_buffer, data, size_bytes);

    /** Write csv data into file */
    FFcode = f_write(&file, &SD_buffer, size_bytes, &bytes_writen);
    if(FFcode != FR_OK || bytes_writen != size_bytes)
    {
        f_close(&file);
        return false;
    }

    /** Push all data into file */
    FFcode = f_sync(&file);
    if(FFcode != FR_OK)
    {
        f_close(&file);
        return false;
    }

    return true;

}
