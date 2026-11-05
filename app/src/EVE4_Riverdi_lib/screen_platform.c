/**
 * @file screen_platform.c
 * @brief Low-level platform implementation for Riverdi EVE4 (BT817) library.
 *
 * @details
 * Implements hardware interface routines required by EVE HAL layer,
 * utilizing drivers/SPI_operations.h, drivers/pin_operations.h,
 * AF_pins_def.h, and GPIO_pins_def.h without direct register manipulation.
 *
 * @author Aleksander Dziągwa
 * @date 01.11.2026
 */

#include "EVE4_Riverdi_lib/screen_platform.h"
#include "AF_pins_def.h"
#include "GPIO_pins_def.h"
#include "drivers/pin_operations.h"
#include "drivers/SPI_operations.h"

#include "FreeRTOS.h"
#include "task.h"

extern uint32_t SystemCoreClock;

/**
 * @brief Platform-level initialization.
 * Clocks and MCU core are already initialized in SystemInit().
 */
bool_t platform_init(Gpu_HalInit_t *halinit)
{
    (void)halinit;
    return TRUE;
}

/**
 * @brief Millisecond sleep/delay function.
 * Uses FreeRTOS vTaskDelay
 */
void platform_sleep_ms(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS((ms > 0U) ? ms : 1U));
}

/**
 * @brief Initialize SPI hardware interface for EVE communication.
 */
bool_t platform_spi_init(Gpu_Hal_Context_t *host)
{
    (void)host;
    return spi_hw_init() ? TRUE : FALSE;
}

/**
 * @brief Deinitialize SPI interface.
 */
void platform_spi_deinit(Gpu_Hal_Context_t *host)
{
    (void)host;
    spi_hw_deinit();
}

/**
 * @brief Transmit and receive a single byte over SPI.
 */
uchar8_t platform_spi_send_recv_byte(Gpu_Hal_Context_t *host,
                                     uchar8_t           data,
                                     uint32_t           opt)
{
    (void)host;
    (void)opt;
    return (uchar8_t)spi_transmit_receive_byte((uint8_t)data);
}

/**
 * @brief Transmit a buffer of data over SPI.
 */
uint16_t platform_spi_send_data(Gpu_Hal_Context_t *host,
                                uchar8_t          *data,
                                uint16_t           size,
                                uint32_t           opt)
{
    (void)host;
    (void)opt;
    return spi_transmit((const uint8_t *)data, size);
}

/**
 * @brief Receive a buffer of data over SPI.
 */
void platform_spi_recv_data(Gpu_Hal_Context_t *host,
                            uchar8_t          *data,
                            uint16_t           size,
                            uint32_t           opt)
{
    (void)host;
    (void)opt;
    (void)spi_receive((uint8_t *)data, size);
}

/**
 * @brief Initialize a GPIO pin for EVE display.
 */
bool_t platform_gpio_init(Gpu_Hal_Context_t *host,
                          gpio_name          ngpio)
{
    (void)host;
    switch (ngpio)
    {
        case GPIO_CS:
            /* Chip select: initially inactive (HIGH) */
            SetPin(SPI_CS_PORT, SPI_CS_PIN, (1U - SPI_CS_ACTIVE));
            break;

        case GPIO_PD:
            /* Reset / Power-down: active low, initially set HIGH (inactive) */
            SetPin(LCD_RST_PORT, LCD_RST_PIN, (1U - LCD_RST_ACTIVE));
            break;

        case GPIO_INT:
            /* LCD Interrupt is an input */
            break;

        default:
            return FALSE;
    }

    return TRUE;
}

/**
 * @brief Set the state of a GPIO pin.
 */
bool_t platform_gpio_value(Gpu_Hal_Context_t *host,
                           gpio_name          ngpio,
                           gpio_val           vgpio)
{
    (void)host;
    const uint32_t val = (vgpio == GPIO_HIGH) ? 1U : 0U;

    switch (ngpio)
    {
        case GPIO_CS:
            SetPin(SPI_CS_PORT, SPI_CS_PIN, val);
            break;

        case GPIO_PD:
            SetPin(LCD_RST_PORT, LCD_RST_PIN, val);
            break;

        case GPIO_INT:
            /* LCD Interrupt is an input */
            break;

        default:
            return FALSE;
    }

    return TRUE;
}
