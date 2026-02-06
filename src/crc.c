/**
* @file crc.c
 * @brief CRC-8 implementation
 */

#include "crc.h"

/* CRC-8-CCITT polynomial: x^8 + x^2 + x + 1 */
#define CRC8_POLYNOMIAL 0x07
#define CRC8_INIT       0x00

uint8_t rc_crc8_calculate(const uint8_t *data, size_t len)
{
    uint8_t crc = CRC8_INIT;

    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];

        for (uint8_t bit = 0; bit < 8; bit++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ CRC8_POLYNOMIAL;
            } else {
                crc = (crc << 1);
            }
        }
    }

    return crc;
}
