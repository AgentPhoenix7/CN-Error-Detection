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

static int test_flip_three_separated_bits(void)
{
  uint8_t data[] = {
    UINT8_C(0x00),
    UINT8_C(0x00)
  };
  const size_t bit_indices[] = {0U, 7U, 12U};

  if (!error_injection_flip_odd_bits(
        data,
        sizeof data,
        bit_indices,
        sizeof bit_indices / sizeof bit_indices[0]
      )) {
    fputs("FAIL: valid odd-bit pattern was rejected\n", stderr);
    return 1;
  }

  if (
    data[0] != UINT8_C(0x81) ||
    data[1] != UINT8_C(0x08)
  ) {
    fputs("FAIL: incorrect odd-bit modification\n", stderr);
    return 1;
  }

  puts("PASS: flip three separated bits");
  return 0;
}

static int test_reject_even_bit_count(void)
{
  uint8_t data[] = {UINT8_C(0x5A)};
  const uint8_t expected = data[0];
  const size_t bit_indices[] = {0U, 7U};

  if (error_injection_flip_odd_bits(
        data,
        sizeof data,
        bit_indices,
        sizeof bit_indices / sizeof bit_indices[0]
      )) {
    fputs("FAIL: even bit count was accepted\n", stderr);
    return 1;
  }

  if (data[0] != expected) {
    fputs("FAIL: rejected even pattern modified data\n", stderr);
    return 1;
  }

  puts("PASS: reject even bit count");
  return 0;
}

static int test_reject_invalid_odd_bit_pattern(void)
{
  uint8_t data[] = {UINT8_C(0x5A)};
  const uint8_t expected = data[0];
  const size_t bit_indices[] = {0U, 7U, 8U};

  if (error_injection_flip_odd_bits(
        data,
        sizeof data,
        bit_indices,
        sizeof bit_indices / sizeof bit_indices[0]
      )) {
    fputs("FAIL: invalid odd-bit pattern was accepted\n", stderr);
    return 1;
  }

  if (data[0] != expected) {
    fputs("FAIL: invalid odd pattern modified data\n", stderr);
    return 1;
  }

  puts("PASS: reject invalid odd-bit pattern");
  return 0;
}

static int test_reject_duplicate_odd_bit_indices(void)
{
  uint8_t data[] = {UINT8_C(0xA5)};
  const uint8_t expected = data[0];
  const size_t bit_indices[] = {0U, 3U, 3U};

  if (error_injection_flip_odd_bits(
        data,
        sizeof data,
        bit_indices,
        sizeof bit_indices / sizeof bit_indices[0]
      )) {
    fputs("FAIL: duplicate odd-bit indices were accepted\n", stderr);
    return 1;
  }

  if (data[0] != expected) {
    fputs("FAIL: duplicate odd indices modified data\n", stderr);
    return 1;
  }

  puts("PASS: reject duplicate odd-bit indices");
  return 0;
}

static int test_reject_null_odd_bit_indices(void)
{
  uint8_t data[] = {UINT8_C(0x5A)};
  const uint8_t expected = data[0];

  if (error_injection_flip_odd_bits(
        data,
        sizeof data,
        NULL,
        3U
      )) {
    fputs("FAIL: null odd-bit indices were accepted\n", stderr);
    return 1;
  }

  if (data[0] != expected) {
    fputs("FAIL: null odd indices modified data\n", stderr);
    return 1;
  }

  puts("PASS: reject null odd-bit indices");
  return 0;
}

static int test_reject_zero_odd_bit_count(void)
{
  uint8_t data[] = {UINT8_C(0x5A)};
  const uint8_t expected = data[0];
  const size_t bit_indices[] = {0U};

  if (error_injection_flip_odd_bits(
        data,
        sizeof data,
        bit_indices,
        0U
      )) {
    fputs("FAIL: zero odd-bit count was accepted\n", stderr);
    return 1;
  }

  if (data[0] != expected) {
    fputs("FAIL: zero odd-bit count modified data\n", stderr);
    return 1;
  }

  puts("PASS: reject zero odd-bit count");
  return 0;
}

