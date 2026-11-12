/**
 * @file SPI_operations.c
 * @brief Implementation of low-level SPI2 driver with DMA1 and FreeRTOS support.
 * 
 * @author Aleksander Dziągwa
 * @date 01.11.2026
 */

#include <stddef.h>
#include <string.h>
#include "stm32f4xx.h"
#include "AF_pins_def.h"
#include "mem_location.h"
#include "GPIO_pins_def.h"
#include "drivers/SPI_operations.h"
#include "drivers/pin_operations.h"

#include "FreeRTOS.h"
#include "task.h"


/** @brief Loop count timeout for polled SPI flag waiting */
#define SPI_TIMEOUT_COUNT       100000UL


/** @brief FreeRTOS DMA transfer timeout in milliseconds */
#define SPI_DMA_TIMEOUT_MS      100UL


/** @brief Minimum transfer size (in bytes) to use DMA instead of polling */
#define SPI_DMA_THRESHOLD       32U


/** @brief 1KiB SPI bounce buffers for non-SRAM transfers */
#define DMA_BUFF_BYTE_SIZE      1024U
static DMA_BUFFER uint8_t TX_buff[DMA_BUFF_BYTE_SIZE];
static DMA_BUFFER uint8_t RX_buff[DMA_BUFF_BYTE_SIZE];


/** @brief Dummy source/sink variables in DMA-accessible SRAM for unidirectional DMA */
static DMA_BUFFER uint8_t dma_dummy_tx = 0x00U;
static DMA_BUFFER uint8_t dma_dummy_rx = 0x00U;


/** @brief Handle of the FreeRTOS task waiting for DMA completion */
static volatile TaskHandle_t spi_waiting_task = NULL;


/** @brief Clear any pending Overrun (OVR) or stale RXNE flag on SPI2. */
static inline void spi_clear_ovr(void)
{
    volatile uint32_t tmpreg;
    tmpreg = SPI2->DR;
    tmpreg = SPI2->SR;
    (void)tmpreg;
}


/**
 * @brief Initialize SPI2 interface and DMA1 Streams 3 (RX) & 4 (TX).
 * 
 * @returns always TRUE 
 */
bool spi_hw_init(void)
{
    /* Enable SPI2 clock on APB1 and DMA1 clock on AHB1 */
    SET_BIT(RCC->APB1ENR, RCC_APB1ENR_SPI2EN);
    (void)READ_REG(RCC->APB1ENR);

    SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_DMA1EN);
    (void)READ_REG(RCC->AHB1ENR);

    /* Reset SPI2 peripheral */
    SET_BIT(RCC->APB1RSTR, RCC_APB1RSTR_SPI2RST);
    CLEAR_BIT(RCC->APB1RSTR, RCC_APB1RSTR_SPI2RST);

    /* Ensure SPI mode (not I2S mode) and clear CR2 */
    WRITE_REG(SPI2->I2SCFGR, 0U);
    WRITE_REG(SPI2->CR2, 0U);

    /*
     * Configure SPI2 CR1:
     * Start at fPCLK1 / 4 = 10.5 MHz (SPI_CR1_BR_0) for safe EVE4 (BT817) bootup (< 11 MHz).
     */
    WRITE_REG(SPI2->CR1, (SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_MSTR | SPI_CR1_BR_0 | SPI_CR1_SPE));

    spi_clear_ovr();

    /* Disable DMA1 Stream 3 (SPI2_RX) and Stream 4 (SPI2_TX) before configuration */
    CLEAR_BIT(DMA1_Stream3->CR, DMA_SxCR_EN);
    while (READ_BIT(DMA1_Stream3->CR, DMA_SxCR_EN) != 0U) {}

    CLEAR_BIT(DMA1_Stream4->CR, DMA_SxCR_EN);
    while (READ_BIT(DMA1_Stream4->CR, DMA_SxCR_EN) != 0U) {}

    /* Set peripheral address to SPI2->DR for both streams */
    DMA1_Stream3->PAR = (uint32_t)&SPI2->DR;
    DMA1_Stream4->PAR = (uint32_t)&SPI2->DR;

    /* Use Direct Mode (disable FIFO) for 8-bit byte transfers */
    DMA1_Stream3->FCR = 0U;
    DMA1_Stream4->FCR = 0U;

    /* Configure NVIC for DMA1_Stream3_IRQn (SPI2_RX completion signals full transaction end). */
    NVIC_SetPriority(DMA1_Stream3_IRQn, 5U);
    NVIC_EnableIRQ(DMA1_Stream3_IRQn);

    return true;

}


