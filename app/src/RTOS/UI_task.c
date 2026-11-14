/**
 * @file UI_task.c
 * @brief Implementation of UI RTOS task controlling screen and user inputs.
 *
 * @details Implements state machine resposible for controlling main screen and user inputs.
 *          Synchronizes with SD to check and optionaly flash screen memory. System should NOT
 *          depend on this task while running and should run even if this task crashes.
 *
 * @author Aleksander Dziągwa
 * @date 5.11.2026
 */

#include "RTOS/temp_task.h"
#include "RTOS/UI_task.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "drivers/buttons_operations.h"
#include "drivers/pin_operations.h"
#include "drivers/RNG_operations.h"
#include "drivers/RTC_operations.h"
#include "GPIO_pins_def.h"

#include "system_state/error.h"
#include "furnace_struck.h"
#include "version.h"
#include <stdlib.h>
#include <stdio.h>

#include "EVE4_Riverdi_lib/screen_platform.h"
#include "EVE4_Riverdi_lib/App_Common.h"

/** Timeout waiting for SD task (1 second) */
#define SD_SYNC_TIMEOUT_MS      1000U

/** EVE RAM_G staging address for 4 KiB flash updates */
#define EVE_RAM_G_STAGING_ADDR  0x000000UL

/** variables for SD task comunication */
volatile bool mag_num = false;
extern void* data_buffer;

extern SemaphoreHandle_t sd_ready;
extern SemaphoreHandle_t ui_ready;

/** Screen driver definitions */
static Gpu_Hal_Context_t host, *phost;
static uint32_t break_time = 1000U;
static uint32_t flash_write_offset = 0U;
static bool flash_ok = false;


/** @brief Editable fields in the RTC_SET screen */
typedef enum {
    RTC_FIELD_YEAR = 0U,
    RTC_FIELD_MONTH,
    RTC_FIELD_DAY,
    RTC_FIELD_HOUR,
    RTC_FIELD_MIN,
    RTC_FIELD_SEC,
    RTC_FIELD_CONFIRM,
    RTC_FIELD_COUNT
} rtc_edit_field_t;


/**
 * @brief Arrow X/Y coordinates matching Font 28 text positions from the .ese layout:
 *        "Date: YYYY.MM.DD" at (134, 66) -> Up Y = 40, Down Y = 92
 *        "Time: HH:MM:SS"   at (150, 124) -> Up Y = 98, Down Y = 150
 */
typedef struct {
    uint16_t x;
    uint16_t up_y;
    uint16_t down_y;
} rtc_arrow_pos_t;

static const rtc_arrow_pos_t rtc_arrow_coords[RTC_FIELD_CONFIRM] = {
    [RTC_FIELD_YEAR]  = { .x = 200U, .up_y = 40U, .down_y = 92U  }, /* YYYY */
    [RTC_FIELD_MONTH] = { .x = 240U, .up_y = 40U, .down_y = 92U  }, /* MM   */
    [RTC_FIELD_DAY]   = { .x = 270U, .up_y = 40U, .down_y = 92U  }, /* DD   */
    [RTC_FIELD_HOUR]  = { .x = 210U, .up_y = 98U, .down_y = 150U }, /* HH   */
    [RTC_FIELD_MIN]   = { .x = 240U, .up_y = 98U, .down_y = 150U }, /* MM   */
    [RTC_FIELD_SEC]   = { .x = 270U, .up_y = 98U, .down_y = 150U }  /* SS   */
};

/** Main state machine */
static UI_states UI_State = INIT_UI;

/** Main state machine code, runs forever. */
static void UI_state_machine(void);


void vUITask(void *pvParameters)
{
    (void)pvParameters;

    phost = &host;

    rng_hw_init();
    configure_buttons();

    for(;;)
    {

        UI_state_machine();
        vTaskDelay(pdMS_TO_TICKS(break_time));

    }

}


/** 
 * @brief Checks magic value from screen flash with one loaded from SD card.
 * 
 * @returns TRUE if they match, FLASE on missmatch or error.
 */
