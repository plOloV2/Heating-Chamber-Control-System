#ifndef UI_TASK_H
#define UI_TASK_H

/**
 * @file UI_task.h
 * @brief UI control definitions and task interface.
 *
 * @details
 * 
 *
 * @author Aleksander Dziągwa
 * @date 05.11.2026
 */

#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"

/** @brief UI state machine state */
typedef enum{
    INIT_UI,
    FLASH_BLOB,
    GREETER,
    RTC_SET,
    SETUP_BAKING,
    BAKING,
    BAKING_DONE,
    UI_ERROR
} UI_states;

/** @brief Stack size for the UI RTOS task */
#define UI_TASK_STACK_SIZE 1024U

/** @brief Handle for the UI RTOS task */
extern TaskHandle_t xUITaskHandle;

/**
 * @brief FreeRTOS task function controlling the UI, screen, user inputs.
 *
 * @param pvParameters Unused task parameter.
 */
void vUITask(void *pvParameters);

#endif
