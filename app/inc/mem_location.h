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

/** @brief  Definition of mem location for zero-initialized arrays/structs - zeroed at boot, uses 0 bytes of FLASH */
#define CCMRAM_BSS   __attribute__((section(".ccmram_bss")))

/** @brief  Definition of mem location for DMA buffers. */
#define DMA_BUFFER   __attribute__((section(".dma_buffers"), aligned(32)))

#endif
