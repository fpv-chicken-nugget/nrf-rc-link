#include "packet.h"
#include "crc.h"
#include <string.h>


/**
 * @brief RC packet initialization
 *
 * @param pkt Packet comprised of header, payload, crc8
 * @param type Packet type of command, telemetry, heartbeat or emergency
 * @param seq Packet sequence number
 */
void rc_packet_init(rc_packet_t *pkt, rc_packet_type_t type, uint8_t seq) {
    memset(pkt, 0, sizeof(rc_packet_t));
    pkt->header.version = RC_PROTOCOL_VERSION;
    pkt->header.type = type;
    pkt->header.sequence = seq;
    pkt->header.flags = 0;
}

/**
 * @brief Populate packet payload and crc8
 *
 * @param pkt Packet comprised of header, payload, crc8
 * @param payload_len Length of payload in bytes
 */
void rc_packet_finalize(rc_packet_t *pkt, uint8_t payload_len) {
    pkt->header.payload_len = payload_len;
    size_t crc_len = sizeof(rc_packet_header_t) + payload_len;
    pkt->payload[payload_len] = rc_crc8((uint8_t*)pkt, crc_len);
}

/**
 * @brief Checks packet version, payload and crc for validity
 *
 * @param pkt Packet comprised of header, payload, crc8
 * @return true if received crc matches computed crc, false otherwise
 */
bool rc_packet_validate(const rc_packet_t *pkt) {
    if (pkt->header.version != RC_PROTOCOL_VERSION) return false;
    if (pkt->header.payload_len > (RC_MAX_PAYLOAD_SIZE - sizeof(rc_packet_header_t) - 1)) return false;

    size_t crc_len = sizeof(rc_packet_header_t) + pkt->header.payload_len;
    uint8_t calc_crc = rc_crc8((uint8_t*)pkt, crc_len);
    uint8_t recv_crc = pkt->payload[pkt->header.payload_len];

    return (calc_crc == recv_crc);
}

/**
 * @brief Returns packet size
 *
 * @param pkt Packet comprised of header, payload, crc8
 * @return size of pkt in bytes
 */
uint8_t rc_packet_size(const rc_packet_t *pkt) {
    return sizeof(rc_packet_header_t) + pkt->header.payload_len + 1;
}

/**
 * @brief encode payload
 *
 * @param pkt Packet comprised of header, payload, crc8
 * @param type Packet type of command, telemetry, heartbeat or emergency
 * @param payload Packet payload
 * @param payload_len Packet payload length
 * @param seq Packet sequence number
 */
void rc_encode_payload(rc_packet_t *pkt, rc_packet_type_t type,
                       const void *payload, uint8_t payload_len, uint8_t seq) {
    rc_packet_init(pkt, type, seq);
    memcpy(pkt->payload, payload, payload_len);
    rc_packet_finalize(pkt, payload_len);
}

/**
 * @brief Decode payload
 *
 * @param pkt Packet comprised of header, payload, crc8
 * @param payload Packet payload
 * @param expected_len Expected packet payload length
 * @return true if packet is valid, false otherwise
 */
bool rc_decode_payload(const rc_packet_t *pkt, void *payload, uint8_t expected_len) {
    if (!rc_packet_validate(pkt)) return false;
    if (pkt->header.payload_len != expected_len) return false;

    memcpy(payload, pkt->payload, expected_len);
    return true;
}

/**
 * @brief Encode heartbeat packet
 *
 * @param pkt Packet comprised of header, empty payload, crc8
 * @param seq Packet sequence number
 */
void rc_encode_heartbeat(rc_packet_t *pkt, uint8_t seq) {
    rc_packet_init(pkt, RC_PKT_HEARTBEAT, seq);
    rc_packet_finalize(pkt, 0);
}
