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

#endif
