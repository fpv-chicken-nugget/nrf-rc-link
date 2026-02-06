/**
 * @file nrf_rc_driver.c
 * @brief RC link protocol implementation
 */

#include "nrf_rc_driver.h"
#include "packet.h"
#include "crc.h"
#include "../drivers/include/nrf24.h"
#include <string.h>

/*============================================================================*/
/* Private Types                                                              */
/*============================================================================*/

/**
 * @brief Link role
 */
typedef enum {
    RC_ROLE_GROUND,     /* Ground station */
    RC_ROLE_AIRCRAFT    /* Aircraft */
} rc_role_t;

/**
 * @brief Link state
 */
struct rc_link {
    /* Hardware */
    rc_hardware_config_t hw;
    nrf24_t nrf24;

    /* Protocol state */
    rc_role_t role;
    bool initialized;

    /* Sequencing */
    uint8_t tx_sequence;
    uint8_t rx_sequence_last;

    /* Link state */
    uint32_t last_rx_time;
    bool link_active;
    uint8_t consecutive_missed;

    /* Failsafe */
    rc_command_payload_t failsafe_command;
    bool failsafe_active;

    /* Buffers */
    rc_packet_t tx_packet;
    rc_packet_t rx_packet;

#if RC_ENABLE_STATISTICS
    rc_stats_t stats;
#endif
};

/*============================================================================*/
/* Private Function Prototypes                                                */
/*============================================================================*/

static void update_link_state(rc_link_t *link);
static void calculate_link_quality(rc_link_t *link);
static rc_status_t encode_and_send(rc_link_t *link, rc_packet_type_t type,
                                    const void *payload, uint8_t payload_len);
static rc_status_t receive_and_decode(rc_link_t *link, rc_packet_type_t expected_type,
                                       void *payload, uint8_t *payload_len);

/*============================================================================*/
/* Initialization                                                             */
/*============================================================================*/

rc_status_t rc_link_init(rc_link_t *link, const rc_hardware_config_t *hw_config)
{
    if (!link || !hw_config || !hw_config->get_tick_ms) {
        return RC_ERROR_INVALID_PARAM;
    }

    /* Clear structure */
    memset(link, 0, sizeof(rc_link_t));

    /* Copy hardware config */
    memcpy(&link->hw, hw_config, sizeof(rc_hardware_config_t));

    /* Initialize nRF24 */
    if (!nrf24_init(&link->nrf24, RC_RF_CHANNEL, 32)) {
        RC_LOG_ERROR("nRF24 initialization failed\n");
        return RC_ERROR_HARDWARE;
    }

    /* Configure nRF24 */
    nrf24_set_tx_power(&link->nrf24, (nrf24_tx_power_t)RC_TX_POWER);
    nrf24_set_data_rate(&link->nrf24, (nrf24_data_rate_t)RC_DATA_RATE);
    nrf24_set_auto_retransmit(&link->nrf24, RC_AUTO_RETRANSMIT_DELAY,
                              RC_AUTO_RETRANSMIT_COUNT);

    /* Set default addresses */
    uint8_t addr[5] = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7};
    nrf24_set_addresses(&link->nrf24, addr, addr);

    /* Initialize state */
    link->role = RC_ROLE_GROUND;
    link->tx_sequence = 0;
    link->rx_sequence_last = 0;
    link->last_rx_time = UINT32_MAX;
    link->link_active = false;
    link->consecutive_missed = 0;
    link->failsafe_active = false;

    /* Set default failsafe */
    rc_command_payload_t default_failsafe = RC_FAILSAFE_COMMAND;
    memcpy(&link->failsafe_command, &default_failsafe, sizeof(rc_command_payload_t));

#if RC_ENABLE_STATISTICS
    memset(&link->stats, 0, sizeof(rc_stats_t));
