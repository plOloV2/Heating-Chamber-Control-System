/**
 * @file diskio.c
 * @brief Low-level SDIO disk I/O backend for FatFs using STM32F4 hardware SDIO and DMA.
 *
 * @details
 * Implements SD card driver for FatFs using SDIO 4-bit wide bus mode and DMA2 Stream 3.
 * All memory is statically allocated at compile time.
 * Supports SDSC, SDHC, and SDXC cards.
 *
 * @author Aleksander Dziągwa
 * @date 23.10.2026
 */

#include <stdint.h>
#include <string.h>
#include "stm32f4xx.h"
#include "ff.h"
#include "diskio.h"

#include "drivers/RTC_operations.h"
#include "mem_location.h"
#include "AF_pins_def.h"
#include "FreeRTOS.h"
#include "task.h"

/** Drive number mapping */
#define DEV_SD                      0U

/** SD Commands */
#define CMD0_GO_IDLE_STATE          0U
#define CMD2_ALL_SEND_CID           2U
#define CMD3_SEND_RELATIVE_ADDR     3U
#define CMD7_SELECT_CARD            7U
#define CMD8_SEND_IF_COND           8U
#define CMD9_SEND_CSD               9U
#define CMD12_STOP_TRANSMISSION     12U
#define CMD13_SEND_STATUS           13U
#define CMD16_SET_BLOCKLEN          16U
#define CMD17_READ_SINGLE_BLOCK     17U
#define CMD24_WRITE_BLOCK           24U
#define CMD55_APP_CMD               55U
#define ACMD6_SET_BUS_WIDTH         6U
#define ACMD41_SD_SEND_OP_COND      41U

/** Response types */
#define SDIO_RESP_NONE              0U
#define SDIO_RESP_SHORT             SDIO_CMD_WAITRESP_0
#define SDIO_RESP_LONG              (SDIO_CMD_WAITRESP_0 | SDIO_CMD_WAITRESP_1)

/** All clearable SDIO status flags */
#define SDIO_ICR_ALL_FLAGS          0x00C007FFU

/** Fast spin count (~100 us) before falling back to vTaskDelay during DMA transfers */
#define DMA_FAST_SPIN_COUNT         2000U
#define DMA_WAIT_TIMEOUT_MS         500U
#define SDIO_READY_TIMEOUT_MS       500U

/** @brief Static DMA buffer */
static uint8_t s_dma_scratch[512] DMA_BUFFER;

/** Static driver state */
static volatile DSTATUS s_disk_status = STA_NOINIT;
static uint16_t s_rca = 0U;
static bool s_is_sdhc = false;
static LBA_t s_card_sector_count = 0U;

/** @brief Delay helper using FreeRTOS vTaskDelay. */
static inline void sdio_delay_ms(uint32_t dur)
{
    const TickType_t ticks = pdMS_TO_TICKS(dur);
    vTaskDelay((ticks > 0U) ? ticks : 1U);
}

/**
 * @brief Send a command over SDIO and wait for response.
 *
 * @param cmd_index Command index (0-63).
 * @param arg Command argument.
 * @param wait_resp SDIO_RESP_NONE, SDIO_RESP_SHORT, or SDIO_RESP_LONG.
 * 
 * @return true if command completed successfully, false on error or timeout.
 */
