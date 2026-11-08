#include "RTOS/UI_task.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "drivers/pin_operations.h"
#include "GPIO_pins_def.h"

#include "system_state/error.h"
#include "version.h"

#include "EVE4_Riverdi_lib/screen_platform.h"
#include "EVE4_Riverdi_lib/App_Common.h"

/** Timeout waiting for SD task (1 second) */
#define SD_SYNC_TIMEOUT_MS      1000U

/** EVE RAM_G staging address for 4 KiB flash updates */
#define EVE_RAM_G_STAGING_ADDR  0x000000UL

volatile bool mag_num = false;
extern void* data_buffer;

extern SemaphoreHandle_t sd_ready;
extern SemaphoreHandle_t ui_ready;

static Gpu_Hal_Context_t host, *phost;
static uint32_t break_time = 1000U;
static UI_states UI_State = INIT_UI;
static uint32_t flash_write_offset = 0U;

static void UI_state_machine(void);
static bool check_screen_flash_magic(void);

void vUITask(void *pvParameters)
{
    (void)pvParameters;

    phost = &host;

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


static void UI_state_machine(void)
{
    const uint32_t total_blob_size =
        flash_asset_list[magic_val_bin].offset + flash_asset_list[magic_val_bin].size;

    switch (UI_State)
    {
        case INIT_UI:

            /** Set wait time to 10ms */
            break_time = 10U;

            /** Initialize the screen, go to error state if failed */
            if (!App_Common_Init(&host))
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
                UI_State = GREETER;
            }
            else
            {
                /** Magic mismatch: stream whole blob in FLASH_BLOB state */
                UI_State = FLASH_BLOB;
            }

            break;

        case FLASH_BLOB:

            break_time = 0U;
            flash_write_offset = 0U;

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

            /** Switch to FULL (fast QSPI) mode right after sector 0 (default_fl_blob) is written */
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

                UI_State = GREETER;
            }

            break;

        case GREETER:

            break_time = 2000U;
            break;

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
