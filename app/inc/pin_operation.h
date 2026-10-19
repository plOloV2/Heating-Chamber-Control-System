#ifndef PIN_OPERATION_H
#define PIN_OPERATION_H

#include "stm32f405xx.h"

void toggle_pin(GPIO_TypeDef *PinPort, uint32_t Pin);
void set_pin(GPIO_TypeDef *PinPort, uint32_t Pin, uint32_t Value);

#endif