static bool sdio_send_command(uint32_t cmd_index, uint32_t arg, uint32_t wait_resp)
{
    /* Clear any existing command flags */
    WRITE_REG(SDIO->ICR, SDIO_ICR_CCRCFAILC | SDIO_ICR_CTIMEOUTC | SDIO_ICR_CMDRENDC | SDIO_ICR_CMDSENTC);

    WRITE_REG(SDIO->ARG, arg);
    WRITE_REG(SDIO->CMD, (cmd_index & SDIO_CMD_CMDINDEX_Msk) |
                         (wait_resp & SDIO_CMD_WAITRESP_Msk) |
                         SDIO_CMD_CPSMEN);

    if (wait_resp == SDIO_RESP_NONE)
    {
        uint32_t timeout = 100000U;
        while((READ_BIT(SDIO->STA, SDIO_STA_CMDSENT) == 0U) && (--timeout > 0U));

        WRITE_REG(SDIO->ICR, SDIO_ICR_CMDSENTC);
        return (timeout > 0U);
    }
    else
    {
        uint32_t timeout = 300000U;
        while((READ_BIT(SDIO->STA, SDIO_STA_CMDREND | SDIO_STA_CTIMEOUT | SDIO_STA_CCRCFAIL) == 0U) && (--timeout > 0U));

        const uint32_t sta = READ_REG(SDIO->STA);
        WRITE_REG(SDIO->ICR, SDIO_ICR_CMDRENDC | SDIO_ICR_CTIMEOUTC | SDIO_ICR_CCRCFAILC);

        if((timeout == 0U) || (READ_BIT(sta, SDIO_STA_CTIMEOUT) != 0U))
        {
            return false;
        }

        /*
         * ACMD41 response (R3) does not have CRC.
         * For commands other than ACMD41, verify CRC passed.
         */
        if(cmd_index != ACMD41_SD_SEND_OP_COND)
        {
            if(READ_BIT(sta, SDIO_STA_CCRCFAIL) != 0U)
            {
                return false;
            }

        }

        return true;

    }

}

/**
 * @brief Wait until SD card finishes any internal programming and returns to TRAN state.
 * Yields CPU via vTaskDelay(1ms) if the card does not report ready on the immediate check.
 */
static bool sdio_wait_ready(void)
{
    uint32_t retries = SDIO_READY_TIMEOUT_MS;
    while(retries-- > 0U)
    {
        if(sdio_send_command(CMD13_SEND_STATUS, (uint32_t)s_rca << 16U, SDIO_RESP_SHORT))
        {
            const uint32_t resp = READ_REG(SDIO->RESP1);
            const uint32_t state = (resp >> 9U) & 0x0FU;
            if((state == 4U) && (READ_BIT(resp, 1UL << 8U) != 0U)) /* State 4 = TRAN, bit 8 = READY_FOR_DATA */
            {
                return true;
            }

        }
        /* Card is busy programming internal flash; yield to other FreeRTOS tasks */
        sdio_delay_ms(1U);

    }

    return false;

}

