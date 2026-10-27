#include "stm32f4xx.h"

#include "system_state/system_init_error.h"
#include "system_state/error.h"
#include "version.h"
#include "GPIO_pins_def.h"
#include "drivers/pin_operations.h"


#include "FreeRTOS.h"
#include "task.h"

#include "furnace_struck.h"


CCMRAM_BSS furnace_stage_data Stages[MAX_STAGES];

volatile bool RTC_valid = false;
volatile system_error_t SystemStatus = NO_ERROR;

void create_tasks(void);

void SystemStatus_update_start(void);


__attribute__((noreturn)) int main(){

    SystemStatus_update_start();

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

/** 
 * @brief Reads SystemInitStatus and updates SystemStatus, 
 * also sets RTC_valid to TRUE if init succeded 
 */
void SystemStatus_update_start(void){

    /** Initialized without problems, noting to do */
    if(SystemInitStatus == SYSTEM_INIT_STATUS_OK)
    {
        RTC_valid = true;
        return;
    }

    /** Some init error ocured */
    if(READ_BIT(SystemInitStatus, SYSTEM_INIT_STATUS_HSE_TIMEOUT) ||
       READ_BIT(SystemInitStatus, SYSTEM_INIT_STATUS_PLL_TIMEOUT) ||
       READ_BIT(SystemInitStatus, SYSTEM_INIT_STATUS_SYSCLK_SWITCH_TIMEOUT) ||
       READ_BIT(SystemInitStatus, SYSTEM_INIT_STATUS_LSE_TIMEOUT))
    {
        SET_BIT(SystemStatus, SYSTEM_INIT_ERROR);
    }

    /** RTC started, but missed date and time */
    if(READ_BIT(SystemInitStatus, SYSTEM_INIT_STATUS_LSE_NINIT))
    {
        SET_BIT(SystemStatus, RTC_DATE_ERROR);
    }

}
