#include "stm32f4xx.h"
#include "pins_def.h"
#include "./system_init_error.h"



#define GPIO_BSRR_INACTIVE(pin, active_state) (((active_state) != 0U) ? (1UL << ((pin) + 16U)) : (1UL << (pin)))

#define GPIO_FIELD_MASK(pin)        (3UL << ((pin) * 2U))
#define GPIO_FIELD_VAL(pin, val)    ((uint32_t)(val) << ((pin) * 2U))


#define GPIO_MODE_INPUT             0U
#define GPIO_MODE_OUTPUT            1U
#define GPIO_MODE_AF                2U
#define GPIO_MODE_ANALOG            3U


system_init_status_t configure_gpio(void)
{

    /** Enables AHB clock for GPIOA, GPIOB and GPIOC. */
    SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN);
    (void)RCC->AHB1ENR;

    /** Set pins to inactive state. */
    GPIOA->BSRR = GPIO_BSRR_INACTIVE(FAN_PIN,  FAN_ACTIVE)  |
                  GPIO_BSRR_INACTIVE(HEAT_PIN, HEAT_ACTIVE) |
                  GPIO_BSRR_INACTIVE(BLS_PIN,  BLS_ACTIVE)  |
                  (1UL << (BUZZ_PIN + 16U)); /** Force Buzzer LOW on boot */

    GPIOC->BSRR = GPIO_BSRR_INACTIVE(LED1_PIN, LED1_ACTIVE) |
                  GPIO_BSRR_INACTIVE(LED2_PIN, LED2_ACTIVE);


    /** Configure Output Type as Push-Pull (0) for all outputs. */
    CLEAR_BIT(GPIOA->OTYPER, (1UL << FAN_PIN) | (1UL << HEAT_PIN) | (1UL << BUZZ_PIN) | (1UL << BLS_PIN));
    CLEAR_BIT(GPIOC->OTYPER, (1UL << LED1_PIN) | (1UL << LED2_PIN));


    /** Set all pull-up/down resistors to off. */
    MODIFY_REG(GPIOA->PUPDR,
               GPIO_FIELD_MASK(FAN_PIN) | GPIO_FIELD_MASK(HEAT_PIN) |
               GPIO_FIELD_MASK(BUZZ_PIN) | GPIO_FIELD_MASK(BLS_PIN),
               0U);

    MODIFY_REG(GPIOC->PUPDR,
               GPIO_FIELD_MASK(LED1_PIN) | GPIO_FIELD_MASK(LED2_PIN),
               0U);


    /** Configure modes 
     * GPIOA:
     * - FAN, HEAT, BUZZ, BLS -> General Purpose Output (01)
     * - BUZZ is kept as a LOW output until reconfigured later
     * - TEMP1..TEMP4 -> Analog Mode (11)
     */
    MODIFY_REG(GPIOA->MODER,
               GPIO_FIELD_MASK(FAN_PIN)   | GPIO_FIELD_MASK(HEAT_PIN)  |
               GPIO_FIELD_MASK(BUZZ_PIN)  | GPIO_FIELD_MASK(BLS_PIN)   |
               GPIO_FIELD_MASK(TEMP1_PIN) | GPIO_FIELD_MASK(TEMP2_PIN) |
               GPIO_FIELD_MASK(TEMP3_PIN) | GPIO_FIELD_MASK(TEMP4_PIN),
               GPIO_FIELD_VAL(FAN_PIN,   GPIO_MODE_OUTPUT) |
               GPIO_FIELD_VAL(HEAT_PIN,  GPIO_MODE_OUTPUT) |
               GPIO_FIELD_VAL(BUZZ_PIN,  GPIO_MODE_OUTPUT) |
               GPIO_FIELD_VAL(BLS_PIN,   GPIO_MODE_OUTPUT) |
               GPIO_FIELD_VAL(TEMP1_PIN, GPIO_MODE_ANALOG) |
               GPIO_FIELD_VAL(TEMP2_PIN, GPIO_MODE_ANALOG) |
               GPIO_FIELD_VAL(TEMP3_PIN, GPIO_MODE_ANALOG) |
               GPIO_FIELD_VAL(TEMP4_PIN, GPIO_MODE_ANALOG));

    /** GPIOB: BUT1..BUT4 -> Digital Inputs (00) */
    MODIFY_REG(GPIOB->MODER,
               GPIO_FIELD_MASK(BUT1_PIN) | GPIO_FIELD_MASK(BUT2_PIN) |
               GPIO_FIELD_MASK(BUT3_PIN) | GPIO_FIELD_MASK(BUT4_PIN),
               0U);

    /** GPIOC: LED1, LED2 -> General Purpose Output (01) */
    MODIFY_REG(GPIOC->MODER,
               GPIO_FIELD_MASK(LED1_PIN) | GPIO_FIELD_MASK(LED2_PIN),
               GPIO_FIELD_VAL(LED1_PIN, GPIO_MODE_OUTPUT) |
               GPIO_FIELD_VAL(LED2_PIN, GPIO_MODE_OUTPUT));

    

    return SYSTEM_INIT_STATUS_OK;

}
