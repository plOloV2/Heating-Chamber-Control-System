/**
 * @file UI_task.c
 * @brief Implementation of UI RTOS task controlling screen and user inputs.
 *
 * @details High-level state machine coordinator for the display and user inputs.
 *          Screen-specific logic and rendering are delegated to RTOS/UI/ modules.
 *
 * @author Aleksander Dziągwa
 * @date 15.11.2026
 */

#include "RTOS/UI/UI_screens.h"
#include "RTOS/UI_task.h"
#include "FreeRTOS.h"
#include "task.h"

#include "EVE4_Riverdi_lib/screen_platform.h"
#include "EVE4_Riverdi_lib/App_Common.h"

#include "drivers/buttons_operations.h"
#include "drivers/RNG_operations.h"
#include "drivers/RTC_operations.h"
#include "system_state/error.h"
#include "furnace_struck.h"

/** Screen driver context and state variables */
static Gpu_Hal_Context_t host, *phost;
static uint32_t break_time = 1000U;
static bool flash_ok = false;
static UI_states UI_State = INIT_UI;


void vUITask(void *pvParameters)
{
    (void)pvParameters;

    phost = &host;

    rng_hw_init();
    configure_buttons();

    while(true)
    {
        
        switch(UI_State)
        {
            case INIT_UI:
                break_time = 10U;
                UI_State   = UI_InitAndCheckFlash(phost, &flash_ok);
                break;

            case FLASH_BLOB:
                break_time = 0U;
                UI_State   = UI_StreamFlashBlob(phost, &flash_ok);
                break;

            case GREETER:
                break_time = 3000U;
                UI_LoadDefaultAssets(phost, flash_ok);
                UI_RenderGreeterScreen(phost, flash_ok);
                UI_State = RTC_valid ? SETUP_BAKING : RTC_SET;
                break;

            case RTC_SET:
                break_time = 150U;
                if(UI_UpdateRtcSetState())
                {
                    UI_State = SETUP_BAKING;
                }
                UI_RenderRtcSetScreen(phost, flash_ok);
                break;

            case SETUP_BAKING:
                break_time = 150U;
                if(UI_UpdateSetupBakingState())
                {
                    UI_State = BAKING;
                }
                UI_RenderSetupBakingScreen(phost);
                break;

            case BAKING:
                if(Furnace_state.stage_ID >= Furnace_state.stages_num)
                {
                    UI_State = BAKING_DONE;
                    break_time = 10U;
                    break;
                }
                break_time = 1000U;
                UI_RenderBakingScreen(phost, flash_ok);
                break;

            case BAKING_DONE:
                break_time = 1000U;
                UI_RenderEndBakingScreen(phost, flash_ok);
                break;

            case UI_ERROR:
                SET_BIT(SystemStatus, LCD_INIT_ERROR);
                App_Common_Close(phost);
                vTaskSuspend(NULL);
                break;
        }

        vTaskDelay(pdMS_TO_TICKS(break_time));

    }

}
