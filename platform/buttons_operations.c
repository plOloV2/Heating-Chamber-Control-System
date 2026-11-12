/**
 * @file button_operations.c
 * @brief Implementation of button EXTI and TIM6 ISRs to track current button state.
 *
 * @author Aleksander Dziągwa
 * @date 12.11.2026
 */

#include "stm32f4xx.h"
#include "GPIO_pins_def.h"
#include "drivers/buttons_operations.h"

volatile button_event_t ButtonState = { .raw = 0U };
volatile bool ButtonStateChanged = false;


/** @brief Mask button EXTI lines and start TIM6 25 ms evaluation window */
static inline void start_button_window(void)
{
    CLEAR_BIT(EXTI->IMR, ALL_BUTTONS_MASK);

    TIM6->CNT = 0U;
    SET_BIT(TIM6->CR1, TIM_CR1_CEN);
}


void EXTI0_IRQHandler(void)
{
    if(READ_BIT(EXTI->PR, BUT1_MASK) != 0U)
    {
        EXTI->PR = BUT1_MASK;
        start_button_window();
    }

}


void EXTI1_IRQHandler(void)
{
    if(READ_BIT(EXTI->PR, BUT2_MASK) != 0U)
    {
        EXTI->PR = BUT2_MASK;
        start_button_window();
    }

}


void EXTI2_IRQHandler(void)
{
    if(READ_BIT(EXTI->PR, BUT3_MASK) != 0U)
    {
        EXTI->PR = BUT3_MASK;
        start_button_window();
    }

}


void EXTI15_10_IRQHandler(void)
{
    if(READ_BIT(EXTI->PR, BUT4_MASK) != 0U)
    {
        EXTI->PR = BUT4_MASK;
        start_button_window();
    }

}


void TIM6_DAC_IRQHandler(void)
{
    if(READ_BIT(TIM6->SR, TIM_SR_UIF) != 0U)
    {
        CLEAR_BIT(TIM6->SR, TIM_SR_UIF);

        /* Read GPIOB input register once */
        const uint32_t idr = GPIOB->IDR;

        /* Build new button state snapshot */
        button_event_t new_state = { .raw = 0U };
        new_state.pressed.but1 = ((idr & BUT1_MASK) != 0U) ? 1U : 0U;
        new_state.pressed.but2 = ((idr & BUT2_MASK) != 0U) ? 1U : 0U;
        new_state.pressed.but3 = ((idr & BUT3_MASK) != 0U) ? 1U : 0U;
        new_state.pressed.but4 = ((idr & BUT4_MASK) != 0U) ? 1U : 0U;

        if(new_state.raw != ButtonState.raw)
        {
            ButtonState.raw = new_state.raw;
            ButtonStateChanged = true;
        }

        /* Clear any pending edges accumulated during the 25 ms window */
        EXTI->PR = ALL_BUTTONS_MASK;

        /* Re-enable button EXTI interrupts */
        SET_BIT(EXTI->IMR, ALL_BUTTONS_MASK);
    }

}
