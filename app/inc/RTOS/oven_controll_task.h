#ifndef OVEN_CONTROLL_TASK_H
#define OVEN_CONTROLL_TASK_H

/**
 * @file oven_controll_task.h
 * @brief Heating and fan control task. Most important task in system.
 *
 * @details This task reads user programed curring parameters and real temperature
 *          from temp task. Controlls heating and fan outputs to match required temp.
 *          Oven controll task is waken up by temp task.
 *
 * @author Aleksander Dziągwa
 * @date 12.11.2026
 */

#include "FreeRTOS.h"
#include "task.h"

/** @brief Stack size for the buzzer RTOS task */
#define OVEN_CONTROLL_TASK_STACK_SIZE 512U

/** @brief Handle for the buzzer RTOS task */
extern TaskHandle_t xOvenControllTaskHandle;

/**
 * @brief FreeRTOS task function controlling the buzzer via TIM2 PWM.
 *
 * @param pvParameters Unused task parameter.
 */
void vOvenControllTask(void *pvParameters);

#endif