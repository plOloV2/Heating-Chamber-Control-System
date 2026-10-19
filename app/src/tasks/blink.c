#include "pins_def.h"
#include "pin_operation.h"

#include "FreeRTOS.h"
#include "task.h"

#define BLINK_TASK_STACK_SIZE   256U

static StaticTask_t xBlinkTaskTCB;
static StackType_t  xBlinkTaskStack[BLINK_TASK_STACK_SIZE];

static void vBlinkTask(void *pvParameters){
    (void)pvParameters;

    for (;;)
    {
        toggle_pin(LED1_PORT, LED1_PIN);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    
}


void create_blink(void){

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