/** @brief Initialize SD card hardware and negotiate 4-bit high-speed mode. */
static bool sdio_card_init(void)
{
    /* Enable SDIO clock on APB2 */
    SET_BIT(RCC->APB2ENR, RCC_APB2ENR_SDIOEN);
    (void)READ_REG(RCC->APB2ENR);

    /* Enable DMA2 clock on AHB1 */
    SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_DMA2EN);
    (void)READ_REG(RCC->AHB1ENR);

    /* Reset SDIO peripheral registers */
    CLEAR_REG(SDIO->POWER);
    sdio_delay_ms(2U);

    /*
     * Configure initial slow clock for card identification (<= 400 kHz).
     * PLL48CLK = 48 MHz. SDIOCLK = 48 MHz / (CLKDIV + 2).
     * With CLKDIV = 118: 48 MHz / 120 = 400 kHz.
     */
    WRITE_REG(SDIO->CLKCR, (118UL << SDIO_CLKCR_CLKDIV_Pos | SDIO_CLKCR_CLKEN));

    /* Power up SDIO peripheral */
    WRITE_REG(SDIO->POWER, 0b11 << SDIO_POWER_PWRCTRL_Pos); /* 0b11: Power ON */
    sdio_delay_ms(2U);

    /* CMD0: Reset card to Idle state */
    (void)sdio_send_command(CMD0_GO_IDLE_STATE, 0U, SDIO_RESP_NONE);
    sdio_delay_ms(1U);

    /* CMD8: Check SD card interface condition (voltage 2.7-3.6V, check pattern 0xAA) */
    bool is_v2 = false;
    if(sdio_send_command(CMD8_SEND_IF_COND, 0x000001AAU, SDIO_RESP_SHORT))
    {
        if(READ_BIT(SDIO->RESP1, 0xFFU) == 0xAAU)
        {
            is_v2 = true;
        }

    }

    /* ACMD41 loop: Initialize card and query capacity support */
    bool card_ready = false;
    uint32_t acmd_arg = 0x00FF8000U; /* 2.7V - 3.6V */
    if(is_v2)
    {
        SET_BIT(acmd_arg, 1UL << 30U); /* HCS: Host Capacity Support (SDHC/SDXC) */
    }

    uint32_t retry = 1000U; /* Up to ~1 second timeout per SD specification */
    while((retry-- > 0U) && !card_ready)
    {
        /* Send CMD55 (APP_CMD) */
        if(sdio_send_command(CMD55_APP_CMD, 0U, SDIO_RESP_SHORT))
        {
            /* Send ACMD41 (SD_SEND_OP_COND) */
            if(sdio_send_command(ACMD41_SD_SEND_OP_COND, acmd_arg, SDIO_RESP_SHORT))
            {
                const uint32_t ocr = READ_REG(SDIO->RESP1);
                if(READ_BIT(ocr, 1UL << 31U) != 0U) /* Busy bit: 1 = ready */
                {
                    card_ready = true;
                    s_is_sdhc = is_v2 && (READ_BIT(ocr, 1UL << 30U) != 0U); /* CCS bit: 1 = SDHC/SDXC */
                }

            }

        }

        if(!card_ready)
        {
            sdio_delay_ms(1U);
        }

    }

    if(!card_ready)
    {
        return false;
    }

    /* CMD2: Read CID register */
    if(!sdio_send_command(CMD2_ALL_SEND_CID, 0U, SDIO_RESP_LONG))
    {
        return false;
    }

    /* CMD3: Read Relative Card Address (RCA) */
    if (!sdio_send_command(CMD3_SEND_RELATIVE_ADDR, 0U, SDIO_RESP_SHORT))
    {
        return false;
    }
    s_rca = (uint16_t)((READ_REG(SDIO->RESP1) >> 16U) & 0xFFFFU);

    /* CMD9: Read CSD register to determine card capacity */
    if(sdio_send_command(CMD9_SEND_CSD, (uint32_t)s_rca << 16U, SDIO_RESP_LONG))
    {
        const uint32_t csd_tab[4] = {
            READ_REG(SDIO->RESP1),
            READ_REG(SDIO->RESP2),
            READ_REG(SDIO->RESP3),
            READ_REG(SDIO->RESP4)
        };

        if(s_is_sdhc)
        {
            /* CSD Version 2.0: C_SIZE is bits [69:48] */
            const uint32_t c_size = ((csd_tab[1] & 0x0000003FU) << 16U) | ((csd_tab[2] >> 16U) & 0x0000FFFFU);
            s_card_sector_count = (LBA_t)(c_size + 1U) * 1024U;
        }
        else
        {
            /* CSD Version 1.0 */
            const uint32_t c_size = ((csd_tab[1] & 0x000003FFU) << 2U) | ((csd_tab[2] >> 30U) & 0x00000003U);
            const uint32_t c_size_mult = (csd_tab[2] >> 15U) & 0x00000007U;
            const uint32_t read_bl_len = csd_tab[1] & 0x0000000FU;
            const uint32_t block_count = (c_size + 1U) << (c_size_mult + 2U);
            s_card_sector_count = (LBA_t)block_count << (read_bl_len - 9U);
        }
        
    }
    else
    {
        s_card_sector_count = 0U;
    }

    /* CMD7: Select card to transition to Transfer (TRAN) state */
    if(!sdio_send_command(CMD7_SELECT_CARD, (uint32_t)s_rca << 16U, SDIO_RESP_SHORT))
    {
        return false;
    }

    /* For SDSC cards, set block length to 512 bytes */
    if(!s_is_sdhc)
    {
        if (!sdio_send_command(CMD16_SET_BLOCKLEN, 512U, SDIO_RESP_SHORT))
        {
            return false;
        }

    }

    /* Switch card to 4-bit wide bus mode: CMD55 followed by ACMD6 */
    if(sdio_send_command(CMD55_APP_CMD, (uint32_t)s_rca << 16U, SDIO_RESP_SHORT))
    {
        if(sdio_send_command(ACMD6_SET_BUS_WIDTH, 2U, SDIO_RESP_SHORT)) /* 2 = 4-bit bus */
        {
            /* Configure SDIO controller for 4-bit bus, high speed (24 MHz), hardware flow control */
            MODIFY_REG(SDIO->CLKCR,
                       SDIO_CLKCR_CLKDIV_Msk | SDIO_CLKCR_WIDBUS_Msk | SDIO_CLKCR_HWFC_EN,
                       (0UL << SDIO_CLKCR_CLKDIV_Pos) |
                       SDIO_CLKCR_WIDBUS_0 | /* 01: 4-bit wide bus */
                       SDIO_CLKCR_CLKEN);
        }

    }

    sdio_delay_ms(1U);

    return true;

}

