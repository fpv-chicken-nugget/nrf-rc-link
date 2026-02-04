/**
 * RC Protocol Driver API
 *
 * Hardware-agnostic RC link driver for nRF24L01+ radios.
 *
 * FEATURES:
 * - Bidirectional communication for command and telemetry
 * - Automatic failsafe on link loss
 * - CRC validation
 * - Link quality monitoring
 * - Configurable RF parameters
 *
 * USAGE:
 * 1. Configure rc_config.h with your payload structures
 * 2. Initialize driver with hardware config
 * 3. Call rc_link_update() in main loop
 * 4. Send/receive using API functions
 *
 * See README for more details
 */

#ifndef RC_DRIVER_H
#define RC_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include "rc_config.h"

#ifdef __cplusplus
extern "C" {
#endif

// STATUS CODES

typedef enum {
    RC_OK = 0,                  // success
    RC_ERROR_INVALID_PARAM,     // invalid parameter passed to function
    RC_ERROR_TIMEOUT,           // operation timed out
    RC_ERROR_NO_DATA,           // no data available
    RC_ERROR_CRC_FAIL,          // packet CRC validation failed
    RC_ERROR_VERSION_MISMATCH,  // protocol version mismatch
    RC_ERROR_BUFFER_FULL,       // transmit buffer full, cannot send
    RC_ERROR_HARDWARE,          // hardware/SPI communication error
    RC_ERROR_NOT_INITIALIZED,   // driver not initialized
    RC_ERROR_PACKET_TOO_LARGE   // payload exceeds maximum size
} rc_status_t;

// HARDWARE CONFIG

/**
 * Hardware configuration structure
 *
 * Passed to rc_driver_init() to configure GPIO pins and peripherals.
 */
typedef struct {
    void *spi_handle;
    // nrf CSN pin
    void *csn_port;
    uint16_t csn_pin;
    // nrf CE pin
    void *ce_port;
    uint16_t ce_pin;

    // platform timing function
    uint32_t (*get_tick_ms)(void);
} rc_hardware_config_t;

// RF CONFIG

/**
 * RF configuration structure
 * These parameters can be changed at runtime using rc_link_set_rf_config()
 */
typedef struct {
    uint8_t channel;            // RF channel (0-125)
    rc_tx_power_t tx_power;     // TX power level
    rc_data_rate_t data_rate;   // air data rate
} rc_rf_config_t;

// DRIVER STATISTICS

#if RC_ENABLE_STATISTICS
typedef struct {
    uint32_t packets_sent;          // total packets transmitted
    uint32_t packets_received;      // total packets received successfully
    uint32_t packets_missed;        // packets missed
    uint32_t crc_errors;            // packets rejected due to CRC failure
    uint32_t version_mismatches;    // packets rejected due to version mismatch
    uint16_t last_latency_ms;       // latency of last received packet
    uint8_t link_quality;           // link quality percentage
} rc_stats_t;
#endif

// DRIVER HANDLE

/**
 * Opaque driver handle, to be allocated by application
 */
typedef struct rc_driver rc_link_t;

// INITIALIZATION

/**
 * @brief Initializes the RC driver
 *
 * Configures the nRF24L01+ radio and prepares driver for use.
 * Must be called before any other driver functions.
 *
 * @param driver Driver handle
 * @param hw_config Hardware configuration
 * @return RC_OK on success, error code otherwise
 */
rc_status_t rc_link_init(rc_link_t *driver, const rc_hardware_config_t *hw_config);

/**
 * @brief Deinitialize driver and power down radio
 *
 * @param link Driver handle
 */
void rc_link_deinit(rc_link_t *link);

/**
 * @brief Set RF configuration
 *
 * Can be called at runtime to change channel, power, etc.
 *
 * @param link Driver handle
 * @param rf_config RF parameters
 * @return RC_OK on success
 */
rc_status_t rc_link_set_rf_config(rc_link_t *link, const rc_rf_config_t *rf_config);

/**
 * @brief Get current RF configuration
 *
 * @param link Driver handle
 * @param rf_config Output buffer
 * @return RC_OK on success
 */
rc_status_t rc_link_get_rf_config(rc_link_t *link, rc_rf_config_t *rf_config);

// GROUNDSIDE API

/**
 * @brief Send RC command to aircraft
 *
 * Encodes and transmits command payload to aircraft.
 *
 * @param link Driver handle
 * @param command Command payload
 * @return RC_OK if sent successfully
 *         RC_ERROR_TIMEOUT if previous transmission not complete
 *         RC_ERROR_HARDWARE on SPI error
 */
rc_status_t rc_driver_send_command(rc_link_t *link, const rc_command_payload_t *command);

/**
 * @brief Receive telemetry from aircraft
 *
 * Checks for available telemetry and decodes if present.
 *
 * @param link Driver handle
 * @param telemetry Output buffer for telemetry data
 * @return RC_OK if telemetry received
 *         RC_ERROR_NO_DATA if no telemetry available
 *         RC_ERROR_CRC_FAIL if packet corrupted
 *         RC_ERROR_VERSION_MISMATCH if protocol mismatch
 */
rc_status_t rc_driver_receive_telemetry(rc_link_t *link, rc_telemetry_payload_t *telemetry);

// AIRCRAFT API

/**
 * @brief Receive RC command from ground
 *
 * Checks for available command and decodes if present.
 * If link is lost, returns failsafe values automatically.
 *
 * @param link Driver handle
 * @param command Output buffer for command data
 * @return RC_OK if command received (or failsafe active)
 *         RC_ERROR_NO_DATA if no command available
 *         RC_ERROR_CRC_FAIL if packet corrupted
 */
rc_status_t rc_driver_receive_command(rc_link_t *link, rc_command_payload_t *command);

/**
 * @brief Send telemetry to ground station
 *
 * Encodes and transmits telemetry payload.
 *
 * @param link Driver handle
 * @param telemetry Telemetry payload (user-defined in rc_config.h)
 * @return RC_OK if sent successfully
 */
rc_status_t rc_driver_send_telemetry(rc_link_t *link, const rc_telemetry_payload_t *telemetry);

// COMMON API

/**
 * @brief Update driver state machine
 *
 * MUST be called regularly.
 * Handles:
 * - RX/TX mode switching
 * - Timeout detection
 * - Link quality updates
 *
 * @param link Driver handle
 * @return RC_OK on success
 */
rc_status_t rc_link_update(rc_link_t *link);

/**
 * @brief Check if link is active
 *
 * Link is active if packets received within RC_LINK_TIMEOUT_MS.
 *
 * @param link Driver handle
 * @return true if link active, false if lost
 */
bool rc_link_is_link_active(rc_link_t *link);

/**
 * @brief Get time since last received packet
 *
 * @param link Driver handle
 * @return Milliseconds since last RX, or UINT32_MAX if never received
 */
uint32_t rc_link_get_time_since_rx(rc_link_t *link);

/**
 * @brief Set failsafe values
 *
 * These values are used when link is lost.
 *
 * @param link Driver handle
 * @param failsafe Failsafe command values
 * @return RC_OK on success
 */
rc_status_t rc_link_set_failsafe(rc_link_t *link, const rc_command_payload_t *failsafe);

/**
 * @brief Get current failsafe values
 *
 * @param link Driver handle
 * @param failsafe Output buffer
 * @return RC_OK on success
 */
rc_status_t rc_link_get_failsafe(rc_link_t *link, rc_command_payload_t *failsafe);

// STATISTICS API

#if RC_ENABLE_STATISTICS
/**
 * Get link statistics
 *
 * Returns packet counts, link quality, latency, etc.
 *
 * @param driver Driver handle
 * @param stats Output buffer for statistics
 * @return RC_OK on success
 */
rc_status_t rc_link_get_stats(rc_link_t *driver, rc_stats_t *stats);

/**
 * @brief Reset statistics counters to zero
 *
 * @param link Driver handle
 */
void rc_link_reset_stats(rc_link_t *link);
#endif

#ifdef __cplusplus
}
#endif

#endif // RC_DRIVER_H