#endif

    link->initialized = true;

    RC_LOG_INFO("RC link initialized (ch=%d, pwr=%d, rate=%d)\n",
                RC_RF_CHANNEL, RC_TX_POWER, RC_DATA_RATE);

    return RC_OK;
}

void rc_link_deinit(rc_link_t *link)
{
    if (!link || !link->initialized) {
        return;
    }

    nrf24_power_down(&link->nrf24);
    link->initialized = false;

    RC_LOG_INFO("RC link deinitialized\n");
}

/*============================================================================*/
/* Ground Station API                                                         */
/*============================================================================*/

rc_status_t rc_link_send_command(rc_link_t *link, const rc_command_payload_t *command)
{
    if (!link || !link->initialized || !command) {
        return RC_ERROR_INVALID_PARAM;
    }

    link->role = RC_ROLE_GROUND;

    rc_status_t status = encode_and_send(link, RC_PKT_COMMAND, command,
                                         sizeof(rc_command_payload_t));

    if (status == RC_OK) {
        link->tx_sequence++;

#if RC_ENABLE_STATISTICS
        link->stats.packets_sent++;
#endif

        RC_LOG_DEBUG("Command sent (seq=%d)\n", link->tx_sequence - 1);
    }

    return status;
}

rc_status_t rc_link_receive_telemetry(rc_link_t *link, rc_telemetry_payload_t *telemetry)
{
    if (!link || !link->initialized || !telemetry) {
        return RC_ERROR_INVALID_PARAM;
    }

    uint8_t payload_len = 0;
    rc_status_t status = receive_and_decode(link, RC_PKT_TELEMETRY, telemetry, &payload_len);

    if (status == RC_OK) {
        link->last_rx_time = link->hw.get_tick_ms();

#if RC_ENABLE_STATISTICS
        link->stats.packets_received++;
#endif

        RC_LOG_DEBUG("Telemetry received (seq=%d)\n", link->rx_packet.header.sequence);
    }

    return status;
}

/*============================================================================*/
/* Aircraft API                                                               */
/*============================================================================*/

rc_status_t rc_link_receive_command(rc_link_t *link, rc_command_payload_t *command)
{
    if (!link || !link->initialized || !command) {
        return RC_ERROR_INVALID_PARAM;
    }

    link->role = RC_ROLE_AIRCRAFT;

    uint8_t payload_len = 0;
    rc_status_t status = receive_and_decode(link, RC_PKT_COMMAND, command, &payload_len);

    if (status == RC_OK) {
        link->last_rx_time = link->hw.get_tick_ms();
        link->failsafe_active = false;

#if RC_ENABLE_STATISTICS
        link->stats.packets_received++;
#endif

        RC_LOG_DEBUG("Command received (seq=%d)\n", link->rx_packet.header.sequence);
        return RC_OK;
    }

    /* If link lost, return failsafe values */
    if (!link->link_active) {
        memcpy(command, &link->failsafe_command, sizeof(rc_command_payload_t));

        if (!link->failsafe_active) {
            link->failsafe_active = true;
            RC_LOG_WARN("Link lost - activating failsafe\n");
        }

        return RC_OK;  /* Return OK with failsafe values */
    }

    return status;
}

rc_status_t rc_link_send_telemetry(rc_link_t *link, const rc_telemetry_payload_t *telemetry)
{
    if (!link || !link->initialized || !telemetry) {
        return RC_ERROR_INVALID_PARAM;
    }

    rc_status_t status = encode_and_send(link, RC_PKT_TELEMETRY, telemetry,
                                         sizeof(rc_telemetry_payload_t));

    if (status == RC_OK) {
        link->tx_sequence++;

#if RC_ENABLE_STATISTICS
        link->stats.packets_sent++;
#endif

        RC_LOG_DEBUG("Telemetry sent (seq=%d)\n", link->tx_sequence - 1);
    }

    return status;
}

/*============================================================================*/
/* Common API                                                                 */
/*============================================================================*/

