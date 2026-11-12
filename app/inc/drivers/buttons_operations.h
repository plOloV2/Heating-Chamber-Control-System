#ifndef BUTTON_OPERATIONS_H
#define BUTTON_OPERATIONS_H

/**
 * @file button_operations.h
 * @brief Definitions and state structures for button inputs.
 *
 * @author Aleksander Dziągwa
 * @date 12.11.2026
 */

#include <stdint.h>
#include "GPIO_pins_def.h"

/** @brief Hardware pin bitmasks on GPIOB */
#define BUT1_MASK               (1UL << BUT1_PIN)
#define BUT2_MASK               (1UL << BUT2_PIN)
#define BUT3_MASK               (1UL << BUT3_PIN)
#define BUT4_MASK               (1UL << BUT4_PIN)
#define ALL_BUTTONS_MASK        (BUT1_MASK | BUT2_MASK | BUT3_MASK | BUT4_MASK)

/** @brief Structure holding current state of all buttons (1 = pressed, 0 = not pressed). */
typedef union {
    struct {
        uint8_t but1     : 1;   /**< Bit 0: BUT1 state (PB0) */
        uint8_t but2     : 1;   /**< Bit 1: BUT2 state (PB1) */
        uint8_t but3     : 1;   /**< Bit 2: BUT3 state (PB2) */
        uint8_t but4     : 1;   /**< Bit 3: BUT4 state (PB10 mapped to bit 3) */
        uint8_t reserved : 4;   /**< Bits 4..7: Unused padding */
    } pressed;
    uint8_t raw;                /**< Combined bitmask (0x00 .. 0x0F) */
} button_event_t;

/** @brief Global variable holding the current debounced state of the buttons */
extern volatile button_event_t ButtonState;

/** @brief Flag set to true whenever a new button state is latched by TIM6 */
extern volatile bool ButtonStateChanged;

/** @brief Initialize button EXTI lines and TIM6 25 ms timer */
void configure_buttons(void);

#endif /* BUTTON_OPERATIONS_H */
