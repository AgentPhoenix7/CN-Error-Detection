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
typedef struct error_injection_rng_t
{
  uint32_t state;
} error_injection_rng_t;

/**
 * @brief Initializes reproducible pseudo-random generator state.
 *
 * Rejects a null state pointer and otherwise copies the supplied seed
 * directly into its 32-bit state field.
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
 * @brief Samples [0, bound) using multiply-high mapping with rejection.
 *
 * `bound` must fit size_t and be in [1, 2^32]. Consumes one or more LCG
 * values, avoiding its weak low bits and modulo bias. Invalid inputs leave
 * the RNG and output unchanged. A bound of one still consumes one value.
 *
 * Validates a bound in [1, 2^32], multiplies each 32-bit generator value by
 * it, rejects incomplete low-product buckets, and returns the product's high
 * 32 bits.
 *
 * @param[in,out] rng Initialized generator state.
 * @param[in] bound Exclusive upper bound.
 * @param[out] value Sampled value.
 * @return Whether a value was produced.
 */
bool error_injection_rng_bounded(
  error_injection_rng_t *rng,
  size_t bound,
  size_t *value
);

/**
 * @brief Selects a frame with reproducible probability `1 / denominator`.
 *
 * A denominator of one always selects the frame without advancing `rng`.
 * Other valid denominators advance the generator exactly once. Invalid input
 * leaves both the generator and output unchanged.
 *
 * Selects every frame without a draw for denominator one; otherwise advances
 * the generator once and selects bucket zero of a multiply-high scaled
 * denominator range.
 *
 * @param[in,out] rng Initialized pseudo-random generator state.
 * @param[in] denominator Positive inverse selection probability.
 * @param[out] selected Whether the current frame was selected.
 * @return `true` when a decision was produced; otherwise `false`.
 */
bool error_injection_select_frame(
  error_injection_rng_t *rng,
  uint32_t denominator,
  bool *selected
);

/**
 * @brief Flips one pseudo-randomly selected bit.
 *
 * Uses and advances the supplied generator state. No data or generator state
 * is modified when the input is invalid.
 * Random mutation buffers are limited to 2^32 bits and must also fit size_t.
 *
 * Validates the buffer and RNG, samples a bounded MSB-first bit index, and
 * delegates the mutation to error_injection_flip_bit().
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
 * Draws two bounded positions until their absolute difference exceeds one,
 * then delegates both validated flips to error_injection_flip_two_bits().
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
 * Checks counts and allocation size, builds a distinct subset with Floyd
 * sampling and collision substitution, applies the controlled odd-bit
 * operation, and frees the temporary indices.
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
 * Validates the requested length, samples among starts where the entire
 * burst fits, and delegates the mutation to error_injection_flip_burst().
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
 * Checks the byte index without multiplying the buffer length, derives the
 * byte offset and shifted 0x80 mask, and XORs that mask into the selected
 * byte.
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
 * Validates both indices and their distinctness before invoking the single-
 * bit primitive for each position.
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
 * Checks the count, bounds, and duplicate indices in a validation pass, then
 * flips all positions in a separate mutation pass.
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
 * Checks nonzero length, addition overflow, and both endpoints before
 * visiting each position with the single-bit primitive.
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
