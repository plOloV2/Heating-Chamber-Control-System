/**
 * @file UI_flash_assets.c
 * @brief EVE QSPI flash synchronization, asset loading, and GREETER screen.
 *
 * @author Aleksander Dziągwa
 * @date 15.11.2026
 */

#include "RTOS/UI/UI_screens.h"
#include "FreeRTOS.h"
#include "semphr.h"

#include "EVE4_Riverdi_lib/screen_platform.h"
#include "EVE4_Riverdi_lib/App_Common.h"

#include "drivers/pin_operations.h"
#include "drivers/RNG_operations.h"
#include "GPIO_pins_def.h"

#include "system_state/error.h"
#include "version.h"
#include <stdio.h>
#include <string.h>

/** Timeout waiting for SD task (1 second) */
#define SD_SYNC_TIMEOUT_MS      1000U

/** EVE RAM_G staging address for 4 KiB flash updates */
#define EVE_RAM_G_STAGING_ADDR  0x000000UL

/** Shared variables for SD task communication */
volatile bool mag_num = false;
extern void *data_buffer;
extern SemaphoreHandle_t sd_ready;
extern SemaphoreHandle_t ui_ready;

static uint32_t flash_write_offset = 0U;


/**
 * @brief Checks magic value from screen flash with one loaded from SD card.
 * 
 * @param phost Pointer to targeted host screen
 * 
 * @returns TRUE if magic numbers match, FALSE otherwise
 */
static bool check_screen_flash_magic(Gpu_Hal_Context_t *phost)
{
    // Prepare buffor for magic number read from screen
    uint8_t eve_magic_buf[4] = {0};
    const uint32_t magic_offset = flash_asset_list[magic_val_bin].offset;
    const uint32_t magic_size   = flash_asset_list[magic_val_bin].size;

    Gpu_CoCmd_FlashHelper_Init(phost);
    (void)Gpu_CoCmd_FlashHelper_SwitchFullMode(phost);

    // Magic num read from screen
    if(Gpu_CoCmd_FlashHelper_Read(
            phost,
            EVE_RAM_G_STAGING_ADDR,
            magic_offset,
            4U,
            eve_magic_buf) != FLASH_CMD_SUCCESS)
    {
        return false;
    }

    // Comparsison to one read from SD card
    return (memcmp(eve_magic_buf, data_buffer, magic_size) == 0);
}


UI_states UI_InitAndCheckFlash(Gpu_Hal_Context_t *phost, bool *p_flash_ok)
{
    // Initialize EVE4 library and screen
    if(!App_Common_Init(phost))
    {
        return UI_ERROR;
    }

    // Activate backlight and render spinner
    SetPin(BLS_PORT, BLS_PIN, BLS_ACTIVE);

    Gpu_CoCmd_Dlstart(phost);
    App_WrCoCmd_Buffer(phost, CLEAR(1, 1, 1));
    Gpu_CoCmd_Text(phost, 240, 140, 27, OPT_CENTERX, "Checking screen flash...");
    Gpu_CoCmd_Spinner(phost, 240, 200, 1, 0);
    App_Flush_Co_Buffer(phost);
    Gpu_Hal_WaitCmdfifo_empty(phost);

    // Return immediately on SD card error
    if(READ_BIT(SystemStatus, SD_CARD_INIT_ERROR | SD_CARD_READ_ERROR) != 0U)
    {
        return GREETER;
    }

    // Wait for SD task to read magic num from SD card
    if(xSemaphoreTake(sd_ready, pdMS_TO_TICKS(SD_SYNC_TIMEOUT_MS)) != pdTRUE)
    {
        SET_BIT(SystemStatus, SD_CARD_READ_ERROR);
        return GREETER;
    }

    // Compare magic numbers
    mag_num = check_screen_flash_magic(phost);
    (void)xSemaphoreGive(ui_ready);

    if(mag_num)
    {
        *p_flash_ok = true;
        return GREETER;
    }

    flash_write_offset = 0U;
    return FLASH_BLOB;
}