static bool check_screen_flash_magic(void)
{
    uint8_t eve_magic_buf[4] = {0};
    const uint32_t magic_offset = flash_asset_list[magic_val_bin].offset;
    const uint32_t magic_size   = flash_asset_list[magic_val_bin].size;

    Gpu_CoCmd_FlashHelper_Init(phost);
    (void)Gpu_CoCmd_FlashHelper_SwitchFullMode(phost);

    /* Read 4 bytes because Gpu_CoCmd_FlashHelper_Read rounds 'num' down to multiple of 4 */
    if(Gpu_CoCmd_FlashHelper_Read(
            phost,
            EVE_RAM_G_STAGING_ADDR,
            magic_offset,
            4U,
            eve_magic_buf) != FLASH_CMD_SUCCESS)
    {
        return false;
    }

    return (memcmp(eve_magic_buf, data_buffer, magic_size) == 0);
}


/** @brief Loads all default assets into RAM G */
static void load_all_assets(void)
{

    if(!flash_ok)
    {
        return;
    }

    uint32_t rng_num;

    if(!rng_get_random32(&rng_num))
    {
        rng_num = 69;
    }

    flash_asset_index background_index = Oven_fire_480x272_ASTC_4X4_raw + (rng_num % 18);
    flash_asset_index logo_index;
    if(rng_num % 19 == 0)
    {
        logo_index = Logo_PWR_RT_GOLD_384x88_ASTC_4X4_raw;
    } 
    else 
    {
        logo_index = Logo_PWR_RT_BLACK_384x88_ASTC_4X4_raw;
    }

    Gpu_CoCmd_FlashRead(phost, BACKGROUND_OFFSET,
                        flash_asset_list[background_index].offset,
                        flash_asset_list[background_index].size);

    Gpu_CoCmd_FlashRead(phost, LOGO_OFFSET,
                        flash_asset_list[logo_index].offset,
                        flash_asset_list[logo_index].size);

    Gpu_CoCmd_FlashRead(phost, ARROW_OFFSET,
                        flash_asset_list[Arrow_medium_24x24_ASTC_4X4_raw].offset,
                        flash_asset_list[Arrow_medium_24x24_ASTC_4X4_raw].size);

    Gpu_CoCmd_FlashRead(phost, FAN_OFFSET,
                        flash_asset_list[Fan_off_64x64_ASTC_4X4_raw].offset,
                        flash_asset_list[Fan_off_64x64_ASTC_4X4_raw].size);

    Gpu_CoCmd_FlashRead(phost, HEAT_OFFSET,
                        flash_asset_list[Heat_off_64x64_ASTC_4X4_raw].offset,
                        flash_asset_list[Heat_off_64x64_ASTC_4X4_raw].size);

    Gpu_CoCmd_FlashRead(phost, KUBICA_OFFSET,
                        flash_asset_list[Giga_Kubica_64x52_ASTC_4X4_raw].offset,
                        flash_asset_list[Giga_Kubica_64x52_ASTC_4X4_raw].size);

}

/** 
 * @brief Loads single asset from screen's flash into RAM G
 * 
 * @param offset RAM G offset at which asset will be loaded
 * @param asset_index ID of asset to be loaded
 */
static void load_one_asset(uint32_t offset, flash_asset_index asset_index)
{

    if(!flash_ok)
    {
        return;
    }

    Gpu_CoCmd_FlashRead(phost, offset,
                        flash_asset_list[asset_index].offset,
                        flash_asset_list[asset_index].size);

}


/** 
 * @brief Returns maximum days in a given month (2000..2099) 
 * 
 * @param year year to check
 * @param month month to check
 * 
 * @returns Number of days in a given month
 * */
static uint8_t rtc_get_days_in_month(uint16_t year, uint8_t month)
{
    static const uint8_t days_table[12] = {
        31U, 28U, 31U, 30U, 31U, 30U, 31U, 31U, 30U, 31U, 30U, 31U
    };

    if ((month < 1U) || (month > 12U))
    {
        return 31U;
    }

    if ((month == 2U) && ((year % 4U) == 0U))
    {
        return 29U;
    }

    return days_table[month - 1U];
}


