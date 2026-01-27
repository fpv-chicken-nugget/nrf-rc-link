#ifndef RC_PROTOCOL_PACKET_H
#define RC_PROTOCOL_PACKET_H

#include <stdint.h>
#include <stdbool.h>
#include "config.h"


/**
 * Packet structure:
 * ┌─────────────┬──────────────────────┬────────┐
 * │   Header    │       Payload        │  CRC   │
 * │   5 bytes   │     0-26 bytes       │ 1 byte │
 * └─────────────┴──────────────────────┴────────┘
 *      ↓                  ↓                ↓
 *   Metadata         Actual data       Validation
 */

// packet header
typedef struct __attribute__((packed)) {
    uint8_t version;
    uint8_t type;
    uint8_t sequence;
    uint8_t flags;
    uint8_t payload_len;
} rc_packet_header_t;

// packet type
typedef enum {
    RC_PKT_COMMAND = 0x01,      // command packet
    RC_PKT_TELEMETRY = 0x02,    // telemetry packet
    RC_PKT_HEARTBEAT = 0x03,    // keep-alive packet
    RC_PKT_EMERGENCY = 0x04     // emergency command
} rc_packet_type_t;

// packet flags
#define RC_FLAG_ACK_REQ     (1 << 0)
#define RC_FLAG_EMERGENCY   (1 << 1)

// command payload
typedef struct __attribute__((packed)) {
    uint8_t bytes[RC_COMMAND_PAYLOAD_SIZE];
} rc_command_payload_t;

// telemetry payload
typedef struct __attribute__((packed)) {
    uint8_t bytes[RC_TELEMETRY_PAYLOAD_SIZE];
} rc_telemetry_payload_t;

// complete packet
typedef struct __attribute__((packed)) {
    rc_packet_header_t header;
    uint8_t payload[RC_MAX_PAYLOAD_SIZE - sizeof(rc_packet_header_t) - 1];
    uint8_t crc8;
} rc_packet_t;

// packet API
void rc_packet_init(rc_packet_t *pkt, rc_packet_type_t type, uint8_t seq);
void rc_packet_finalize(rc_packet_t *pkt, uint8_t payload_len);
bool rc_packet_validate(const rc_packet_t *pkt);
uint8_t rc_packet_size(const rc_packet_t *pkt);

// generic payload encoding/decoding
// applications should memcpy with their own struct types
void rc_encode_payload(rc_packet_t *pkt, rc_packet_type_t type,
                       const void *payload, uint8_t payload_len, uint8_t seq);
bool rc_decode_payload(const rc_packet_t *pkt, void *payload, uint8_t expected_len);

// empty packet utility functions
void rc_encode_heartbeat(rc_packet_t *pkt, uint8_t seq);
void rc_encode_emergency(rc_packet_t *pkt, uint8_t seq);
bool rc_is_emergency(const rc_packet_t *pkt);

#endif // RC_PROTOCOL_PACKET_H