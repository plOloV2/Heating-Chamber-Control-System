#include "RTOS/RTOS_tasks.h"

#include "FreeRTOS.h"
#include "task.h"

void create_tasks(void){

    xTaskCreateStatic(
        vBlinkTask,
        "Blink",
        BLINK_TASK_STACK_SIZE,
        NULL,
        tskIDLE_PRIORITY + 1U,
        xBlinkTaskStack,
        &xBlinkTaskTCB
    );

}
