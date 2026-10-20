#ifndef RTOS_TASKS_H
#define RTOS_TASKS_H

/**
 * @file RTOS_tasks.h
 * @brief Definition all RTOS tasks used in code.
 *
 * @details
 * This file provides definitions of all main tasks functions and they stack sizes.
 *
 * @author Aleksander Dziągwa
 * @date 20.10.2026
 */

#include "FreeRTOS.h"
#include "task.h"



#define BLINK_TASK_STACK_SIZE   256U

static StaticTask_t xBlinkTaskTCB;
static StackType_t  xBlinkTaskStack[BLINK_TASK_STACK_SIZE];

void vBlinkTask(void *pvParameters);


#endif
