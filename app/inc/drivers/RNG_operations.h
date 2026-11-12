#ifndef RNG_OPERATIONS_H
#define RNG_OPERATIONS_H

/**
 * @file RNG_operations.h
 * @brief Hardware RNG driver for STM32F4.
 * 
 * @author Aleksander Dziągwa
 * @date 8.11.2026
 */

#include <stdint.h>


/** @brief Enable the RNG generator. */
void rng_hw_init(void);


/**
 * @brief Read a 32-bit true random number from hardware RNG.
 * 
 * @param out_val Pointer where the 32-bit random value will be stored.
 * @return true if hardware random number was read, false on timeout/clock error.
 */
bool rng_get_random32(uint32_t *out_val);

#endif
