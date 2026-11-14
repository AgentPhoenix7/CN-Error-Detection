#include "checksum.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/**
 * @brief Checks checksum addition for an even-length buffer.
 *
 * Computes the checksum of the fixed eight-byte vector and compares it with
 * the known value 0x220D.
 */
static int test_even_length(void)
{
  const uint8_t data[] = {
    0x00, 0x01, 0xF2, 0x03,
    0xF4, 0xF5, 0xF6, 0xF7
  };
  const uint16_t expected = 0x220D;
  uint16_t actual;

  actual = checksum16_compute(data, sizeof data);

  if (actual != expected) {
    fprintf(
      stderr,
      "FAIL: expected 0x%04X, received 0x%04X\n",
      (unsigned int)expected,
      (unsigned int)actual
    );
    return 1;
  }

  puts("PASS: even-length checksum");
  return 0;
}

/**
 * @brief Checks zero-extension of the last odd payload byte.
 *
 * Computes the checksum of 12 34 56 and requires 0x97CB, which treats 56 as
 * the high byte of the final word.
 */
static int test_odd_length(void)
{
  const uint8_t data[] = {0x12, 0x34, 0x56};
  const uint16_t expected = 0x97CB;
  uint16_t actual;

  actual = checksum16_compute(data, sizeof data);

  if (actual != expected) {
    fprintf(
      stderr,
      "FAIL odd-length: expected 0x%04X, received 0x%04X\n",
      (unsigned int)expected,
      (unsigned int)actual
    );
    return 1;
  }

  puts("PASS: odd-length checksum");
  return 0;
}

/**
 * @brief Checks the checksum of an empty buffer.
 *
 * Calls checksum16_compute with NULL and zero length and requires the
 * one's-complement empty sum, UINT16_MAX.
 */
static int test_empty_input(void)
{
  const uint16_t expected = UINT16_MAX;
  uint16_t actual;

  actual = checksum16_compute(NULL, 0U);

  if (actual != expected) {
    fprintf(
      stderr,
      "FAIL empty input: expected 0x%04X, received 0x%04X\n",
      (unsigned int)expected,
      (unsigned int)actual
    );
    return 1;
  }

  puts("PASS: empty-input checksum");
  return 0;
}

/**
 * @brief Checks that a correct received checksum is accepted.
 *
 * Verifies the fixed eight-byte vector against its known 0x220D checksum and
 * requires success.
 */
static int test_valid_verification(void)
{
  const uint8_t data[] = {
    0x00, 0x01, 0xF2, 0x03,
    0xF4, 0xF5, 0xF6, 0xF7
  };
  const uint16_t received_checksum = 0x220D;

  if (!checksum16_verify(
        data,
        sizeof data,
        received_checksum
      )) {
    fputs("FAIL: valid checksum was rejected\n", stderr);
    return 1;
  }

  puts("PASS: valid checksum verification");
  return 0;
}

/**
 * @brief Checks detection of a data-bit error.
 *
 * Flips one bit of the fixed data vector while retaining its original
 * checksum and requires verification to fail.
 */
static int test_corrupted_data_verification(void)
{
  uint8_t data[] = {
    0x00, 0x01, 0xF2, 0x03,
    0xF4, 0xF5, 0xF6, 0xF7
  };
  const uint16_t received_checksum = 0x220D;

  data[3] ^= 0x01U;

  if (checksum16_verify(
        data,
        sizeof data,
        received_checksum
      )) {
    fputs("FAIL: corrupted data was accepted\n", stderr);
    return 1;
  }

  puts("PASS: corrupted data verification");
  return 0;
}

/**
 * @brief Checks detection of a transmitted checksum-bit error.
 *
 * Keeps the data intact, toggles one bit of the known checksum, and requires
 * verification to fail.
 */
static int test_corrupted_checksum_verification(void)
{
  const uint8_t data[] = {
    0x00, 0x01, 0xF2, 0x03,
    0xF4, 0xF5, 0xF6, 0xF7
  };
  uint16_t received_checksum = 0x220D;

  received_checksum ^= 0x0001U;

  if (checksum16_verify(
        data,
        sizeof data,
        received_checksum
      )) {
    fputs("FAIL: corrupted checksum was accepted\n", stderr);
    return 1;
  }

  puts("PASS: corrupted checksum verification");
  return 0;
}

/**
 * @brief Checks verification of the valid empty codeword.
 *
 * Verifies NULL data with zero length and UINT16_MAX as the received
 * checksum and requires success.
 */
static int test_empty_verification(void)
{
  if (!checksum16_verify(NULL, 0U, UINT16_MAX)) {
    fputs("FAIL: valid empty input was rejected\n", stderr);
    return 1;
  }

  puts("PASS: empty-input verification");
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

  failures += test_even_length();
  failures += test_odd_length();
  failures += test_empty_input();
  failures += test_valid_verification();
  failures += test_corrupted_data_verification();
  failures += test_corrupted_checksum_verification();
  failures += test_empty_verification();
  return failures == 0 ? 0 : 1;
}
