#include "stm32f4xx.h"
#include "drivers/RTC_operations.h"

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