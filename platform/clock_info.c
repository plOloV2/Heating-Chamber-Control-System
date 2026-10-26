/**
 * @file clock_info.c
 * @brief Implementation of system core clock speed update function.
 *
 * @details
 * This function updates SystemCoreClock and sets it to curent clock speed in Hz.
 *
 * @author Aleksander Dziągwa
 * @date 20.10.2026
 */

#include "stm32f4xx.h"
#include "system_clocks.h"
#include "system_state/system_init_error.h"

extern uint32_t SystemCoreClock;

/** @brief Helper table used for AHB clock speed calculation */
const uint8_t AHBPrescTable[16] = {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 1U, 2U, 3U, 4U, 6U, 7U, 8U, 9U};

/** @brief Helper table used for APB clock speed calculation, curently unused */
const uint8_t APBPrescTable[8] = {0U, 0U, 0U, 0U, 1U, 2U, 3U, 4U};


/** @brief Calculates current system's core clock and updates SystemCoreClock. */
void SystemCoreClockUpdate(void)
{
    uint32_t system_clock;

    /* Check SYSCLOCK source. */
    switch (READ_BIT(RCC->CFGR, RCC_CFGR_SWS)) {
        case RCC_CFGR_SWS_HSE:          // HSE used as SYSCLOCK source
            system_clock = HSE_VALUE;
            break;

        case RCC_CFGR_SWS_PLL: {        // PLL used as SYSCLOCK source
            const uint32_t pll_source = (READ_BIT(RCC->PLLCFGR, RCC_PLLCFGR_PLLSRC) != 0U) ? HSE_VALUE : HSI_VALUE;
            const uint32_t pll_m = READ_BIT(RCC->PLLCFGR, RCC_PLLCFGR_PLLM);
            const uint32_t pll_n = READ_BIT(RCC->PLLCFGR, RCC_PLLCFGR_PLLN) >> RCC_PLLCFGR_PLLN_Pos;
            const uint32_t pll_p = (((READ_BIT(RCC->PLLCFGR, RCC_PLLCFGR_PLLP) >> RCC_PLLCFGR_PLLP_Pos) + 1UL) * 2UL);

            if (pll_m == 0U) {
                system_clock = HSI_VALUE;
            } else {
                system_clock = ((pll_source / pll_m) * pll_n) / pll_p;
            }
            break;
        }

        case RCC_CFGR_SWS_HSI:          // HSI used as SYSCLOCK source
        default:
            system_clock = HSI_VALUE;
            break;
    }

    /* Write Clock value shifted by AHBPrescaller value. */
    SystemCoreClock = system_clock >> AHBPrescTable[READ_BIT(RCC->CFGR, RCC_CFGR_HPRE) >> RCC_CFGR_HPRE_Pos];

}
