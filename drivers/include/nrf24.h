/**
 * @file nrf24.h
 * @brief nRF24L01+ driver for STM32
 *
 * Low-level driver for nRF24L01+ 2.4GHz transceiver.
 * Provides register access, mode control, and data transfer.
 */

#ifndef NRF24_H
#define NRF24_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*/
/* Public Types                                                               */
/*============================================================================*/

/**
 * @brief nRF24 data rates
 */
typedef enum {
    NRF24_DATA_RATE_250KBPS = 0,
    NRF24_DATA_RATE_1MBPS   = 1,
    NRF24_DATA_RATE_2MBPS   = 2
} nrf24_data_rate_t;

/**
 * @brief nRF24 TX power levels
 */
typedef enum {
    NRF24_TX_POWER_N18DBM = 0,
    NRF24_TX_POWER_N12DBM = 1,
    NRF24_TX_POWER_N6DBM  = 2,
    NRF24_TX_POWER_0DBM   = 3
} nrf24_tx_power_t;

/**
 * @brief nRF24 driver handle
 */
typedef struct {
    uint8_t channel;            /* RF channel (0-125) */
    uint8_t payload_size;       /* Payload size in bytes (1-32) */
    bool is_rx_mode;            /* Current mode: true=RX, false=TX */
    bool initialized;           /* Initialization status */
} nrf24_t;

/*============================================================================*/
/* Initialization & Configuration                                             */
/*============================================================================*/

/**
 * @brief Initialize nRF24L01+
 *
 * Configures the nRF24 module with default settings and powers it up.
 * Must be called before any other nRF24 functions.
 *
 * @param nrf         Pointer to nRF24 handle
 * @param channel     RF channel (0-125)
 * @param payload_size Payload size in bytes (1-32)
 * @return true if initialization successful
 */
bool nrf24_init(nrf24_t *nrf, uint8_t channel, uint8_t payload_size);

/**
 * @brief Set RF channel
 *
 * @param nrf     Pointer to nRF24 handle
 * @param channel RF channel (0-125) -> 2400-2525 MHz
 */
void nrf24_set_channel(nrf24_t *nrf, uint8_t channel);

/**
 * @brief Set TX power level
 *
 * @param nrf   Pointer to nRF24 handle
 * @param power TX power level
 */
void nrf24_set_tx_power(nrf24_t *nrf, nrf24_tx_power_t power);

/**
 * @brief Set data rate
 *
 * @param nrf  Pointer to nRF24 handle
 * @param rate Data rate
 */
void nrf24_set_data_rate(nrf24_t *nrf, nrf24_data_rate_t rate);

/**
 * @brief Set TX and RX addresses
 *
 * @param nrf     Pointer to nRF24 handle
 * @param tx_addr TX address (5 bytes)
 * @param rx_addr RX address for pipe 0 (5 bytes)
 *
 * @note For auto-ACK to work, TX address must match RX pipe 0 address
 */
void nrf24_set_addresses(nrf24_t *nrf, const uint8_t *tx_addr, const uint8_t *rx_addr);

/**
 * @brief Set auto-retransmit parameters
 *
 * @param nrf   Pointer to nRF24 handle
 * @param delay Delay between retries (0-15) -> (delay+1)*250µs
 * @param count Max retry count (0-15)
 */
void nrf24_set_auto_retransmit(nrf24_t *nrf, uint8_t delay, uint8_t count);

/*============================================================================*/
/* Mode Control                                                               */
/*============================================================================*/

/**
 * @brief Enter TX mode
 *
 * @param nrf Pointer to nRF24 handle
 */
void nrf24_mode_tx(nrf24_t *nrf);

/**
 * @brief Enter RX mode and start listening
 *
 * @param nrf Pointer to nRF24 handle
 */
void nrf24_mode_rx(nrf24_t *nrf);

/**
 * @brief Power down the nRF24
 *
 * @param nrf Pointer to nRF24 handle
 */
void nrf24_power_down(nrf24_t *nrf);

/*============================================================================*/
/* Data Transfer                                                              */
/*============================================================================*/

/**
 * @brief Transmit packet
 *
 * Switches to TX mode, sends packet, and waits for ACK (if enabled).
 *
 * @param nrf  Pointer to nRF24 handle
 * @param data Data buffer to transmit
 * @param len  Data length (must equal payload_size)
 * @return true if transmission successful
 *
 * @note Blocking call with timeout (~10ms)
 */
bool nrf24_transmit(nrf24_t *nrf, const uint8_t *data, uint8_t len);

/**
 * @brief Receive packet (non-blocking)
 *
 * Switches to RX mode and checks for available data.
 *
 * @param nrf    Pointer to nRF24 handle
 * @param buffer Output buffer for received data
 * @param len    Output: received data length
 * @return true if packet received
 */
bool nrf24_receive(nrf24_t *nrf, uint8_t *buffer, uint8_t *len);

/**
 * @brief Check if RX data is available
 *
 * @param nrf Pointer to nRF24 handle
 * @return true if data available in RX FIFO
 */
bool nrf24_is_data_available(nrf24_t *nrf);

/*============================================================================*/
/* Low-Level Register Access                                                  */
/*============================================================================*/

/**
 * @brief Read nRF24 register
 *
 * @param nrf Pointer to nRF24 handle
 * @param reg Register address
 * @return Register value
 */
uint8_t nrf24_read_register(nrf24_t *nrf, uint8_t reg);

/**
 * @brief Write nRF24 register
 *
 * @param nrf   Pointer to nRF24 handle
 * @param reg   Register address
 * @param value Value to write
 */
void nrf24_write_register(nrf24_t *nrf, uint8_t reg, uint8_t value);

/**
 * @brief Get status register
 *
 * @param nrf Pointer to nRF24 handle
 * @return Status register value
 */
uint8_t nrf24_get_status(nrf24_t *nrf);

/**
 * @brief Clear interrupt flags
 *
 * @param nrf Pointer to nRF24 handle
 */
void nrf24_clear_interrupts(nrf24_t *nrf);

/**
 * @brief Flush TX FIFO
 *
 * @param nrf Pointer to nRF24 handle
 */
void nrf24_flush_tx(nrf24_t *nrf);

/**
 * @brief Flush RX FIFO
 *
 * @param nrf Pointer to nRF24 handle
 */
void nrf24_flush_rx(nrf24_t *nrf);

#ifdef __cplusplus
}
#endif

#endif /* NRF24_H */