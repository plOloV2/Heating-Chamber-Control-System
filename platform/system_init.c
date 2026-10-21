/**
 * @file system_init.c
 * @brief Implementation whole system initialization.
 *
 * @details
 * This file holds implementation of main function used to configure MCU.
 * This function calls other functions from /platform directory.
 *
 * @author Aleksander Dziągwa
 * @date 20.10.2026
 */

#include "stm32f4xx.h"
#include "./system_init.h"

/*
 * Clock tree:
 *   HSE        = 16 MHz
 *   PLL input  = HSE / 8 = 2 MHz
 *   PLL VCO    = 2 MHz * 168 = 336 MHz
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
void configure_gpio(void);
void SystemCoreClockUpdate(void);
void reset_clock_tree(void);


/** @brief Main configuration function, calls function from other _init.c files and configures MCU. */
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
    SystemCoreClockUpdate();

    const system_init_status_t rtc_clock_status = configure_rtc_clock();

    configure_gpio();

    NVIC_SetPriorityGrouping(0U);
    
    SystemInitStatus = (system_init_status_t)((uint32_t)main_clock_status | (uint32_t)rtc_clock_status);
    
}
