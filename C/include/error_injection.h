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
 * @brief Stores state for reproducible pseudo-random error selection.
 */
typedef struct
{
  uint32_t state;
} error_injection_rng_t;

/**
 * @brief Initializes reproducible pseudo-random generator state.
 *
 * @param[out] rng State object to initialize.
 * @param[in] seed Starting value for the reproducible sequence.
 * @return `true` when the state was initialized; `false` when `rng` is `NULL`.
 */
bool error_injection_rng_seed(
  error_injection_rng_t *rng,
  uint32_t seed
);

/**
 * @brief Flips one pseudo-randomly selected bit.
 *
 * Uses and advances the supplied generator state. No data or generator state
 * is modified when the input is invalid.
 *
 * @param[in,out] data Bytes to modify.
 * @param[in] length Number of bytes in `data`.
 * @param[in,out] rng Initialized pseudo-random generator state.
 * @return `true` when one bit was flipped; otherwise `false`.
 */
bool error_injection_flip_random_bit(
  uint8_t *data,
  size_t length,
  error_injection_rng_t *rng
);

/**
 * @brief Flips two pseudo-randomly selected nonadjacent bits.
 *
 * The selected positions are distinct and differ by more than one. Uses and
 * advances the supplied generator state. No data or generator state is
 * modified when the input is invalid.
 *
 * @param[in,out] data Bytes to modify.
 * @param[in] length Number of bytes in `data`.
 * @param[in,out] rng Initialized pseudo-random generator state.
 * @return `true` when two nonadjacent bits were flipped; otherwise `false`.
 */
bool error_injection_flip_random_two_bits(
  uint8_t *data,
  size_t length,
  error_injection_rng_t *rng
);

/**
 * @brief Flips a requested odd number of pseudo-randomly selected distinct bits.
 *
 * `bit_count` must be positive, odd, and no greater than the number of bits in
 * `data`. Uses and advances the supplied generator state. No data or generator
 * state is modified when the input is invalid.
 *
 * @param[in,out] data Bytes to modify.
 * @param[in] length Number of bytes in `data`.
 * @param[in] bit_count Positive odd number of distinct bits to select.
 * @param[in,out] rng Initialized pseudo-random generator state.
 * @return `true` when every selected bit was flipped; otherwise `false`.
 */
bool error_injection_flip_random_odd_bits(
  uint8_t *data,
  size_t length,
  size_t bit_count,
  error_injection_rng_t *rng
);

/**
 * @brief Flips a pseudo-randomly positioned contiguous burst.
 *
 * `burst_length` must be positive and no greater than the number of bits in
 * `data`. Selects one valid start position and advances the supplied generator
 * state. No data or generator state is modified when the input is invalid.
 *
 * @param[in,out] data Bytes to modify.
 * @param[in] length Number of bytes in `data`.
 * @param[in] burst_length Positive number of consecutive bits to flip.
 * @param[in,out] rng Initialized pseudo-random generator state.
 * @return `true` when the complete burst was applied; otherwise `false`.
 */
bool error_injection_flip_random_burst(
  uint8_t *data,
  size_t length,
  size_t burst_length,
  error_injection_rng_t *rng
);

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

/**
 * @brief Flips two distinct specified bits in a byte array.
 *
 * Bit index zero identifies the most-significant bit of `data[0]`.
 * No data is modified when either index is invalid or the indices match.
 *
 * @param[in,out] data Bytes to modify.
 * @param[in] length Number of bytes in `data`.
 * @param[in] first_bit_index First zero-based bit position.
 * @param[in] second_bit_index Second zero-based bit position.
 * @return `true` when both bits were flipped; otherwise `false`.
 */
bool error_injection_flip_two_bits(
  uint8_t *data,
  size_t length,
  size_t first_bit_index,
  size_t second_bit_index
);

/**
 * @brief Flips an odd number of distinct specified bits.
 *
 * All indices are validated before any data is modified. Bit index zero
 * identifies the most-significant bit of `data[0]`.
 *
 * @param[in,out] data Bytes to modify.
 * @param[in] length Number of bytes in `data`.
 * @param[in] bit_indices Array of zero-based bit positions.
 * @param[in] bit_count Number of positions; it must be positive and odd.
 * @return `true` when every specified bit was flipped; otherwise `false`.
 */
bool error_injection_flip_odd_bits(
  uint8_t *data,
  size_t length,
  const size_t *bit_indices,
  size_t bit_count
);

/**
 * @brief Flips every bit in a contiguous burst.
 *
 * Bit index zero identifies the most-significant bit of `data[0]`.
 * The complete range is validated before any data is modified.
 *
 * @param[in,out] data Bytes to modify.
 * @param[in] length Number of bytes in `data`.
 * @param[in] start_bit_index First bit in the burst.
 * @param[in] burst_length Positive number of consecutive bits to flip.
 * @return `true` when the complete burst was applied; otherwise `false`.
 */
bool error_injection_flip_burst(
  uint8_t *data,
  size_t length,
  size_t start_bit_index,
  size_t burst_length
);

#endif
