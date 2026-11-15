#ifndef UI_SCREENS_H
#define UI_SCREENS_H

/**
 * @file UI_screens.h
 * @brief Internal declarations for modular UI screens and flash asset helpers.
 *
 * @author Aleksander Dziągwa
 * @date 15.11.2026
 */

#include <stdint.h>
#include "RTOS/UI_task.h"
#include "EVE4_Riverdi_lib/screen_platform.h"
#include "EVE4_Riverdi_lib/screen_flash_map.h"


/**
 * @brief Unitializes screen and checks magic number in flash.
 * 
 * @param phost Pointer to host for EVE4 library
 * @param p_flash_ok Pointer to flag symbolising status of magic num check
 * 
 * @returns UI_ERROR on initialization error, GREETER if magic num matches or check is imposible, FLASH_BLOB if magic num mismatches
 */
UI_states UI_InitAndCheckFlash(Gpu_Hal_Context_t *phost, bool *p_flash_ok);

/**
 * @brief Flashes screen's flash with blob from SD card.
 * 
 * @param phost Pointer to host for EVE4 library
 * @param p_flash_ok Pointer to flag symbolising status of magic num check, seted to TRUE on succesful flash
 * 
 * @returns UI_ERROR on error, GREETER if flashing succeds or hangs becouse of SD card problem, FLASH_BLOB if flashing is not over
 */
UI_states UI_StreamFlashBlob(Gpu_Hal_Context_t *phost, bool *p_flash_ok);

/**
 * @brief Loads default assets into screen's G RAM.
 * 
 * @param phost Pointer to host for EVE4 library
 * @param flash_ok Flag symbolising status of magic num check
 */
void UI_LoadDefaultAssets(Gpu_Hal_Context_t *phost, bool flash_ok);

/**
 * @brief Loads default assets into screen's G RAM.
 * 
 * @param phost Pointer to host for EVE4 library
 * @param flash_ok Flag symbolising status of magic num check
 * @param offset G RAM offset at which asset should be loaded
 * @param asset_index ID of asset to load
 */
void UI_LoadOneAsset(Gpu_Hal_Context_t *phost, bool flash_ok, uint32_t offset, flash_asset_index asset_index);

/**
 * @brief Renders Greeter screen using EVE4 lib.
 * 
 * @param phost Pointer to host for EVE4 library
 * @param flash_ok Flag symbolising status of magic num check
 */
void UI_RenderGreeterScreen(Gpu_Hal_Context_t *phost, bool flash_ok);

/** 
 * @brief Reads buttons inputs and updates date and time values
 * 
 * @returns TRUE if user wants to write date and time into RTC 
 */
bool UI_UpdateRtcSetState(void);

/**
 * @brief Renders RTC update screen using EVE4 lib.
 * 
 * @param phost Pointer to host for EVE4 library
 * @param flash_ok Flag symbolising status of magic num check
 */
void UI_RenderRtcSetScreen(Gpu_Hal_Context_t *phost, bool flash_ok);

/** 
 * @brief Reads buttons inputs and updates furnace_stage_data.
 * 
 * @returns TRUE if user wants to write date into Stages[]
 */
bool UI_UpdateSetupBakingState(void);

/**
 * @brief Renders baking setup screen using EVE4 lib.
 * 
 * @param phost Pointer to host for EVE4 library
 */
void UI_RenderSetupBakingScreen(Gpu_Hal_Context_t *phost);

/**
 * @brief Renders main baking screen using EVE4 lib.
 * 
 * @param phost Pointer to host for EVE4 library
 * @param flash_ok Flag symbolising status of magic num check
 */
void UI_RenderBakingScreen(Gpu_Hal_Context_t *phost, bool flash_ok);

/**
 * @brief Renders screen at the end of baking using EVE4 lib.
 * 
 * @param phost Pointer to host for EVE4 library
 * @param flash_ok Flag symbolising status of magic num check
 */
void UI_RenderEndBakingScreen(Gpu_Hal_Context_t *phost, bool flash_ok);

#endif
