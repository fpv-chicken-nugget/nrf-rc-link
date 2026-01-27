#ifndef RC_PROTOCOL_CONFIG_H
#define RC_PROTOCOL_CONFIG_H

#define RC_PROTOCOL_VERSION 1

// header (5) + payload + CRC (1) <= 32 bytes (nRF24L01 limit)
// 24 bytes for payload leaves 2 bytes margin
#define RC_COMMAND_PAYLOAD_SIZE 24
#define RC_TELEMETRY_PAYLOAD_SIZE 24

// link timing params
#define RC_PACKET_TIMEOUT_MS 100
#define RC_LINK_LOSS_THRESHOLD 10

#define RC_MAX_PAYLOAD_SIZE 32      // see nrf24l01 datasheet

#endif // RC_PROTOCOL_CONFIG_H