rc_status_t rc_link_update(rc_link_t *link)
{
    if (!link || !link->initialized) {
        return RC_ERROR_INVALID_PARAM;
    }

    update_link_state(link);
    calculate_link_quality(link);

    return RC_OK;
}

bool rc_link_is_active(rc_link_t *link)
{
    if (!link || !link->initialized) {
        return false;
    }

    return link->link_active;
}

uint32_t rc_link_get_time_since_rx(rc_link_t *link)
{
    if (!link || !link->initialized || link->last_rx_time == UINT32_MAX) {
        return UINT32_MAX;
    }

    return link->hw.get_tick_ms() - link->last_rx_time;
}

rc_status_t rc_link_set_failsafe(rc_link_t *link, const rc_command_payload_t *failsafe)
{
    if (!link || !link->initialized || !failsafe) {
        return RC_ERROR_INVALID_PARAM;
    }

    memcpy(&link->failsafe_command, failsafe, sizeof(rc_command_payload_t));

    RC_LOG_INFO("Failsafe values updated\n");
    return RC_OK;
}

rc_status_t rc_link_get_failsafe(rc_link_t *link, rc_command_payload_t *failsafe)
{
    if (!link || !link->initialized || !failsafe) {
        return RC_ERROR_INVALID_PARAM;
    }

    memcpy(failsafe, &link->failsafe_command, sizeof(rc_command_payload_t));
    return RC_OK;
}

/*============================================================================*/
/* Statistics API                                                             */
/*============================================================================*/

#if RC_ENABLE_STATISTICS
rc_status_t rc_link_get_stats(rc_link_t *link, rc_stats_t *stats)
{
    if (!link || !link->initialized || !stats) {
        return RC_ERROR_INVALID_PARAM;
    }

    memcpy(stats, &link->stats, sizeof(rc_stats_t));
    return RC_OK;
}

void rc_link_reset_stats(rc_link_t *link)
{
    if (!link || !link->initialized) {
        return;
    }

    memset(&link->stats, 0, sizeof(rc_stats_t));
    RC_LOG_INFO("Statistics reset\n");
}
#endif

/*============================================================================*/
/* Private Functions                                                          */
/*============================================================================*/

static void update_link_state(rc_link_t *link)
{
    uint32_t time_since_rx = rc_link_get_time_since_rx(link);
    bool was_active = link->link_active;

    /* Check timeout condition */
    bool timeout = (time_since_rx != UINT32_MAX) && (time_since_rx > RC_LINK_TIMEOUT_MS);

    /* Check sequence gap condition */
    bool sequence_gap = (link->consecutive_missed >= RC_LINK_LOSS_THRESHOLD);

    /* Link active if neither condition met */
    link->link_active = !timeout && !sequence_gap;

    /* Log state changes */
    if (was_active && !link->link_active) {
        if (timeout) {
            RC_LOG_WARN("Link lost: timeout (%lu ms)\n", time_since_rx);
        } else {
            RC_LOG_WARN("Link lost: %d consecutive missed packets\n",
                       link->consecutive_missed);
        }
    } else if (!was_active && link->link_active) {
        RC_LOG_INFO("Link restored\n");
        link->consecutive_missed = 0;
    }
}

static void calculate_link_quality(rc_link_t *link)
{
#if RC_ENABLE_STATISTICS
    if (link->stats.packets_sent == 0) {
        link->stats.link_quality = 0;
        return;
    }

    uint32_t total = link->stats.packets_sent;
    uint32_t lost = link->stats.packets_missed;

    if (total > 0) {
        uint32_t received = total - lost;
        link->stats.link_quality = (uint8_t)((received * 100) / total);

        if (link->stats.link_quality > 100) {
            link->stats.link_quality = 100;
        }
    }
#endif
}

