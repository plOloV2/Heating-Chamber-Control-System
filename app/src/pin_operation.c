#include "stm32f405xx.h"


void toggle_pin(GPIO_TypeDef *PinPort, uint32_t Pin){

    uint32_t odr = PinPort->ODR;
    PinPort->BSRR = ((odr & (1U << Pin)) != 0U) ? (1U << (Pin + 16U)) : (1U << Pin);

}


void set_pin(GPIO_TypeDef *PinPort, uint32_t Pin, uint32_t Value){

    if(Value) {
        // Lower 16 bits set the pin HIGH
        PinPort->BSRR = (1U << Pin); 
    }else {
        // Upper 16 bits clear the pin LOW
        PinPort->BSRR = (1U << (Pin + 16U)); 
    }

}