/** @brief Read one 512-byte block from SD card using DMA. */
static DRESULT sdio_read_block(uint8_t *dest, uint32_t sector)
{
    const uint32_t addr = s_is_sdhc ? sector : (sector * 512U);

    /* Disable DMA stream */
    CLEAR_BIT(DMA2_Stream3->CR, DMA_SxCR_EN);
    while (READ_BIT(DMA2_Stream3->CR, DMA_SxCR_EN) != 0U) {}

    /* Clear DMA flags for Stream 3 */
    WRITE_REG(DMA2->LIFCR, DMA_LIFCR_CTCIF3 | DMA_LIFCR_CTEIF3 | DMA_LIFCR_CDMEIF3 |
                           DMA_LIFCR_CFEIF3 | DMA_LIFCR_CHTIF3);

    /* Clear SDIO flags */
    WRITE_REG(SDIO->ICR, SDIO_ICR_ALL_FLAGS);

    /* Program DMA parameters */
    WRITE_REG(DMA2_Stream3->PAR, (uint32_t)&SDIO->FIFO);
    WRITE_REG(DMA2_Stream3->M0AR, (uint32_t)dest);
    WRITE_REG(DMA2_Stream3->NDTR, 512U / 4U); /* 128 32-bit words */

    WRITE_REG(DMA2_Stream3->FCR, DMA_SxFCR_DMDIS | DMA_SxFCR_FTH_1 | DMA_SxFCR_FTH_0); /* FIFO full */
    WRITE_REG(DMA2_Stream3->CR, (4UL << DMA_SxCR_CHSEL_Pos) | /* Channel 4 */
                                (3UL << DMA_SxCR_PL_Pos)    | /* Very High priority */
                                (2UL << DMA_SxCR_MSIZE_Pos) | /* 32-bit memory word */
                                (2UL << DMA_SxCR_PSIZE_Pos) | /* 32-bit peripheral word */
                                DMA_SxCR_MINC               | /* Memory increment */
                                DMA_SxCR_PFCTRL             | /* Peripheral flow controller (SDIO) */
                                DMA_SxCR_PBURST_0);           /* INCR4 burst */

    /* Enable DMA stream */
    SET_BIT(DMA2_Stream3->CR, DMA_SxCR_EN);

    /* Configure SDIO data path */
    WRITE_REG(SDIO->DTIMER, 24000000U); /* ~1 second hardware data timeout */
    WRITE_REG(SDIO->DLEN, 512U);
    WRITE_REG(SDIO->DCTRL, SDIO_DCTRL_DTEN  |
                           SDIO_DCTRL_DTDIR | /* Direction: card to controller */
                           SDIO_DCTRL_DMAEN |
                           (POSITION_VAL(512U) << SDIO_DCTRL_DBLOCKSIZE_Pos)); /* 2^9 = 512 bytes */

    /* Send CMD17 (READ_SINGLE_BLOCK) */
    if (!sdio_send_command(CMD17_READ_SINGLE_BLOCK, addr, SDIO_RESP_SHORT))
    {
        CLEAR_BIT(DMA2_Stream3->CR, DMA_SxCR_EN);
        CLEAR_REG(SDIO->DCTRL);
        return RES_ERROR;
    }

    /* Stage 1: Fast spin (~100 us) so normal 43 us transfers complete with zero RTOS overhead */
    const uint32_t err_mask = SDIO_STA_DCRCFAIL | SDIO_STA_DTIMEOUT | SDIO_STA_RXOVERR | SDIO_STA_STBITERR;
    const uint32_t dma_err_mask = DMA_LISR_TEIF3 | DMA_LISR_DMEIF3 | DMA_LISR_FEIF3;
    uint32_t spin = DMA_FAST_SPIN_COUNT;
    while ((READ_BIT(DMA2->LISR, DMA_LISR_TCIF3 | dma_err_mask) == 0U) &&
           (READ_BIT(SDIO->STA, err_mask) == 0U) &&
           (spin > 0U))
    {
        --spin;
    }

    /* Stage 2: If card is stalling the read, yield CPU in 1 ms increments */
    uint32_t timeout_ms = DMA_WAIT_TIMEOUT_MS;
    while ((READ_BIT(DMA2->LISR, DMA_LISR_TCIF3 | dma_err_mask) == 0U) &&
           (READ_BIT(SDIO->STA, err_mask) == 0U) &&
           (timeout_ms > 0U))
    {
        sdio_delay_ms(1U);
        --timeout_ms;
    }

    const uint32_t sta = READ_REG(SDIO->STA);
    const uint32_t lisr = READ_REG(DMA2->LISR);
    CLEAR_BIT(DMA2_Stream3->CR, DMA_SxCR_EN);

    if((timeout_ms == 0U) || (READ_BIT(sta, err_mask) != 0U) || (READ_BIT(lisr, dma_err_mask) != 0U))
    {
        CLEAR_REG(SDIO->DCTRL);
        WRITE_REG(SDIO->ICR, SDIO_ICR_ALL_FLAGS);
        return RES_ERROR;
    }

    /* Wait for DATAEND (tight loop, takes only a few SDIO_CK cycles after DMA TC) */
    uint32_t timeout = 100000U;
    while ((READ_BIT(SDIO->STA, SDIO_STA_DATAEND) == 0U) && (--timeout > 0U));
    CLEAR_REG(SDIO->DCTRL);
    WRITE_REG(SDIO->ICR, SDIO_ICR_ALL_FLAGS);

    return (timeout > 0U) ? RES_OK : RES_ERROR;
}

