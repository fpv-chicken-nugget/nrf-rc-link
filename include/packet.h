/**
* @file packet.h
 * @brief RC protocol packet definitions
 *
 * Packet structure:
 * ┌─────────────┬──────────────────────┬────────┐
 * │   Header    │       Payload        │  CRC   │
 * │   5 bytes   │     0-26 bytes       │ 1 byte │
 * └─────────────┴──────────────────────┴────────┘
 *      ↓                  ↓                ↓
 *   Metadata         Actual data       Validation
 */

#ifndef PACKET_H
#define PACKET_H

#include <stdint.h>
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

    /*============================================================================*/
    /* Packet Types                                                               */
    /*============================================================================*/

    /**
     * @brief Packet types
     */
    typedef enum {
        RC_PKT_COMMAND   = 0x01,    /* Ground → Aircraft: RC commands */
        RC_PKT_TELEMETRY = 0x02,    /* Aircraft → Ground: Telemetry */
        RC_PKT_ACK       = 0x03,    /* Acknowledgment (future use) */
        RC_PKT_HEARTBEAT = 0x04     /* Keep-alive (future use) */
    } rc_packet_type_t;

    /*============================================================================*/
    /* Packet Structure                                                           */
    /*============================================================================*/

    /**
     * @brief Packet header (5 bytes)
     */
    typedef struct __attribute__((packed)) {
        uint8_t version;            /* Protocol version */
        uint8_t type;               /* Packet type */
        uint8_t sequence;           /* Sequence number (wraps at 255) */
        uint8_t flags;              /* Status flags (reserved) */
        uint8_t payload_len;        /* Payload length in bytes */
    } rc_packet_header_t;

    /**
     * @brief Complete packet (32 bytes)
     */
    typedef struct __attribute__((packed)) {
        rc_packet_header_t header;              /* 5 bytes */
        uint8_t payload[RC_MAX_PAYLOAD_SIZE];   /* 26 bytes */
        uint8_t crc8;                           /* 1 byte */
    } rc_packet_t;

    /* Compile-time validation */
    _Static_assert(sizeof(rc_packet_t) == 32, "Packet must be exactly 32 bytes");

#ifdef __cplusplus
}
#endif

#endif /* PACKET_H */