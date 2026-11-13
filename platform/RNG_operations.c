/**
 * @file RNG_operations.c
 * @brief Implementation of STM32F4 hardware RNG driver.
 * 
 * @author Aleksander Dziągwa
 * @date 8.11.2026
 */
#include "drivers/RNG_operations.h"
#include "stm32f4xx.h"
#include "stdlib.h"

/** Loop timeout (~40 PLL48CLK cycles = < 1 us per random word) */
#define RNG_TIMEOUT_COUNT  10000UL


void rng_hw_init(void)
{
    /* Ebable RNG clock */
    SET_BIT(RCC->AHB2ENR, RCC_AHB2ENR_RNGEN);
    (void)READ_REG(RCC->AHB2ENR);
    /* Enable the RNG peripheral */
    SET_BIT(RNG->CR, RNG_CR_RNGEN);
}


bool rng_get_random32(uint32_t *out_val)
{
    if(out_val == NULL)
    {
        return false;
    }

    /* Recover from any pending Seed Error (SEIS) by resetting RNGEN */
    if(READ_BIT(RNG->SR, RNG_SR_SEIS | RNG_SR_SECS) != 0U)
    {
        CLEAR_BIT(RNG->SR, RNG_SR_SEIS);
        CLEAR_BIT(RNG->CR, RNG_CR_RNGEN);
        SET_BIT(RNG->CR, RNG_CR_RNGEN);
    }

    /* Check for PLL48CLK Clock Error (CEIS / CECS) */
    if(READ_BIT(RNG->SR, RNG_SR_CEIS | RNG_SR_CECS) != 0U)
    {
        CLEAR_BIT(RNG->SR, RNG_SR_CEIS);
        return false;
    }

    /* Wait until Data Ready (DRDY) flag is set */
    uint32_t timeout = RNG_TIMEOUT_COUNT;
    while((READ_BIT(RNG->SR, RNG_SR_DRDY) == 0U) && (--timeout != 0U));

    if(timeout == 0U)
    {
        return false;
    }

    /* Reading RNG->DR automatically clears the DRDY bit */
    *out_val = RNG->DR;
    return true;

}
