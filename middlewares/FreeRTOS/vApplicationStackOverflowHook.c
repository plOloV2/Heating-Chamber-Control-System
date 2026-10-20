/**
 * @file vApplicationStackOverflowHook.ld
 * @brief Custom inplementation of vApplicationStackOverflowHook().
 *
 * @details
 * This is a custom inplementation of vApplicationStackOverflowHook for FreeRTOS.
 *
 * @author Aleksander Dziągwa
 * @date 20.10.2026
 */


#include "stm32f4xx.h"
#include "FreeRTOS.h"
#include "task.h"


void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;

    taskDISABLE_INTERRUPTS();
    for (;;)
    {
        __NOP();
    }
    
}