UI_states UI_StreamFlashBlob(Gpu_Hal_Context_t *phost, bool *p_flash_ok)
{
    const uint32_t total_blob_size = flash_asset_list[magic_val_bin].offset + flash_asset_list[magic_val_bin].size;

    // Return if SD task fails or hungs
    if((READ_BIT(SystemStatus, SD_CARD_READ_ERROR) != 0U) ||
       (xSemaphoreTake(sd_ready, pdMS_TO_TICKS(SD_SYNC_TIMEOUT_MS)) != pdTRUE))
    {
        return GREETER;
    }

    // Copy data read from SD card into screen's G RAM 
    Gpu_Hal_WrMem(phost, EVE_RAM_G_STAGING_ADDR, (uint8_t *)data_buffer, FLASH_UPDATE_ALIGN_BYTE);
    (void)xSemaphoreGive(ui_ready);

    // Tell screen to write G RAM content into flash
    if(Gpu_CoCmd_FlashHelper_Update(
            phost,
            flash_write_offset,
            EVE_RAM_G_STAGING_ADDR,
            FLASH_UPDATE_ALIGN_BYTE) != FLASH_CMD_SUCCESS)
    {
        return UI_ERROR;
    }

    // Switch screen's flash into faster mode (requires first 4kb to be written)
    if(flash_write_offset == 0U)
    {
        (void)Gpu_CoCmd_FlashHelper_SwitchFullMode(phost);
    }

    // Update write offset and check if flashing is over
    flash_write_offset += FLASH_UPDATE_ALIGN_BYTE;

    if(flash_write_offset >= total_blob_size)
    {
        Gpu_CoCmd_Stop(phost);
        App_Flush_Co_Buffer(phost);
        Gpu_Hal_WaitCmdfifo_empty(phost);

        *p_flash_ok = true;
        return GREETER;
    }

    return FLASH_BLOB;
}


void UI_LoadDefaultAssets(Gpu_Hal_Context_t *phost, bool flash_ok)
{
    // Stop spinner if coming straight from INIT_UI
    Gpu_CoCmd_Stop(phost);
    App_Flush_Co_Buffer(phost);
    Gpu_Hal_WaitCmdfifo_empty(phost);

    // Return of uninitialized flash
    if(!flash_ok)
    {
        return;
    }

    // Random number for Greeter background lotery
    uint32_t rng_num;
    if(!rng_get_random32(&rng_num))
    {
        rng_num = 69U;
    }

    flash_asset_index background_index = Oven_fire_480x272_ASTC_4X4_raw + (rng_num % 18U);
    flash_asset_index logo_index = (rng_num % 19U == 0U) ? Logo_PWR_RT_GOLD_384x88_ASTC_4X4_raw : Logo_PWR_RT_BLACK_384x88_ASTC_4X4_raw;

    // Assets loading
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

    App_Flush_Co_Buffer(phost);
    Gpu_Hal_WaitCmdfifo_empty(phost);
}


void UI_LoadOneAsset(Gpu_Hal_Context_t *phost, bool flash_ok, uint32_t offset, flash_asset_index asset_index)
{
    if(!flash_ok)
    {
        return;
    }

    Gpu_CoCmd_FlashRead(phost, offset,
                        flash_asset_list[asset_index].offset,
                        flash_asset_list[asset_index].size);
}


void UI_RenderGreeterScreen(Gpu_Hal_Context_t *phost, bool flash_ok)
{
    // Clear screen
    Gpu_CoCmd_Dlstart(phost);
    App_WrCoCmd_Buffer(phost, CLEAR(1, 1, 1));
    App_WrCoCmd_Buffer(phost, VERTEX_FORMAT(0U));

    // Only load assets if flash content is valid
    if(flash_ok)
    {
        App_WrCoCmd_Buffer(phost, BITMAP_HANDLE(0U));
        Gpu_CoCmd_SetBitmap(phost, BACKGROUND_OFFSET, COMPRESSED_RGBA_ASTC_4x4_KHR, 480, 272);
        App_WrCoCmd_Buffer(phost, BEGIN(BITMAPS));
        App_WrCoCmd_Buffer(phost, VERTEX2F(0U, 0U));
        App_WrCoCmd_Buffer(phost, END());

        App_WrCoCmd_Buffer(phost, BITMAP_HANDLE(1U));
        Gpu_CoCmd_SetBitmap(phost, LOGO_OFFSET, COMPRESSED_RGBA_ASTC_4x4_KHR, 384, 88);
        App_WrCoCmd_Buffer(phost, BEGIN(BITMAPS));
        App_WrCoCmd_Buffer(phost, VERTEX2F(48U, 5U));
        App_WrCoCmd_Buffer(phost, END());
    }

    // Print code varsion and project name
    char version[64];
    snprintf(version, sizeof(version), "Code version: %s", APP_VERSION);

    Gpu_CoCmd_Text(phost, 16, 232, 27, 0, "Aero oven");
    Gpu_CoCmd_Text(phost, 16, 248, 26, 0, version);

    App_WrCoCmd_Buffer(phost, DISPLAY());
    Gpu_CoCmd_Swap(phost);
    App_Flush_Co_Buffer(phost);
    Gpu_Hal_WaitCmdfifo_empty(phost);
}