static rc_status_t encode_and_send(rc_link_t *link, rc_packet_type_t type,
                                    const void *payload, uint8_t payload_len)
{
    if (payload_len > RC_MAX_PAYLOAD_SIZE) {
        return RC_ERROR_INVALID_PARAM;
    }

    /* Build header */
    link->tx_packet.header.version = RC_PROTOCOL_VERSION;
    link->tx_packet.header.type = type;
    link->tx_packet.header.sequence = link->tx_sequence;
    link->tx_packet.header.flags = 0;
    link->tx_packet.header.payload_len = payload_len;

    /* Copy payload */
    if (payload && payload_len > 0) {
        memcpy(link->tx_packet.payload, payload, payload_len);
    }

    /* Calculate CRC over header + payload */
    uint8_t crc_len = sizeof(rc_packet_header_t) + payload_len;
    link->tx_packet.crc8 = rc_crc8_calculate((uint8_t*)&link->tx_packet, crc_len);

    /* Transmit */
    if (!nrf24_transmit(&link->nrf24, (uint8_t*)&link->tx_packet, 32)) {
        return RC_ERROR_HARDWARE;
    }

    return RC_OK;
}

static rc_status_t receive_and_decode(rc_link_t *link, rc_packet_type_t expected_type,
                                       void *payload, uint8_t *payload_len)
{
    /* Check if data available */
    if (!nrf24_is_data_available(&link->nrf24)) {
        return RC_ERROR_NO_DATA;
    }

    /* Receive packet */
    uint8_t rx_len = 0;
    if (!nrf24_receive(&link->nrf24, (uint8_t*)&link->rx_packet, &rx_len)) {
        return RC_ERROR_HARDWARE;
    }

    /* Validate minimum size */
    if (rx_len < sizeof(rc_packet_header_t) + 1) {
        RC_LOG_WARN("Packet too small: %d bytes\n", rx_len);
        return RC_ERROR_CRC_FAIL;
    }

    /* Validate CRC */
    uint8_t crc_len = sizeof(rc_packet_header_t) + link->rx_packet.header.payload_len;
    uint8_t expected_crc = rc_crc8_calculate((uint8_t*)&link->rx_packet, crc_len);

    if (link->rx_packet.crc8 != expected_crc) {
        RC_LOG_WARN("CRC mismatch: expected 0x%02X, got 0x%02X\n",
                   expected_crc, link->rx_packet.crc8);
#if RC_ENABLE_STATISTICS
        link->stats.crc_errors++;
#endif
        return RC_ERROR_CRC_FAIL;
    }

    /* Validate version */
    if (link->rx_packet.header.version != RC_PROTOCOL_VERSION) {
        RC_LOG_WARN("Version mismatch: expected %d, got %d\n",
                   RC_PROTOCOL_VERSION, link->rx_packet.header.version);
#if RC_ENABLE_STATISTICS
        link->stats.version_mismatches++;
#endif
        return RC_ERROR_VERSION_MISMATCH;
    }

    /* Check packet type */
    if (link->rx_packet.header.type != expected_type) {
        return RC_ERROR_NO_DATA;
    }

    /* Check sequence gaps */
    if (link->last_rx_time != UINT32_MAX) {
        uint8_t expected_seq = link->rx_sequence_last + 1;
        uint8_t gap = link->rx_packet.header.sequence - expected_seq;

        if (gap > 0) {
            link->consecutive_missed += gap;

#if RC_ENABLE_STATISTICS
            link->stats.packets_missed += gap;
#endif

            RC_LOG_DEBUG("Sequence gap: missed %d packets\n", gap);
        } else {
            link->consecutive_missed = 0;
        }
    }

    link->rx_sequence_last = link->rx_packet.header.sequence;

    /* Copy payload */
    if (payload && payload_len) {
        *payload_len = link->rx_packet.header.payload_len;
        if (*payload_len > 0) {
            memcpy(payload, link->rx_packet.payload, *payload_len);
        }
    }

    return RC_OK;
}