#ifndef PIN_OPERATION_H
#define PIN_OPERATION_H

/**
 * @file pin_operation.h
 * @brief Definition of functions to change GPIO pins state.
 *
 * @details
 * This file provides definitions of functions to toggle and set GPIO pins state.
 * Should be used with definitions provided by pins_def.h
 *
 * @author Aleksander Dziągwa
 * @date 20.10.2026
 */

#include "stm32f405xx.h"
#include "pins_def.h"

void TogglePin(GPIO_TypeDef *PinPort, uint32_t Pin);
void SetPin(GPIO_TypeDef *PinPort, uint32_t Pin, uint32_t Value);

#endif
