/**
 * @file nrf_rc_driver.h
 * @brief RC link protocol driver
 *
 * High-level API for bidirectional RC communication.
 */

#ifndef NRF_RC_DRIVER_H
#define NRF_RC_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*/
/* Status Codes                                                               */
/*============================================================================*/

typedef enum {
    RC_OK = 0,                  /* Success */
    RC_ERROR_INVALID_PARAM,     /* Invalid parameter */
    RC_ERROR_TIMEOUT,           /* Operation timed out */
    RC_ERROR_NO_DATA,           /* No data available */
    RC_ERROR_CRC_FAIL,          /* CRC validation failed */
    RC_ERROR_VERSION_MISMATCH,  /* Protocol version mismatch */
    RC_ERROR_HARDWARE,          /* Hardware/SPI error */
    RC_ERROR_NOT_INITIALIZED    /* Driver not initialized */
} rc_status_t;

/*============================================================================*/
/* Hardware Configuration                                                     */
/*============================================================================*/

/**
 * @brief Hardware configuration
 *
 * All pin/peripheral configuration is in nrf24_config.h.
 * This structure only needs timing function.
 */
typedef struct {
    uint32_t (*get_tick_ms)(void);  /* Millisecond tick function */
} rc_hardware_config_t;

/*============================================================================*/
/* Statistics                                                                 */
/*============================================================================*/

#if RC_ENABLE_STATISTICS
typedef struct {
    uint32_t packets_sent;          /* Total packets transmitted */
    uint32_t packets_received;      /* Total packets received */
    uint32_t packets_missed;        /* Packets missed (sequence gaps) */
    uint32_t crc_errors;            /* CRC validation failures */
    uint32_t version_mismatches;    /* Protocol version mismatches */
    uint8_t link_quality;           /* Link quality 0-100% */
} rc_stats_t;
#endif

/*============================================================================*/
/* Driver Handle                                                              */
/*============================================================================*/

typedef struct rc_link rc_link_t;

/*============================================================================*/
/* Initialization                                                             */
/*============================================================================*/

/**
 * @brief Initialize RC link
 *
 * @param link      Pointer to link handle
 * @param hw_config Hardware configuration
 * @return RC_OK on success
 */
rc_status_t rc_link_init(rc_link_t *link, const rc_hardware_config_t *hw_config);

/**
 * @brief Deinitialize RC link
 *
 * @param link Pointer to link handle
 */
void rc_link_deinit(rc_link_t *link);

/*============================================================================*/
/* Ground Station API                                                         */
/*============================================================================*/

/**
 * @brief Send RC command to aircraft
 *
 * @param link    Pointer to link handle
 * @param command Command payload
 * @return RC_OK if sent successfully
 */
rc_status_t rc_link_send_command(rc_link_t *link, const rc_command_payload_t *command);

/**
 * @brief Receive telemetry from aircraft
 *
 * @param link      Pointer to link handle
 * @param telemetry Output buffer for telemetry
 * @return RC_OK if received, RC_ERROR_NO_DATA if none available
 */
rc_status_t rc_link_receive_telemetry(rc_link_t *link, rc_telemetry_payload_t *telemetry);

/*============================================================================*/
/* Aircraft API                                                               */
/*============================================================================*/

/**
 * @brief Receive RC command from ground
 *
 * Returns failsafe values automatically if link is lost.
 *
 * @param link    Pointer to link handle
 * @param command Output buffer for command
 * @return RC_OK if received (or failsafe active)
 */
rc_status_t rc_link_receive_command(rc_link_t *link, rc_command_payload_t *command);

/**
 * @brief Send telemetry to ground station
 *
 * @param link      Pointer to link handle
 * @param telemetry Telemetry payload
 * @return RC_OK if sent successfully
 */
rc_status_t rc_link_send_telemetry(rc_link_t *link, const rc_telemetry_payload_t *telemetry);

/*============================================================================*/
/* Common API                                                                 */
/*============================================================================*/

/**
 * @brief Update driver state machine
 *
 * Must be called regularly in main loop.
 *
 * @param link Pointer to link handle
 * @return RC_OK on success
 */
rc_status_t rc_link_update(rc_link_t *link);

/**
 * @brief Check if link is active
 *
 * @param link Pointer to link handle
 * @return true if link active
 */
bool rc_link_is_active(rc_link_t *link);

/**
 * @brief Get time since last received packet
 *
 * @param link Pointer to link handle
 * @return Milliseconds since last RX, or UINT32_MAX if never received
 */
uint32_t rc_link_get_time_since_rx(rc_link_t *link);

/**
 * @brief Set failsafe values
 *
 * @param link     Pointer to link handle
 * @param failsafe Failsafe command values
 * @return RC_OK on success
 */
rc_status_t rc_link_set_failsafe(rc_link_t *link, const rc_command_payload_t *failsafe);

/**
 * @brief Get failsafe values
 *
 * @param link     Pointer to link handle
 * @param failsafe Output buffer
 * @return RC_OK on success
 */
rc_status_t rc_link_get_failsafe(rc_link_t *link, rc_command_payload_t *failsafe);

/*============================================================================*/
/* Statistics API                                                             */
/*============================================================================*/

#if RC_ENABLE_STATISTICS
/**
 * @brief Get link statistics
 *
 * @param link  Pointer to link handle
 * @param stats Output buffer
 * @return RC_OK on success
 */
rc_status_t rc_link_get_stats(rc_link_t *link, rc_stats_t *stats);

/**
 * @brief Reset statistics
 *
 * @param link Pointer to link handle
 */
void rc_link_reset_stats(rc_link_t *link);
#endif

/*============================================================================*/
/* Logging                                                                    */
/*============================================================================*/

#if RC_ENABLE_LOGGING
/**
 * @brief Logging function (user must implement)
 *
 * Example:
 *   void rc_log(const char *fmt, ...) {
 *       char buf[128];
 *       va_list args;
 *       va_start(args, fmt);
 *       vsnprintf(buf, sizeof(buf), fmt, args);
 *       va_end(args);
 *       HAL_UART_Transmit(&huart1, (uint8_t*)buf, strlen(buf), 100);
 *   }
 */
void rc_log(const char *fmt, ...);

#define RC_LOG_ERROR(...)   rc_log("[ERROR] " __VA_ARGS__)
#define RC_LOG_WARN(...)    rc_log("[WARN]  " __VA_ARGS__)
#define RC_LOG_INFO(...)    rc_log("[INFO]  " __VA_ARGS__)
#define RC_LOG_DEBUG(...)   rc_log("[DEBUG] " __VA_ARGS__)
#else
#define RC_LOG_ERROR(...)
#define RC_LOG_WARN(...)
#define RC_LOG_INFO(...)
#define RC_LOG_DEBUG(...)
#endif

#ifdef __cplusplus
}
#endif

#endif /* NRF_RC_DRIVER_H */