static int test_reject_null_odd_bit_data(void)
{
  const size_t bit_indices[] = {0U, 1U, 2U};

  if (error_injection_flip_odd_bits(
        NULL,
        0U,
        bit_indices,
        sizeof bit_indices / sizeof bit_indices[0]
      )) {
    fputs("FAIL: null odd-bit data was accepted\n", stderr);
    return 1;
  }

  puts("PASS: reject null odd-bit data");
  return 0;
}

static int test_flip_burst_across_byte_boundary(void)
{
  uint8_t data[] = {
    UINT8_C(0x00),
    UINT8_C(0x00)
  };

  if (!error_injection_flip_burst(
        data,
        sizeof data,
        6U,
        5U
      )) {
    fputs("FAIL: valid burst was accepted\n", stderr);
    return 1;
  }

  if (
    data[0] != UINT8_C(0x03) ||
    data[1] != UINT8_C(0xE0)
  ) {
    fputs("FAIL: incorrect burst modification\n", stderr);
    return 1;
  }

  puts("PASS: flip burst across byte boundary");
  return 0;
}

static int test_reject_zero_length_burst(void)
{
  uint8_t data[] = {UINT8_C(0x5A)};
  const uint8_t expected = data[0];

  if (error_injection_flip_burst(
        data,
        sizeof data,
        3U,
        0U
      )) {
    fputs("FAIL: zero-length burst was accepted\n", stderr);
    return 1;
  }

  if (data[0] != expected) {
    fputs("FAIL: zero-length burst modified data\n", stderr);
    return 1;
  }

  puts("PASS: reject zero-length burst");
  return 0;
}

static int test_reject_out_of_range_burst_start(void)
{
  uint8_t data[] = {UINT8_C(0xA5)};
  const uint8_t expected = data[0];

  if (error_injection_flip_burst(
        data,
        sizeof data,
        8U,
        1U
      )) {
    fputs("FAIL: out-of-range burst start was accepted\n", stderr);
    return 1;
  }

  if (data[0] != expected) {
    fputs("FAIL: out-of-range burst start modified data\n", stderr);
    return 1;
  }

  puts("PASS: reject out-of-range burst start");
  return 0;
}

static int test_reject_overrunning_burst(void)
{
  uint8_t data[] = {UINT8_C(0xA5)};
  const uint8_t expected = data[0];

  if (error_injection_flip_burst(
        data,
        sizeof data,
        6U,
        3U
      )) {
    fputs("FAIL: overrunning burst was accepted\n", stderr);
    return 1;
  }

  if (data[0] != expected) {
    fputs("FAIL: overrunning burst modified data\n", stderr);
    return 1;
  }

  puts("PASS: reject overrunning burst");
  return 0;
}

static int test_reject_null_burst_data(void)
{
  if (error_injection_flip_burst(
        NULL,
        1U,
        0U,
        1U
      )) {
    fputs("FAIL: null burst data was accepted\n", stderr);
    return 1;
  }

  puts("PASS: reject null burst data");
  return 0;
}

static int test_reject_overflowing_burst_range(void)
{
  uint8_t data[] = {UINT8_C(0xA5)};
  const uint8_t expected = data[0];

  if (error_injection_flip_burst(
        data,
        sizeof data,
        7U,
        SIZE_MAX
      )) {
    fputs("FAIL: overflowing burst range was accepted\n", stderr);
    return 1;
  }

  if (data[0] != expected) {
    fputs("FAIL: overflowing burst range modified data\n", stderr);
    return 1;
  }

  puts("PASS: reject overflowing burst range");
  return 0;
}

static int test_seed_rng_state(void)
{
  error_injection_rng_t rng = {UINT32_C(0)};
  const uint32_t seed = UINT32_C(0x12345678);

  if (!error_injection_rng_seed(&rng, seed)) {
    fputs("FAIL: valid RNG seed was rejected\n", stderr);
    return 1;
  }

  if (rng.state != seed) {
    fputs("FAIL: RNG state does not match seed\n", stderr);
    return 1;
  }

  puts("PASS: seed RNG state");
  return 0;
}

