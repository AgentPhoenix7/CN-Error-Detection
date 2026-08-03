#include "error_injection.h"

#include <stdint.h>
#include <stdio.h>

static int test_flip_first_bit(void)
{
  uint8_t data[] = {UINT8_C(0x00)};
  const uint8_t expected = UINT8_C(0x80);

  if (!error_injection_flip_bit(data, sizeof data, 0U)) {
    fputs("FAIL: valid bit position was rejected\n", stderr);
    return 1;
  }

  if (data[0] != expected) {
    fprintf(
      stderr,
      "FAIL first bit: expected 0x%02X, received 0x%02X\n",
      (unsigned int)expected,
      (unsigned int)data[0]
    );
    return 1;
  }

  puts("PASS: flip first bit");
  return 0;
}

static int test_flip_last_bit(void)
{
  uint8_t data[] = {
    UINT8_C(0xFF),
    UINT8_C(0x00)
  };

  if (!error_injection_flip_bit(
        data,
        sizeof data,
        15U
      )) {
    fputs("FAIL: last valid bit was rejected\n", stderr);
    return 1;
  }

  if (
    data[0] != UINT8_C(0xFF) ||
    data[1] != UINT8_C(0x01)
  ) {
    fputs("FAIL: incorrect last-bit modification\n", stderr);
    return 1;
  }

  puts("PASS: flip last bit");
  return 0;
}

static int test_reject_out_of_range_bit(void)
{
  uint8_t data[] = {UINT8_C(0x5A)};
  const uint8_t expected = data[0];

  if (error_injection_flip_bit(
        data,
        sizeof data,
        8U
      )) {
    fputs("FAIL: out-of-range bit was accepted\n", stderr);
    return 1;
  }

  if (data[0] != expected) {
    fputs("FAIL: rejected operation modified data\n", stderr);
    return 1;
  }

  puts("PASS: reject out-of-range bit");
  return 0;
}

static int test_reject_null_data(void)
{
  if (error_injection_flip_bit(NULL, 0U, 0U)) {
    fputs("FAIL: null data was accepted\n", stderr);
    return 1;
  }

  puts("PASS: reject null data");
  return 0;
}

int main(void)
{
  int failures = 0;

  failures += test_flip_first_bit();
  failures += test_flip_last_bit();
  failures += test_reject_out_of_range_bit();
  failures += test_reject_null_data();

  return failures == 0 ? 0 : 1;
}