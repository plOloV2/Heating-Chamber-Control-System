#include "stm32f405xx.h"
#include "../../platform/system_init.h"
#include "error.h"
#include "version.h"


volatile uint32_t iter = 0;
volatile system_error_t SystemStatus = NO_ERROR;


_Static_assert((sizeof(APP_VERSION)/sizeof(char)) <= 63);
const char VersionNum[64] = APP_VERSION;


__attribute__((noreturn)) int main(){

    if(SystemInitStatus != SYSTEM_INIT_STATUS_OK){
        
        SystemStatus |= SYSTEM_INIT_ERROR;

    }

    while(true){

        iter++;

    }

}
