#include "stm32f4xx.h"
#include "./system_init.h"


#define PLL_M 16UL
#define PLL_N 336UL
#define PLL_P 2UL
#define PLL_Q 7UL


/* Removed 'static' so rtc_init.c can use it */
uint32_t wait_for_register_value(volatile const uint32_t *register_address, uint32_t mask, uint32_t expected_value)
{
    uint32_t timeout = SystemCoreClock;
    while (((*register_address & mask) != expected_value) && (timeout != 0U)) {
        --timeout;
    }
    return ((*register_address & mask) == expected_value) ? 1U : 0U;
}

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

system_init_status_t configure_main_clock(void)
{
    /* Scale 1 is required for operation at 168 MHz. */
    SET_BIT(RCC->APB1ENR, RCC_APB1ENR_PWREN);
    (void)RCC->APB1ENR;
    SET_BIT(PWR->CR, PWR_CR_VOS);

    /* Y1 is a crystal, not an externally driven clock. */
    CLEAR_BIT(RCC->CR, RCC_CR_HSEBYP);
    SET_BIT(RCC->CR, RCC_CR_HSEON);
    if (wait_for_register_value(&RCC->CR, RCC_CR_HSERDY,
                                RCC_CR_HSERDY) == 0U) {
        CLEAR_BIT(RCC->CR, RCC_CR_HSEON);
        return SYSTEM_INIT_STATUS_HSE_TIMEOUT;
    }

    /* Set Flash timing before increasing HCLK. The board runs at 3.3 V. */
    MODIFY_REG(FLASH->ACR, FLASH_ACR_LATENCY | FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN,
                FLASH_ACR_LATENCY_5WS | FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN);
    while (READ_BIT(FLASH->ACR, FLASH_ACR_LATENCY) != FLASH_ACR_LATENCY_5WS) {
    }

    RCC->PLLCFGR = (PLL_M << RCC_PLLCFGR_PLLM_Pos) | (PLL_N << RCC_PLLCFGR_PLLN_Pos) |
                    (((PLL_P / 2UL) - 1UL) << RCC_PLLCFGR_PLLP_Pos) | RCC_PLLCFGR_PLLSRC_HSE |
                    (PLL_Q << RCC_PLLCFGR_PLLQ_Pos);

    MODIFY_REG(RCC->CFGR, RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2,
                RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE1_DIV4 | RCC_CFGR_PPRE2_DIV2);

    SET_BIT(RCC->CR, RCC_CR_PLLON);
    if (wait_for_register_value(&RCC->CR, RCC_CR_PLLRDY, RCC_CR_PLLRDY) == 0U) {
        retain_hsi_clock();
        return SYSTEM_INIT_STATUS_PLL_TIMEOUT;
    }

    MODIFY_REG(RCC->CFGR, RCC_CFGR_SW, RCC_CFGR_SW_PLL);
    if (wait_for_register_value(&RCC->CFGR, RCC_CFGR_SWS, RCC_CFGR_SWS_PLL) == 0U) {
        retain_hsi_clock();
        return SYSTEM_INIT_STATUS_SYSCLK_SWITCH_TIMEOUT;
    }

    SystemCoreClock = SYSTEM_CLOCK_HZ;
    return SYSTEM_INIT_STATUS_OK;

}
