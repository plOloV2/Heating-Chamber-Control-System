#include "stm32f405xx.h"
#include "../../platform/system_init.h"
#include "error.h"
#include "version.h"
#include "pins_def.h"
#include "pin_operation.h"


#include "FreeRTOS.h"
#include "task.h"



volatile uint32_t iter = 0;
volatile system_error_t SystemStatus = NO_ERROR;

void create_blink(void);


__attribute__((noreturn)) int main(){

    if(SystemInitStatus != SYSTEM_INIT_STATUS_OK){
        
        SystemStatus |= SYSTEM_INIT_ERROR;

    }

    set_pin(LED1_PORT, LED1_PIN, LED1_ACTIVE);

    create_blink();

    vTaskStartScheduler();

    SystemStatus |= RTOS_ERROR;

    while(true)
    {
        __NOP();
    }

}
