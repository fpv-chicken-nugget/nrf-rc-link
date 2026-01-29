#ifndef RC_PROTOCOL_CONFIG_H
#define RC_PROTOCOL_CONFIG_H

#define RC_PROTOCOL_VERSION 1

// header (5) + payload (26) + CRC (1) <= 32 bytes (nRF24L01 limit)
#define RC_PAYLOAD_SIZE 26

// link timing params
#define RC_PACKET_TIMEOUT_MS 100
#define RC_LINK_LOSS_THRESHOLD 10

#define RC_MAX_PAYLOAD_SIZE 32      // see nrf24l01 datasheet

#endif // RC_PROTOCOL_CONFIG_H