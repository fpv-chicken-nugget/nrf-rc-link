/**
* @file crc.h
 * @brief CRC-8 calculation
 */

#ifndef CRC_H
#define CRC_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

    /**
     * @brief Calculate CRC-8 checksum
     *
     * Uses CRC-8-CCITT polynomial: 0x07
     * Initial value: 0x00
     *
     * @param data Data buffer
     * @param len  Length of data
     * @return CRC-8 checksum
     */
    uint8_t rc_crc8_calculate(const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* CRC_H */