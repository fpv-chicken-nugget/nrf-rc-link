/**
 * @file config.h
 * @brief RC protocol configuration
 *
 * User-configurable protocol parameters.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

/*============================================================================*/
/* Timing Configuration                                                       */
/*============================================================================*/

/** Link timeout in milliseconds */
#ifndef RC_LINK_TIMEOUT_MS
#define RC_LINK_TIMEOUT_MS          1000
#endif

/** Consecutive missed packets before link loss */
#ifndef RC_LINK_LOSS_THRESHOLD
#define RC_LINK_LOSS_THRESHOLD      10
#endif

/** Update rate in Hz */
#ifndef RC_UPDATE_RATE_HZ
#define RC_UPDATE_RATE_HZ           50
#endif

/*============================================================================*/
/* RF Configuration                                                           */
/*============================================================================*/

/** RF channel (0-125) */
#ifndef RC_RF_CHANNEL
#define RC_RF_CHANNEL               76
#endif

/** TX power level (0-3) */
#ifndef RC_TX_POWER
#define RC_TX_POWER                 3
#endif

/** Data rate (0=250k, 1=1M, 2=2M) */
#ifndef RC_DATA_RATE
#define RC_DATA_RATE                2
#endif

/** Auto-retransmit count (0-15) */
#ifndef RC_AUTO_RETRANSMIT_COUNT
#define RC_AUTO_RETRANSMIT_COUNT    3
#endif

/** Auto-retransmit delay (0-15) -> (value+1)*250µs */
#ifndef RC_AUTO_RETRANSMIT_DELAY
#define RC_AUTO_RETRANSMIT_DELAY    1
#endif

/*============================================================================*/
/* Payload Structures                                                         */
/*============================================================================*/

/** Maximum payload size (nRF24 packet = 32 bytes - 5 header - 1 CRC) */
#define RC_MAX_PAYLOAD_SIZE         26

/**
 * @brief RC command payload (ground → aircraft)
 */
typedef struct __attribute__((packed)) {
    uint16_t channels[8];       /* RC channels 0-2047 */
    uint8_t switches;           /* 8 binary switches */
    uint8_t mode;               /* Flight mode */
} rc_command_payload_t;

/**
 * @brief Telemetry payload (aircraft → ground)
 */
typedef struct __attribute__((packed)) {
    int32_t gps_lat;            /* Latitude * 1e7 */
    int32_t gps_lon;            /* Longitude * 1e7 */
    int16_t gps_alt;            /* Altitude (m) */
    uint16_t groundspeed;       /* Speed (cm/s) */
    uint8_t gps_sats;           /* Satellite count */
    uint16_t battery_mv;        /* Battery (mV) */
    uint16_t current_ma;        /* Current (mA) */
    int16_t heading;            /* Heading (deg*10) */
    uint8_t flight_mode;        /* Flight mode */
    uint8_t rssi;               /* Signal strength */
    uint8_t error_flags;        /* Error bits */
} rc_telemetry_payload_t;

/** Failsafe command values */
#ifndef RC_FAILSAFE_COMMAND
#define RC_FAILSAFE_COMMAND { \
    .channels = {1024, 1024, 0, 1024, 1024, 1024, 1024, 1024}, \
    .switches = 0, \
    .mode = 0 \
}
#endif

/* Compile-time validation */
_Static_assert(sizeof(rc_command_payload_t) <= RC_MAX_PAYLOAD_SIZE,
               "Command payload too large");
_Static_assert(sizeof(rc_telemetry_payload_t) <= RC_MAX_PAYLOAD_SIZE,
               "Telemetry payload too large");

/*============================================================================*/
/* Optional Features                                                          */
/*============================================================================*/

/** Enable statistics tracking */
#ifndef RC_ENABLE_STATISTICS
#define RC_ENABLE_STATISTICS        1
#endif

/** Enable debug logging */
#ifndef RC_ENABLE_LOGGING
#define RC_ENABLE_LOGGING           0
#endif

/** Protocol version */
#define RC_PROTOCOL_VERSION         1

#endif /* CONFIG_H */