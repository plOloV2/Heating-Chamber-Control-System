#include "stm32f405xx.h"

#include "../../platform/system_init.h"
#include "error.h"
#include "version.h"
#include "GPIO_pins_def.h"
#include "pin_operation.h"


#include "FreeRTOS.h"
#include "task.h"

#include "furnace_struck.h"


CCMRAM_BSS furnace_stage_data Stages[MAX_STAGES];

volatile uint32_t iter = 0;
volatile system_error_t SystemStatus = NO_ERROR;

void create_tasks(void);


__attribute__((noreturn)) int main(){

    /* Check if initialization went without any problems. */
    if(SystemInitStatus != SYSTEM_INIT_STATUS_OK){
        
        SystemStatus |= SYSTEM_INIT_ERROR;

    }

    /* Flash LEDs to signal boot process ended. */
    SetPin(LED1_PORT, LED1_PIN, LED1_ACTIVE);
    SetPin(LED2_PORT, LED2_PIN, LED2_ACTIVE);

    create_tasks();

    /* FreeRTOS takes controll of code. */
    vTaskStartScheduler();

    /* Code should never reach this part of main(). */
    SystemStatus |= RTOS_ERROR;

    while(true)
    {
        __NOP();
    }

}
