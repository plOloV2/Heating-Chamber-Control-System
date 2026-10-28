#ifndef RTC_OPERATION_H
#define RTC_OPERATION_H

/**
 * @file RTC_operation.h
 * @brief Definition of functions used to RTC.
 *
 * @details
 * This file provides definitions of functions to 
 *
 * @author Aleksander Dziągwa
 * @date 27.10.2026
 */

#include <stdint.h>

/* Magic number for the backup register */
#define RTC_MAGIC_NUMBER ((uint32_t)0x0F0690F0)

/** @brief Status of RTC, true if RTC was initialized properly and has valid date */
extern bool RTC_valid;

/* Unpacking macros for convenience */
#define RTC_UNPACK_YEAR(date)   (((date) >> 16U) & 0xFFFFU)
#define RTC_UNPACK_MONTH(date)  (((date) >> 8U)  & 0xFFU)
#define RTC_UNPACK_DAY(date)    ((date)          & 0xFFU)

#define RTC_UNPACK_HOUR(time)   (((time) >> 16U) & 0xFFU)
#define RTC_UNPACK_MIN(time)    (((time) >> 8U)  & 0xFFU)
#define RTC_UNPACK_SEC(time)    ((time)          & 0xFFU)

/**
 * @brief Read time from RTC register.
 * 
 * @returns Time packed as [23:16]=HH, [15:8]=MM, [7:0]=SS.
 *          Returns 21:37:00 (0x00152500) on error/uninitialized.
 */
uint32_t read_time(void);

/**
 * @brief Read date from RTC register.
 * 
 * @returns Date packed as [31:16]=YYYY, [15:8]=MM, [7:0]=DD.
 *          Returns 2005.04.02 (0x07D50402) on error/uninitialized.
 */
uint32_t read_date(void);

/**
 * @brief Write date and time to RTC registers.
 * 
 * @param year Current year 
 * @param month Current month
 * @param day Current day
 * @param hour Current hour
 * @param minute Current minute
 * @param seconds Current seconds count   
 * 
 * @returns TRUE on succesful time and date set, FALSE on fail  
 */
bool set_date_time(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t seconds);

#endif
