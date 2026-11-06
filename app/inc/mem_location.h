#ifndef MEM_LOCATION_H
#define MEM_LOCATION_H

/**
 * @file mem_location.h
 * @brief Definition of alternative locations for data.
 *
 * @details
 * This file provides definitions used to place data in CCMRAM and in special DMA BUFFER regions of memory.
 *
 * @author Aleksander Dziągwa
 * @date 20.10.2026
 */


/** @brief Definition of mem location for initialized variables - copied from FLASH to CCMRAM at boot */
#define CCMRAM_DATA  __attribute__((section(".ccmram")))

/** @brief Definition of mem location for zero-initialized arrays/structs - zeroed at boot, uses 0 bytes of FLASH */
#define CCMRAM_BSS   __attribute__((section(".ccmram_bss")))

/** @brief Definition of mem location for DMA buffers. */
#define DMA_BUFFER   __attribute__((section(".dma_buffers"), aligned(4)))

/** @brief Adresses of start and end of SRAM. */
#define SRAM_start  0x20000000UL
#define SRAM_end    0x20020000UL

/**
 * @brief Checks if buffer is inside SRAM and is aligned.
 * 
 * @param buf Pointer to buffer to be checked.
 * @param bytes_aligment Power-of-2 byte alignment required.
 * 
 * @returns TRUE if buf is inside SRAM and is aligned to bytes_aligment, FALSE otherwise.
 */
static inline bool is_DMA_safe(const void *buf, const uint32_t bytes_aligment)
{
    const uintptr_t addr = (uintptr_t)buf;
    if((bytes_aligment > 1U) && ((addr & (bytes_aligment - 1U)) != 0U))
    {
        return false;
    }
    if((addr < SRAM_start) || (addr >= SRAM_end))
    {
        return false;
    }

    return true; 
} 

#endif
