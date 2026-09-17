#include "stm32f4xx.h"
#include "./system_init.h"

extern uint32_t SystemCoreClock;


const uint8_t AHBPrescTable[16] = {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 1U, 2U, 3U, 4U, 6U, 7U, 8U, 9U};
const uint8_t APBPrescTable[8] = {0U, 0U, 0U, 0U, 1U, 2U, 3U, 4U};


void SystemCoreClockUpdate(void)
{
    uint32_t system_clock;

    /* Check SYSCLOCK source. */
    switch (READ_BIT(RCC->CFGR, RCC_CFGR_SWS)) {
        case RCC_CFGR_SWS_HSE:
            system_clock = HSE_VALUE;
            break;

        case RCC_CFGR_SWS_PLL: {
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

        case RCC_CFGR_SWS_HSI:
        default:
            system_clock = HSI_VALUE;
            break;
    }

    /* Write Clock value shifted by AHBPrescaller value. */
    SystemCoreClock = system_clock >> AHBPrescTable[READ_BIT(RCC->CFGR, RCC_CFGR_HPRE) >> RCC_CFGR_HPRE_Pos];

}