static int test_reject_null_rng_state(void)
{
  if (error_injection_rng_seed(
        NULL,
        UINT32_C(0x12345678)
      )) {
    fputs("FAIL: null RNG state was accepted\n", stderr);
    return 1;
  }

  puts("PASS: reject null RNG state");
  return 0;
}

static int test_reproduce_random_bit(void)
{
  uint8_t data[] = {UINT8_C(0x00)};
  error_injection_rng_t rng = {UINT32_C(0)};
  const uint32_t seed = UINT32_C(0x12345678);

  if (!error_injection_rng_seed(&rng, seed)) {
    fputs("FAIL: random-bit RNG setup failed\n", stderr);
    return 1;
  }

  if (!error_injection_flip_random_bit(
        data,
        sizeof data,
        &rng
      )) {
    fputs("FAIL: random bit was not flipped\n", stderr);
    return 1;
  }

  if (
    rng.state != UINT32_C(0x75432777) ||
    data[0] != UINT8_C(0x01)
  ) {
    fputs("FAIL: random-bit result was not reproducible\n", stderr);
    return 1;
  }

  puts("PASS: reproduce random bit");
  return 0;
}

static int test_reject_null_random_bit_data(void)
{
  error_injection_rng_t rng = {UINT32_C(0)};
  const uint32_t seed = UINT32_C(0x12345678);

  if (!error_injection_rng_seed(&rng, seed)) {
    fputs("FAIL: null-data RNG setup failed\n", stderr);
    return 1;
  }

  if (error_injection_flip_random_bit(
        NULL,
        1U,
        &rng
      )) {
    fputs("FAIL: null random-bit data was accepted\n", stderr);
    return 1;
  }

  if (rng.state != seed) {
    fputs("FAIL: null data advanced RNG state\n", stderr);
    return 1;
  }

  puts("PASS: reject null random-bit data");
  return 0;
}

static int test_reject_null_random_bit_rng(void)
{
  uint8_t data[] = {UINT8_C(0x5A)};
  const uint8_t expected = data[0];

  if (error_injection_flip_random_bit(
        data,
        sizeof data,
        NULL
      )) {
    fputs("FAIL: null random-bit RNG was accepted\n", stderr);
    return 1;
  }

  if (data[0] != expected) {
    fputs("FAIL: null RNG modified random-bit data\n", stderr);
    return 1;
  }

  puts("PASS: reject null random-bit RNG");
  return 0;
}

static int test_reject_zero_length_random_bit(void)
{
  uint8_t data[] = {UINT8_C(0x5A)};
  error_injection_rng_t rng = {UINT32_C(0)};
  const uint8_t expected = data[0];
  const uint32_t seed = UINT32_C(0x12345678);

  if (!error_injection_rng_seed(&rng, seed)) {
    fputs("FAIL: zero-length RNG setup failed\n", stderr);
    return 1;
  }

  if (error_injection_flip_random_bit(
        data,
        0U,
        &rng
      )) {
    fputs("FAIL: zero-length random bit was accepted\n", stderr);
    return 1;
  }

  if (data[0] != expected) {
    fputs("FAIL: zero length modified random-bit data\n", stderr);
    return 1;
  }

  if (rng.state != seed) {
    fputs("FAIL: zero length advanced RNG state\n", stderr);
    return 1;
  }

  puts("PASS: reject zero-length random bit");
  return 0;
}

