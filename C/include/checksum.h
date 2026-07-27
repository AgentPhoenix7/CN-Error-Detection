#ifndef CHECKSUM_H
#define CHECKSUM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Calculates the 16-bit one's-complement checksum of a byte array.
 * 
 * If length is odd, the final byte is treated as the high byte of
 * a 16-bit word, with zero used as its low byte.
*/
uint16_t checksum16_compute(const uint8_t *data, size_t length);

/*
 * Returns true if data and its received checksum contain no
 * detectable error; otherwise returns false.
*/
bool checksum16_verify(
    const uint8_t *data,
    size_t length,
    uint16_t received_checksum
);

#endif /* CHECKSUM_H */