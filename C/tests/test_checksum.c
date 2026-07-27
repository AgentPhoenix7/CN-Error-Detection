#include "checksum.h"

#include <stdint.h>
#include <stdio.h>

static int test_even_length(void) {
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

static int test_odd_length(void) {
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

int main(void) {
  int failures = 0;

  failures += test_even_length();
  failures += test_odd_length();
  return failures == 0 ? 0 : 1;
}