static int test_reject_overflowing_random_bit_length(void)
{
  uint8_t data[] = {UINT8_C(0x5A)};
  error_injection_rng_t rng = {UINT32_C(0)};
  const uint8_t expected = data[0];
  const uint32_t seed = UINT32_C(0x12345678);
  const size_t overflowing_length = SIZE_MAX / 8U + 1U;

  if (!error_injection_rng_seed(&rng, seed)) {
    fputs("FAIL: overflow-length RNG setup failed\n", stderr);
    return 1;
  }

  if (error_injection_flip_random_bit(
        data,
        overflowing_length,
        &rng
      )) {
    fputs("FAIL: overflowing random-bit length was accepted\n", stderr);
    return 1;
  }

  if (data[0] != expected) {
    fputs("FAIL: overflowing length modified random-bit data\n", stderr);
    return 1;
  }

  if (rng.state != seed) {
    fputs("FAIL: overflowing length advanced RNG state\n", stderr);
    return 1;
  }

  puts("PASS: reject overflowing random-bit length");
  return 0;
}

static int test_reproduce_random_two_bits(void)
{
  uint8_t data[] = {UINT8_C(0x00)};
  error_injection_rng_t rng = {UINT32_C(0)};
  const uint32_t seed = UINT32_C(0x12345678);

  if (!error_injection_rng_seed(&rng, seed)) {
    fputs("FAIL: random two-bit RNG setup failed\n", stderr);
    return 1;
  }

  if (!error_injection_flip_random_two_bits(
        data,
        sizeof data,
        &rng
      )) {
    fputs("FAIL: random two-bit pattern was rejected\n", stderr);
    return 1;
  }

  if (
    rng.state != UINT32_C(0xCD305E6A) ||
    data[0] != UINT8_C(0x21)
  ) {
    fputs("FAIL: random two-bit result was not reproducible\n", stderr);
    return 1;
  }

  puts("PASS: reproduce random two-bit pattern");
  return 0;
}

static int test_reject_null_random_two_bit_data(void)
{
  error_injection_rng_t rng = {UINT32_C(0)};
  const uint32_t seed = UINT32_C(0x12345678);

  if (!error_injection_rng_seed(&rng, seed)) {
    fputs("FAIL: null two-bit data RNG setup failed\n", stderr);
    return 1;
  }

  if (error_injection_flip_random_two_bits(
        NULL,
        1U,
        &rng
      )) {
    fputs("FAIL: null random two-bit data was accepted\n", stderr);
    return 1;
  }

  if (rng.state != seed) {
    fputs("FAIL: null two-bit data advanced RNG state\n", stderr);
    return 1;
  }

  puts("PASS: reject null random two-bit data");
  return 0;
}

static int test_reject_null_random_two_bit_rng(void)
{
  uint8_t data[] = {UINT8_C(0x5A)};
  const uint8_t expected = data[0];

  if (error_injection_flip_random_two_bits(
        data,
        sizeof data,
        NULL
      )) {
    fputs("FAIL: null random two-bit RNG was accepted\n", stderr);
    return 1;
  }

  if (data[0] != expected) {
    fputs("FAIL: null RNG modified random two-bit data\n", stderr);
    return 1;
  }

  puts("PASS: reject null random two-bit RNG");
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
  failures += test_flip_three_separated_bits();
  failures += test_reject_even_bit_count();
  failures += test_reject_invalid_odd_bit_pattern();
  failures += test_reject_duplicate_odd_bit_indices();
  failures += test_reject_null_odd_bit_indices();
  failures += test_reject_zero_odd_bit_count();
  failures += test_reject_null_odd_bit_data();
  failures += test_flip_burst_across_byte_boundary();
  failures += test_reject_zero_length_burst();
  failures += test_reject_out_of_range_burst_start();
  failures += test_reject_overrunning_burst();
  failures += test_reject_null_burst_data();
  failures += test_reject_overflowing_burst_range();
  failures += test_seed_rng_state();
  failures += test_reject_null_rng_state();
  failures += test_reproduce_random_bit();
  failures += test_reject_null_random_bit_data();
  failures += test_reject_null_random_bit_rng();
  failures += test_reject_zero_length_random_bit();
  failures += test_reject_overflowing_random_bit_length();
  failures += test_reproduce_random_two_bits();
  failures += test_reject_null_random_two_bit_data();
  failures += test_reject_null_random_two_bit_rng();

  return failures == 0 ? 0 : 1;
}
