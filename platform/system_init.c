/**
 * @file system_init.c
 * @brief STM32F405RG system clock and RTC clock initialization.
 */

#include "stm32f4xx.h"
#include "./system_init.h"

/*
 * Clock tree:
 *   HSE        = 16 MHz
 *   PLL input  = HSE / 16 = 1 MHz
 *   PLL VCO    = 1 MHz * 336 = 336 MHz
 *   SYSCLK     = VCO / 2 = 168 MHz
 *   PLL48CLK   = VCO / 7 = 48 MHz
 *   HCLK       = SYSCLK / 1 = 168 MHz
 *   PCLK1      = HCLK / 4 = 42 MHz
 *   PCLK2      = HCLK / 2 = 84 MHz
 */

uint32_t SystemCoreClock = HSI_VALUE;
volatile system_init_status_t SystemInitStatus = SYSTEM_INIT_STATUS_OK;

system_init_status_t configure_main_clock(void);
system_init_status_t configure_rtc_clock(void);
void reset_clock_tree(void);

const uint8_t AHBPrescTable[16] = {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 1U, 2U, 3U, 4U, 6U, 7U, 8U, 9U};
const uint8_t APBPrescTable[8] = {0U, 0U, 0U, 0U, 1U, 2U, 3U, 4U};

void SystemInit(void)
{
#if (__FPU_PRESENT == 1U) && (__FPU_USED == 1U)
    SET_BIT(SCB->CPACR, (3UL << (10U * 2U)) | (3UL << (11U * 2U)));
    __DSB();
    __ISB();
#endif
    SCB->VTOR = FLASH_BASE;

    reset_clock_tree();
    
    const system_init_status_t main_clock_status = configure_main_clock();
    const system_init_status_t rtc_clock_status = configure_rtc_clock();
    SystemInitStatus = (system_init_status_t)((uint32_t)main_clock_status | (uint32_t)rtc_clock_status);
}

void SystemCoreClockUpdate(void)
{
    uint32_t system_clock;

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

    SystemCoreClock = system_clock >> AHBPrescTable[READ_BIT(RCC->CFGR, RCC_CFGR_HPRE) >> RCC_CFGR_HPRE_Pos];

}
