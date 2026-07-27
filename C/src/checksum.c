#include "checksum.h"

uint16_t checksum16_compute(const uint8_t *data, size_t length)
{
  uint32_t sum = 0U;
  size_t index = 0U;

  while (index + 1U < length) {
    uint16_t word;

    word = (uint16_t)((uint16_t)data[index] << 8U);
    word = (uint16_t)(word | data[index + 1U]);

    sum += word;
    sum = (sum & UINT16_MAX) + (sum >> 16U);

    index += 2U;
  }

  if (index < length) {
    uint16_t final_word;

    final_word = (uint16_t)((uint16_t)data[index] << 8U);
    sum += final_word;
    sum = (sum & UINT16_MAX) + (sum >> 16U);
  }

  return (uint16_t)~sum;
}
