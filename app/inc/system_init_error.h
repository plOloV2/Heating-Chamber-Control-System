#ifndef SYSTEM_INIT_ERROR_H
#define SYSTEM_INIT_ERROR_H

/**
 * @file system_init_error.h
 * @brief Definition of system_init_status_t enum, holds current state of system initialization.
 *
 * @details
 * This file provides definitions and brief descriptions of all system initialization states.
 *
 * @author Aleksander Dziągwa
 * @date 20.10.2026
 */


/** @brief Enum holding current state of system initialization. */
typedef enum {

    /** @brief Nothing bad happend */
    SYSTEM_INIT_STATUS_OK = 0U,

    /** @brief HSE clock startup failed */
    SYSTEM_INIT_STATUS_HSE_TIMEOUT = 1U << 0U,

    /** @brief PLL startup failed  */
    SYSTEM_INIT_STATUS_PLL_TIMEOUT = 1U << 1U,

    /** @brief System core clock failed HSI to PLL switch */
    SYSTEM_INIT_STATUS_SYSCLK_SWITCH_TIMEOUT = 1U << 2U,

    /** @brief LSE clock startup failed */
    SYSTEM_INIT_STATUS_LSE_TIMEOUT = 1U << 3U,

    /** @brief GPIO initialization failed */
    SYSTEM_INIT_STATUS_GPIO_FAIL = 1U << 4U
} system_init_status_t;

/** @brief Main system_init_status_t status declared in system_init.c */
extern volatile system_init_status_t SystemInitStatus;

#endif