/** @brief Switch SPI2 clock speed between boot speed (10.5 MHz) and high speed (21 MHz). */
void spi_set_high_speed(bool high_speed)
{
    (void)spi_wait_idle();
    CLEAR_BIT(SPI2->CR1, SPI_CR1_SPE);

    if (high_speed)
    {
        /* BR = 000: fPCLK1 / 2 = 21 MHz */
        CLEAR_BIT(SPI2->CR1, SPI_CR1_BR);
    }
    else
    {
        /* BR = 001: fPCLK1 / 4 = 10.5 MHz */
        MODIFY_REG(SPI2->CR1, SPI_CR1_BR, SPI_CR1_BR_0);
    }

    SET_BIT(SPI2->CR1, SPI_CR1_SPE);
}


/** @brief Deinitialize SPI2 interface and disable DMA1 streams. */
void spi_hw_deinit(void)
{
    (void)spi_wait_idle();

    NVIC_DisableIRQ(DMA1_Stream3_IRQn);
    CLEAR_BIT(DMA1_Stream3->CR, DMA_SxCR_EN);
    CLEAR_BIT(DMA1_Stream4->CR, DMA_SxCR_EN);

    CLEAR_BIT(SPI2->CR1, SPI_CR1_SPE);
    CLEAR_BIT(RCC->APB1ENR, RCC_APB1ENR_SPI2EN);

    SetPin(SPI_CS_PORT, SPI_CS_PIN, 1U);
}


/**
 * @brief Wait until SPI bus is idle (TX buffer empty and not busy).
 * 
 * @returns TRUE if SPI bus becomes before SPI_TIMEOUT_COUNT, FALSE otherwise
 */
bool spi_wait_idle(void)
{
    uint32_t timeout = SPI_TIMEOUT_COUNT;
    while(((SPI2->SR & SPI_SR_TXE) == 0U) && (--timeout != 0U)) {}
    if(timeout == 0U)
    {
        return false;
    }

    timeout = SPI_TIMEOUT_COUNT;
    while(((SPI2->SR & SPI_SR_BSY) != 0U) && (--timeout != 0U)) {}

    return(timeout != 0U);

}


/**
 * @brief Internal helper: Perform a full-duplex DMA1 transfer on SPI2.
 * 
 * @param tx_ptr Pointer to SRAM TX buffer, or NULL to send 0x00 dummy bytes.
 * @param rx_ptr Pointer to SRAM RX buffer, or NULL to discard received bytes.
 * @param len    Number of bytes to transfer (must be > 0 and <= 65535).
 * 
 * @returns TRUE on succesfull transfer, FALSE on error.
 */
