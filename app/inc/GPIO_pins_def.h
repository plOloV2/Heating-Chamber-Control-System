#ifndef GPIO_PINS_DEF_H
#define GPIO_PINS_DEF_H

/**
 * @file GPIO_pins_def.h
 * @brief Definition of STM32 GPIO inputs/outputs.
 *
 * @details
 * This file provides definitions of ports, pins, and active states for all GPIO inputs/outputs.
 *
 * @author Aleksander Dziągwa
 * @date 8.10.2026
 */


/**
 * @name FAN Net
 * @brief Fan control output.
 * @{
 */
#define FAN_PORT        GPIOA
#define FAN_PIN         0U
#define FAN_ACTIVE      1U
#define FAN_EXTI        0U
/** @} */


/**
 * @name HEAT Net
 * @brief Heater control output.
 * @{
 */
#define HEAT_PORT       GPIOA
#define HEAT_PIN        1U
#define HEAT_ACTIVE     1U
#define HEAT_EXTI       1U
/** @} */


/**
 * @name TEMP1 Net
 * @brief Temperature measurement input No. 1.
 * @{
 */
#define TEMP1_PORT      GPIOA
#define TEMP1_PIN       4U
#define TEMP1_EXTI      4U
/** @} */


/**
 * @name TEMP2 Net
 * @brief Temperature measurement input No. 2.
 * @{
 */
#define TEMP2_PORT      GPIOA
#define TEMP2_PIN       5U
#define TEMP2_EXTI      5U
/** @} */


/**
 * @name TEMP3 Net
 * @brief Temperature measurement input No. 3.
 * @{
 */
#define TEMP3_PORT      GPIOA
#define TEMP3_PIN       6U
#define TEMP3_EXTI      6U
/** @} */


/**
 * @name TEMP4 Net
 * @brief Temperature measurement input No. 4.
 * @{
 */
#define TEMP4_PORT      GPIOA
#define TEMP4_PIN       7U
#define TEMP4_EXTI      7U
/** @} */


/**
 * @name BACKLIGHT_SIGNAL Net
 * @brief Display backlight control output. On B revision changed to pin PA15!
 * @{
 */
#define BLS_PORT        GPIOA
#define BLS_PIN         9U
#define BLS_ACTIVE      1U
#define BLS_EXTI        9U
/** @} */


/**
 * @name BUT1 Net
 * @brief Button 1 input.
 * @{
 */
#define BUT1_PORT       GPIOB
#define BUT1_PIN        0U
#define BUT1_ACTIVE     1U
#define BUT1_EXTI       0U
/** @} */


/**
 * @name BUT2 Net
 * @brief Button 2 input.
 * @{
 */
#define BUT2_PORT       GPIOB
#define BUT2_PIN        1U
#define BUT2_ACTIVE     1U
#define BUT2_EXTI       1U
/** @} */


/**
 * @name BUT3 Net
 * @brief Button 3 input.
 * @{
 */
#define BUT3_PORT       GPIOB
#define BUT3_PIN        2U
#define BUT3_ACTIVE     1U
#define BUT3_EXTI       2U
/** @} */


/**
 * @name BUT4 Net
 * @brief Button 4 input.
 * @{
 */
#define BUT4_PORT       GPIOB
#define BUT4_PIN        10U
#define BUT4_ACTIVE     1U
#define BUT4_EXTI       10U
/** @} */


/**
 * @name LED2 Net
 * @brief Debug LED 2 output.
 * @{
 */
#define LED2_PORT       GPIOC
#define LED2_PIN        4U
#define LED2_ACTIVE     1U
#define LED2_EXTI       4U
/** @} */


/**
 * @name LED1 Net
 * @brief Debug LED 1 output.
 * @{
 */
#define LED1_PORT       GPIOC
#define LED1_PIN        5U
#define LED1_ACTIVE     1U
#define LED1_EXTI       5U
/** @} */


/**
 * @name LCD_INT Net
 * @brief Interrupt signal from device to the system.
 * @{
 */
#define LCD_INT_PORT       GPIOC
#define LCD_INT_PIN        6U
#define LCD_INT_ACTIVE     0U
#define LCD_INT_EXTI       6U
/** @} */


/**
 * @name LCD_RST Net
 * @brief LCD Reset / Power down signal.
 * @{
 */
#define LCD_RST_PORT       GPIOC
#define LCD_RST_PIN        7U
#define LCD_RST_ACTIVE     0U
#define LCD_RST_EXTI       7U
/** @} */

#endif
