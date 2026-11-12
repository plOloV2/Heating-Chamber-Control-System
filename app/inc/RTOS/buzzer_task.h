#ifndef BUZZER_TASK_H
#define BUZZER_TASK_H

/**
 * @file buzzer_task.h
 * @brief Buzzer control definitions and task interface.
 *
 * @details Defines the note structure (frequency in Hz, duration in ms)
 *          and the RTOS task controlling the passive buzzer via PWM (TIM2 CH3).
 *
 * @author Aleksander Dziągwa
 * @date 23.10.2026
 */

#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"

/** @brief Number of notes in a buzzer melody */
#define BUZZER_MELODY_NOTES_COUNT 8U

/**
 * @brief Structure representing an individual musical note or pause.
 */
typedef struct {
    uint32_t frequency_hz; /**< Sound frequency in Hz (0 Hz means silence / pause) */
    uint32_t duration_ms;  /**< Note duration in milliseconds (0 ms means end of melody) */
} buzzer_note_t;

/** @brief Stack size for the buzzer RTOS task */
#define BUZZER_TASK_STACK_SIZE 256U

/** @brief Handle for the buzzer RTOS task */
extern TaskHandle_t xBuzzerTaskHandle;

/**
 * @brief FreeRTOS task function controlling the buzzer via TIM2 PWM.
 *
 * @param pvParameters Unused task parameter.
 */
void vBuzzerTask(void *pvParameters);

#endif