static bool spi_dma_transfer_chunk(const uint8_t *tx_ptr, uint8_t *rx_ptr, uint16_t len)
{
    /* Ensure streams are stopped */
    CLEAR_BIT(DMA1_Stream3->CR, DMA_SxCR_EN);
    CLEAR_BIT(DMA1_Stream4->CR, DMA_SxCR_EN);
    while(((DMA1_Stream3->CR & DMA_SxCR_EN) != 0U) || ((DMA1_Stream4->CR & DMA_SxCR_EN) != 0U));

    /* Clear all interrupt flags for Stream 3 (RX) in LIFCR and Stream 4 (TX) in HIFCR */
    DMA1->LIFCR = DMA_LIFCR_CTCIF3 | DMA_LIFCR_CHTIF3 | DMA_LIFCR_CTEIF3 |
                  DMA_LIFCR_CDMEIF3 | DMA_LIFCR_CFEIF3;
    DMA1->HIFCR = DMA_HIFCR_CTCIF4 | DMA_HIFCR_CHTIF4 | DMA_HIFCR_CTEIF4 |
                  DMA_HIFCR_CDMEIF4 | DMA_HIFCR_CFEIF4;

    spi_clear_ovr();

    /* Configure RX Stream (DMA1_Stream3, Channel 0: Periph-to-Mem) */
    uint32_t rx_cr = (0UL << DMA_SxCR_CHSEL_Pos) | /* Channel 0 */
                     DMA_SxCR_PL_1               | /* High priority */
                     DMA_SxCR_TCIE               | /* Transfer complete IRQ */
                     DMA_SxCR_TEIE;                /* Transfer error IRQ */

    if(rx_ptr != NULL)
    {
        DMA1_Stream3->M0AR = (uint32_t)rx_ptr;
        rx_cr |= DMA_SxCR_MINC; /* Increment RX buffer pointer */
    }
    else
    {
        DMA1_Stream3->M0AR = (uint32_t)&dma_dummy_rx;
        /* MINC = 0: overwrite dummy byte */
    }
    DMA1_Stream3->NDTR = len;
    DMA1_Stream3->CR   = rx_cr;

    /* Configure TX Stream (DMA1_Stream4, Channel 0: Mem-to-Periph) */
    uint32_t tx_cr = (0UL << DMA_SxCR_CHSEL_Pos) | /* Channel 0 */
                     DMA_SxCR_PL_0               | /* Medium priority (lower than RX) */
                     DMA_SxCR_DIR_0;               /* Memory-to-peripheral */

    if(tx_ptr != NULL)
    {
        DMA1_Stream4->M0AR = (uint32_t)tx_ptr;
        tx_cr |= DMA_SxCR_MINC; /* Increment TX buffer pointer */
    }
    else
    {
        dma_dummy_tx = 0x00U;
        DMA1_Stream4->M0AR = (uint32_t)&dma_dummy_tx;
        /* MINC = 0: repeatedly send 0x00 */
    }
    DMA1_Stream4->NDTR = len;
    DMA1_Stream4->CR   = tx_cr;

    /* Register current FreeRTOS task for notification and clear any pending notification */
    spi_waiting_task = xTaskGetCurrentTaskHandle();
    (void)ulTaskNotifyTake(pdTRUE, 0U);

    /* Enable RX DMA requests on SPI2, enable RX stream first, then TX stream, then TX DMA request */
    SET_BIT(SPI2->CR2, SPI_CR2_RXDMAEN);
    SET_BIT(DMA1_Stream3->CR, DMA_SxCR_EN);
    SET_BIT(DMA1_Stream4->CR, DMA_SxCR_EN);
    SET_BIT(SPI2->CR2, SPI_CR2_TXDMAEN);

    /* Block FreeRTOS task until RX DMA completes */
    uint32_t notified = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(SPI_DMA_TIMEOUT_MS));

    /* Disable SPI2 DMA requests */
    CLEAR_BIT(SPI2->CR2, SPI_CR2_TXDMAEN | SPI_CR2_RXDMAEN);

    if((notified == 0U) || ((DMA1->LISR & DMA_LISR_TEIF3) != 0U))
    {
        /* Timeout or DMA transfer error: abort streams and clean up */
        CLEAR_BIT(DMA1_Stream3->CR, DMA_SxCR_EN);
        CLEAR_BIT(DMA1_Stream4->CR, DMA_SxCR_EN);
        spi_clear_ovr();
        return false;
    }

    return spi_wait_idle();

}


/** @brief DMA1 Stream 3 (SPI2_RX) Interrupt Handler. */
void DMA1_Stream3_IRQHandler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if((DMA1->LISR & (DMA_LISR_TCIF3 | DMA_LISR_TEIF3)) != 0U)
    {
        /* Clear interrupt flags */
        DMA1->LIFCR = DMA_LIFCR_CTCIF3 | DMA_LIFCR_CHTIF3 | DMA_LIFCR_CTEIF3 |
                      DMA_LIFCR_CDMEIF3 | DMA_LIFCR_CFEIF3;

        TaskHandle_t task = spi_waiting_task;
        if(task != NULL)
        {
            spi_waiting_task = NULL;
            vTaskNotifyGiveFromISR(task, &xHigherPriorityTaskWoken);
        }

    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

}


/**
 * @brief Polled byte-by-byte transfer helper for small payloads.
 * 
 * @param tx_data Pointer to TX data, or NULL to send 0x00 dummy bytes.
 * @param rx_data Pointer to RX data, or NULL to discard received bytes.
 * @param size Number of bytes to transfer (must be > 0 and <= 65535). 
 * 
 * @returns Size of data received.
 */
