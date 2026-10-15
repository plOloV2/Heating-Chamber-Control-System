#include "stm32f405xx.h"
#include "../../platform/system_init.h"
#include "error.h"
#include "version.h"
#include "pins_def.h"
#include "pin_opertion.h"


volatile uint32_t iter = 0;
volatile system_error_t SystemStatus = NO_ERROR;


__attribute__((noreturn)) int main(){

    if(SystemInitStatus != SYSTEM_INIT_STATUS_OK){
        
        SystemStatus |= SYSTEM_INIT_ERROR;

    }

    set_pin(LED1_PORT, LED1_PIN, LED1_ACTIVE);

    while(true){

        iter++;

    }

}
