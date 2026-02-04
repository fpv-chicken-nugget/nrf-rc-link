#include "link.h"
#include "rc_config.h"

/**
 * @brief RC link state initialization
 *
 * @param link Link state tracking structure
 * @param current_time_ms Current system time in milliseconds
 */
void rc_link_init(rc_link_state_t *link, uint32_t current_time_ms) {
    link->tx_seq = 0;
    link->rx_seq = 0;
    link->last_rx_time_ms = current_time_ms;
    link->missed_packets = 0;
    link->link_active = true;
}

/**
 * @brief Update link state based on packet timeout
 *
 * @param link Link state tracking structure
 * @param current_time_ms Current system time in milliseconds
 */
void rc_link_update(rc_link_state_t *link, uint32_t current_time_ms) {
    uint32_t time_since_rx = current_time_ms - link->last_rx_time_ms;

    if (time_since_rx > RC_PACKET_TIMEOUT_MS) {
        if (link->missed_packets < UINT16_MAX) {
            link->missed_packets++;
        }

        if (link->missed_packets >= RC_LINK_LOSS_THRESHOLD) {
            link->link_active = false;
        }
    }
}

/**
 * @brief Mark a packet as received and reset link state
 *
 * @param link Link state tracking structure
 * @param seq Received packet sequence number
 * @param current_time_ms Current system time in milliseconds
 */
void rc_link_mark_received(rc_link_state_t *link, uint8_t seq, uint32_t current_time_ms) {
    link->last_rx_time_ms = current_time_ms;
    link->missed_packets = 0;
    link->link_active = true;
    link->rx_seq = seq + 1;
}

/**
 * @brief Check if link is active
 *
 * @param link Link state tracking structure
 * @return true if link is active, false otherwise
 */
bool rc_link_is_active(const rc_link_state_t *link) {
    return link->link_active;
}

/**
 * @brief Returns elapsed time since last received packet
 *
 * @param link Link state tracking structure
 * @param current_time_ms Current system time in milliseconds
 * @return elapsed time in milliseconds since last received packet
 */
uint32_t rc_link_time_since_rx(const rc_link_state_t *link, uint32_t current_time_ms) {
    return current_time_ms - link->last_rx_time_ms;
}
