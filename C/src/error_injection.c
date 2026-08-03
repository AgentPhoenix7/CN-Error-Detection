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
