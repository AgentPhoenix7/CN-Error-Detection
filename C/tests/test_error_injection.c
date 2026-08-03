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

static int test_flip_two_separated_bits(void)
{
  uint8_t data[] = {
    UINT8_C(0x00),
    UINT8_C(0x00)
  };

  if (!error_injection_flip_two_bits(
        data,
        sizeof data,
        1U,
        14U
      )) {
    fputs("FAIL: valid two-bit pattern was rejected\n", stderr);
    return 1;
  }

  if (
    data[0] != UINT8_C(0x40) ||
    data[1] != UINT8_C(0x02)
  ) {
    fputs("FAIL: incorrect two-bit modification\n", stderr);
    return 1;
  }

  puts("PASS: flip two separated bits");
  return 0;
}

static int test_reject_duplicate_bit_indices(void)
{
  uint8_t data[] = {UINT8_C(0xA5)};
  const uint8_t expected = data[0];

  if (error_injection_flip_two_bits(
        data,
        sizeof data,
        3U,
        3U
      )) {
    fputs("FAIL: duplicate bit indices were accepted\n", stderr);
    return 1;
  }

  if (data[0] != expected) {
    fputs("FAIL: duplicate indices modified data\n", stderr);
    return 1;
  }

  puts("PASS: reject duplicate bit indices");
  return 0;
}

static int test_reject_partially_invalid_two_bit_pattern(void)
{
  uint8_t data[] = {UINT8_C(0x00)};
  const uint8_t expected = data[0];

  if (error_injection_flip_two_bits(
        data,
        sizeof data,
        0U,
        8U
      )) {
    fputs("FAIL: partially invalid pattern was accepted\n", stderr);
    return 1;
  }

  if (data[0] != expected) {
    fputs("FAIL: partially invalid pattern modified data\n", stderr);
    return 1;
  }

  puts("PASS: reject partially invalid two-bit pattern");
  return 0;
}

static int test_reject_null_two_bit_data(void)
{
  if (error_injection_flip_two_bits(NULL, 0U, 0U, 1U)) {
    fputs("FAIL: null two-bit data was accepted\n", stderr);
    return 1;
  }

  puts("PASS: reject null two-bit data");
  return 0;
}

int main(void)
{
  int failures = 0;

  failures += test_flip_first_bit();
  failures += test_flip_last_bit();
  failures += test_reject_out_of_range_bit();
  failures += test_reject_null_data();
  failures += test_flip_two_separated_bits();
  failures += test_reject_duplicate_bit_indices();
  failures += test_reject_partially_invalid_two_bit_pattern();
  failures += test_reject_null_two_bit_data();

  return failures == 0 ? 0 : 1;
}
