#include "stm32f4xx.h"
#include "./system_init.h"

/* Define magic number for the backup register */
#define RTC_MAGIC_NUMBER ((uint32_t)0x0F0690F0)


bool wait_for_register_value(volatile const uint32_t *register_address, uint32_t mask, uint32_t expected_value);


system_init_status_t configure_rtc_clock(void)
{
    const uint32_t previous_dbp = READ_BIT(PWR->CR, PWR_CR_DBP);

    /* Backup-domain writes require the PWR clock and DBP access. */
    SET_BIT(PWR->CR, PWR_CR_DBP);
    while (READ_BIT(PWR->CR, PWR_CR_DBP) == 0U) {
    }

    /* 
     * If the magic number is present, the RTC was kept alive by the battery.
     * Code can skip hardware initialization and exit immediately.
     */
    if(RTC->BKP0R == RTC_MAGIC_NUMBER)
    {
        if(previous_dbp == 0U) 
        {
            CLEAR_BIT(PWR->CR, PWR_CR_DBP);
        }

        return SYSTEM_INIT_STATUS_OK;
    }

    /*
     * RTCSEL can only be changed by resetting the backup domain. Reset it only
     * when a different source is already selected; normal resets then retain
     * the RTC calendar and backup registers.
     */
    if((READ_BIT(RCC->BDCR, RCC_BDCR_RTCSEL) != RCC_BDCR_RTCSEL_0) && (READ_BIT(RCC->BDCR, RCC_BDCR_RTCSEL) != 0U))
    {
        SET_BIT(RCC->BDCR, RCC_BDCR_BDRST);
        CLEAR_BIT(RCC->BDCR, RCC_BDCR_BDRST);
    }

    /* LSEBYP may only be changed while the LSE clock is disabled. */
    if(READ_BIT(RCC->BDCR, RCC_BDCR_LSEBYP) != 0U)
    {
        CLEAR_BIT(RCC->BDCR, RCC_BDCR_LSEON);
        if(!wait_for_register_value(&RCC->BDCR, RCC_BDCR_LSERDY, 0U))
        {
            CLEAR_BIT(RCC->BDCR, RCC_BDCR_RTCEN);
            if(previous_dbp == 0U)
            {
                CLEAR_BIT(PWR->CR, PWR_CR_DBP);
            }

            return SYSTEM_INIT_STATUS_LSE_TIMEOUT;
        }

        CLEAR_BIT(RCC->BDCR, RCC_BDCR_LSEBYP);
    }

    SET_BIT(RCC->BDCR, RCC_BDCR_LSEON);
    
    /* Check against false instead of 0U */
    if(!wait_for_register_value(&RCC->BDCR, RCC_BDCR_LSERDY, RCC_BDCR_LSERDY))
    {
        CLEAR_BIT(RCC->BDCR, RCC_BDCR_RTCEN | RCC_BDCR_LSEON);

        if(previous_dbp == 0U)
        {
            CLEAR_BIT(PWR->CR, PWR_CR_DBP);
        }

        return SYSTEM_INIT_STATUS_LSE_TIMEOUT;
    }

    MODIFY_REG(RCC->BDCR, RCC_BDCR_RTCSEL, RCC_BDCR_RTCSEL_0);
    SET_BIT(RCC->BDCR, RCC_BDCR_RTCEN);

    if(previous_dbp == 0U)
    {
        CLEAR_BIT(PWR->CR, PWR_CR_DBP);
    }

    return SYSTEM_INIT_STATUS_OK;

}
