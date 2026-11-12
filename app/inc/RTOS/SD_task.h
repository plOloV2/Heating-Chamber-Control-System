#ifndef SD_TASK_H
#define SD_TASK_H

/**
 * @file SD_task.h
 * @brief SD Card control definitions and task interface.
 *
 * @details SD task is resposible for all FatFs and SD card operations.
 *          Used for 2 things:
 *              1. Check screen's flash content with SD card and flash from SD if missmatched;
 *              2. Log curring parameters while in main operation
 *
 * @author Aleksander Dziągwa
 * @date 26.10.2026
 */

#include "FreeRTOS.h"
#include "task.h"


/** @brief Stack size for the SD RTOS task */
#define SD_TASK_STACK_SIZE 2048U

/** @brief Handle for the SD RTOS task */
extern TaskHandle_t xSDTaskHandle;

/**
 * @brief FreeRTOS task function controlling the SD via SDIO.
 *
 * @param pvParameters Unused task parameter.
 */
void vSDTask(void *pvParameters);

#endif
