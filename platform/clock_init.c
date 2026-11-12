/**
 * @file clock_init.c
 * @brief Implementation of system core clock initialization.
 *
 * @author Aleksander Dziągwa
 * @date 20.10.2026
 */

#include "stm32f4xx.h"
#include "system_clocks.h"
#include "system_state/system_init_error.h"


/** PLL config */
#define PLL_M 8UL
#define PLL_N 168UL
#define PLL_P 2UL
#define PLL_Q 7UL


/** 
 * @brief Wait ~4s for register to reach expected value
 * 
 * @param register_address  address to register
 * @param mask              mask used when checkig
 * @param expected_value    value for register to reach
 * 
 * @return TRUE when register equals expected_value before timout, otherwise returns FALSE
 */
bool wait_for_register_value(volatile const uint32_t *register_address, uint32_t mask, uint32_t expected_value)
{
    uint32_t timeout = SystemCoreClock / 2;
    while (((*register_address & mask) != expected_value) && (timeout != 0U)) {
        --timeout;
    }

    return ((*register_address & mask) == expected_value);

}

/**
 * @brief Revert system clock to HSI and reset bus prescalers and Flash latency
 *        after a clock configuration failure
 */
static void retain_hsi_clock(void)
{
    MODIFY_REG(RCC->CFGR, RCC_CFGR_SW, RCC_CFGR_SW_HSI);
    while (READ_BIT(RCC->CFGR, RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI) {
    }

    CLEAR_BIT(RCC->CR, RCC_CR_PLLON | RCC_CR_HSEON);

    MODIFY_REG(RCC->CFGR, RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2,
                RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE1_DIV1 | RCC_CFGR_PPRE2_DIV1);

    MODIFY_REG(FLASH->ACR, FLASH_ACR_LATENCY, FLASH_ACR_LATENCY_0WS);

    SystemCoreClock = HSI_VALUE;

}

/**
 * @brief Switch system clock to HSI, disable HSE and PLLs, and reset RCC
 *        configuration and interrupt registers to default values
 */
void reset_clock_tree(void)
{
    /* HSI is the safe clock used while the PLL is reconfigured. */
    SET_BIT(RCC->CR, RCC_CR_HSION);
    while (READ_BIT(RCC->CR, RCC_CR_HSIRDY) == 0U) {
    }

    MODIFY_REG(RCC->CFGR, RCC_CFGR_SW, RCC_CFGR_SW_HSI);
    while (READ_BIT(RCC->CFGR, RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI) {
    }

    CLEAR_BIT(RCC->CR, RCC_CR_PLLON | RCC_CR_PLLI2SON | RCC_CR_CSSON | RCC_CR_HSEON);
    while (READ_BIT(RCC->CR, RCC_CR_PLLRDY | RCC_CR_PLLI2SRDY | RCC_CR_HSERDY) != 0U) {
    }

    RCC->CFGR = 0U;
    RCC->PLLCFGR = RCC_PLLCFGR_RST_VALUE;
    RCC->PLLI2SCFGR = RCC_PLLI2SCFGR_RST_VALUE;
    CLEAR_BIT(RCC->CR, RCC_CR_HSEBYP);

    /* Disable RCC ready interrupts and clear all pending RCC flags. */
    RCC->CIR = RCC_CIR_LSIRDYC | RCC_CIR_LSERDYC | RCC_CIR_HSIRDYC | RCC_CIR_HSERDYC |
               RCC_CIR_PLLRDYC | RCC_CIR_PLLI2SRDYC | RCC_CIR_CSSC;
    
}

/**
 * @brief Configure Flash latency, bus prescalers, HSE, and main PLL to run
 *        the system clock at 168 MHz
 * 
 * @return SYSTEM_INIT_STATUS_OK when main clock is configured successfully,
 *         otherwise returns specific timeout status (SYSTEM_INIT_STATUS_HSE_TIMEOUT,
 *         SYSTEM_INIT_STATUS_PLL_TIMEOUT, or SYSTEM_INIT_STATUS_SYSCLK_SWITCH_TIMEOUT)
 */
system_init_status_t configure_main_clock(void)
{
    /* Scale 1 is required for operation at 168 MHz. */
    SET_BIT(RCC->APB1ENR, RCC_APB1ENR_PWREN);
    (void)RCC->APB1ENR;
    SET_BIT(PWR->CR, PWR_CR_VOS);

    /* Y1 is a crystal, not an externally driven clock. */
    CLEAR_BIT(RCC->CR, RCC_CR_HSEBYP);
    SET_BIT(RCC->CR, RCC_CR_HSEON);
    if (wait_for_register_value(&RCC->CR, RCC_CR_HSERDY, RCC_CR_HSERDY) == 0U)
    {
        CLEAR_BIT(RCC->CR, RCC_CR_HSEON);
        return SYSTEM_INIT_STATUS_HSE_TIMEOUT;
    }

    /* Set Flash timing before increasing HCLK. The board runs at 3.3 V. */
    MODIFY_REG(FLASH->ACR, FLASH_ACR_LATENCY | FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN,
                FLASH_ACR_LATENCY_5WS | FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN);
    while (READ_BIT(FLASH->ACR, FLASH_ACR_LATENCY) != FLASH_ACR_LATENCY_5WS) {
    }

    /* Set PLL dividers. */
    RCC->PLLCFGR = (PLL_M << RCC_PLLCFGR_PLLM_Pos) | (PLL_N << RCC_PLLCFGR_PLLN_Pos) |
                    (((PLL_P / 2UL) - 1UL) << RCC_PLLCFGR_PLLP_Pos) | RCC_PLLCFGR_PLLSRC_HSE |
                    (PLL_Q << RCC_PLLCFGR_PLLQ_Pos);

    /* Set system prescallers. */
    MODIFY_REG(RCC->CFGR, RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2,
                RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE1_DIV4 | RCC_CFGR_PPRE2_DIV2);

    /* Wait for PLL to switch on. */
    SET_BIT(RCC->CR, RCC_CR_PLLON);
    if (wait_for_register_value(&RCC->CR, RCC_CR_PLLRDY, RCC_CR_PLLRDY) == 0U)
    {
        retain_hsi_clock();
        return SYSTEM_INIT_STATUS_PLL_TIMEOUT;
    }

    /* Enabling Clock Security System. */
    SET_BIT(RCC->CR, RCC_CR_CSSON);

    /* Wait for SYSCLOCK to switch on. */
    MODIFY_REG(RCC->CFGR, RCC_CFGR_SW, RCC_CFGR_SW_PLL);
    if (wait_for_register_value(&RCC->CFGR, RCC_CFGR_SWS, RCC_CFGR_SWS_PLL) == 0U)
    {
        retain_hsi_clock();
        return SYSTEM_INIT_STATUS_SYSCLK_SWITCH_TIMEOUT;
    }

    return SYSTEM_INIT_STATUS_OK;

}
