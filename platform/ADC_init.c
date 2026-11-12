/**
 * @file ADC_init.c
 * @brief Implementation of ADC1 and DMA2_Stream0 initialization for PT1000 sensors.
 *
 * @details Configures ADC1 to scan IN4..IN7 (PA4..PA7) and transfer results via
 *          DMA2_Stream0 (Channel 0) into RAM, generating a DMA interrupt upon completion.
 *
 * @author Aleksander Dziągwa
 * @date 12.11.2026
 */

#include "stm32f4xx.h"
#include "GPIO_pins_def.h"
#include "drivers/ADC_operations.h"

/** @brief 144 ADC cycles sampling time (3b110) for optimal settling with 470R + 3.3nF */
#define ADC_CYCLES     6UL


/**  @brief Configure DMA2_Stream0 (Channel 0) and ADC1 for 4-channel scan */
void configure_adc_dma(void)
{
    /** Enable AHB1 clock for DMA2 and APB2 clock for ADC1. */
    SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_DMA2EN);
    (void)RCC->AHB1ENR;

    SET_BIT(RCC->APB2ENR, RCC_APB2ENR_ADC1EN);
    (void)RCC->APB2ENR;

    /** Disable DMA2_Stream0 and wait until EN bit is cleared before configuring. */
    CLEAR_BIT(DMA2_Stream0->CR, DMA_SxCR_EN);
    while(READ_BIT(DMA2_Stream0->CR, DMA_SxCR_EN) != 0U);

    /** Clear all interrupt flags for DMA2 Stream 0 in LIFCR. */
    DMA2->LIFCR = DMA_LIFCR_CFEIF0 | DMA_LIFCR_CDMEIF0 |
                  DMA_LIFCR_CTEIF0 | DMA_LIFCR_CHTIF0  | DMA_LIFCR_CTCIF0;

    /** Set peripheral address (ADC1->DR), memory address (TempAdcRaw), and item count (4). */
    DMA2_Stream0->PAR  = (uint32_t)&ADC1->DR;
    DMA2_Stream0->M0AR = (uint32_t)&TempAdcRaw[0];
    DMA2_Stream0->NDTR = TEMP_SENSOR_COUNT;

    /**
     * Configure DMA2_Stream0->CR:
     * - Channel 0 (ADC1)
     * - Memory & Peripheral data size: 16-bit half-word (MSIZE_0, PSIZE_0)
     * - Memory increment mode enabled (MINC)
     * - Circular mode enabled (CIRC) so NDTR reloads to 4 automatically after each scan
     * - Direction: Peripheral-to-Memory (00)
     * - Transfer Complete Interrupt Enabled (TCIE)
     */
    DMA2_Stream0->CR = (0UL << DMA_SxCR_CHSEL_Pos) |
                       DMA_SxCR_PL_0               |
                       DMA_SxCR_MSIZE_0            |
                       DMA_SxCR_PSIZE_0            |
                       DMA_SxCR_MINC               |
                       DMA_SxCR_CIRC               |
                       DMA_SxCR_TCIE;

    /** Enable DMA2_Stream0 IRQ in NVIC. */
    NVIC_SetPriority(DMA2_Stream0_IRQn, 6U);
    NVIC_EnableIRQ(DMA2_Stream0_IRQn);

    /** Enable DMA2_Stream0. */
    SET_BIT(DMA2_Stream0->CR, DMA_SxCR_EN);

    /**
     * Configure ADC Common Prescaler:
     * PCLK2 = 84 MHz -> ADCCLK = PCLK2 / 4 = 21 MHz (Max allowed on STM32F4 is 36 MHz).
     * ADCPRE = 01 (DIV4).
     */
    MODIFY_REG(ADC->CCR, ADC_CCR_ADCPRE, ADC_CCR_ADCPRE_0);

    /** Configure Sampling Time (144 cycles) for Channels 4, 5, 6, and 7 in SMPR2. */
    MODIFY_REG(ADC1->SMPR2,
               ADC_SMPR2_SMP4 | ADC_SMPR2_SMP5 | ADC_SMPR2_SMP6 | ADC_SMPR2_SMP7,
               (ADC_CYCLES << ADC_SMPR2_SMP4_Pos) |
               (ADC_CYCLES << ADC_SMPR2_SMP5_Pos) |
               (ADC_CYCLES << ADC_SMPR2_SMP6_Pos) |
               (ADC_CYCLES << ADC_SMPR2_SMP7_Pos));

    /** Configure Regular Sequence Length = 4 conversions (L = 4 - 1 = 3 in SQR1)
     *  and Sequence Order: SQ1 = IN4, SQ2 = IN5, SQ3 = IN6, SQ4 = IN7 in SQR3.
     */
    MODIFY_REG(ADC1->SQR1, ADC_SQR1_L, (TEMP_SENSOR_COUNT - 1UL) << ADC_SQR1_L_Pos);

    MODIFY_REG(ADC1->SQR3,
               ADC_SQR3_SQ1 | ADC_SQR3_SQ2 | ADC_SQR3_SQ3 | ADC_SQR3_SQ4,
               (TEMP1_PIN << ADC_SQR3_SQ1_Pos) |
               (TEMP2_PIN << ADC_SQR3_SQ2_Pos) |
               (TEMP3_PIN << ADC_SQR3_SQ3_Pos) |
               (TEMP4_PIN << ADC_SQR3_SQ4_Pos));

    /**
     * Enable Scan Mode (CR1_SCAN), continuous DMA requests (CR2_DDS + CR2_DMA),
     * and power on ADC1 (CR2_ADON).
     */
    SET_BIT(ADC1->CR1, ADC_CR1_SCAN);
    SET_BIT(ADC1->CR2, ADC_CR2_DMA | ADC_CR2_DDS | ADC_CR2_ADON);

}
