#include "checksum.h"

/**
 * @brief Computes the 16-bit one's-complement checksum of a byte buffer.
 *
 * Adds big-endian 16-bit words with end-around carry, treats a final odd
 * byte as a word's high byte, then complements the folded sum.
 */
uint16_t checksum16_compute(const uint8_t *data, const size_t length)
{
  uint32_t sum = 0U;
  size_t index = 0U;

  while (index + 1U < length) {
    uint16_t word = (uint16_t)((uint16_t)data[index] << 8U);
    word = (uint16_t)(word | data[index + 1U]);

    sum += word;
    sum = (sum & UINT16_MAX) + (sum >> 16U);

    index += 2U;
  }

  if (index < length) {
    const uint16_t final_word = (uint16_t)((uint16_t)data[index] << 8U);
    sum += final_word;
    sum = (sum & UINT16_MAX) + (sum >> 16U);
  }

  return (uint16_t)~sum;
}

/**
 * @brief Checks whether received data matches its transmitted checksum.
 *
 * Recovers the folded data sum from checksum16_compute(), adds the received
 * checksum with end-around carry, and requires an all-ones result.
 */
bool checksum16_verify(
  const uint8_t *data,
  const size_t length,
  const uint16_t received_checksum
)
{
  uint32_t sum = (uint16_t)~checksum16_compute(data, length);
  sum += received_checksum;
  sum = (sum & UINT16_MAX) + (sum >> 16U);

  return sum == UINT16_MAX;
}
