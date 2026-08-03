#include "crc.h"

#include <stdio.h>

static int test_parameters(
  const char *name,
  const crc_parameters_t *parameters,
  uint8_t expected_width,
  uint32_t expected_polynomial
)
{
  if (
    parameters->width != expected_width ||
    parameters->polynomial != expected_polynomial
  ) {
    fprintf(stderr, "FAIL: %s parameters\n", name);
    return 1;
  }

  printf("PASS: %s parameters\n", name);
  return 0;
}

static int test_empty_input(void)
{
  const uint32_t expected = 0U;
  uint32_t actual;

  actual = crc_compute(NULL, 0U, &crc32_parameters);

  if (actual != expected) {
    fprintf(
      stderr,
      "FAIL empty input: expected 0x%08X, received 0x%08X\n",
      (unsigned int)expected,
      (unsigned int)actual
    );
    return 1;
  }

  puts("PASS: empty-input CRC");
  return 0;
}

int main(void)
{
  int failures = 0;

  failures += test_parameters(
    "CRC-8",
    &crc8_parameters,
    8U,
    UINT32_C(0xD5)
  );
  failures += test_parameters(
    "CRC-10",
    &crc10_parameters,
    10U,
    UINT32_C(0x233)
  );
  failures += test_parameters(
    "CRC-16",
    &crc16_parameters,
    16U,
    UINT32_C(0x8005)
  );
  failures += test_parameters(
    "CRC-32",
    &crc32_parameters,
    32U,
    UINT32_C(0x04C11DB7)
  );
  failures += test_empty_input();

  return failures == 0 ? 0 : 1;
}
