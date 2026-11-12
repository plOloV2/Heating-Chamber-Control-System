#ifndef TEMP_TASK_H
#define TEMP_TASK_H

/**
 * @file temp_task.h
 * @brief Temperature conversion definitions and task interface.
 *
 * @details This task is waken up every second by DMA2 interrupt.
 *          It converts raw ADC1 readings into actual temperature values
 *          in Celsius and wakes up Controll task later.
 *
 * @author Aleksander Dziągwa
 * @date 25.10.2026
 */

#include "FreeRTOS.h"
#include "task.h"

#include "mem_location.h"

/** Number of PT1000 channels on PCB */
#define TEMP_SENSOR_COUNT   4U

/** @brief Stack size for the temp RTOS task */
#define TEMP_TASK_STACK_SIZE 512U

/** @brief Handle for the temp RTOS task */
extern TaskHandle_t xTempTaskHandle;

/** Acual temperature value read from PT1000 */
extern CCMRAM_DATA volatile float TempReal[TEMP_SENSOR_COUNT];

/**
 * @brief FreeRTOS task function controlling the raw ADC value to temperature in C conversion.
 *
 * @param pvParameters Unused task parameter.
 */
void vTempTask(void *pvParameters);

#endif
