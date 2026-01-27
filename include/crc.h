#ifndef RC_PROTOCOL_CRC_H
#define RC_PROTOCOL_CRC_H

#include <stdint.h>
#include <stddef.h>

uint8_t rc_crc8(const uint8_t* data, size_t len);

#endif // RC_PROTOCOL_CRC_H
