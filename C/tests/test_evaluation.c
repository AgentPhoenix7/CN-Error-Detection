#include "checksum.h"
#include "crc.h"
#include "error_injection.h"

#include <stdint.h>
#include <stdio.h>

/**
 * @brief Demonstrates a corruption rejected by both checksum and CRC-16.
 *
 * Computes both original FCS values for 12 34, flips bit zero, and requires
 * both verifiers to reject the modified bytes.
 */
static int test_error_detected_by_checksum_and_crc(void)
{
  uint8_t data[] = {UINT8_C(0x12), UINT8_C(0x34)};
  const uint16_t checksum = checksum16_compute(data, sizeof data);
  const uint32_t crc = crc_compute(data, sizeof data, &crc16_parameters);

  if (!error_injection_flip_bit(data, sizeof data, 0U)) {
    fputs("FAIL: both-detect fixture mutation failed\n", stderr);
    return 1;
  }

  if (
    checksum16_verify(data, sizeof data, checksum) ||
    crc_verify(data, sizeof data, crc, &crc16_parameters)
  ) {
    fputs("FAIL: checksum and CRC did not both detect the error\n", stderr);
    return 1;
  }

  puts("PASS: error detected by checksum and CRC");
  return 0;
}

/**
 * @brief Demonstrates a corruption detected by checksum but missed by
 * CRC-10.
 *
 * Computes both FCS values for 256 zero bytes, flips bits zero and 511, and
 * requires checksum rejection with CRC-10 acceptance.
 */
static int test_error_detected_by_checksum_only(void)
{
  uint8_t data[256] = {UINT8_C(0)};
  const uint16_t checksum = checksum16_compute(data, sizeof data);
  const uint32_t crc = crc_compute(data, sizeof data, &crc10_parameters);

  if (!error_injection_flip_two_bits(data, sizeof data, 0U, 511U)) {
    fputs("FAIL: checksum-only fixture mutation failed\n", stderr);
    return 1;
  }

  if (
    checksum16_verify(data, sizeof data, checksum) ||
    !crc_verify(data, sizeof data, crc, &crc10_parameters)
  ) {
    fputs("FAIL: checksum-only outcome was not reproduced\n", stderr);
    return 1;
  }

  puts("PASS: error detected by checksum but not CRC-10");
  return 0;
}

/**
 * @brief Demonstrates a corruption missed by checksum but detected by
 * CRC-16.
 *
 * Flips bits 15 and 31 of 00 00 00 01 so one's-complement word changes
 * compensate, then requires checksum acceptance and CRC-16 rejection.
 */
static int test_error_detected_by_crc_only(void)
{
  uint8_t data[] = {
    UINT8_C(0x00), UINT8_C(0x00),
    UINT8_C(0x00), UINT8_C(0x01)
  };
  const uint16_t checksum = checksum16_compute(data, sizeof data);
  const uint32_t crc = crc_compute(data, sizeof data, &crc16_parameters);

  if (!error_injection_flip_two_bits(data, sizeof data, 15U, 31U)) {
    fputs("FAIL: CRC-only fixture mutation failed\n", stderr);
    return 1;
  }

  if (
    !checksum16_verify(data, sizeof data, checksum) ||
    crc_verify(data, sizeof data, crc, &crc16_parameters)
  ) {
    fputs("FAIL: CRC-only outcome was not reproduced\n", stderr);
    return 1;
  }

  puts("PASS: error detected by CRC-16 but not checksum");
  return 0;
}

/**
 * @brief Runs this module's tests and reports an overall process status.
 *
 * Calls each test helper, accumulates its failure count, and returns zero
 * only when every check passes.
 */
int main(void)
{
  int failures = 0;

  failures += test_error_detected_by_checksum_and_crc();
  failures += test_error_detected_by_checksum_only();
  failures += test_error_detected_by_crc_only();

  return failures == 0 ? 0 : 1;
}
