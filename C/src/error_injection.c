#include "error_injection.h"

#include <stdlib.h>

static uint32_t error_injection_rng_next(
  error_injection_rng_t *rng
)
{
  rng->state =
    rng->state * UINT32_C(1664525) +
    UINT32_C(1013904223);

  return rng->state;
}

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

bool error_injection_flip_random_bit(
  uint8_t *data,
  const size_t length,
  error_injection_rng_t *rng
)
{
  if (
    data == NULL ||
    rng == NULL ||
    length == 0U ||
    length > SIZE_MAX / 8U
  ) {
    return false;
  }

  const size_t bit_count = length * 8U;
  const uint32_t random_value = error_injection_rng_next(rng);
  const size_t bit_index = (size_t)random_value % bit_count;

  return error_injection_flip_bit(
    data,
    length,
    bit_index
  );
}

bool error_injection_flip_random_two_bits(
  uint8_t *data,
  const size_t length,
  error_injection_rng_t *rng
)
{
  size_t second_bit_index;
  size_t separation;

  if (
    data == NULL ||
    rng == NULL ||
    length == 0U ||
    length > SIZE_MAX / 8U
  ) {
    return false;
  }

  const size_t bit_count = length * 8U;
  const size_t first_bit_index = (size_t)error_injection_rng_next(rng) % bit_count;

  do {
    second_bit_index =
      (size_t)error_injection_rng_next(rng) % bit_count;

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

bool error_injection_flip_random_odd_bits(
  uint8_t *data,
  const size_t length,
  const size_t bit_count,
  error_injection_rng_t *rng
)
{
  size_t *bit_indices;
  size_t previous_index;

  if (
    data == NULL ||
    rng == NULL ||
    length == 0U ||
    length > SIZE_MAX / 8U ||
    bit_count == 0U ||
    bit_count % 2U == 0U
  ) {
    return false;
  }

  const size_t total_bit_count = length * 8U;

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

  size_t selected_count = 0U;

  while (selected_count < bit_count) {
    const size_t candidate = (size_t)error_injection_rng_next(rng) % total_bit_count;

    for (
      previous_index = 0U;
      previous_index < selected_count;
      previous_index++
    ) {
      if (candidate == bit_indices[previous_index]) {
        break;
      }
    }

    if (previous_index == selected_count) {
      bit_indices[selected_count] = candidate;
      selected_count++;
    }
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

bool error_injection_flip_random_burst(
  uint8_t *data,
  const size_t length,
  const size_t burst_length,
  error_injection_rng_t *rng
)
{
  if (
    data == NULL ||
    rng == NULL ||
    length == 0U ||
    length > SIZE_MAX / 8U ||
    burst_length == 0U
  ) {
    return false;
  }

  const size_t total_bit_count = length * 8U;

  if (burst_length > total_bit_count) {
    return false;
  }

  const size_t valid_start_count = total_bit_count - burst_length + 1U;
  const size_t start_bit_index = (size_t)error_injection_rng_next(rng) % valid_start_count;

  return error_injection_flip_burst(
    data,
    length,
    start_bit_index,
    burst_length
  );
}

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
