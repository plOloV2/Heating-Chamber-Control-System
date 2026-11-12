/**
 * @file RTC_operation.c
 * @brief Implementation of functions used to interact with RTC.
 *
 * @author Aleksander Dziągwa
 * @date 27.10.2026
 */

#include "stm32f4xx.h"
#include "drivers/RTC_operations.h"
#include "system_state/system_init_error.h"
#include "system_state/error.h"

/** 
 * @brief Helper packing function
 * 
 * @param part1 year/hour 
 * @param part2 month/minute
 * @param part3 day/second
 * 
 * @returns part1[31-16], part2[15-8], part3[7-0] packed into uint32_t
 */
static inline uint32_t pack_result(uint32_t part1, uint32_t part2, uint32_t part3){
    return (uint32_t)(((part1 & 0xFFFFU) << 16U) | ((part2 & 0xFFU) << 8U) | (part3 & 0xFFU));
}

uint32_t read_time(void)
{
    if (RTC_valid)
    {
        /* Reading TR locks the shadow registers */
        const uint32_t tr = READ_REG(RTC->TR);
        /* Mandatory read of DR to release the shadow register lock */
        (void)READ_REG(RTC->DR);

        const uint32_t sec  = (READ_BIT(tr, RTC_TR_ST_Msk)  >> RTC_TR_ST_Pos)  * 10U +
                              (READ_BIT(tr, RTC_TR_SU_Msk)  >> RTC_TR_SU_Pos);
        const uint32_t min  = (READ_BIT(tr, RTC_TR_MNT_Msk) >> RTC_TR_MNT_Pos) * 10U +
                              (READ_BIT(tr, RTC_TR_MNU_Msk) >> RTC_TR_MNU_Pos);
        const uint32_t hour = (READ_BIT(tr, RTC_TR_HT_Msk)  >> RTC_TR_HT_Pos)  * 10U +
                              (READ_BIT(tr, RTC_TR_HU_Msk)  >> RTC_TR_HU_Pos);

        if(hour < 24U && min < 60U && sec < 60U)
        {
            return pack_result(hour, min, sec);
        }

    }

    /* Fallback time: 21:37:00 */
    return pack_result(21U, 37U, 0U);

}


uint32_t read_date(void)
{
    if (RTC_valid)
    {
        /* Read TR first to latch shadow registers, then read DR to unlock */
        (void)READ_REG(RTC->TR);
        const uint32_t dr = READ_REG(RTC->DR);

        const uint32_t day   = (READ_BIT(dr, RTC_DR_DT_Msk) >> RTC_DR_DT_Pos) * 10U +
                               (READ_BIT(dr, RTC_DR_DU_Msk) >> RTC_DR_DU_Pos);
        const uint32_t month = (READ_BIT(dr, RTC_DR_MT_Msk) >> RTC_DR_MT_Pos) * 10U +
                               (READ_BIT(dr, RTC_DR_MU_Msk) >> RTC_DR_MU_Pos);
        const uint32_t year  = 2000U +
                               (READ_BIT(dr, RTC_DR_YT_Msk) >> RTC_DR_YT_Pos) * 10U +
                               (READ_BIT(dr, RTC_DR_YU_Msk) >> RTC_DR_YU_Pos);

        if(month >= 1U && month <= 12U && day >= 1U && day <= 31U)
        {
            return pack_result(year, month, day);
        }

    }

    /* Fallback date: 2005.04.02 */
    return pack_result(2005U, 4U, 2U);

}


/** 
 * @brief Helper function to check if year is a leap year
 * 
 * @param year Year value to check, should be between [0-99]
 * 
 * @returns TRUE on leap year, FALSE otherwise
 */
static inline bool is_year_intercalary(uint8_t year)
{
    return (year % 4U) == 0U;
}

/**
 * @brief Helper function to check if a date is valid.
 * 
 * @param year  Offset from year 2000 [0-99].
 * @param month Current month [1-12].
 * @param day   Current day [1-31].
 * 
 * @return true if date is valid, false otherwise.
 */
static bool valid_date(uint8_t year, uint8_t month, uint8_t day)
{
    static const uint8_t days_in_month[] = {
        0U, 31U, 28U, 31U, 30U, 31U, 30U, 31U, 31U, 30U, 31U, 30U, 31U
    };

    if(year >= 100U || month < 1U || month > 12U || day == 0U)
    {
        return false;
    }

    uint8_t max_days = days_in_month[month];
    if(month == 2U && is_year_intercalary(year))
    {
        max_days = 29U;
    }

    return day <= max_days;
}


