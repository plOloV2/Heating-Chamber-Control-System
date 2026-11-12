#ifndef TEMP_OPERATIONS_H
#define TEMP_OPERATIONS_H

/**
 * @file ADC_operations.h
 * @brief Definitions and buffer declarations for PT1000 ADC+DMA measurements.
 *
 * @author Aleksander Dziągwa
 * @date 12.11.2026
 */

#include <stdint.h>
#include "mem_location.h"

/** Number of PT1000 channels on PCB */
#define TEMP_SENSOR_COUNT   4U

/** @brief Raw 12-bit ADC values updated every 1s by DMA2_Stream0 */
extern DMA_BUFFER volatile uint16_t TempAdcRaw[TEMP_SENSOR_COUNT];

/** @brief Initialize ADC1 and DMA2_Stream0 for 4-channel PT1000 scan */
void configure_adc_dma(void);

/** @brief Trigger a 4-channel ADC scan via DMA (called by TIM7_IRQHandler) */
void start_temp_dma_read(void);

#endif
