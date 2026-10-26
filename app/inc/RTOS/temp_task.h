#ifndef TEMP_TASK_H
#define TEMP_TASK_H

/**
 * @file temp_task.h
 * @brief Temperature conversion definitions and task interface.
 *
 * @details
 *
 * @author Aleksander Dziągwa
 * @date 25.10.2026
 */

#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"

/** @brief Stack size for the temp RTOS task */
#define TEMP_TASK_STACK_SIZE 512U

/** @brief Handle for the temp RTOS task */
extern TaskHandle_t xTempTaskHandle;

/**
 * @brief FreeRTOS task function controlling the raw ADC value to temperature in C conversion.
 *
 * @param pvParameters Unused task parameter.
 */
void vTempTask(void *pvParameters);

#endif
