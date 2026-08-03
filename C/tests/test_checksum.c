#include "checksum.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

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

int main(void)
{
  int failures = 0;

  failures += test_even_length();
  failures += test_odd_length();
  failures += test_empty_input();
  failures += test_valid_verification();
  failures += test_corrupted_data_verification();
  return failures == 0 ? 0 : 1;
}
