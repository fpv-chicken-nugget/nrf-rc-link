/**
 * RC Protocol User Configuration
 * Edit this file according to your application's requirements.
 */

#ifndef RC_CONFIG_H
#define RC_CONFIG_H

#include <stdint.h>

// COMMUNICATION SETTINGS

/**
 * Link timeout in milliseconds
 * If no packet received for this duration, link is considered lost
 */
#ifndef RC_LINK_TIMEOUT_MS
#define RC_LINK_TIMEOUT_MS          1000
#endif

/**
 * Number of consecutive missed packets before declaring link loss
 * Verified against sequence number received in packet header
 */
#ifndef RC_LINK_LOSS_THRESHOLD
#define RC_LINK_LOSS_THRESHOLD      10
#endif

/**
 * Update rate in Hz
 */
#ifndef RC_UPDATE_RATE_HZ
#define RC_UPDATE_RATE_HZ           50
#endif

/**
 * RF Channel (0-125)
 * Frequency = 2400 + RC_RF_CHANNEL MHz
 * Use 2MHz spacing at 2Mbps: 2, 26, 50, 76, 98, 122
 */
#ifndef RC_RF_CHANNEL
#define RC_RF_CHANNEL               76
#endif

/**
 * Air Data Rate
 */
typedef enum {
 RC_DATA_RATE_250KBPS = 0,
 RC_DATA_RATE_1MBPS = 1,
 RC_DATA_RATE_2MBPS = 2
} rc_data_rate_t;

#ifndef RC_DATA_RATE
#define RC_DATA_RATE                RC_DATA_RATE_2MBPS
#endif

/**
 * TX Power Level
 */
typedef enum {
 RC_TX_POWER_MIN = 0,        // -18dBm
 RC_TX_POWER_LOW = 1,        // -12dBm
 RC_TX_POWER_MED = 2,        // -6dBm
 RC_TX_POWER_MAX = 3         // 0dBm
} rc_tx_power_t;

#ifndef RC_TX_POWER
#define RC_TX_POWER                 RC_TX_POWER_MAX
#endif

// PAYLOAD STRUCTURES

/**
 * Maximum payload size
 *
 * nRF24 packet = 32 bytes:
 *   - Header: 5 bytes (version, type, seq, flags, len)
 *   - Payload: 26 bytes
 *   - CRC8: 1 byte
 */
#define RC_MAX_PAYLOAD_SIZE 26

/**
 * RC Command Payload
 *
 * Define your application's command structure here.
 *
 * CONSTRAINTS:
 * - Maximum size: 26 bytes
 */
typedef struct __attribute__((packed)) {
 uint16_t channels[8];       // 8 RC channels, 0-2047 (11-bit resolution)
 uint8_t switches;           // 8 binary switches as bits (bit 0-7)
 uint8_t mode;               // Flight mode (0-255)

 // Total: 8*2 + 1 + 1 = 18 bytes
 // You have 26 - 18 = 8 bytes remaining for custom fields!

 // Add your custom fields here:
 // uint16_t gimbal_pan;     // Example: gimbal control
 // uint16_t gimbal_tilt;
 // uint8_t camera_trigger;
 // uint8_t aux_flags;
} rc_command_payload_t;

/**
 * Telemetry Payload
 *
 * Define your application's telemetry structure here.
 *
 * CONSTRAINTS:
 * - Maximum size: 26 bytes
 */
typedef struct __attribute__((packed)) {
 // GPS data (13 bytes)
 int32_t gps_lat;            // Latitude * 1e7 (4 bytes)
 int32_t gps_lon;            // Longitude * 1e7 (4 bytes)
 int16_t gps_alt;            // Altitude in meters (2 bytes)
 uint16_t groundspeed;       // Ground speed in cm/s (2 bytes)
 uint8_t gps_sats;           // Number of satellites (1 byte)

 // Power (4 bytes)
 uint16_t battery_mv;        // Battery voltage in millivolts (2 bytes)
 uint16_t current_ma;        // Current draw in milliamps (2 bytes)

 // Orientation (2 bytes)
 int16_t heading;            // Heading in degrees * 10 (2 bytes)

 // Status (3 bytes)
 uint8_t flight_mode;        // Current flight mode (1 byte)
 uint8_t rssi;               // Signal strength 0-100% (1 byte)
 uint8_t error_flags;        // Error status bits (1 byte)

 // Total: 13 + 4 + 2 + 3 = 22 bytes
 // You have 26 - 22 = 4 bytes remaining!

 // Add your custom fields here:
 // int16_t altitude_baro;   // Example: barometric altitude
 // uint8_t fuel_percent;
 // uint8_t custom_status;
} rc_telemetry_payload_t;

/**
 * Failsafe Command Values
 *
 * These values are used when link is lost.
 * Define safe defaults for your aircraft.
 */
#ifndef RC_FAILSAFE_COMMAND
#define RC_FAILSAFE_COMMAND { \
    .channels = {1024, 1024, 0, 1024, 1024, 1024, 1024, 1024}, \
    .switches = 0, \
    .mode = 0 \
}
#endif

// COMPILE-TIME VALIDATION

_Static_assert(sizeof(rc_command_payload_t) <= RC_MAX_PAYLOAD_SIZE, "rc_command_payload_t exceeds RC_MAX_PAYLOAD_SIZE.");

_Static_assert(sizeof(rc_telemetry_payload_t) <= RC_MAX_PAYLOAD_SIZE, "rc_telemetry_payload_t exceeds RC_MAX_PAYLOAD_SIZE.");

// OPTIONAL FEATURES

/**
 * Enable statistics tracking
 * Tracks packet loss, link quality, latency, etc.
 */
#ifndef RC_ENABLE_STATISTICS
#define RC_ENABLE_STATISTICS        1
#endif

/**
 * Protocol version
 * Mismatched versions will reject packets
 */
#define RC_PROTOCOL_VERSION         1

/**
 * Maximum retransmit attempts
 */
#ifndef RC_AUTO_RETRANSMIT_COUNT
#define RC_AUTO_RETRANSMIT_COUNT    3
#endif

/**
 * Auto-retransmit delay
 *
 * Delay between retransmits in microseconds (value + 1) * 250us
 */
#ifndef RC_AUTO_RETRANSMIT_DELAY
#define RC_AUTO_RETRANSMIT_DELAY    1  // 500us
#endif

#endif // RC_CONFIG_H