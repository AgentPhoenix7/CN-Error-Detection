/**
 * @file error_injection.h
 * @brief Declares controlled bit-error injection operations.
 */

#ifndef ERROR_INJECTION_H
#define ERROR_INJECTION_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Flips one specified bit in a byte array.
 *
 * Bit index zero identifies the most-significant bit of `data[0]`.
 *
 * @param[in,out] data Bytes to modify.
 * @param[in] length Number of bytes in `data`.
 * @param[in] bit_index Zero-based bit position to flip.
 * @return `true` when the bit was flipped; `false` when `data` is `NULL`
 *         or `bit_index` is outside the supplied byte array.
 */
bool error_injection_flip_bit(
  uint8_t *data,
  size_t length,
  size_t bit_index
);

#endif