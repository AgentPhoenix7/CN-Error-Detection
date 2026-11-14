#include "error_injection.h"

#include <stdlib.h>

/**
 * @brief Advances and returns the reproducible generator state.
 *
 * Applies the fixed linear congruential recurrence with uint32_t wraparound,
 * updating the caller-owned state in place.
 */
static uint32_t error_injection_rng_next(
  error_injection_rng_t *rng
)
{
  rng->state =
    rng->state * UINT32_C(1664525) +
    UINT32_C(1013904223);

  return rng->state;
}

/**
 * @brief Selects an integer below a bound without low-bit modulo bias.
 *
 * Validates a bound in [1, 2^32], multiplies each 32-bit generator value by
 * it, rejects incomplete low-product buckets, and returns the product's high
 * 32 bits.
 */
bool error_injection_rng_bounded(
  error_injection_rng_t *rng,
  const size_t bound,
  size_t *value
)
{
  const uint64_t range = UINT64_C(0x100000000);
  uint64_t product;

  if (rng == NULL || value == NULL || bound == 0U || (uint64_t)bound > range) {
    return false;
  }

  const uint32_t threshold = (uint32_t)((range - bound) % bound);
  do {
    product = (uint64_t)error_injection_rng_next(rng) * (uint64_t)bound;
  } while ((uint32_t)product < threshold);

  *value = (size_t)(product >> 32U);
  return true;
}

/**
 * @brief Checks whether a buffer fits the random selector's bit-address
 * range.
 *
 * Rejects empty buffers, byte-to-bit size_t overflow, and lengths exceeding
 * 2^32 bits before storing length multiplied by eight.
 */
static bool error_injection_is_valid_random_bit_length(
  const size_t length,
  size_t *total_bit_count
)
{
  if (length == 0U || length > SIZE_MAX / 8U ||
      (uint64_t)length > UINT64_C(0x100000000) / 8U) {
    return false;
  }
  *total_bit_count = length * 8U;
  return true;
}

/**
 * @brief Initializes a caller-owned reproducible generator.
 *
 * Rejects a null state pointer and otherwise copies the supplied seed
 * directly into its 32-bit state field.
 */
bool error_injection_rng_seed(
  error_injection_rng_t *rng,
  const uint32_t seed
)
{
  if (rng == NULL) {
    return false;
  }

  rng->state = seed;

  return true;
}

/**
 * @brief Decides whether the current frame should receive an injected error.
 *
 * Selects every frame without a draw for denominator one; otherwise advances
 * the generator once and selects bucket zero of a multiply-high scaled
 * denominator range.
 */
bool error_injection_select_frame(
  error_injection_rng_t *rng,
  const uint32_t denominator,
  bool *selected
)
{
  if (rng == NULL || selected == NULL || denominator == 0U) {
    return false;
  }

  if (denominator == 1U) {
    *selected = true;
    return true;
  }

  const uint64_t scaled_value = (uint64_t)error_injection_rng_next(rng) * denominator;
  *selected = (scaled_value >> 32U) == 0U;

  return true;
}

/**
 * @brief Flips one reproducibly selected bit in a buffer.
 *
 * Validates the buffer and RNG, samples a bounded MSB-first bit index, and
 * delegates the mutation to error_injection_flip_bit().
 */
bool error_injection_flip_random_bit(
  uint8_t *data,
  const size_t length,
  error_injection_rng_t *rng
)
{
  size_t bit_count;
  size_t bit_index;
  if (
    data == NULL ||
    rng == NULL ||
    !error_injection_is_valid_random_bit_length(length, &bit_count)
  ) {
    return false;
  }

  (void)error_injection_rng_bounded(rng, bit_count, &bit_index);

  return error_injection_flip_bit(
    data,
    length,
    bit_index
  );
}

/**
 * @brief Flips two reproducibly selected nonadjacent bits.
 *
 * Draws two bounded positions until their absolute difference exceeds one,
 * then delegates both validated flips to error_injection_flip_two_bits().
 */
bool error_injection_flip_random_two_bits(
  uint8_t *data,
  const size_t length,
  error_injection_rng_t *rng
)
{
  size_t bit_count;
  size_t first_bit_index;
  size_t second_bit_index;
  size_t separation;

  if (
    data == NULL ||
    rng == NULL ||
    !error_injection_is_valid_random_bit_length(length, &bit_count)
  ) {
    return false;
  }

  do {
    (void)error_injection_rng_bounded(rng, bit_count, &first_bit_index);
    (void)error_injection_rng_bounded(rng, bit_count, &second_bit_index);

    if (first_bit_index > second_bit_index) {
      separation = first_bit_index - second_bit_index;
    } else {
      separation = second_bit_index - first_bit_index;
    }
  } while (separation <= 1U);

  return error_injection_flip_two_bits(
    data,
    length,
    first_bit_index,
    second_bit_index
  );
}

/**
 * @brief Flips a requested positive odd number of distinct random bits.
 *
 * Checks counts and allocation size, builds a distinct subset with Floyd
 * sampling and collision substitution, applies the controlled odd-bit
 * operation, and frees the temporary indices.
 */
