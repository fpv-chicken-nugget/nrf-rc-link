#ifndef RC_PROTOCOL_LINK_H
#define RC_PROTOCOL_LINK_H

#include <stdint.h>
#include <stdbool.h>

// link state tracking
typedef struct {
    uint8_t tx_seq;     // sequence number for next packet
    uint8_t rx_seq;     // used to detect missing packets
    uint32_t last_rx_time_ms;
    uint16_t missed_packets;
    bool link_active;
} rc_link_state_t;

// link management
void rc_link_init(rc_link_state_t *link, uint32_t current_time_ms);
void rc_link_update(rc_link_state_t *link, uint32_t current_time_ms);
void rc_link_mark_received(rc_link_state_t *link, uint8_t seq, uint32_t current_time_ms);
void rc_link_mark_error(rc_link_state_t *link);

// link queries
bool rc_link_is_active(const rc_link_state_t *link);
uint32_t rc_link_time_since_rx(const rc_link_state_t *link, uint32_t current_time_ms);

#endif // RC_PROTOCOL_LINK_H
