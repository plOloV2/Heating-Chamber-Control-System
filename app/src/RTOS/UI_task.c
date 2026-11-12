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
#include "version.h"
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

/** Main state machine */
static UI_states UI_State = INIT_UI;

/** Main state machine code, runs forever. */
static void UI_state_machine(void);

/** 
 * @brief Checks magic value from screen flash with one loaded from SD card.
 * 
 * @returns TRUE if they match, FLASE on missmatch or error.
 */
static bool check_screen_flash_magic(void);


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
    if(rng_num % 2137 == 0)
    {
        logo_index = Logo_PWR_RT_GOLD_384x88_ASTC_4X4_raw;
    } else if(rng_num % 2 == 0)
    {
        logo_index = Logo_PWR_RT_WHITE_384x88_ASTC_4X4_raw;
    } else
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


[[maybe_unused]] static void load_one_asset(uint32_t offset, flash_asset_index asset_index)
{

    if(!flash_ok)
    {
        return;
    }

    Gpu_CoCmd_FlashRead(phost, offset,
                        flash_asset_list[asset_index].offset,
                        flash_asset_list[asset_index].size);

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
                (void)Gpu_CoCmd_FlashHelper_SwitchFullMode(phost);
                Gpu_CoCmd_FlashHelper_ClearCache(phost);

                flash_ok = true;
                UI_State = GREETER;
            }

            break;
        }
        case GREETER:
        {
            /** Stop the coprocessor spinner started in INIT_UI */
            Gpu_CoCmd_Stop(phost);

            /** Load assets from QSPI flash into RAM_G before starting the display list */
            if(flash_ok)
            {
                load_all_assets();
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
            snprintf(version, sizeof(version), "Version: %s", APP_VERSION);

            Gpu_CoCmd_Text(phost, 16, 232, 27, 0, "Aero oven");
            Gpu_CoCmd_Text(phost, 16, 248, 26, 0, version);

            /** End and swap the display list so GREETER renders on screen */
            App_WrCoCmd_Buffer(phost, DISPLAY());
            Gpu_CoCmd_Swap(phost);
            App_Flush_Co_Buffer(phost);
            Gpu_Hal_WaitCmdfifo_empty(phost);

            break_time = 2000U;

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

            break_time = 250U;
            break;

        case SETUP_BAKING:

            break_time = 250U;
            break;

        case BAKING:

            break_time = 1000U;
            break;

        case BAKING_DONE:
            break;

        case UI_ERROR:

            SET_BIT(SystemStatus, LCD_INIT_ERROR);

            App_Common_Close(phost);

            vTaskSuspend(NULL);

            break;
    }

}