static uint16_t spi_poll_transfer(const uint8_t *tx_data, uint8_t *rx_data, uint16_t size)
{
    /** Loop thru TX and RX data size times. */
    for (uint16_t i = 0U; i < size; i++)
    {
        /** Check if transmit buffer is empty. */
        uint32_t timeout = SPI_TIMEOUT_COUNT;
        while ((READ_BIT(SPI2->SR, SPI_SR_TXE) == 0U) && (--timeout != 0U)) {}
        if (timeout == 0U)
        {
            spi_clear_ovr();
            return i;
        }

        /** Load data into SPI2 data register, 0x00U if tx_data == NULL. */
        *(__IO uint8_t *)&SPI2->DR = (tx_data != NULL) ? tx_data[i] : 0x00U;

        /** Wait for data from slave */
        timeout = SPI_TIMEOUT_COUNT;
        while ((READ_BIT(SPI2->SR, SPI_SR_RXNE) == 0U) && (--timeout != 0U)) {}
        if (timeout == 0U)
        {
            spi_clear_ovr();
            return i;
        }

        /** Load received data into rx_data[] if rx_data != NULL. */
        uint8_t received = (uint8_t)(SPI2->DR);
        if (rx_data != NULL)
        {
            rx_data[i] = received;
        }

    }

    (void)spi_wait_idle();
    return size;

}


/**
 * @brief Transmit and receive a single byte over SPI (always polled).
 * 
 * @returns Byte received thru SPI.
 */
uint8_t spi_transmit_receive_byte(uint8_t data)
{

    uint8_t rx = 0xFFU;
    (void)spi_poll_transfer(&data, &rx, 1U);
    return rx;

}


/**
 * @brief Transmit a buffer of data over SPI (uses DMA1 for size >= SPI_DMA_THRESHOLD).
 * 
 * @returns Size of data send over SPI.
 */
uint16_t spi_transmit(const uint8_t *data, uint16_t size)
{
    if ((data == NULL) || (size == 0U))
    {
        return 0U;
    }

    /* Use polling for small transfers */
    if((size < SPI_DMA_THRESHOLD))
    {
        return spi_poll_transfer(data, NULL, size);
    }

    /* Direct DMA if buffer is in SRAM1/SRAM2 */
    if(is_DMA_safe(data, 1U))
    {
        return spi_dma_transfer_chunk(data, NULL, size) ? size : 0U;
    }

    /* Buffer is in Flash or CCMRAM: bounce through TX_buff in 1 KiB chunks */
    uint16_t sent = 0U;
    while (sent < size)
    {
        uint16_t chunk = size - sent;
        if(chunk > DMA_BUFF_BYTE_SIZE)
        {
            chunk = DMA_BUFF_BYTE_SIZE;
        }

        memcpy(TX_buff, &data[sent], chunk);
        if(!spi_dma_transfer_chunk(TX_buff, NULL, chunk))
        {
            return sent;
        }
        sent += chunk;
    }

    return sent;

}


/** 
 * @brief Receive a buffer of data over SPI (uses DMA1 for size >= SPI_DMA_THRESHOLD). 
 * 
 * @returns Size of received data from slave.
 */
uint16_t spi_receive(uint8_t *data, uint16_t size)
{
    if((data == NULL) || (size == 0U))
    {
        return 0U;
    }

    if((size < SPI_DMA_THRESHOLD))
    {
        return spi_poll_transfer(NULL, data, size);
    }

    if(is_DMA_safe(data, 1U))
    {
        return spi_dma_transfer_chunk(NULL, data, size) ? size : 0U;
    }

    /* Destination is in CCMRAM: bounce through RX_buff in 1 KiB chunks */
    uint16_t received = 0U;
    while(received < size)
    {
        uint16_t chunk = size - received;
        if (chunk > DMA_BUFF_BYTE_SIZE)
        {
            chunk = DMA_BUFF_BYTE_SIZE;
        }

        if (!spi_dma_transfer_chunk(NULL, RX_buff, chunk))
        {
            return received;
        }
        memcpy(&data[received], RX_buff, chunk);
        received += chunk;
    }

    return received;

}
