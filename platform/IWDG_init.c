/**
 * @file iwdg_init.c
 * @brief Implementation of 5-second Independent Watchdog (IWDG) initialization.
 *
 * @author Aleksander Dziągwa
 * @date 13.11.2026
 */

#include "stm32f4xx.h"
#include "drivers/IWDG_operations.h"

/**
 * @brief IWDG timing constants (LSI = 32 kHz)
 * @details
 * Prescaler /64 (PR = 4) -> 32000 Hz / 64 = 500 Hz (2 ms tick)
 * Reload = 2500 - 1 = 2499 -> 2500 * 2 ms = 5000 ms (5.0 seconds)
 */
#define IWDG_PRESCALER_DIV64    4UL
#define IWDG_RELOAD_5S          (2500UL - 1UL)


void configure_iwdg(void)
{
    /** Freeze IWDG counter when core is halted in debug mode. */
    SET_BIT(DBGMCU->APB1FZ, DBGMCU_APB1_FZ_DBG_IWDG_STOP);

    /** Enable LSI oscillator and wait for it to stabilize. */
    SET_BIT(RCC->CSR, RCC_CSR_LSION);
    while(READ_BIT(RCC->CSR, RCC_CSR_LSIRDY) == 0U);

    /** Unlock write protection on IWDG_PR and IWDG_RLR. */
    IWDG->KR = IWDG_KEY_WRITE_ACCESS;

    /** Wait until PVU and RVU flags are cleared before writing. */
    while(READ_BIT(IWDG->SR, IWDG_SR_PVU | IWDG_SR_RVU) != 0U);

    IWDG->PR  = IWDG_PRESCALER_DIV64;
    IWDG->RLR = IWDG_RELOAD_5S;

    /** Wait for register update to complete. */
    while(READ_BIT(IWDG->SR, IWDG_SR_PVU | IWDG_SR_RVU) != 0U);

    /** Reload counter to 2499 and start the watchdog. */
    IWDG->KR = IWDG_KEY_RELOAD;
    IWDG->KR = IWDG_KEY_ENABLE;

}
