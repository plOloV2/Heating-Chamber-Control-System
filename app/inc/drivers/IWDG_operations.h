#ifndef IWDG_OPERATIONS_H
#define IWDG_OPERATIONS_H

/**
 * @file IWDG_operations.h
 * @brief Independent Watchdog (IWDG) initialization and refresh helper.
 *
 * @author Aleksander Dziągwa
 * @date 13.11.2026
 */

#include "stm32f4xx.h"

/** @brief IWDG Key Values */
#define IWDG_KEY_RELOAD         0xAAAAU
#define IWDG_KEY_ENABLE         0xCCCCU
#define IWDG_KEY_WRITE_ACCESS   0x5555U

/** @brief Initialize and start IWDG with a 5-second timeout */
void configure_iwdg(void);

/** @brief Reset (feed) the 5-second IWDG counter */
static inline void reload_iwdg(void)
{
    IWDG->KR = IWDG_KEY_RELOAD;
}

#endif 
