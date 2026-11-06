#include "RTOS/UI_task.h"
#include "FreeRTOS.h"
#include "task.h"

#include "drivers/pin_operations.h"
#include "GPIO_pins_def.h"

#include "system_state/error.h"
#include "version.h"

#include "EVE4_Riverdi_lib/screen_platform.h"
#include "EVE4_Riverdi_lib/App_Common.h"


static Gpu_Hal_Context_t host, *phost;
static uint32_t break_time = 1000U;
static UI_states UI_State = INIT_UI;
static void UI_state_machine(void);

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

static void UI_state_machine(void)
{

    switch(UI_State)
    {
        case INIT_UI:

            break_time = 10U;

            if(!App_Common_Init(&host))
            {
                UI_State = UI_ERROR;
                break;
            }

            SetPin(BLS_PORT, BLS_PIN, BLS_ACTIVE);

            Gpu_CoCmd_Spinner(phost, 240, 220, 1, 0);

            UI_State = GREETER;

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
