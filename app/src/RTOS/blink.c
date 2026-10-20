#include "pins_def.h"
#include "pin_operation.h"

#include "FreeRTOS.h"
#include "task.h"


void vBlinkTask(void *pvParameters){
    (void)pvParameters;

    for (;;)
    {
        TogglePin(LED1_PORT, LED1_PIN);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    
}