bool error_injection_flip_random_odd_bits(
  uint8_t *data,
  const size_t length,
  const size_t bit_count,
  error_injection_rng_t *rng
)
{
  size_t *bit_indices;
  size_t previous_index;
  size_t total_bit_count;

  if (
    data == NULL ||
    rng == NULL ||
    !error_injection_is_valid_random_bit_length(length, &total_bit_count) ||
    bit_count == 0U ||
    bit_count % 2U == 0U
  ) {
    return false;
  }

  if (
    bit_count > total_bit_count ||
    bit_count > SIZE_MAX / sizeof *bit_indices
  ) {
    return false;
  }

  bit_indices = malloc(bit_count * sizeof *bit_indices);

  if (bit_indices == NULL) {
    return false;
  }

  /* Floyd's algorithm selects a distinct subset with exactly bit_count draws
     (apart from bounded-mapping rejection), even for near-full subsets. */
  for (size_t selected_count = 0U; selected_count < bit_count; selected_count++) {
    const size_t current_bound = total_bit_count - bit_count + selected_count;
    size_t candidate = 0U;
    (void)error_injection_rng_bounded(rng, current_bound + 1U, &candidate);

    for (
      previous_index = 0U;
      previous_index < selected_count;
      previous_index++
    ) {
      if (candidate == bit_indices[previous_index]) {
        break;
      }
    }

    bit_indices[selected_count] =
      previous_index == selected_count ? candidate : current_bound;
  }

  const bool result = error_injection_flip_odd_bits(
    data,
    length,
    bit_indices,
    bit_count
  );

  free(bit_indices);

  return result;
}

/**
 * @brief Flips a contiguous burst at a reproducibly selected valid position.
 *
 * Validates the requested length, samples among starts where the entire
 * burst fits, and delegates the mutation to error_injection_flip_burst().
 */
bool error_injection_flip_random_burst(
  uint8_t *data,
  const size_t length,
  const size_t burst_length,
  error_injection_rng_t *rng
)
{
  size_t total_bit_count;
  size_t start_bit_index;
  if (
    data == NULL ||
    rng == NULL ||
    !error_injection_is_valid_random_bit_length(length, &total_bit_count) ||
    burst_length == 0U
  ) {
    return false;
  }

  if (burst_length > total_bit_count) {
    return false;
  }

  const size_t valid_start_count = total_bit_count - burst_length + 1U;
  (void)error_injection_rng_bounded(rng, valid_start_count, &start_bit_index);

  return error_injection_flip_burst(
    data,
    length,
    start_bit_index,
    burst_length
  );
}

/**
 * @brief Toggles one specified MSB-first bit in a byte buffer.
 *
 * Checks the byte index without multiplying the buffer length, derives the
 * byte offset and shifted 0x80 mask, and XORs that mask into the selected
 * byte.
 */
bool error_injection_flip_bit(
  uint8_t *data,
  const size_t length,
  const size_t bit_index
)
{
  if (data == NULL || bit_index / 8U >= length) {
    return false;
  }

  const size_t byte_index = bit_index / 8U;
  const uint8_t bit_offset = (uint8_t)(bit_index % 8U);
  const uint8_t bit_mask = (uint8_t)(UINT8_C(0x80) >> bit_offset);

  data[byte_index] ^= bit_mask;

  return true;
}

/**
 * @brief Toggles two distinct specified bits without partial invalid
 * mutations.
 *
 * Validates both indices and their distinctness before invoking the single-
 * bit primitive for each position.
 */
bool error_injection_flip_two_bits(
  uint8_t *data,
  const size_t length,
  const size_t first_bit_index,
  const size_t second_bit_index
)
{
  if (
    data == NULL ||
    first_bit_index == second_bit_index ||
    first_bit_index / 8U >= length ||
    second_bit_index / 8U >= length
  ) {
    return false;
  }

  (void)error_injection_flip_bit(
    data,
    length,
    first_bit_index
  );
  (void)error_injection_flip_bit(
    data,
    length,
    second_bit_index
  );

  return true;
}

/**
 * @brief Toggles a positive odd set of distinct specified positions.
 *
 * Checks the count, bounds, and duplicate indices in a validation pass, then
 * flips all positions in a separate mutation pass.
 */
bool error_injection_flip_odd_bits(
  uint8_t *data,
  const size_t length,
  const size_t *bit_indices,
  const size_t bit_count
)
{
  size_t index;

  if (
    data == NULL ||
    bit_indices == NULL ||
    bit_count == 0U ||
    bit_count % 2U == 0U
  ) {
    return false;
  }

  for (
    index = 0U;
    index < bit_count;
    index++
  )
  {
    if (bit_indices[index] / 8U >= length) {
      return false;
    }

    for (
      size_t previous_index = 0U;
      previous_index < index;
      previous_index++
    ) {
      if (bit_indices[index] == bit_indices[previous_index]) {
        return false;
      }
    }
  }

  for (
    index = 0U;
    index < bit_count;
    index++
  )
  {
    (void)error_injection_flip_bit(
      data,
      length,
      bit_indices[index]
    );
  }

  return true;
}

/**
 * @brief Toggles every bit in one specified contiguous range.
 *
 * Checks nonzero length, addition overflow, and both endpoints before
 * visiting each position with the single-bit primitive.
 */
bool error_injection_flip_burst(
  uint8_t *data,
  const size_t length,
  const size_t start_bit_index,
  const size_t burst_length
)
{
  if (
    data == NULL ||
    burst_length == 0U ||
    start_bit_index / 8U >= length ||
    burst_length - 1U > SIZE_MAX - start_bit_index
  ) {
    return false;
  }

  const size_t final_bit_index = start_bit_index + burst_length - 1U;

  if (final_bit_index / 8U >= length) {
    return false;
  }

  for (
    size_t offset = 0U;
    offset < burst_length;
    offset++
  )
  {
    (void)error_injection_flip_bit(
      data,
      length,
      start_bit_index + offset
    );
  }

  return true;
}