/** @brief Write one 512-byte block to SD card using DMA. */
static DRESULT sdio_write_block(const uint8_t *src, uint32_t sector)
{
    const uint32_t addr = s_is_sdhc ? sector : (sector * 512U);

    /* Disable DMA stream */
    CLEAR_BIT(DMA2_Stream3->CR, DMA_SxCR_EN);
    while (READ_BIT(DMA2_Stream3->CR, DMA_SxCR_EN) != 0U) {}

    /* Clear DMA flags for Stream 3 */
    WRITE_REG(DMA2->LIFCR, DMA_LIFCR_CTCIF3 | DMA_LIFCR_CTEIF3 | DMA_LIFCR_CDMEIF3 |
                           DMA_LIFCR_CFEIF3 | DMA_LIFCR_CHTIF3);

    /* Clear SDIO flags */
    WRITE_REG(SDIO->ICR, SDIO_ICR_ALL_FLAGS);

    /* Program DMA parameters */
    WRITE_REG(DMA2_Stream3->PAR, (uint32_t)&SDIO->FIFO);
    WRITE_REG(DMA2_Stream3->M0AR, (uint32_t)src);
    WRITE_REG(DMA2_Stream3->NDTR, 512U / 4U); /* 128 words */

    WRITE_REG(DMA2_Stream3->FCR, DMA_SxFCR_DMDIS | DMA_SxFCR_FTH_1 | DMA_SxFCR_FTH_0); /* FIFO full */
    WRITE_REG(DMA2_Stream3->CR, (4UL << DMA_SxCR_CHSEL_Pos) | /* Channel 4 */
                                (3UL << DMA_SxCR_PL_Pos)    | /* Very High priority */
                                (2UL << DMA_SxCR_MSIZE_Pos) | /* 32-bit memory word */
                                (2UL << DMA_SxCR_PSIZE_Pos) | /* 32-bit peripheral word */
                                DMA_SxCR_MINC               | /* Memory increment */
                                DMA_SxCR_DIR_0              | /* Direction: memory to peripheral */
                                DMA_SxCR_PFCTRL             | /* Peripheral flow controller (SDIO) */
                                DMA_SxCR_PBURST_0);           /* INCR4 burst */

    /* Enable DMA stream */
    SET_BIT(DMA2_Stream3->CR, DMA_SxCR_EN);

    /* Send CMD24 (WRITE_BLOCK) */
    if (!sdio_send_command(CMD24_WRITE_BLOCK, addr, SDIO_RESP_SHORT))
    {
        CLEAR_BIT(DMA2_Stream3->CR, DMA_SxCR_EN);
        return RES_ERROR;
    }

    /* Configure SDIO data path */
    WRITE_REG(SDIO->DTIMER, 24000000U); /* ~1 second timeout */
    WRITE_REG(SDIO->DLEN, 512U);
    WRITE_REG(SDIO->DCTRL, SDIO_DCTRL_DTEN  |
                           SDIO_DCTRL_DMAEN |
                           (POSITION_VAL(512U) << SDIO_DCTRL_DBLOCKSIZE_Pos)); /* Direction: controller to card (0) */

    /* Stage 1: Fast spin (~100 us) for the 43 us wire transfer */
    const uint32_t err_mask = SDIO_STA_DCRCFAIL | SDIO_STA_DTIMEOUT | SDIO_STA_TXUNDERR | SDIO_STA_STBITERR;
    const uint32_t dma_err_mask = DMA_LISR_TEIF3 | DMA_LISR_DMEIF3 | DMA_LISR_FEIF3;
    uint32_t spin = DMA_FAST_SPIN_COUNT;
    while ((READ_BIT(DMA2->LISR, DMA_LISR_TCIF3 | dma_err_mask) == 0U) &&
           (READ_BIT(SDIO->STA, err_mask) == 0U) &&
           (spin > 0U))
    {
        --spin;
    }

    /* Stage 2: Fallback to 1 ms FreeRTOS delay if bus is stalled */
    uint32_t timeout_ms = DMA_WAIT_TIMEOUT_MS;
    while ((READ_BIT(DMA2->LISR, DMA_LISR_TCIF3 | dma_err_mask) == 0U) &&
           (READ_BIT(SDIO->STA, err_mask) == 0U) &&
           (timeout_ms > 0U))
    {
        sdio_delay_ms(1U);
        --timeout_ms;
    }

    const uint32_t sta = READ_REG(SDIO->STA);
    const uint32_t lisr = READ_REG(DMA2->LISR);
    CLEAR_BIT(DMA2_Stream3->CR, DMA_SxCR_EN);

    if ((timeout_ms == 0U) || (READ_BIT(sta, err_mask) != 0U) || (READ_BIT(lisr, dma_err_mask) != 0U))
    {
        CLEAR_REG(SDIO->DCTRL);
        WRITE_REG(SDIO->ICR, SDIO_ICR_ALL_FLAGS);
        return RES_ERROR;
    }

    /* Wait for last 128 bytes in SDIO FIFO to clock out and card CRC token (DATAEND & DBCKEND) */
    uint32_t timeout = 100000U;
    while((READ_BIT(SDIO->STA, SDIO_STA_DATAEND | err_mask) == 0U) && (--timeout > 0U));

    const uint32_t end_sta = READ_REG(SDIO->STA);
    CLEAR_REG(SDIO->DCTRL);
    WRITE_REG(SDIO->ICR, SDIO_ICR_ALL_FLAGS);

    if((timeout == 0U) || (READ_BIT(end_sta, err_mask) != 0U))
    {
        return RES_ERROR;
    }

    /* Wait until card finishes writing internal flash (yields via vTaskDelay if busy) */
    return sdio_wait_ready() ? RES_OK : RES_ERROR;
}

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/
[[maybe_unused]] DSTATUS disk_status(BYTE pdrv)
{
    if(pdrv != DEV_SD)
    {
        return STA_NOINIT;
    }

    return s_disk_status;

}

