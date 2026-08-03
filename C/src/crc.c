#include "crc.h"

#include <assert.h>

const crc_parameters_t crc8_parameters = {
  .width = 8U,
  .polynomial = UINT32_C(0xD5)
};

const crc_parameters_t crc10_parameters = {
  .width = 10U,
  .polynomial = UINT32_C(0x233)
};

const crc_parameters_t crc16_parameters = {
  .width = 16U,
  .polynomial = UINT32_C(0x8005)
};

const crc_parameters_t crc32_parameters = {
  .width = 32U,
  .polynomial = UINT32_C(0x04C11DB7)
};

static uint32_t crc_width_mask(uint8_t width)
{
  assert(width >= 1U);
  assert(width <= 32U);

  if (width == 32U) {
    return UINT32_MAX;
  }

  return (UINT32_C(1) << width) - UINT32_C(1);
}

uint32_t crc_compute(
  const uint8_t *data,
  size_t length,
  const crc_parameters_t *parameters
)
{
  uint32_t width_mask;
  uint32_t top_bit_mask;
  uint32_t remainder = 0U;
  size_t byte_index;
  uint8_t bit_index;

  assert(parameters != NULL);
  assert(data != NULL || length == 0U);

  width_mask = crc_width_mask(parameters->width);
  assert((parameters->polynomial & ~width_mask) == 0U);

  top_bit_mask = UINT32_C(1) << (parameters->width - 1U);

  for (byte_index = 0U; byte_index < length; byte_index++) {
    for (bit_index = 0U; bit_index < 8U; bit_index++) {
      bool data_bit_set;
      bool top_bit_set;

      data_bit_set = (data[byte_index] &
                     (UINT8_C(0x80) >> bit_index)) != 0U;
      top_bit_set = (remainder & top_bit_mask) != 0U;

      remainder = (remainder << 1U) & width_mask;

      if (data_bit_set != top_bit_set) {
        remainder ^= parameters->polynomial;
      }
    }
  }

  return remainder & width_mask;
}

bool crc_verify(
  const uint8_t *data,
  size_t length,
  uint32_t received_crc,
  const crc_parameters_t *parameters
)
{
  return crc_compute(data, length, parameters) == received_crc;
}
