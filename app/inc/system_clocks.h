#ifndef SYSTEM_CLOCKS_H
#define SYSTEM_CLOCKS_H

/**
 * @file system_clocks.h
 * @brief Definition of system clocks speeds.
 *
 * @details
 * This file provides definitions of system clocks speeds if they haven't been defined yet.
 *
 * @author Aleksander Dziągwa
 * @date 20.10.2026
 */

/** HSI clock speed (16MHz) */
#ifndef HSI_VALUE
    #define HSI_VALUE 16'000'000UL
#endif

/** HSE clock speed (16MHz) */
#ifndef HSE_VALUE
    #define HSE_VALUE 16'000'000UL
#endif

/** LSE clock speed (32.768kHz) */
#ifndef LSE_VALUE
    #define LSE_VALUE 32'768UL
#endif

#endif
