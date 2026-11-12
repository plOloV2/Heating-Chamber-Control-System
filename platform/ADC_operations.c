/**
 * @file ADC_operations.c
 * @brief Implementation of TIM7 trigger and DMA2_Stream0 completion interrupt for PT1000.
 * 
 * @details ADC1 reads all 4 PT1000 raw reading with DMA2 placing them inside TempAdcRaw[].
 *          After all values are transferred DMA2 interrupt resumes Temp task.
 *
 * @author Aleksander Dziągwa
 * @date 12.11.2026
 */

#include "stm32f4xx.h"
#include "drivers/ADC_operations.h"

#include "FreeRTOS.h"
#include "task.h"

#include "RTOS/temp_task.h"

volatile uint16_t TempAdcRaw[TEMP_SENSOR_COUNT] = {0U, 0U, 0U, 0U};

/** @brief Start a single 4-channel ADC1 scan transferred by DMA2_Stream0 */
void start_temp_dma_read(void)
{
    /* Clear any leftover ADC Overrun or End-Of-Conversion flags */
    ADC1->SR = 0U;

    /* Trigger regular group conversion via software start */
    SET_BIT(ADC1->CR2, ADC_CR2_SWSTART);
}


/** @brief TIM7 1-second periodic ISR: triggers ADC1+DMA scan of all 4 PT1000 sensors */
void TIM7_IRQHandler(void)
{
    if(READ_BIT(TIM7->SR, TIM_SR_UIF) != 0U)
    {
        CLEAR_BIT(TIM7->SR, TIM_SR_UIF);

        start_temp_dma_read();
    }

}


/** @brief DMA2 Stream 0 ISR: fires after all 4 PT1000 values are transferred to TempAdcRaw[] */
void DMA2_Stream0_IRQHandler(void)
{
    /* Check Transfer Complete Interrupt Flag for Stream 0 */
    if(READ_BIT(DMA2->LISR, DMA_LISR_TCIF0) != 0U)
    {
        /* Clear Transfer Complete and Half-Transfer flags for Stream 0 */
        DMA2->LIFCR = DMA_LIFCR_CTCIF0 | DMA_LIFCR_CHTIF0;

        /* Resume Temp task to convert raw readings into actual values */
        xTaskResumeFromISR(xTempTaskHandle);
    }

}
