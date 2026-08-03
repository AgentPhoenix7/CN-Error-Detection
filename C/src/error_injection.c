#include "error_injection.h"

bool error_injection_flip_bit(
  uint8_t *data,
  size_t length,
  size_t bit_index
)
{
  size_t byte_index;
  uint8_t bit_offset;
  uint8_t bit_mask;

  if (data == NULL || bit_index / 8U >= length) {
    return false;
  }

  byte_index = bit_index / 8U;
  bit_offset = (uint8_t)(bit_index % 8U);
  bit_mask = (uint8_t)(UINT8_C(0x80) >> bit_offset);

  data[byte_index] ^= bit_mask;

  return true;
}

bool error_injection_flip_two_bits(
  uint8_t *data,
  size_t length,
  size_t first_bit_index,
  size_t second_bit_index
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
  size_t length,
  const size_t *bit_indices,
  size_t bit_count
)
{
  size_t index;
  size_t previous_index;

  if (
    data == NULL ||
    bit_indices == NULL ||
    bit_count == 0U ||
    bit_count % 2U == 0U
  ) {
    return false;
  }

  for (index = 0U; index < bit_count; index++) {
    if (bit_indices[index] / 8U >= length) {
      return false;
    }

    for (
      previous_index = 0U;
      previous_index < index;
      previous_index++
    ) {
      if (bit_indices[index] == bit_indices[previous_index]) {
        return false;
      }
    }
  }

  for (index = 0U; index < bit_count; index++) {
    (void)error_injection_flip_bit(
      data,
      length,
      bit_indices[index]
    );
  }

  return true;
}
