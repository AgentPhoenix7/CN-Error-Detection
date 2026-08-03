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

static int test_crc8_known_vector(void)
{
  const uint8_t data[] = "123456789";
  const uint32_t expected = UINT32_C(0xBC);
  uint32_t actual;

  actual = crc_compute(
    data,
    sizeof data - 1U,
    &crc8_parameters
  );

  if (actual != expected) {
    fprintf(
      stderr,
      "FAIL CRC-8 vector: expected 0x%02X, received 0x%02X\n",
      (unsigned int)expected,
      (unsigned int)actual
    );
    return 1;
  }

  puts("PASS: CRC-8 known vector");
  return 0;
}

static int test_crc10_known_vector(void)
{
  const uint8_t data[] = "123456789";
  const uint32_t expected = UINT32_C(0x199);
  uint32_t actual;

  actual = crc_compute(
    data,
    sizeof data - 1U,
    &crc10_parameters
  );

  if (actual != expected) {
    fprintf(
      stderr,
      "FAIL CRC-10 vector: expected 0x%03X, received 0x%03X\n",
      (unsigned int)expected,
      (unsigned int)actual
    );
    return 1;
  }

  puts("PASS: CRC-10 known vector");
  return 0;
}

static int test_crc16_known_vector(void)
{
  const uint8_t data[] = "123456789";
  const uint32_t expected = UINT32_C(0xFEE8);
  uint32_t actual;

  actual = crc_compute(
    data,
    sizeof data - 1U,
    &crc16_parameters
  );

  if (actual != expected) {
    fprintf(
      stderr,
      "FAIL CRC-16 vector: expected 0x%04X, received 0x%04X\n",
      (unsigned int)expected,
      (unsigned int)actual
    );
    return 1;
  }

  puts("PASS: CRC-16 known vector");
  return 0;
}

static int test_crc32_known_vector(void)
{
  const uint8_t data[] = "123456789";
  const uint32_t expected = UINT32_C(0x89A1897F);
  uint32_t actual;

  actual = crc_compute(
    data,
    sizeof data - 1U,
    &crc32_parameters
  );

  if (actual != expected) {
    fprintf(
      stderr,
      "FAIL CRC-32 vector: expected 0x%08X, received 0x%08X\n",
      (unsigned int)expected,
      (unsigned int)actual
    );
    return 1;
  }

  puts("PASS: CRC-32 known vector");
  return 0;
}

static int test_valid_crc8_verification(void)
{
  const uint8_t data[] = "123456789";
  const uint32_t received_crc = UINT32_C(0xBC);

  if (!crc_verify(
        data,
        sizeof data - 1U,
        received_crc,
        &crc8_parameters
      )) {
    fputs("FAIL: valid CRC-8 was rejected\n", stderr);
    return 1;
  }

  puts("PASS: valid CRC-8 verification");
  return 0;
}

static int test_corrupted_data_verification(void)
{
  uint8_t data[] = "123456789";
  const uint32_t received_crc = UINT32_C(0xBC);

  data[3] ^= UINT8_C(0x01);

  if (crc_verify(
        data,
        sizeof data - 1U,
        received_crc,
        &crc8_parameters
      )) {
    fputs("FAIL: corrupted CRC-8 data was accepted\n", stderr);
    return 1;
  }

  puts("PASS: corrupted CRC-8 data verification");
  return 0;
}

static int test_corrupted_crc_verification(void)
{
  const uint8_t data[] = "123456789";
  uint32_t received_crc = UINT32_C(0xBC);

  received_crc ^= UINT32_C(0x01);

  if (crc_verify(
        data,
        sizeof data - 1U,
        received_crc,
        &crc8_parameters
      )) {
    fputs("FAIL: corrupted CRC-8 remainder was accepted\n", stderr);
    return 1;
  }

  puts("PASS: corrupted CRC-8 remainder verification");
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
  failures += test_crc8_known_vector();
  failures += test_crc10_known_vector();
  failures += test_crc16_known_vector();
  failures += test_crc32_known_vector();
  failures += test_valid_crc8_verification();
  failures += test_corrupted_data_verification();
  failures += test_corrupted_crc_verification();

  return failures == 0 ? 0 : 1;
}