/** 
 * @brief Helper function to check if time is valid
 * 
 * @param hour Current hour [0-23]
 * @param minute Curent minute [0-59]
 * @param seconds Current seconds count [0-59]
 * 
 * @returns TRUE if time is valid, FALSE otherwise
 */
static bool valid_time(uint8_t hour, uint8_t minute, uint8_t seconds)
{
    return (hour < 24U && minute < 60U && seconds < 60U);
}


bool wait_for_register_value(volatile const uint32_t *register_address, uint32_t mask, uint32_t expected_value);


bool set_date_time(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t seconds)
{
    /** Ensure Year is in proper range */
    if(year < 2000U || year > 2099U)
    {
        return false;
    }

    /** Subtract 2000 for proper BCD */
    const uint8_t year_offset = year - 2000; 

    /** RTC must be active, date and time must be valid */
    if( READ_BIT(SystemInitStatus, SYSTEM_INIT_STATUS_LSE_TIMEOUT) != 0U ||
        !valid_date(year_offset, month, day) ||
        !valid_time(hour, minute, seconds))
    {
        return false;
    }

    /** BCD date coding/ */
    const uint32_t date = (((uint32_t)(year_offset / 10U) << RTC_DR_YT_Pos) |
                           ((uint32_t)(year_offset % 10U) << RTC_DR_YU_Pos) |
                           ((uint32_t)(month / 10U)       << RTC_DR_MT_Pos) |
                           ((uint32_t)(month % 10U)       << RTC_DR_MU_Pos) |
                           ((uint32_t)(day / 10U)         << RTC_DR_DT_Pos) |
                           ((uint32_t)(day % 10U)         << RTC_DR_DU_Pos) |
                           (1UL                           << RTC_DR_WDU_Pos));

    /** BCD time coding */
    const uint32_t time = (((uint32_t)(hour / 10U)    << RTC_TR_HT_Pos)  |
                           ((uint32_t)(hour % 10U)    << RTC_TR_HU_Pos)  |
                           ((uint32_t)(minute / 10U)  << RTC_TR_MNT_Pos) |
                           ((uint32_t)(minute % 10U)  << RTC_TR_MNU_Pos) |
                           ((uint32_t)(seconds / 10U) << RTC_TR_ST_Pos)  |
                           ((uint32_t)(seconds % 10U) << RTC_TR_SU_Pos));

    
    /** Enable DBP access */
    const uint32_t previous_dbp = READ_BIT(PWR->CR, PWR_CR_DBP);
    SET_BIT(PWR->CR, PWR_CR_DBP);
    if(!wait_for_register_value(&PWR->CR, PWR_CR_DBP_Msk, PWR_CR_DBP))
    {
        return false;
    }

    /** Disable RTC register write protection */
    WRITE_REG(RTC->WPR, 0xCAU);
    WRITE_REG(RTC->WPR, 0x53U);

    /** Enter Initialization mode */
    WRITE_REG(RTC->ISR, RTC_ISR_INIT);

    if(!wait_for_register_value(&RTC->ISR, RTC_ISR_INITF_Msk, RTC_ISR_INITF))
    {
        WRITE_REG(RTC->ISR, ~RTC_ISR_INIT);
        WRITE_REG(RTC->WPR, 0xFFU);
        if (previous_dbp == 0U)
        {
            CLEAR_BIT(PWR->CR, PWR_CR_DBP);
        }

        return false;

    }

    /** Update Calendar & Backup Register */
    WRITE_REG(RTC->TR, time);
    WRITE_REG(RTC->DR, date);
    WRITE_REG(RTC->BKP0R, RTC_MAGIC_NUMBER);

    /** Exit Initialization mode and wait for shadow registers to sync */
    WRITE_REG(RTC->ISR, ~(RTC_ISR_INIT | RTC_ISR_RSF));
    (void)wait_for_register_value(&RTC->ISR, RTC_ISR_RSF_Msk, RTC_ISR_RSF);

    /** Re-enable write protection */
    WRITE_REG(RTC->WPR, 0xFFU);
    if(previous_dbp == 0U)
    {
        CLEAR_BIT(PWR->CR, PWR_CR_DBP);
    }

    /* Date and time are now valid; clear RTC error status */
    RTC_valid = true;
    CLEAR_BIT(SystemStatus, RTC_DATE_ERROR);

    return true;

}
