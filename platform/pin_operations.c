/**
 * @file pin_operations.c
 * @brief Implememtation of functions to change GPIO pins state.
 *
 * @details This file provides implememtation of functions to toggle and set GPIO pins state.
 *          Should be used with definitions provided by pins_def.h
 *
 * @author Aleksander Dziągwa
 * @date 20.10.2026
 */


#include "stm32f405xx.h"


/** 
 * @brief Atomic pin toggle.
 * 
 * @param PinPort   GPIO port of desired pin
 * @param Pin       pin number in GPIO port
 */
void TogglePin(GPIO_TypeDef *PinPort, uint32_t Pin){

    uint32_t odr = PinPort->ODR;
    PinPort->BSRR = ((odr & (1U << Pin)) != 0U) ? (1U << (Pin + 16U)) : (1U << Pin);

}


/** 
 * @brief Atomic pin set.
 * 
 * @param PinPort   GPIO port of desired pin
 * @param Pin       pin number in GPIO port
 * @param Value     value to set pin to
 */
void SetPin(GPIO_TypeDef *PinPort, uint32_t Pin, uint32_t Value){

    if(Value) {
        // Lower 16 bits set the pin HIGH
        PinPort->BSRR = (1U << Pin); 
    }else {
        // Upper 16 bits clear the pin LOW
        PinPort->BSRR = (1U << (Pin + 16U)); 
    }

}
