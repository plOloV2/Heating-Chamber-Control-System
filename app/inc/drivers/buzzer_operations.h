#ifndef BUZZER_OPERATIONS_H
#define BUZZER_OPERATIONS_H

/**
 * @file buzzer_operations.h
 * @brief Definition of buzzer driver functions.
 *
 * @details Defines functions that are ptovided by buzzer driver in buzzer_operations.c file.
 *
 * @author Aleksander Dziągwa
 * @date 25.10.2026
 */

#include <stdint.h>

/**
 * @brief Initialize TIM2 Channel 3 for PWM generation.
 */
void buzzer_hw_init(void);

/**
 * @brief Generate a square wave at the specified frequency with 50% duty cycle.
 *
 * @param frequency_hz Frequency in Hz. If 0, tone is stopped.
 */
void buzzer_play_tone(uint32_t frequency_hz);

#endif
