/**
 * @file pin_init.c
 * @brief Implementation of GPIO pins initialization.
 *
 * @details
 * This file holds implementation of functions used to configure GPIO pins.
 *
 * @author Aleksander Dziągwa
 * @date 20.10.2026
 */

#include "stm32f4xx.h"
#include "AF_pins_def.h"
#include "GPIO_pins_def.h"
#include "system_state/system_init_error.h"


/** @brief Helper macro for BSR register manipulation */
#define GPIO_BSRR_INACTIVE(pin, active_state) (((active_state) != 0U) ? (1UL << ((pin) + 16U)) : (1UL << (pin)))

/** @brief Helper macros for register manipulation (2-bit fields: MODER, OSPEEDR, PUPDR) */
#define GPIO_FIELD_MASK(pin)        (3UL << ((pin) * 2U))
#define GPIO_FIELD_VAL(pin, val)    ((uint32_t)(val) << ((pin) * 2U))

/** @brief Helper macros for AFR manipulation (4-bit fields) */
#define GPIO_AFR_MASK(pin)          (0xFUL << (((pin) % 8U) * 4U))
#define GPIO_AFR_VAL(pin, af)       ((uint32_t)(af) << (((pin) % 8U) * 4U))

/** @brief GPIO pins modes */
#define GPIO_MODE_INPUT             0U
#define GPIO_MODE_OUTPUT            1U
#define GPIO_MODE_AF                2U
#define GPIO_MODE_ANALOG            3U

/** @brief GPIO pull-up/pull-down */
#define GPIO_PUPD_NONE              0U
#define GPIO_PUPD_PULLUP            1U
#define GPIO_PUPD_PULLDOWN          2U

/** @brief GPIO output speed */
#define GPIO_SPEED_LOW              0U /** Up to 8MHz */
#define GPIO_SPEED_MEDIUM           1U /** Up to 50MHz */
#define GPIO_SPEED_FAST             2U /** Up to 100MHz */
#define GPIO_SPEED_HIGH             3U /** Up to 180MHz */