/*-----------------------------------------------------------------------*/
/* Initialize a Drive                                                    */
/*-----------------------------------------------------------------------*/
[[maybe_unused]] DSTATUS disk_initialize(BYTE pdrv)
{
    if(pdrv != DEV_SD)
    {
        return STA_NOINIT;
    }

    if(sdio_card_init())
    {
        CLEAR_BIT(s_disk_status, STA_NOINIT);
    }
    else
    {
        SET_BIT(s_disk_status, STA_NOINIT);
    }

    return s_disk_status;

}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/
[[maybe_unused]] DRESULT disk_read(BYTE pdrv, BYTE* buff, LBA_t sector, UINT count)
{
    if (pdrv != DEV_SD || count == 0U || buff == NULL)
    {
        return RES_PARERR;
    }
    if (READ_BIT(s_disk_status, STA_NOINIT) != 0U)
    {
        return RES_NOTRDY;
    }

    for(UINT i = 0U; i < count; i++)
    {
        DRESULT res;
        if(is_DMA_safe(buff, 4U))
        {
            res = sdio_read_block(buff, (uint32_t)(sector + i));
        }
        else
        {
            res = sdio_read_block(s_dma_scratch, (uint32_t)(sector + i));
            if (res == RES_OK)
            {
                (void)memcpy(buff, s_dma_scratch, 512U);
            }
        }

        if (res != RES_OK)
        {
            return res;
        }
        buff += 512U;
    }

    return RES_OK;
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/
[[maybe_unused]] DRESULT disk_write(BYTE pdrv, const BYTE* buff, LBA_t sector, UINT count)
{
    if (pdrv != DEV_SD || count == 0U || buff == NULL)
    {
        return RES_PARERR;
    }
    if (READ_BIT(s_disk_status, STA_NOINIT) != 0U)
    {
        return RES_NOTRDY;
    }

    for (UINT i = 0U; i < count; i++)
    {
        DRESULT res;
        if (is_DMA_safe(buff, 4U))
        {
            res = sdio_write_block(buff, (uint32_t)(sector + i));
        }
        else
        {
            (void)memcpy(s_dma_scratch, buff, 512U);
            res = sdio_write_block(s_dma_scratch, (uint32_t)(sector + i));
        }

        if (res != RES_OK)
        {
            return res;
        }
        buff += 512U;
    }

    return RES_OK;
}

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/
[[maybe_unused]] DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void* buff)
{
    if(pdrv != DEV_SD)
    {
        return RES_PARERR;
    }
    if(READ_BIT(s_disk_status, STA_NOINIT) != 0U)
    {
        return RES_NOTRDY;
    }

    switch(cmd)
    {
        case CTRL_SYNC:
            return sdio_wait_ready() ? RES_OK : RES_ERROR;

        case GET_SECTOR_COUNT:
            if(buff == NULL)
            {
                return RES_PARERR;
            }
            *(LBA_t *)buff = s_card_sector_count;
            return RES_OK;

        case GET_SECTOR_SIZE:
            if(buff == NULL)
            {
                return RES_PARERR;
            }
            *(WORD *)buff = 512U;
            return RES_OK;

        case GET_BLOCK_SIZE:
            if(buff == NULL)
            {
                return RES_PARERR;
            }
            *(DWORD *)buff = 1U;
            return RES_OK;

        case CTRL_TRIM:
            return RES_OK;

        default:
            return RES_PARERR;
    }
}

/*-----------------------------------------------------------------------*/
/* User Provided Timer Function for FatFs                                */
/*-----------------------------------------------------------------------*/
[[maybe_unused]] DWORD get_fattime(void)
{

    const uint32_t date = read_date();
    const uint32_t time = read_time();

    const uint32_t sec  = RTC_UNPACK_SEC(time);
    const uint32_t min  = RTC_UNPACK_MIN(time);
    const uint32_t hour = RTC_UNPACK_HOUR(time);

    const uint32_t day   = RTC_UNPACK_DAY(date);
    const uint32_t month = RTC_UNPACK_MONTH(date);
    const uint32_t year  = RTC_UNPACK_YEAR(date);


    return ((year - 1980U) << 25U) |
            (month << 21U)         |
            (day << 16U)           |
            (hour << 11U)          |
            (min << 5U)            |
            (sec >> 1U);

}
