/**
 * @file button_init.c
 * @brief Implementation of button EXTI and 25 ms timer initialization.
 *
 * @author Aleksander Dziągwa
 * @date 12.11.2026
 */

#include "stm32f4xx.h"
#include "GPIO_pins_def.h"
#include "drivers/buttons_operations.h"


/** @brief Configure SYSCFG EXTI lines, Rising/Falling interrupts, and TIM6 25 ms timer */
void configure_buttons(void)
{
    /** Enable APB2 clock for SYSCFG. */
    SET_BIT(RCC->APB2ENR, RCC_APB2ENR_SYSCFGEN);
    (void)RCC->APB2ENR;

    /** Route PB0 (EXTI0), PB1 (EXTI1), and PB2 (EXTI2) to Port B in EXTICR[0]. */
    MODIFY_REG(SYSCFG->EXTICR[0],
               SYSCFG_EXTICR1_EXTI0 | SYSCFG_EXTICR1_EXTI1 | SYSCFG_EXTICR1_EXTI2,
               SYSCFG_EXTICR1_EXTI0_PB | SYSCFG_EXTICR1_EXTI1_PB | SYSCFG_EXTICR1_EXTI2_PB);

    /** Route PB10 (EXTI10) to Port B in EXTICR[2]. */
    MODIFY_REG(SYSCFG->EXTICR[2],
               SYSCFG_EXTICR3_EXTI10,
               SYSCFG_EXTICR3_EXTI10_PB);

    /** Configure EXTI lines for both Rising (press) and Falling (release) edges. */
    SET_BIT(EXTI->RTSR, ALL_BUTTONS_MASK);
    SET_BIT(EXTI->FTSR, ALL_BUTTONS_MASK);

    /** Clear any pending flags and unmask button EXTI lines. */
    EXTI->PR = ALL_BUTTONS_MASK;
    SET_BIT(EXTI->IMR, ALL_BUTTONS_MASK);

    /** Configure NVIC priorities and enable EXTI IRQs. */
    NVIC_SetPriority(EXTI0_IRQn, 6U);
    NVIC_SetPriority(EXTI1_IRQn, 6U);
    NVIC_SetPriority(EXTI2_IRQn, 6U);
    NVIC_SetPriority(EXTI15_10_IRQn, 6U);

    NVIC_EnableIRQ(EXTI0_IRQn);
    NVIC_EnableIRQ(EXTI1_IRQn);
    NVIC_EnableIRQ(EXTI2_IRQn);
    NVIC_EnableIRQ(EXTI15_10_IRQn);
}
