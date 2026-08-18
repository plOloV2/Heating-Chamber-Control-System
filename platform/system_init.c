// #include "stm32f4xx.h"

// Sets HSI freq
#ifndef HSI_VALUE
    #define HSI_VALUE ((uint32_t)64'000'000)
#endif

// Sets HSE freq
#ifndef HSE_VALUE
    #define HSE_VALUE ((uint32_t)16'000'000)
#endif


uint32_t SystemCoreClock = 64'000'000;

void SystemInit(void){

    /* FPU settings ------------------------------------------------------------*/
    #if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
        SET_BIT(SCB->CPACR, ((3UL << (10*2)) | (3UL << (11*2))));  /* set CP10 and CP11 Full Access */
    #endif

    /* Set HSI on */
    SET_BIT(RCC->CR, RCC_CR_HSION);

    /* Wait for HSI to start */
    while(!READ_BIT(RCC->CR, RCC_CR_HSIRDY));

    /* Reset CFGR register */
    CLEAR_REG(RCC->CFGR);

    /* Reset HSEON, HSECSSON, CSION, HSI48ON, CSIKERON, PLL1ON, PLL2ON and PLL3ON bits */
    RCC->CR &= 0xEAF6'ED7FU;

    /* Decreasing the number of wait states because of lower CPU frequency */
    if(FLASH_LATENCY_DEFAULT  < (READ_BIT((FLASH->ACR), FLASH_ACR_LATENCY))){

        /* Program the new number of wait states to the LATENCY bits in the FLASH_ACR register */
        MODIFY_REG(FLASH->ACR, FLASH_ACR_LATENCY, (uint32_t)(FLASH_LATENCY_DEFAULT));

    }

    /* Reset D1 domain prescaler */
    CLEAR_REG(RCC->D1CFGR);

    /* Reset D2 domain prescaler */
    CLEAR_REG(RCC->D2CFGR);

    /* Reset D3 domain prescaler */
    CLEAR_REG(RCC->D3CFGR);

    /* Reset PLLCKSELR register */
    WRITE_REG(RCC->PLLCKSELR, 0x0202'0200);

    /* Reset PLLCFGR register */
    WRITE_REG(RCC->PLLCFGR, 0x01FF'0000);

    /* Reset PLL1DIVR register */
    WRITE_REG(RCC->PLL1DIVR, 0x0101'0280);

    /* Reset PLL1FRACR register */
    CLEAR_REG(RCC->PLL1FRACR);

    /* Reset PLL2DIVR register */
    WRITE_REG(RCC->PLL2DIVR, 0x0101'0280);

    /* Reset PLL2FRACR register */
    CLEAR_REG(RCC->PLL2FRACR);

    /* Reset PLL3DIVR register */
    WRITE_REG(RCC->PLL3DIVR, 0x0101'0280);

    /* Reset PLL3FRACR register */
    CLEAR_REG(RCC->PLL3FRACR);

    /* Reset HSEBYP bit */
    RCC->CR &= 0xFFFB'FFFFU;

    /* Disable all interrupts */
    CLEAR_REG(RCC->CIER);
    

}

/* Set SystemCoreClock variable */
void SystemCoreClockUpdate(void){

    SystemCoreClock = 64'000'000;

}
