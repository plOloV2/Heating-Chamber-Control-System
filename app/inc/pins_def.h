#ifndef PINS_DEF_H
#define PINS_DEF_H

/**
 * @file pins_def.h
 * @brief Definicja wyjść/wejść GPIO z STM32.
 * 
 * @details
 * Ten plik dostarcza definicje portów, pinów oraz stanów aktywnych dla wszystkich wyjść/wejść GPIO.
 * 
 * @author Aleksander Dziągwa
 * @date 8.10.2026
 */


 /**
  * @name Sieć FAN
  * @brief Wyjście sterowania termoobiegiem.
  * @{
  */
#define FAN_PORT        GPIOA
#define FAN_PIN         0U
#define FAN_ACTIVE      1U
#define FAN_EXTI        0U
/** @} */


 /**
  * @name Sieć HEAT
  * @brief Wyjście sterowania grzałkami.
  * @{
  */
#define HEAT_PORT       GPIOA
#define HEAT_PIN        1U
#define HEAT_ACTIVE     1U
#define HEAT_EXTI       1U
/** @} */


 /**
  * @name Sieć BUZZER
  * @brief Wyjście sterowania brzęczkiem.
  * @{
  */
#define BUZZ_PORT       GPIOA
#define BUZZ_PIN        2U
#define BUZZ_EXTI       2U
/** @} */


 /**
  * @name Sieć TEMP1
  * @brief Wejście pomiaru temperatury nr.1.
  * @{
  */
#define TEMP1_PORT      GPIOA
#define TEMP1_PIN       4U
#define TEMP1_EXTI      4U
/** @} */


 /**
  * @name Sieć TEMP2
  * @brief Wejście pomiaru temperatury nr.2.
  * @{
  */
#define TEMP2_PORT      GPIOA
#define TEMP2_PIN       5U
#define TEMP2_EXTI      5U
/** @} */


 /**
  * @name Sieć TEMP3
  * @brief Wejście pomiaru temperatury nr.3.
  * @{
  */
#define TEMP3_PORT      GPIOA
#define TEMP3_PIN       6U
#define TEMP3_EXTI      6U
/** @} */


 /**
  * @name Sieć TEMP4
  * @brief Wejście pomiaru temperatury nr.4.
  * @{
  */
#define TEMP4_PORT      GPIOA
#define TEMP4_PIN       7U
#define TEMP4_EXTI      7U
/** @} */


 /**
  * @name Sieć BACKLIGHT_SIGNAL
  * @brief Wyjście sterowania podświetleniem ekranu.
  * @{
  */
#define BLS_PORT        GPIOA
#define BLS_PIN         15U
#define BLS_ACTIVE      1U
#define BLS_EXTI        15U
/** @} */


 /**
  * @name Sieć BUT1
  * @brief Wejście z przycisku 1.
  * @{
  */
#define BUT1_PORT       GPIOB
#define BUT1_PIN        0U
#define BUT1_ACTIVE     1U
#define BUT1_EXTI       0U
/** @} */


 /**
  * @name Sieć BUT2
  * @brief Wejście z przycisku 2.
  * @{
  */
#define BUT2_PORT       GPIOB
#define BUT2_PIN        1U
#define BUT2_ACTIVE     1U
#define BUT2_EXTI       1U
/** @} */


 /**
  * @name Sieć BUT3
  * @brief Wejście z przycisku 3.
  * @{
  */
#define BUT3_PORT       GPIOB
#define BUT3_PIN        2U
#define BUT3_ACTIVE     1U
#define BUT3_EXTI       2U
/** @} */


 /**
  * @name Sieć BUT4
  * @brief Wejście z przycisku 4.
  * @{
  */
#define BUT4_PORT       GPIOB
#define BUT4_PIN        10U
#define BUT4_ACTIVE     1U
#define BUT4_EXTI       10U
/** @} */


 /**
  * @name Sieć LED1
  * @brief Wyjście na diode debug led 1.
  * @{
  */
#define LED1_PORT       GPIOC
#define LED1_PIN        4U
#define LED1_ACTIVE     1U
#define LED1_EXTI       4U
/** @} */


 /**
  * @name Sieć LED2
  * @brief Wyjście na diode debug led 2.
  * @{
  */
#define LED2_PORT       GPIOC
#define LED2_PIN        5U
#define LED2_ACTIVE     1U
#define LED2_EXTI       5U
/** @} */

#endif