static void UI_state_machine(void)
{
    const uint32_t total_blob_size = flash_asset_list[magic_val_bin].offset + flash_asset_list[magic_val_bin].size;

    switch (UI_State)
    {
        case INIT_UI:
        {
            /** Set wait time to 10ms */
            break_time = 10U;

            /** Initialize the screen, go to error state if failed */
            if(!App_Common_Init(&host))
            {
                UI_State = UI_ERROR;
                break;
            }

            /** Enable screen backlight and display status text + spinner */
            SetPin(BLS_PORT, BLS_PIN, BLS_ACTIVE);

            Gpu_CoCmd_Dlstart(phost);
            App_WrCoCmd_Buffer(phost, CLEAR(1, 1, 1));
            Gpu_CoCmd_Text(phost, 240, 140, 27, OPT_CENTERX, "Checking screen flash...");
            Gpu_CoCmd_Spinner(phost, 240, 200, 1, 0);
            App_Flush_Co_Buffer(phost);
            Gpu_Hal_WaitCmdfifo_empty(phost);

            /** Check if SD card task already reported an error */
            if(READ_BIT(SystemStatus, SD_CARD_INIT_ERROR | SD_CARD_READ_ERROR) != 0U)
            {
                UI_State = GREETER;
                break;
            }

            /** Wait up to 1s for SD task to load magic_val_bin into data_buffer */
            if(xSemaphoreTake(sd_ready, pdMS_TO_TICKS(SD_SYNC_TIMEOUT_MS)) != pdTRUE)
            {
                SET_BIT(SystemStatus, SD_CARD_READ_ERROR);
                UI_State = GREETER;
                break;
            }

            /** Compare magic num from SD card against screen QSPI flash */
            mag_num = check_screen_flash_magic();

            /** Notify SD task that comparison is complete and mag_num is set */
            (void)xSemaphoreGive(ui_ready);

            if(mag_num)
            {
                /** Magic numbers match: go straight to GREETER */
                flash_ok = true;
                UI_State = GREETER;
            }
            else
            {
                /** Magic mismatch: initialize offset & stream whole blob in FLASH_BLOB */
                flash_write_offset = 0U;
                break_time = 0U;
                UI_State = FLASH_BLOB;
            }

            break;
        }
        case FLASH_BLOB:
        {
            /** Check if SD task failed mid-transfer or timed out after 1s */
            if((READ_BIT(SystemStatus, SD_CARD_READ_ERROR) != 0U) ||
               (xSemaphoreTake(sd_ready, pdMS_TO_TICKS(SD_SYNC_TIMEOUT_MS)) != pdTRUE))
            {
                UI_State = GREETER;
                break;
            }

            /** Copy 4 KiB chunk from shared DMA buffer into EVE RAM_G */
            Gpu_Hal_WrMem(phost, EVE_RAM_G_STAGING_ADDR, (uint8_t *)data_buffer, FLASH_UPDATE_ALIGN_BYTE);

            /** Immediately release buffer so SD task can read next chunk in parallel */
            (void)xSemaphoreGive(ui_ready);

            /** Program 4 KiB sector in EVE QSPI flash from RAM_G */
            if(Gpu_CoCmd_FlashHelper_Update(
                    phost,
                    flash_write_offset,
                    EVE_RAM_G_STAGING_ADDR,
                    FLASH_UPDATE_ALIGN_BYTE) != FLASH_CMD_SUCCESS)
            {
                UI_State = UI_ERROR;
                break;
            }

            /** Switch to FULL (QSPI) mode right after sector 0 is written */
            if(flash_write_offset == 0U)
            {
                (void)Gpu_CoCmd_FlashHelper_SwitchFullMode(phost);
            }

            flash_write_offset += FLASH_UPDATE_ALIGN_BYTE;

            /** Check if entire screen_flash.bin has been written */
            if(flash_write_offset >= total_blob_size)
            {
                /** Stop the coprocessor spinner */
                Gpu_CoCmd_Stop(phost);
                App_Flush_Co_Buffer(phost);
                Gpu_Hal_WaitCmdfifo_empty(phost);

                flash_ok = true;
                UI_State = GREETER;
            }

            break;
        }
        case GREETER:
        {
            /** Stop spinner and wait for coprocessor to complete (needed if coming directly from INIT_UI) */
            Gpu_CoCmd_Stop(phost);
            App_Flush_Co_Buffer(phost);
            Gpu_Hal_WaitCmdfifo_empty(phost);

            /** Load assets from QSPI flash into RAM_G */
            if(flash_ok)
            {
                load_all_assets();
                App_Flush_Co_Buffer(phost);
                Gpu_Hal_WaitCmdfifo_empty(phost);
            }

            /** Clear previouse screen */
            Gpu_CoCmd_Dlstart(phost);
            App_WrCoCmd_Buffer(phost, CLEAR(1, 1, 1));
            App_WrCoCmd_Buffer(phost, VERTEX_FORMAT(0U));

            if(flash_ok)
            {
                /** Draw background */
                App_WrCoCmd_Buffer(phost, BITMAP_HANDLE(0U));
                Gpu_CoCmd_SetBitmap(phost, BACKGROUND_OFFSET, COMPRESSED_RGBA_ASTC_4x4_KHR, 480, 272);
                App_WrCoCmd_Buffer(phost, BEGIN(BITMAPS));
                App_WrCoCmd_Buffer(phost, VERTEX2F(0U, 0U));
                App_WrCoCmd_Buffer(phost, END());

                /** Draw logo */
                App_WrCoCmd_Buffer(phost, BITMAP_HANDLE(1U));
                Gpu_CoCmd_SetBitmap(phost, LOGO_OFFSET, COMPRESSED_RGBA_ASTC_4x4_KHR, 384, 88);
                App_WrCoCmd_Buffer(phost, BEGIN(BITMAPS));
                App_WrCoCmd_Buffer(phost, VERTEX2F(48U, 5U));
                App_WrCoCmd_Buffer(phost, END());
            }

            char version[64];
            snprintf(version, sizeof(version), "Code version: %s", APP_VERSION);

            Gpu_CoCmd_Text(phost, 16, 232, 27, 0, "Aero oven");
            Gpu_CoCmd_Text(phost, 16, 248, 26, 0, version);

            /** End and swap the display list so GREETER renders on screen */
            App_WrCoCmd_Buffer(phost, DISPLAY());
            Gpu_CoCmd_Swap(phost);
            App_Flush_Co_Buffer(phost);
            Gpu_Hal_WaitCmdfifo_empty(phost);

            break_time = 3000U;

            if(RTC_valid)
            {
                UI_State = SETUP_BAKING;
            }
            else
            {
                UI_State = RTC_SET;
            }

            break;
        }
        case RTC_SET:
            {
                static bool rtc_screen_init = false;
                static rtc_edit_field_t current_field = RTC_FIELD_YEAR;
                static uint16_t edit_year  = 2000U;
                static uint8_t  edit_month = 1U;
                static uint8_t  edit_day   = 1U;
                static uint8_t  edit_hour  = 0U;
                static uint8_t  edit_min   = 0U;
                static uint8_t  edit_sec   = 0U;

                break_time = 250U;

                /** 1. Load default date/time from RTC_operations on first entry */
                if(!rtc_screen_init)
                {

                    current_field   = RTC_FIELD_YEAR;
                    rtc_screen_init = true;
                }

                /** 2. Process button inputs (BUT1=UP, BUT2=DOWN, BUT3=LEFT, BUT4=RIGHT) */
                const button_event_t btn = ButtonState;
                ButtonStateChanged = false;

                if(btn.pressed.but4) /* RIGHT: Move cursor forward */
                {
                    if(current_field < RTC_FIELD_CONFIRM)
                    {
                        current_field++;
                    }
                    else
                    {
                        current_field = RTC_FIELD_YEAR;
                    }
                }
                else if(btn.pressed.but3) /* LEFT: Move cursor backward */
                {
                    if(current_field > RTC_FIELD_YEAR)
                    {
                        current_field--;
                    }
                    else
                    {
                        current_field = RTC_FIELD_CONFIRM;
                    }
                }
                else if(btn.pressed.but1 || btn.pressed.but2) /* UP or DOWN: Modify value or confirm */
                {
                    const bool is_up = (btn.pressed.but1 != 0U);

                    switch(current_field)
                    {
                        case RTC_FIELD_YEAR:
                            if(is_up) 
                            {
                                edit_year = (edit_year < 2099U) ? (edit_year + 1U) : 2000U;
                            } 
                            else 
                            {
                                edit_year = (edit_year > 2000U) ? (edit_year - 1U) : 2099U;
                            }
                            break;

                        case RTC_FIELD_MONTH:
                            if(is_up) 
                            {
                                edit_month = (edit_month < 12U) ? (edit_month + 1U) : 1U;
                            } 
                            else 
                            {
                                edit_month = (edit_month > 1U) ? (edit_month - 1U) : 12U;
                            }
                            break;

                        case RTC_FIELD_DAY:
                            const uint8_t max_day = rtc_get_days_in_month(edit_year, edit_month);
                            if(is_up)
                            {
                                edit_day = (edit_day < max_day) ? (edit_day + 1U) : 1U;
                            }
                            else
                            {
                                edit_day = (edit_day > 1U) ? (edit_day - 1U) : max_day;
                            }
                            break;

                        case RTC_FIELD_HOUR:
                            if(is_up)
                            {
                                edit_hour = (edit_hour < 23U) ? (edit_hour + 1U) : 0U;
                            }
                            else
                            {
                                edit_hour = (edit_hour > 0U) ? (edit_hour - 1U) : 23U;
                            }
                            break;

                        case RTC_FIELD_MIN:
                            if(is_up)
                            {
                                edit_min = (edit_min < 59U) ? (edit_min + 1U) : 0U;
                            }
                            else
                            {
                                edit_min = (edit_min > 0U) ? (edit_min - 1U) : 59U;
                            }
                            break;

                        case RTC_FIELD_SEC:
                            if(is_up)
                            {
                                edit_sec = (edit_sec < 59U) ? (edit_sec + 1U) : 0U;
                            }
                            else
                            {
                                edit_sec = (edit_sec > 0U) ? (edit_sec - 1U) : 59U;
                            }
                            break;

                        case RTC_FIELD_CONFIRM:
                            /** Write date and time to STM32 RTC registers */
                            if(is_up)
                            {
                                if(set_date_time(edit_year, edit_month, edit_day, edit_hour, edit_min, edit_sec))
                                {
                                    RTC_valid = true;
                                    rtc_screen_init = false;
                                    UI_State = SETUP_BAKING;
                                }

                            }
                            break;

                        default:
                            break;
                    }

                    /** Clamp day if month/year change reduced the days in the current month */
                    const uint8_t max_valid_day = rtc_get_days_in_month(edit_year, edit_month);
                    if(edit_day > max_valid_day)
                    {
                        edit_day = max_valid_day;
                    }

                }

                /** Render the RTC_SET screen layout */
                Gpu_CoCmd_Dlstart(phost);
                App_WrCoCmd_Buffer(phost, CLEAR_COLOR_RGB(150, 150, 150));
                App_WrCoCmd_Buffer(phost, CLEAR(1, 1, 1));
                App_WrCoCmd_Buffer(phost, VERTEX_FORMAT(0U));

                /** Top warning banner in red */
                App_WrCoCmd_Buffer(phost, COLOR_RGB(225, 0, 0));
                Gpu_CoCmd_Text(phost, 20, 16, 26, 0, "The RTC clock is not initialized. Please enter current date and time.");

                /** Date, Time, and bottom help instructions in black */
                char date_str[24];
                char time_str[24];
                snprintf(date_str, sizeof(date_str), "Date: %04u.%02u.%02u", edit_year, edit_month, edit_day);
                snprintf(time_str, sizeof(time_str), "Time: %02u:%02u:%02u", edit_hour, edit_min, edit_sec);

                App_WrCoCmd_Buffer(phost, COLOR_RGB(0, 0, 0));
                Gpu_CoCmd_Text(phost, 134, 66, 28, 0, date_str);
                Gpu_CoCmd_Text(phost, 150, 124, 28, 0, time_str);
                Gpu_CoCmd_Text(phost, 20, 230, 26, 0, "Use UP and DOWN arrows to change values,\nRIGHT to move forward (or confirm), LEFT backward.");

                /** "Set RTC" confirmation text: bright green when selected, dark gray otherwise */
                if(current_field == RTC_FIELD_CONFIRM)
                {
                    App_WrCoCmd_Buffer(phost, COLOR_RGB(59, 223, 0));
                }
                else
                {
                    App_WrCoCmd_Buffer(phost, COLOR_RGB(60, 60, 60));
                }
                Gpu_CoCmd_Text(phost, 190, 170, 28, 0, "Set RTC");

                /** Draw Up and Down arrow bitmaps around the active date/time field */
                if(flash_ok && (current_field < RTC_FIELD_CONFIRM))
                {
                    const uint16_t arrow_x      = rtc_arrow_coords[current_field].x;
                    const uint16_t arrow_up_y   = rtc_arrow_coords[current_field].up_y;
                    const uint16_t arrow_down_y = rtc_arrow_coords[current_field].down_y;

                    App_WrCoCmd_Buffer(phost, COLOR_RGB(255, 255, 255));

                    /** Down Arrow (rotated 90 deg = 16384 furmans around center 12,12) */
                    Gpu_CoCmd_LoadIdentity(phost);
                    Gpu_CoCmd_RotateAround(phost, 12, 12, 16384, 65536);
                    Gpu_CoCmd_SetMatrix(phost);

                    App_WrCoCmd_Buffer(phost, BITMAP_HANDLE(3U));
                    Gpu_CoCmd_SetBitmap(phost, ARROW_OFFSET, COMPRESSED_RGBA_ASTC_4x4_KHR, 24, 24);
                    App_WrCoCmd_Buffer(phost, BEGIN(BITMAPS));
                    App_WrCoCmd_Buffer(phost, VERTEX2F(arrow_x, arrow_down_y));
                    App_WrCoCmd_Buffer(phost, END());

                    /** Up Arrow (rotated 270 deg = 49152 furmans around center 12,12) */
                    Gpu_CoCmd_LoadIdentity(phost);
                    Gpu_CoCmd_RotateAround(phost, 12, 12, 49152, 65536);
                    Gpu_CoCmd_SetMatrix(phost);

                    App_WrCoCmd_Buffer(phost, BITMAP_HANDLE(4U));
                    Gpu_CoCmd_SetBitmap(phost, ARROW_OFFSET, COMPRESSED_RGBA_ASTC_4x4_KHR, 24, 24);
                    App_WrCoCmd_Buffer(phost, BEGIN(BITMAPS));
                    App_WrCoCmd_Buffer(phost, VERTEX2F(arrow_x, arrow_up_y));
                    App_WrCoCmd_Buffer(phost, END());

                    /** Restore identity matrix so subsequent bitmaps are not rotated */
                    Gpu_CoCmd_LoadIdentity(phost);
                    Gpu_CoCmd_SetMatrix(phost);
                }

                App_WrCoCmd_Buffer(phost, DISPLAY());
                Gpu_CoCmd_Swap(phost);
                App_Flush_Co_Buffer(phost);
                Gpu_Hal_WaitCmdfifo_empty(phost);

                break;
            }
        case SETUP_BAKING:
            {
                break_time = 250U;
                break;
            }
        case BAKING:
            {
                /** Start new comand list */
                Gpu_CoCmd_Dlstart(phost);

                /** Set background colour as light gray */
                App_WrCoCmd_Buffer(phost, CLEAR_COLOR_RGB(150, 150, 150));
                App_WrCoCmd_Buffer(phost, CLEAR(1, 1, 1));
                

                /** Set line colour to black */
                App_WrCoCmd_Buffer(phost, COLOR_RGB(0, 0, 0));
                App_WrCoCmd_Buffer(phost, VERTEX_FORMAT(0U));

                /** Draw lines for down part of screen */
                App_WrCoCmd_Buffer(phost, BEGIN(LINES));
                App_WrCoCmd_Buffer(phost, VERTEX2F(0, 188));
                App_WrCoCmd_Buffer(phost, VERTEX2F(480, 188));
                App_WrCoCmd_Buffer(phost, END());

                App_WrCoCmd_Buffer(phost, BEGIN(LINES));
                App_WrCoCmd_Buffer(phost, VERTEX2F(80, 188));
                App_WrCoCmd_Buffer(phost, VERTEX2F(80, 272));
                App_WrCoCmd_Buffer(phost, END());

                App_WrCoCmd_Buffer(phost, BEGIN(LINES));
                App_WrCoCmd_Buffer(phost, VERTEX2F(160, 188));
                App_WrCoCmd_Buffer(phost, VERTEX2F(160, 272));
                App_WrCoCmd_Buffer(phost, END());

                App_WrCoCmd_Buffer(phost, BEGIN(LINES));
                App_WrCoCmd_Buffer(phost, VERTEX2F(380, 188));
                App_WrCoCmd_Buffer(phost, VERTEX2F(380, 272));
                App_WrCoCmd_Buffer(phost, END());


                /** Draw 4 gauges for temp readings */
                const float temp1 = TempReal[0];
                const float temp2 = TempReal[1];
                const float temp3 = TempReal[2];
                const float temp4 = TempReal[3];

                App_WrCoCmd_Buffer(phost, COLOR_RGB(255, 16, 72)); //Needle and mark colour
                Gpu_CoCmd_BgColor(phost, 0x222222); //Background colour

                Gpu_CoCmd_Gauge(phost, 60, 88, 36, OPT_FLAT, 4, 8, (uint16_t)(temp1 + 30.0f), 230);
                Gpu_CoCmd_Gauge(phost, 180, 88, 36, OPT_FLAT, 4, 8, (uint16_t)(temp2 + 30.0f), 230);
                Gpu_CoCmd_Gauge(phost, 300, 88, 36, OPT_FLAT, 4, 8, (uint16_t)(temp3 + 30.0f), 230);
                Gpu_CoCmd_Gauge(phost, 420, 88, 36, OPT_FLAT, 4, 8, (uint16_t)(temp4 + 30.0f), 230);


                /** Draw stage progress bar (time left in stage) */
                const uint16_t min_passed = Furnace_state.minutes_passed;
                const uint16_t min_all = Stages[Furnace_state.stage_ID].stage_time_min;

                App_WrCoCmd_Buffer(phost, COLOR_RGB(255, 158, 73)); //New colour for bar
                Gpu_CoCmd_Progress(phost, 40, 172, 400, 12, OPT_FLAT, min_passed, min_all);


                /** Draw clock in botom right corner */
                const uint32_t time = read_time();
                const uint16_t hour = RTC_UNPACK_HOUR(time);
                const uint16_t min = RTC_UNPACK_MIN(time);
                const uint16_t sec = RTC_UNPACK_SEC(time);

                App_WrCoCmd_Buffer(phost, COLOR_RGB(255, 226, 6)); //New colour for clock
                Gpu_CoCmd_Clock(phost, 430, 238, 32, OPT_FLAT, hour, min, sec, 0);


                /** Time to draw all the text */
                App_WrCoCmd_Buffer(phost, COLOR_RGB(0, 0, 0)); //Black text
                Gpu_CoCmd_Text(phost, 240, 0, 30, OPT_CENTERX, "BAKING"); //Top of screen

                Gpu_CoCmd_Text(phost, 60, 36, 26, OPT_CENTERX, "Temp 1"); //Names of gauges
                Gpu_CoCmd_Text(phost, 180, 36, 26, OPT_CENTERX, "Temp 2");
                Gpu_CoCmd_Text(phost, 300, 36, 26, OPT_CENTERX, "Temp 3");
                Gpu_CoCmd_Text(phost, 420, 36, 26, OPT_CENTERX, "Temp 4");

                char temp_val1[8];
                char temp_val2[8];
                char temp_val3[8];
                char temp_val4[8];
                
                strfromf(temp_val1, sizeof(temp_val1), "%.1f", temp1);
                strfromf(temp_val2, sizeof(temp_val2), "%.1f", temp2);
                strfromf(temp_val3, sizeof(temp_val3), "%.1f", temp3);
                strfromf(temp_val4, sizeof(temp_val4), "%.1f", temp4);

                Gpu_CoCmd_Text(phost, 60, 128, 26, OPT_CENTERX, temp_val1); //Values of gauges
                Gpu_CoCmd_Text(phost, 180, 128, 26, OPT_CENTERX, temp_val2);
                Gpu_CoCmd_Text(phost, 300, 128, 26, OPT_CENTERX, temp_val3);
                Gpu_CoCmd_Text(phost, 420, 128, 26, OPT_CENTERX, temp_val4);

                char prog_bar_text[64];
                snprintf(prog_bar_text, sizeof(prog_bar_text), "%u out of %u minutes passed", min_passed, min_all);
                Gpu_CoCmd_Text(phost, 240, 153, 27, OPT_CENTERX, prog_bar_text); //Progress bar description
                
                extern bool heater_on;
                Gpu_CoCmd_Text(phost, 40, 188, 27, OPT_CENTERX, heater_on? "Heat ON" : "Heat OFF"); //Heat status

                extern bool fan_on;
                Gpu_CoCmd_Text(phost, 120, 188, 27, OPT_CENTERX, fan_on? "Fan ON" : "Fan OFF"); //Fan status

                char stage_info[128];
                const float avg_tempt = Furnace_state.average_temp;
                const uint32_t cur_stage = Furnace_state.stage_ID;
                const uint32_t all_stage = Furnace_state.stages_num;
                const uint32_t tem_stage = Stages[cur_stage].stage_required_temp;
                const uint32_t gro_stage = Stages[cur_stage].temp_grow;
                const uint32_t tim_stage = Stages[cur_stage].stage_time_min;
                snprintf(stage_info, sizeof(stage_info), "Stage %u out of %u.\nStage temp: %u\nAverage temp: %.1f\nTemp groth: %u\nStage lenght: %u",
                        cur_stage, all_stage, tem_stage, avg_tempt, gro_stage, tim_stage);
                Gpu_CoCmd_Text(phost, 164, 190, 26, 0, stage_info);

                char time_str[8];
                snprintf(time_str, sizeof(time_str), "%02u:%02u:%02u", hour, min, sec);
                Gpu_CoCmd_Text(phost, 430, 188, 27, OPT_CENTERX, time_str); //Print time next to clock

                if(flash_ok)
                {
                    /** Fix colour for sprites */
                    App_WrCoCmd_Buffer(phost, COLOR_RGB(255, 255, 255));

                    /** Draw heat sprite */
                    load_one_asset(HEAT_OFFSET, heater_on? Heat_on_64x64_ASTC_4X4_raw : Heat_off_64x64_ASTC_4X4_raw);
                    App_WrCoCmd_Buffer(phost, BITMAP_HANDLE(2U));
                    Gpu_CoCmd_SetBitmap(phost, HEAT_OFFSET, COMPRESSED_RGBA_ASTC_4x4_KHR, 64, 64);
                    App_WrCoCmd_Buffer(phost, BEGIN(BITMAPS));
                    App_WrCoCmd_Buffer(phost, VERTEX2F(4, 208));
                    App_WrCoCmd_Buffer(phost, END());

                    /** Draw fan sprite */
                    load_one_asset(FAN_OFFSET, fan_on? Fan_rot_0_64x64_ASTC_4X4_raw : Fan_off_64x64_ASTC_4X4_raw);
                    App_WrCoCmd_Buffer(phost, BITMAP_HANDLE(3U));
                    Gpu_CoCmd_SetBitmap(phost, FAN_OFFSET, COMPRESSED_RGBA_ASTC_4x4_KHR, 64, 64);
                    App_WrCoCmd_Buffer(phost, BEGIN(BITMAPS));
                    App_WrCoCmd_Buffer(phost, VERTEX2F(88, 208));
                    App_WrCoCmd_Buffer(phost, END());
                }

                App_WrCoCmd_Buffer(phost, DISPLAY());
                Gpu_CoCmd_Swap(phost);
                App_Flush_Co_Buffer(phost);
                Gpu_Hal_WaitCmdfifo_empty(phost);

                break_time = 1000U;
                break;

            }
        case BAKING_DONE:
            {
                break;
            }
        case UI_ERROR:
            {        
                SET_BIT(SystemStatus, LCD_INIT_ERROR);

                App_Common_Close(phost);

                vTaskSuspend(NULL);

                break;
            }

    }

}