/** @brief Configure AHB clock and GPIO registers */
void configure_gpio(void)
{
    /** Enables AHB clock for GPIOA, GPIOB, GPIOC, and GPIOD. */
    SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_GPIODEN);
    (void)RCC->AHB1ENR;

    /** Set output pins to initial inactive state. */
    GPIOA->BSRR = GPIO_BSRR_INACTIVE(FAN_PIN,  FAN_ACTIVE)  |
                  GPIO_BSRR_INACTIVE(HEAT_PIN, HEAT_ACTIVE) |
                  GPIO_BSRR_INACTIVE(BLS_PIN,  BLS_ACTIVE)  |
                  (1UL << (BUZZ_PIN + 16U)); /** Force Buzzer LOW on boot */

    GPIOC->BSRR = GPIO_BSRR_INACTIVE(LED1_PIN,    LED1_ACTIVE)    |
                  GPIO_BSRR_INACTIVE(LED2_PIN,    LED2_ACTIVE)    |
                  GPIO_BSRR_INACTIVE(LCD_RST_PIN, LCD_RST_ACTIVE); /** Active low, set HIGH (inactive) */


    /** Configure Output Type as Push-Pull (0) for all outputs and AF pins. */
    CLEAR_BIT(GPIOA->OTYPER, (1UL << FAN_PIN) | (1UL << HEAT_PIN) | (1UL << BUZZ_PIN) | (1UL << BLS_PIN));
    CLEAR_BIT(GPIOB->OTYPER, (1UL << SPI_CS_PIN) | (1UL << SPI_SCLK_PIN) | (1UL << SPI_MISO_PIN) | (1UL << SPI_MOSI_PIN));
    CLEAR_BIT(GPIOC->OTYPER, (1UL << LED1_PIN) | (1UL << LED2_PIN) | (1UL << LCD_RST_PIN) |
                             (1UL << SDIO_D0_PIN) | (1UL << SDIO_D1_PIN) | (1UL << SDIO_D2_PIN) |
                             (1UL << SDIO_D3_PIN) | (1UL << SDIO_CK_PIN));
    CLEAR_BIT(GPIOD->OTYPER, (1UL << SDIO_CMD_PIN));


    /** Configure I/O output speed */
    /** Buzzer set to medium */
    MODIFY_REG(GPIOA->OSPEEDR,
               GPIO_FIELD_MASK(BUZZ_PIN),
               GPIO_FIELD_VAL(BUZZ_PIN, GPIO_SPEED_MEDIUM));

    MODIFY_REG(GPIOB->OSPEEDR,
               GPIO_FIELD_MASK(SPI_CS_PIN) | GPIO_FIELD_MASK(SPI_SCLK_PIN) |
               GPIO_FIELD_MASK(SPI_MISO_PIN) | GPIO_FIELD_MASK(SPI_MOSI_PIN),
               GPIO_FIELD_VAL(SPI_CS_PIN,   GPIO_SPEED_HIGH) |
               GPIO_FIELD_VAL(SPI_SCLK_PIN, GPIO_SPEED_HIGH) |
               GPIO_FIELD_VAL(SPI_MISO_PIN, GPIO_SPEED_HIGH) |
               GPIO_FIELD_VAL(SPI_MOSI_PIN, GPIO_SPEED_HIGH));

    MODIFY_REG(GPIOC->OSPEEDR,
               GPIO_FIELD_MASK(SDIO_D0_PIN) | GPIO_FIELD_MASK(SDIO_D1_PIN) |
               GPIO_FIELD_MASK(SDIO_D2_PIN) | GPIO_FIELD_MASK(SDIO_D3_PIN) |
               GPIO_FIELD_MASK(SDIO_CK_PIN),
               GPIO_FIELD_VAL(SDIO_D0_PIN, GPIO_SPEED_HIGH) |
               GPIO_FIELD_VAL(SDIO_D1_PIN, GPIO_SPEED_HIGH) |
               GPIO_FIELD_VAL(SDIO_D2_PIN, GPIO_SPEED_HIGH) |
               GPIO_FIELD_VAL(SDIO_D3_PIN, GPIO_SPEED_HIGH) |
               GPIO_FIELD_VAL(SDIO_CK_PIN, GPIO_SPEED_HIGH));

    MODIFY_REG(GPIOD->OSPEEDR,
               GPIO_FIELD_MASK(SDIO_CMD_PIN),
               GPIO_FIELD_VAL(SDIO_CMD_PIN, GPIO_SPEED_HIGH));


    /** Configure pull-up / pull-down resistors */
    MODIFY_REG(GPIOA->PUPDR,
               GPIO_FIELD_MASK(FAN_PIN) | GPIO_FIELD_MASK(HEAT_PIN) |
               GPIO_FIELD_MASK(BUZZ_PIN) | GPIO_FIELD_MASK(BLS_PIN),
               GPIO_PUPD_NONE);

    MODIFY_REG(GPIOB->PUPDR,
               GPIO_FIELD_MASK(BUT1_PIN) | GPIO_FIELD_MASK(BUT2_PIN) |
               GPIO_FIELD_MASK(BUT3_PIN) | GPIO_FIELD_MASK(BUT4_PIN) |
               GPIO_FIELD_MASK(SPI_CS_PIN) | GPIO_FIELD_MASK(SPI_SCLK_PIN) |
               GPIO_FIELD_MASK(SPI_MISO_PIN) | GPIO_FIELD_MASK(SPI_MOSI_PIN),
               GPIO_FIELD_VAL(BUT1_PIN,     GPIO_PUPD_NONE)   |
               GPIO_FIELD_VAL(BUT2_PIN,     GPIO_PUPD_NONE)   |
               GPIO_FIELD_VAL(BUT3_PIN,     GPIO_PUPD_NONE)   |
               GPIO_FIELD_VAL(BUT4_PIN,     GPIO_PUPD_NONE)   |
               GPIO_FIELD_VAL(SPI_CS_PIN,   GPIO_PUPD_NONE)   |
               GPIO_FIELD_VAL(SPI_SCLK_PIN, GPIO_PUPD_NONE)   |
               GPIO_FIELD_VAL(SPI_MISO_PIN, GPIO_PUPD_PULLUP) | /** Only MISO is pulled-up */
               GPIO_FIELD_VAL(SPI_MOSI_PIN, GPIO_PUPD_NONE));

    MODIFY_REG(GPIOC->PUPDR,
               GPIO_FIELD_MASK(LED1_PIN) | GPIO_FIELD_MASK(LED2_PIN) |
               GPIO_FIELD_MASK(LCD_INT_PIN) | GPIO_FIELD_MASK(LCD_RST_PIN) |
               GPIO_FIELD_MASK(SDIO_D0_PIN) | GPIO_FIELD_MASK(SDIO_D1_PIN) |
               GPIO_FIELD_MASK(SDIO_D2_PIN) | GPIO_FIELD_MASK(SDIO_D3_PIN) |
               GPIO_FIELD_MASK(SDIO_CK_PIN),
               GPIO_FIELD_VAL(LED1_PIN,    GPIO_PUPD_NONE) |
               GPIO_FIELD_VAL(LED2_PIN,    GPIO_PUPD_NONE) |
               GPIO_FIELD_VAL(LCD_INT_PIN, GPIO_PUPD_NONE) |    /** LCD has internal pull-up */
               GPIO_FIELD_VAL(LCD_RST_PIN, GPIO_PUPD_NONE) |    /** LCD has internal pull-down */
               GPIO_FIELD_VAL(SDIO_D0_PIN, GPIO_PUPD_NONE) |    /** Pull-up on pcb */
               GPIO_FIELD_VAL(SDIO_D1_PIN, GPIO_PUPD_NONE) |    /** Pull-up on pcb */
               GPIO_FIELD_VAL(SDIO_D2_PIN, GPIO_PUPD_NONE) |    /** Pull-up on pcb */
               GPIO_FIELD_VAL(SDIO_D3_PIN, GPIO_PUPD_NONE));    /** Pull-up on pcb */

    MODIFY_REG(GPIOD->PUPDR,
               GPIO_FIELD_MASK(SDIO_CMD_PIN),
               GPIO_FIELD_VAL(SDIO_CMD_PIN, GPIO_PUPD_NONE));   /** Pull-up on pcb */


    /** Configure Alternate Function Mapping */
    /* GPIOA: BUZZ (PA2) -> AF1 (TIM2_CH3) */
    MODIFY_REG(GPIOA->AFR[0],
               GPIO_AFR_MASK(BUZZ_PIN),
               GPIO_AFR_VAL(BUZZ_PIN, BUZZ_AF));

    /* GPIOB: SPI2 pins (PB12..PB15) -> AF5 */
    MODIFY_REG(GPIOB->AFR[1],
               GPIO_AFR_MASK(SPI_CS_PIN) | GPIO_AFR_MASK(SPI_SCLK_PIN) |
               GPIO_AFR_MASK(SPI_MISO_PIN) | GPIO_AFR_MASK(SPI_MOSI_PIN),
               GPIO_AFR_VAL(SPI_CS_PIN, SPI_CS_AF)     |
               GPIO_AFR_VAL(SPI_SCLK_PIN, SPI_SCLK_AF) |
               GPIO_AFR_VAL(SPI_MISO_PIN, SPI_MISO_AF) |
               GPIO_AFR_VAL(SPI_MOSI_PIN, SPI_MOSI_AF));

    /* GPIOC: SDIO data/clock pins (PC8..PC12) -> AF12 */
    MODIFY_REG(GPIOC->AFR[1],
               GPIO_AFR_MASK(SDIO_D0_PIN) | GPIO_AFR_MASK(SDIO_D1_PIN) |
               GPIO_AFR_MASK(SDIO_D2_PIN) | GPIO_AFR_MASK(SDIO_D3_PIN) |
               GPIO_AFR_MASK(SDIO_CK_PIN),
               GPIO_AFR_VAL(SDIO_D0_PIN, SDIO_D0_AF) |
               GPIO_AFR_VAL(SDIO_D1_PIN, SDIO_D1_AF) |
               GPIO_AFR_VAL(SDIO_D2_PIN, SDIO_D2_AF) |
               GPIO_AFR_VAL(SDIO_D3_PIN, SDIO_D3_AF) |
               GPIO_AFR_VAL(SDIO_CK_PIN, SDIO_CK_AF));

    /* GPIOD: SDIO command pin (PD2) -> AF12 */
    MODIFY_REG(GPIOD->AFR[0],
               GPIO_AFR_MASK(SDIO_CMD_PIN),
               GPIO_AFR_VAL(SDIO_CMD_PIN, SDIO_CMD_AF));


    /** Configure pin modes
     * GPIOA:
     * - FAN, HEAT, BLS -> General Purpose Output (01)
     * - BUZZ -> Alternate Function (10)
     * - TEMP1..TEMP4 -> Analog Mode (11)
     */
    MODIFY_REG(GPIOA->MODER,
               GPIO_FIELD_MASK(FAN_PIN)   | GPIO_FIELD_MASK(HEAT_PIN)  |
               GPIO_FIELD_MASK(BUZZ_PIN)  | GPIO_FIELD_MASK(BLS_PIN)   |
               GPIO_FIELD_MASK(TEMP1_PIN) | GPIO_FIELD_MASK(TEMP2_PIN) |
               GPIO_FIELD_MASK(TEMP3_PIN) | GPIO_FIELD_MASK(TEMP4_PIN),
               GPIO_FIELD_VAL(FAN_PIN,   GPIO_MODE_OUTPUT) |
               GPIO_FIELD_VAL(HEAT_PIN,  GPIO_MODE_OUTPUT) |
               GPIO_FIELD_VAL(BUZZ_PIN,  GPIO_MODE_AF)     |
               GPIO_FIELD_VAL(BLS_PIN,   GPIO_MODE_OUTPUT) |
               GPIO_FIELD_VAL(TEMP1_PIN, GPIO_MODE_ANALOG) |
               GPIO_FIELD_VAL(TEMP2_PIN, GPIO_MODE_ANALOG) |
               GPIO_FIELD_VAL(TEMP3_PIN, GPIO_MODE_ANALOG) |
               GPIO_FIELD_VAL(TEMP4_PIN, GPIO_MODE_ANALOG));

    /** GPIOB:
     * - BUT1..BUT4 -> Digital Inputs (00)
     * - SPI_CS, SPI_SCLK, SPI_MISO, SPI_MOSI -> Alternate Function (10)
     */
    MODIFY_REG(GPIOB->MODER,
               GPIO_FIELD_MASK(BUT1_PIN)     | GPIO_FIELD_MASK(BUT2_PIN)     |
               GPIO_FIELD_MASK(BUT3_PIN)     | GPIO_FIELD_MASK(BUT4_PIN)     |
               GPIO_FIELD_MASK(SPI_CS_PIN)   | GPIO_FIELD_MASK(SPI_SCLK_PIN) |
               GPIO_FIELD_MASK(SPI_MISO_PIN) | GPIO_FIELD_MASK(SPI_MOSI_PIN),
               GPIO_FIELD_VAL(BUT1_PIN,     GPIO_MODE_INPUT) |
               GPIO_FIELD_VAL(BUT2_PIN,     GPIO_MODE_INPUT) |
               GPIO_FIELD_VAL(BUT3_PIN,     GPIO_MODE_INPUT) |
               GPIO_FIELD_VAL(BUT4_PIN,     GPIO_MODE_INPUT) |
               GPIO_FIELD_VAL(SPI_CS_PIN,   GPIO_MODE_AF)    |
               GPIO_FIELD_VAL(SPI_SCLK_PIN, GPIO_MODE_AF)    |
               GPIO_FIELD_VAL(SPI_MISO_PIN, GPIO_MODE_AF)    |
               GPIO_FIELD_VAL(SPI_MOSI_PIN, GPIO_MODE_AF));

    /** GPIOC:
     * - LED1, LED2, LCD_RST -> General Purpose Output (01)
     * - LCD_INT -> Digital Input (00)
     * - SDIO_D0..D3, SDIO_CK -> Alternate Function (10)
     */
    MODIFY_REG(GPIOC->MODER,
               GPIO_FIELD_MASK(LED1_PIN)    | GPIO_FIELD_MASK(LED2_PIN)    |
               GPIO_FIELD_MASK(LCD_INT_PIN) | GPIO_FIELD_MASK(LCD_RST_PIN) |
               GPIO_FIELD_MASK(SDIO_D0_PIN) | GPIO_FIELD_MASK(SDIO_D1_PIN) |
               GPIO_FIELD_MASK(SDIO_D2_PIN) | GPIO_FIELD_MASK(SDIO_D3_PIN) |
               GPIO_FIELD_MASK(SDIO_CK_PIN),
               GPIO_FIELD_VAL(LED1_PIN,    GPIO_MODE_OUTPUT) |
               GPIO_FIELD_VAL(LED2_PIN,    GPIO_MODE_OUTPUT) |
               GPIO_FIELD_VAL(LCD_RST_PIN, GPIO_MODE_OUTPUT) |
               GPIO_FIELD_VAL(LCD_INT_PIN, GPIO_MODE_INPUT)  |
               GPIO_FIELD_VAL(SDIO_D0_PIN, GPIO_MODE_AF)     |
               GPIO_FIELD_VAL(SDIO_D1_PIN, GPIO_MODE_AF)     |
               GPIO_FIELD_VAL(SDIO_D2_PIN, GPIO_MODE_AF)     |
               GPIO_FIELD_VAL(SDIO_D3_PIN, GPIO_MODE_AF)     |
               GPIO_FIELD_VAL(SDIO_CK_PIN, GPIO_MODE_AF));

    /** GPIOD:
     * - SDIO_CMD -> Alternate Function (10)
     */
    MODIFY_REG(GPIOD->MODER,
               GPIO_FIELD_MASK(SDIO_CMD_PIN),
               GPIO_FIELD_VAL(SDIO_CMD_PIN, GPIO_MODE_AF));
}
