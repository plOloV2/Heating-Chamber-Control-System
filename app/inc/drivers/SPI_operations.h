#ifndef SPI_OPERATIONS_H
#define SPI_OPERATIONS_H

/**
 * @file SPI_operations.h
 * @brief Definition of low-level SPI driver functions for STM32F405.
 *
 * @details
 * This file provides low level driver functions to initialize, deinitialize,
 * and transfer data over the SPI2 interface connected to the EVE display.
 *
 * @author Aleksander Dziągwa
 * @date 01.11.2026
 */

#include <stdint.h>

/**
 * @brief Initialize SPI2 hardware and corresponding GPIO pins for EVE display.
 *
 * @return TRUE if initialization succeeded, FALSE otherwise.
 */
bool spi_hw_init(void);

/**
 * @brief Deinitialize SPI2 peripheral and related pins.
 */
void spi_hw_deinit(void);

/**
 * @brief Transmit and receive a single byte over SPI.
 *
 * @param data Byte to transmit.
 * @return Received byte.
 */
uint8_t spi_transmit_receive_byte(uint8_t data);

/**
 * @brief Transmit a buffer of data over SPI.
 *
 * @param data Pointer to data buffer to transmit.
 * @param size Number of bytes to transmit.
 * @return Number of bytes successfully transmitted.
 */
uint16_t spi_transmit(const uint8_t *data, uint16_t size);

/**
 * @brief Receive a buffer of data over SPI by sending dummy bytes.
 *
 * @param data Pointer to buffer where received data will be stored.
 * @param size Number of bytes to receive.
 * @return Number of bytes successfully received.
 */
uint16_t spi_receive(uint8_t *data, uint16_t size);

/**
 * @brief Wait until SPI bus is idle (TXE=1 and BSY=0).
 *
 * @return TRUE if idle, FALSE on timeout.
 */
bool spi_wait_idle(void);

#endif /* SPI_OPERATIONS_H */
