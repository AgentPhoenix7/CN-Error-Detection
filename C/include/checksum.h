/**
 * @file checksum.h
 * @brief Declares reusable 16-bit one's-complement checksum operations.
 */

#ifndef CHECKSUM_H
#define CHECKSUM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Calculates the 16-bit one's-complement checksum of a byte array.
 *
 * @param[in] data Pointer to the input bytes. It may be `NULL` only when
 *                 `length` is zero.
 * @param[in] length Number of bytes in `data`.
 * @return The calculated 16-bit checksum.
 *
 * @note If `length` is odd, the final byte becomes the high byte of the
 *       last 16-bit word and zero is used as its low byte.
 */
uint16_t checksum16_compute(
  const uint8_t *data,
  size_t length
);

/**
 * @brief Verifies data against a received 16-bit checksum.
 *
 * @param[in] data Pointer to the received data bytes. It may be `NULL` only
 *                 when `length` is zero.
 * @param[in] length Number of bytes in `data`.
 * @param[in] received_checksum Checksum transmitted with the data.
 * @return `true` when no checksum error is detected; otherwise `false`.
 */
bool checksum16_verify(
  const uint8_t *data,
  size_t length,
  uint16_t received_checksum
);

#endif
