/**
 * @file create_tasks.c
 * @brief Static creation of all FreeRTOS tasks.
 *
 * @author Aleksander Dziągwa
 * @date 20.10.2026
 */

#include "RTOS/buzzer_task.h"
#include "RTOS/temp_task.h"
#include "RTOS/SD_task.h"
#include "RTOS/UI_task.h"
#include "mem_location.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"


/* Static memory allocation for Buzzer task */
CCMRAM_BSS TaskHandle_t xBuzzerTaskHandle = NULL;
CCMRAM_BSS static StaticTask_t xBuzzerTaskTCB;
CCMRAM_BSS static StackType_t  xBuzzerTaskStack[BUZZER_TASK_STACK_SIZE];

/* Static memory allocation for Temp task */
CCMRAM_BSS TaskHandle_t xTempTaskHandle = NULL;
CCMRAM_BSS static StaticTask_t xTempTaskTCB;
CCMRAM_BSS static StackType_t  xTempTaskStack[TEMP_TASK_STACK_SIZE];

/* Static memory allocation for SD task */
CCMRAM_BSS TaskHandle_t xSDTaskHandle = NULL;
CCMRAM_BSS static StaticTask_t xSDTaskTCB;
static StackType_t  xSDTaskStack[SD_TASK_STACK_SIZE];

/* Static memory allocation for UI task */
CCMRAM_BSS TaskHandle_t xUITaskHandle = NULL;
CCMRAM_BSS static StaticTask_t xUITaskTCB;
CCMRAM_BSS static StackType_t  xUITaskStack[UI_TASK_STACK_SIZE];


/* Semaphores used by UI and SD card to check screen flash */
SemaphoreHandle_t sd_ready = NULL;
static StaticSemaphore_t sd_ready_buff;

SemaphoreHandle_t ui_ready = NULL;
static StaticSemaphore_t ui_ready_buff;


/** @brief Calls xTaskCreateStatic() to create RTOS tasks. */
void create_tasks(void){

    sd_ready = xSemaphoreCreateBinaryStatic(&sd_ready_buff);
    ui_ready = xSemaphoreCreateBinaryStatic(&ui_ready_buff);

    xBuzzerTaskHandle = xTaskCreateStatic(
        vBuzzerTask,
        "Buzzer",
        BUZZER_TASK_STACK_SIZE,
        NULL,
        tskIDLE_PRIORITY + 2U,
        xBuzzerTaskStack,
        &xBuzzerTaskTCB
    );

    xTempTaskHandle = xTaskCreateStatic(
        vTempTask,
        "Temp conv",
        TEMP_TASK_STACK_SIZE,
        NULL,
        tskIDLE_PRIORITY + 1U,
        xTempTaskStack,
        &xTempTaskTCB
    );

    xSDTaskHandle = xTaskCreateStatic(
        vSDTask,
        "SD Card",
        SD_TASK_STACK_SIZE,
        NULL,
        tskIDLE_PRIORITY + 3U,
        xSDTaskStack,
        &xSDTaskTCB
    );

    xUITaskHandle = xTaskCreateStatic(
        vUITask,
        "UI",
        UI_TASK_STACK_SIZE,
        NULL,
        tskIDLE_PRIORITY + 3U,
        xUITaskStack,
        &xUITaskTCB
    );

}
