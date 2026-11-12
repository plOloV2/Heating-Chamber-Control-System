#ifndef ERROR_H
#define ERROR_H

/**
 * @file error.h
 * @brief Definition of system_error_t enum, holds current error state of system.
 *
 * @author Aleksander Dziągwa
 * @date 20.10.2026
 */


/** @brief Enum holding current error state of system. */
typedef enum {

    /** @brief Nothing bad happend */
    NO_ERROR = 0U,

    /** @brief System initializaton failed */
    SYSTEM_INIT_ERROR = 1U << 0,  

    /** @brief RTC subsystem initialization failed, date maybe missing */  
    RTC_DATE_ERROR = 1U << 1,

    /** @brief LCD subsystem initialization failed */
    LCD_INIT_ERROR = 1U << 2,

    /** @brief SD card subsystem initialization failed */
    SD_CARD_INIT_ERROR = 1U << 3,

    /** @brief SD card read failed */
    SD_CARD_READ_ERROR = 1U << 4,

    /** @brief SD card write failed */
    SD_CARD_WRITE_ERROR = 1U << 5,

    /** @brief Code executed after vTaskStartScheduler() in main() */
    RTOS_ERROR = 1U << 6

} system_error_t;

/** @brief Main system_error_t status declared in main.c */
extern volatile system_error_t SystemStatus;

/** @brief Number of all states SystemStatus can be */
#define SYSTEM_STATES 8

#endif
