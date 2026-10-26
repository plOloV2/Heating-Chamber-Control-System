/**
 * @file buzzer_operations.c
 * @brief Implementation of buzzer driver.
 *
 * @details Inplements code to initialize and controll buzzer via TIM2 channel 3 PWM signal.
 *
 * @author Aleksander Dziągwa
 * @date 25.10.2026
 */

#include "stm32f4xx.h"
#include "AF_pins_def.h"
#include "GPIO_pins_def.h"

/**
 * @brief Initialize TIM2 Channel 3 for PWM generation.
 */
void buzzer_hw_init(void)
{
    /* Enable TIM2 clock on APB1 */
    SET_BIT(RCC->APB1ENR, RCC_APB1ENR_TIM2EN);
    (void)RCC->APB1ENR;

    /*
     * APB1 timer clock runs at 84 MHz (HCLK 168MHz / 4 * 2).
     * Set prescaler to 84 - 1 so the timer counts at 1 MHz (1 tick = 1 microsecond).
     */
    TIM2->PSC = 84U - 1U;
    TIM2->ARR = 1000U - 1U;
    TIM2->CCR3 = 0U;

    /* Configure Channel 3 as PWM Mode 1 with preload enabled */
    MODIFY_REG(TIM2->CCMR2,
               TIM_CCMR2_OC3M | TIM_CCMR2_OC3PE,
               (6UL << TIM_CCMR2_OC3M_Pos) | TIM_CCMR2_OC3PE);

    /* Output polarity active high (PA2 high turns on Q5 pulling buzzer to GND) */
    CLEAR_BIT(TIM2->CCER, TIM_CCER_CC3P);

    /* Output initially disabled */
    CLEAR_BIT(TIM2->CCER, TIM_CCER_CC3E);

    /* Enable Auto-reload preload */
    SET_BIT(TIM2->CR1, TIM_CR1_ARPE);

    /* Force register reload via update event */
    TIM2->EGR = TIM_EGR_UG;
}

/**
 * @brief Stop sound and drive pin low so Q5 is completely OFF.
 */
static inline void stop_PWM(void)
{
    CLEAR_BIT(TIM2->CR1, TIM_CR1_CEN);
    CLEAR_BIT(TIM2->CCER, TIM_CCER_CC3E);
    TIM2->CCR3 = 0U;
    TIM2->CNT = 0U;
}

/**
 * @brief Generate a square wave at the specified frequency with 50% duty cycle.
 *
 * @param frequency_hz Frequency in Hz. If 0, tone is stopped.
 */
void buzzer_play_tone(uint32_t frequency_hz)
{
    if (frequency_hz == 0U)
    {
        stop_PWM();
        return;
    }

    /* Period in microseconds for 1 MHz timer count rate */
    uint32_t period_us = 1000000UL / frequency_hz;
    if(period_us < 2U)
    {
        period_us = 2U;
    }

    TIM2->ARR = period_us - 1U;
    TIM2->CCR3 = period_us / 2U; /* 50% duty cycle */

    TIM2->EGR = TIM_EGR_UG;
    SET_BIT(TIM2->CCER, TIM_CCER_CC3E);
    SET_BIT(TIM2->CR1, TIM_CR1_CEN);
}
