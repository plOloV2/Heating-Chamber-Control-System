#ifndef AF_PINS_DEF_H
#define AF_PINS_DEF_H

/**
 * @file AF_pins_def.h
 * @brief Definition of STM32 AF pins.
 *
 * @details
 * This file provides definitions of ports, pins, and Alternate Functions for all AF pins.
 *
 * @author Aleksander Dziągwa
 * @date 22.10.2026
 */


/**
 * @name BUZZER Net
 * @brief Buzzer PWM control output.
 * 
 * @details Uses TIM2 CHANNEL 3 PWM signal.
 * @{
 */
#define BUZZ_PORT       GPIOA
#define BUZZ_PIN        2U
#define BUZZ_AF         1U
#define BUZZ_EXTI       2U
/** @} */


/**
 * @name USART_CK Net
 * @brief USART1 Clock signal.
 * @{
 */
#define USART_CK_PORT       GPIOA
#define USART_CK_PIN        8U
#define USART_CK_AF         7U
#define USART_CK_EXTI       8U
/** @} */


/**
 * @name USART_TX Net
 * @brief USART1 TX line.
 * @{
 */
#define USART_TX_PORT       GPIOA
#define USART_TX_PIN        9U
#define USART_TX_AF         7U
#define USART_TX_EXTI       9U
/** @} */


/**
 * @name USART_RX Net
 * @brief USART1 RX line.
 * @{
 */
#define USART_RX_PORT       GPIOA
#define USART_RX_PIN        10U
#define USART_RX_AF         7U
#define USART_RX_EXTI       10U
/** @} */


/**
 * @name USART_CTS Net
 * @brief USART1 CTS line.
 * @{
 */
#define USART_CTS_PORT      GPIOA
#define USART_CTS_PIN       11U
#define USART_CTS_AF        7U
#define USART_CTS_EXTI      11U
/** @} */


/**
 * @name USART_RTS Net
 * @brief USART1 RTS line.
 * @{
 */
#define USART_RTS_PORT      GPIOA
#define USART_RTS_PIN       12U
#define USART_RTS_AF        7U
#define USART_RTS_EXTI      12U
/** @} */


/**
 * @name I2C_SCL Net
 * @brief I2C1 SCL line.
 * @{
 */
#define I2C_SCL_PORT      GPIOB
#define I2C_SCL_PIN       6U
#define I2C_SCL_AF        4U
#define I2C_SCL_EXTI      6U
/** @} */


/**
 * @name I2C_SDA Net
 * @brief I2C1 SDA line.
 * @{
 */
#define I2C_SDA_PORT      GPIOB
#define I2C_SDA_PIN       7U
#define I2C_SDA_AF        4U
#define I2C_SDA_EXTI      7U
/** @} */


/**
 * @name LCD_SPI_CS Net
 * @brief SPI2 CS line for LCD screen.
 * @{
 */
#define SPI_CS_PORT      GPIOB
#define SPI_CS_PIN       12U
#define SPI_CS_AF        5U
#define SPI_CS_EXTI      12U
/** @} */


/**
 * @name LCD_SCLK Net
 * @brief SPI2 Clock line for LCD screen.
 * @{
 */
#define SPI_SCLK_PORT      GPIOB
#define SPI_SCLK_PIN       13U
#define SPI_SCLK_AF        5U
#define SPI_SCLK_EXTI      13U
/** @} */


/**
 * @name LCD_MISO Net
 * @brief SPI2 MISO line for LCD screen.
 * @{
 */
#define SPI_MISO_PORT      GPIOB
#define SPI_MISO_PIN       14U
#define SPI_MISO_AF        5U
#define SPI_MISO_EXTI      14U
/** @} */


/**
 * @name LCD_MOSI Net
 * @brief SPI2 MOSI line for LCD screen.
 * @{
 */
#define SPI_MOSI_PORT      GPIOB
#define SPI_MOSI_PIN       15U
#define SPI_MOSI_AF        5U
#define SPI_MOSI_EXTI      15U
/** @} */


/**
 * @name DAT0 Net
 * @brief SD Card SDIO_D0 line.
 * @{
 */
#define SDIO_D0_PORT      GPIOC
#define SDIO_D0_PIN       8U
#define SDIO_D0_AF        12U
#define SDIO_D0_EXTI      8U
/** @} */


/**
 * @name DAT1 Net
 * @brief SD Card SDIO_D1 line.
 * @{
 */
#define SDIO_D1_PORT      GPIOC
#define SDIO_D1_PIN       9U
#define SDIO_D1_AF        12U
#define SDIO_D1_EXTI      9U
/** @} */


/**
 * @name DAT2 Net
 * @brief SD Card SDIO_D2 line.
 * @{
 */
#define SDIO_D2_PORT      GPIOC
#define SDIO_D2_PIN       10U
#define SDIO_D2_AF        12U
#define SDIO_D2_EXTI      10U
/** @} */


/**
 * @name DAT3 Net
 * @brief SD Card SDIO_D3 line.
 * @{
 */
#define SDIO_D3_PORT      GPIOC
#define SDIO_D3_PIN       11U
#define SDIO_D3_AF        12U
#define SDIO_D3_EXTI      11U
/** @} */


/**
 * @name CLK Net
 * @brief SD Card SDIO_CK line.
 * @{
 */
#define SDIO_CK_PORT      GPIOC
#define SDIO_CK_PIN       12U
#define SDIO_CK_AF        12U
#define SDIO_CK_EXTI      12U
/** @} */


/**
 * @name CMD Net
 * @brief SD Card SDIO_CMD line.
 * @{
 */
#define SDIO_CMD_PORT      GPIOD
#define SDIO_CMD_PIN       2U
#define SDIO_CMD_AF        12U
#define SDIO_CMD_EXTI      2U
/** @} */


#endif
