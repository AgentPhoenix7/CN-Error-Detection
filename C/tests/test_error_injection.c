#include "error_injection.h"

#include <stdint.h>
#include <stdio.h>

/**
 * @brief Checks the MSB-first meaning of bit index zero.
 *
 * Flips bit zero of a zero byte and requires the resulting byte to be 0x80.
 */
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

/**
 * @brief Checks addressing of the final bit in a multibyte buffer.
 *
 * Flips bit 15 of two bytes and requires only the low bit of the second byte
 * to change.
 */
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

/**
 * @brief Checks rejection of a bit immediately beyond the buffer.
 *
 * Requests bit eight of a one-byte buffer and requires failure with the
 * original byte unchanged.
 */
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

/**
 * @brief Checks rejection of a null single-bit buffer.
 *
 * Calls the single-bit operation with NULL data and requires a false result.
 */
static int test_reject_null_data(void)
{
  if (error_injection_flip_bit(NULL, 0U, 0U)) {
    fputs("FAIL: null data was accepted\n", stderr);
    return 1;
  }

  puts("PASS: reject null data");
  return 0;
}

/**
 * @brief Checks a controlled two-bit pattern spanning two bytes.
 *
 * Flips positions one and 14 in zero bytes and compares the result with 0x40
 * and 0x02.
 */
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

/**
 * @brief Checks that a two-bit request cannot name one bit twice.
 *
 * Requests the same position twice and requires rejection without modifying
 * the original byte.
 */
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

/**
 * @brief Checks validation of both indices before either bit is changed.
 *
 * Combines a valid position with an out-of-range one and requires failure
 * with the buffer unchanged.
 */
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

/**
 * @brief Checks rejection of a null two-bit buffer.
 *
 * Calls the two-bit operation with NULL data and requires failure before any
 * mutation.
 */
static int test_reject_null_two_bit_data(void)
{
  if (error_injection_flip_two_bits(NULL, 0U, 0U, 1U)) {
    fputs("FAIL: null two-bit data was accepted\n", stderr);
    return 1;
  }

  puts("PASS: reject null two-bit data");
  return 0;
}

/**
 * @brief Checks an odd controlled pattern with distinct positions.
 *
 * Flips bits zero, seven, and 12 in two zero bytes and requires the fixed
 * 0x81, 0x08 result.
 */
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

/**
 * @brief Checks that controlled odd-pattern requests reject even counts.
 *
 * Supplies two valid indices and requires failure without changing the saved
 * byte.
 */
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

/**
 * @brief Checks atomic rejection of an out-of-range odd-pattern index.
 *
 * Supplies three indices with the last beyond the buffer and requires all
 * data to remain unchanged.
 */
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

/**
 * @brief Checks that controlled odd patterns require distinct indices.
 *
 * Supplies an odd-length index array containing a duplicate and requires
 * rejection without any mutation.
 */
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

/**
 * @brief Checks rejection of a missing odd-pattern index array.
 *
 * Passes NULL indices with an otherwise valid buffer and count, then checks
 * failure and unchanged data.
 */
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

/**
 * @brief Checks rejection of an empty controlled odd pattern.
 *
 * Passes zero as the index count and verifies a false result and unchanged
 * buffer.
 */
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

/**
 * @brief Checks rejection of a null controlled odd-pattern buffer.
 *
 * Supplies a valid three-position index array with NULL data and requires
 * failure.
 */
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

/**
 * @brief Checks a burst that crosses from one byte into the next.
 *
 * Flips five consecutive bits beginning at bit six and requires the
 * resulting bytes 0x03 and 0xE0.
 */
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

/**
 * @brief Checks that a controlled burst must contain at least one bit.
 *
 * Requests zero burst length and requires failure with the saved byte
 * unchanged.
 */
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

/**
 * @brief Checks rejection of a burst starting beyond the buffer.
 *
 * Starts at bit eight of one byte and requires failure without modifying
 * that byte.
 */
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

/**
 * @brief Checks validation of a burst's final endpoint.
 *
 * Starts a three-bit burst at bit six of one byte and requires rejection
 * without partial flips.
 */
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

/**
 * @brief Checks rejection of a null controlled-burst buffer.
 *
 * Requests a nonempty burst with NULL data and requires failure.
 */
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

/**
 * @brief Checks overflow-safe calculation of the burst's endpoint.
 *
 * Combines a valid start with SIZE_MAX as the burst length and verifies
 * rejection without changing data.
 */
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

/**
 * @brief Checks that seeding stores the requested 32-bit state exactly.
 *
 * Seeds an initialized generator with 0x12345678 and compares its state
 * field with that value.
 */
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

/**
 * @brief Checks rejection of a missing generator during seeding.
 *
 * Passes NULL to error_injection_rng_seed and requires failure.
 */
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

/**
 * @brief Checks the deterministic single-bit mutation sequence.
 *
 * Seeds the generator with the fixed test seed, flips one random bit, and
 * compares both the mutated byte and final RNG state with pinned values.
 */
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
    data[0] != UINT8_C(0x10)
  ) {
    fputs("FAIL: random-bit result was not reproducible\n", stderr);
    return 1;
  }

  puts("PASS: reproduce random bit");
  return 0;
}

/**
 * @brief Checks rejection of null data in the random single-bit operation.
 *
 * Seeds the generator, supplies NULL data, and requires failure without
 * advancing its state.
 */
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

/**
 * @brief Checks rejection of a missing RNG in the random single-bit
 * operation.
 *
 * Passes a valid saved buffer with NULL generator state and requires failure
 * without changing its bytes.
 */
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

/**
 * @brief Checks rejection of an empty random single-bit buffer.
 *
 * Passes zero buffer length with saved data and a seeded generator,
 * requiring failure with both unchanged.
 */
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

/**
 * @brief Checks overflow-safe rejection of a random single-bit buffer
 * length.
 *
 * Supplies SIZE_MAX divided by eight plus one as the byte count and checks
 * that failure preserves both data and RNG state.
 */
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

/**
 * @brief Checks reproducible selection of two nonadjacent bits.
 *
 * Applies the seeded two-bit operation and compares its byte result and
 * final generator state with the stored vector.
 */
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
    data[0] != UINT8_C(0x12)
  ) {
    fputs("FAIL: random two-bit result was not reproducible\n", stderr);
    return 1;
  }

  puts("PASS: reproduce random two-bit pattern");
  return 0;
}

/**
 * @brief Checks rejection of null data in the random two-bit operation.
 *
 * Seeds the generator, supplies NULL data, and requires failure without
 * advancing its state.
 */
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

/**
 * @brief Checks rejection of a missing RNG in the random two-bit operation.
 *
 * Passes a valid saved buffer with NULL generator state and requires failure
 * without changing its bytes.
 */
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

/**
 * @brief Checks rejection of an empty random two-bit buffer.
 *
 * Passes zero buffer length with saved data and a seeded generator,
 * requiring failure with both unchanged.
 */
static int test_reject_zero_length_random_two_bits(void)
{
  uint8_t data[] = {UINT8_C(0x5A)};
  error_injection_rng_t rng = {UINT32_C(0)};
  const uint8_t expected = data[0];
  const uint32_t seed = UINT32_C(0x12345678);

  if (!error_injection_rng_seed(&rng, seed)) {
    fputs("FAIL: zero-length two-bit RNG setup failed\n", stderr);
    return 1;
  }

  if (error_injection_flip_random_two_bits(
        data,
        0U,
        &rng
      )) {
    fputs("FAIL: zero-length random two-bit pattern was accepted\n", stderr);
    return 1;
  }

  if (data[0] != expected) {
    fputs("FAIL: zero length modified random two-bit data\n", stderr);
    return 1;
  }

  if (rng.state != seed) {
    fputs("FAIL: zero length advanced random two-bit RNG state\n", stderr);
    return 1;
  }

  puts("PASS: reject zero-length random two-bit pattern");
  return 0;
}

/**
 * @brief Checks overflow-safe rejection of a random two-bit buffer length.
 *
 * Supplies SIZE_MAX divided by eight plus one as the byte count and checks
 * that failure preserves both data and RNG state.
 */
static int test_reject_overflowing_random_two_bit_length(void)
{
  uint8_t data[] = {UINT8_C(0x5A)};
  error_injection_rng_t rng = {UINT32_C(0)};
  const uint8_t expected = data[0];
  const uint32_t seed = UINT32_C(0x12345678);
  const size_t overflowing_length = SIZE_MAX / 8U + 1U;

  if (!error_injection_rng_seed(&rng, seed)) {
    fputs("FAIL: overflow-length two-bit RNG setup failed\n", stderr);
    return 1;
  }

  if (error_injection_flip_random_two_bits(
        data,
        overflowing_length,
        &rng
      )) {
    fputs("FAIL: overflowing random two-bit length was accepted\n", stderr);
    return 1;
  }

  if (data[0] != expected) {
    fputs("FAIL: overflowing length modified random two-bit data\n", stderr);
    return 1;
  }

  if (rng.state != seed) {
    fputs("FAIL: overflowing length advanced random two-bit RNG state\n", stderr);
    return 1;
  }

  puts("PASS: reject overflowing random two-bit length");
  return 0;
}

/**
 * @brief Checks reproducible selection of three distinct random positions.
 *
 * Applies the seeded odd-bit operation and pins both the changed byte and
 * the generator state after subset selection.
 */
static int test_reproduce_random_odd_bits(void)
{
  uint8_t data[] = {UINT8_C(0x00)};
  error_injection_rng_t rng = {UINT32_C(0)};
  const uint32_t seed = UINT32_C(0x12345678);

  if (!error_injection_rng_seed(&rng, seed)) {
    fputs("FAIL: random odd-bit RNG setup failed\n", stderr);
    return 1;
  }

  if (!error_injection_flip_random_odd_bits(
        data,
        sizeof data,
        3U,
        &rng
      )) {
    fputs("FAIL: random odd-bit pattern was rejected\n", stderr);
    return 1;
  }

  if (
    rng.state != UINT32_C(0x25DBFAC1) ||
    data[0] != UINT8_C(0x64)
  ) {
    fputs("FAIL: random odd-bit result was not reproducible\n", stderr);
    return 1;
  }

  puts("PASS: reproduce random odd-bit pattern");
  return 0;
}

/**
 * @brief Checks rejection of null data in the random odd-bit operation.
 *
 * Seeds the generator, supplies NULL data, and requires failure without
 * advancing its state.
 */
static int test_reject_null_random_odd_bit_data(void)
{
  error_injection_rng_t rng = {UINT32_C(0)};
  const uint32_t seed = UINT32_C(0x12345678);

  if (!error_injection_rng_seed(&rng, seed)) {
    fputs("FAIL: null odd-bit data RNG setup failed\n", stderr);
    return 1;
  }

  if (error_injection_flip_random_odd_bits(
        NULL,
        1U,
        3U,
        &rng
      )) {
    fputs("FAIL: null random odd-bit data was accepted\n", stderr);
    return 1;
  }

  if (rng.state != seed) {
    fputs("FAIL: null odd-bit data advanced RNG state\n", stderr);
    return 1;
  }

  puts("PASS: reject null random odd-bit data");
  return 0;
}

/**
 * @brief Checks rejection of a missing RNG in the random odd-bit operation.
 *
 * Passes a valid saved buffer with NULL generator state and requires failure
 * without changing its bytes.
 */
static int test_reject_null_random_odd_bit_rng(void)
{
  uint8_t data[] = {UINT8_C(0x5A)};
  const uint8_t expected = data[0];

  if (error_injection_flip_random_odd_bits(
        data,
        sizeof data,
        3U,
        NULL
      )) {
    fputs("FAIL: null random odd-bit RNG was accepted\n", stderr);
    return 1;
  }

  if (data[0] != expected) {
    fputs("FAIL: null RNG modified random odd-bit data\n", stderr);
    return 1;
  }

  puts("PASS: reject null random odd-bit RNG");
  return 0;
}

/**
 * @brief Checks rejection of an empty random odd-bit buffer.
 *
 * Passes zero buffer length with saved data and a seeded generator,
 * requiring failure with both unchanged.
 */
static int test_reject_zero_length_random_odd_bits(void)
{
  uint8_t data[] = {UINT8_C(0x5A)};
  error_injection_rng_t rng = {UINT32_C(0)};
  const uint8_t expected = data[0];
  const uint32_t seed = UINT32_C(0x12345678);

  if (!error_injection_rng_seed(&rng, seed)) {
    fputs("FAIL: zero-length odd-bit RNG setup failed\n", stderr);
    return 1;
  }

  if (error_injection_flip_random_odd_bits(
        data,
        0U,
        3U,
        &rng
      )) {
    fputs("FAIL: zero-length random odd-bit pattern was accepted\n", stderr);
    return 1;
  }

  if (data[0] != expected) {
    fputs("FAIL: zero length modified random odd-bit data\n", stderr);
    return 1;
  }

  if (rng.state != seed) {
    fputs("FAIL: zero length advanced random odd-bit RNG state\n", stderr);
    return 1;
  }

  puts("PASS: reject zero-length random odd-bit pattern");
  return 0;
}

/**
 * @brief Checks overflow-safe rejection of a random odd-bit buffer length.
 *
 * Supplies SIZE_MAX divided by eight plus one as the byte count and checks
 * that failure preserves both data and RNG state.
 */
static int test_reject_overflowing_random_odd_bit_length(void)
{
  uint8_t data[] = {UINT8_C(0x5A)};
  error_injection_rng_t rng = {UINT32_C(0)};
  const uint8_t expected = data[0];
  const uint32_t seed = UINT32_C(0x12345678);
  const size_t overflowing_length = SIZE_MAX / 8U + 1U;

  if (!error_injection_rng_seed(&rng, seed)) {
    fputs("FAIL: overflow-length odd-bit RNG setup failed\n", stderr);
    return 1;
  }

  if (error_injection_flip_random_odd_bits(
        data,
        overflowing_length,
        3U,
        &rng
      )) {
    fputs("FAIL: overflowing random odd-bit length was accepted\n", stderr);
    return 1;
  }

  if (data[0] != expected) {
    fputs("FAIL: overflowing length modified random odd-bit data\n", stderr);
    return 1;
  }

  if (rng.state != seed) {
    fputs("FAIL: overflowing length advanced random odd-bit RNG state\n", stderr);
    return 1;
  }

  puts("PASS: reject overflowing random odd-bit length");
  return 0;
}

/**
 * @brief Checks rejection of a zero random odd-pattern count.
 *
 * Requests no flipped bits from a valid buffer and seeded generator,
 * requiring failure without changing either.
 */
static int test_reject_zero_random_odd_bit_count(void)
{
  uint8_t data[] = {UINT8_C(0x5A)};
  error_injection_rng_t rng = {UINT32_C(0)};
  const uint8_t expected = data[0];
  const uint32_t seed = UINT32_C(0x12345678);

  if (!error_injection_rng_seed(&rng, seed)) {
    fputs("FAIL: zero-count odd-bit RNG setup failed\n", stderr);
    return 1;
  }

  if (error_injection_flip_random_odd_bits(
        data,
        sizeof data,
        0U,
        &rng
      )) {
    fputs("FAIL: zero random odd-bit count was accepted\n", stderr);
    return 1;
  }

  if (data[0] != expected) {
    fputs("FAIL: zero count modified random odd-bit data\n", stderr);
    return 1;
  }

  if (rng.state != seed) {
    fputs("FAIL: zero count advanced random odd-bit RNG state\n", stderr);
    return 1;
  }

  puts("PASS: reject zero random odd-bit count");
  return 0;
}

/**
 * @brief Checks rejection of an even random odd-pattern count.
 *
 * Requests two flipped bits through the odd-pattern interface and requires
 * failure with bytes and RNG state intact.
 */
static int test_reject_even_random_odd_bit_count(void)
{
  uint8_t data[] = {UINT8_C(0x5A)};
  error_injection_rng_t rng = {UINT32_C(0)};
  const uint8_t expected = data[0];
  const uint32_t seed = UINT32_C(0x12345678);

  if (!error_injection_rng_seed(&rng, seed)) {
    fputs("FAIL: even-count odd-bit RNG setup failed\n", stderr);
    return 1;
  }

  if (error_injection_flip_random_odd_bits(
        data,
        sizeof data,
        2U,
        &rng
      )) {
    fputs("FAIL: even random odd-bit count was accepted\n", stderr);
    return 1;
  }

  if (data[0] != expected) {
    fputs("FAIL: even count modified random odd-bit data\n", stderr);
    return 1;
  }

  if (rng.state != seed) {
    fputs("FAIL: even count advanced random odd-bit RNG state\n", stderr);
    return 1;
  }

  puts("PASS: reject even random odd-bit count");
  return 0;
}

/**
 * @brief Checks that a random subset cannot exceed the buffer's bit count.
 *
 * Requests nine distinct bits from a one-byte buffer and requires rejection
 * before any data or RNG change.
 */
static int test_reject_excessive_random_odd_bit_count(void)
{
  uint8_t data[] = {UINT8_C(0x5A)};
  error_injection_rng_t rng = {UINT32_C(0)};
  const uint8_t expected = data[0];
  const uint32_t seed = UINT32_C(0x12345678);

  if (!error_injection_rng_seed(&rng, seed)) {
    fputs("FAIL: excessive-count odd-bit RNG setup failed\n", stderr);
    return 1;
  }

  if (error_injection_flip_random_odd_bits(
        data,
        sizeof data,
        9U,
        &rng
      )) {
    fputs("FAIL: excessive random odd-bit count was accepted\n", stderr);
    return 1;
  }

  if (data[0] != expected) {
    fputs("FAIL: excessive count modified random odd-bit data\n", stderr);
    return 1;
  }

  if (rng.state != seed) {
    fputs("FAIL: excessive count advanced random odd-bit RNG state\n", stderr);
    return 1;
  }

  puts("PASS: reject excessive random odd-bit count");
  return 0;
}

/**
 * @brief Checks rejection of an unrepresentable random index-array request.
 *
 * Supplies a very large byte length and an odd count above the index-array
 * size limit, then checks failure with bytes and RNG state unchanged.
 * Length validation may reject the request before the allocation guard.
 */
static int test_reject_overflowing_random_odd_bit_allocation(void)
{
  uint8_t data[] = {UINT8_C(0x5A)};
  error_injection_rng_t rng = {UINT32_C(0)};
  const uint8_t expected = data[0];
  const uint32_t seed = UINT32_C(0x12345678);
  const size_t large_length = SIZE_MAX / 8U;
  const size_t overflowing_bit_count =
    SIZE_MAX / sizeof(size_t) + 2U;

  if (!error_injection_rng_seed(&rng, seed)) {
    fputs("FAIL: allocation-overflow RNG setup failed\n", stderr);
    return 1;
  }

  if (error_injection_flip_random_odd_bits(
        data,
        large_length,
        overflowing_bit_count,
        &rng
      )) {
    fputs("FAIL: overflowing odd-bit allocation was accepted\n", stderr);
    return 1;
  }

  if (data[0] != expected) {
    fputs("FAIL: allocation overflow modified random odd-bit data\n", stderr);
    return 1;
  }

  if (rng.state != seed) {
    fputs("FAIL: allocation overflow advanced odd-bit RNG state\n", stderr);
    return 1;
  }

  puts("PASS: reject overflowing random odd-bit allocation");
  return 0;
}

/**
 * @brief Checks Floyd sampling when a candidate repeats a selected position.
 *
 * Requests 13 distinct bits from 24 using a fixed seed and compares all
 * three mutated bytes and the final RNG state with the collision-handling
 * vector.
 */
static int test_reselect_duplicate_random_odd_bits(void)
{
  uint8_t data[] = {
    UINT8_C(0x00),
    UINT8_C(0x00),
    UINT8_C(0x00)
  };
  error_injection_rng_t rng = {UINT32_C(0)};
  const uint32_t seed = UINT32_C(0x12345678);

  if (!error_injection_rng_seed(&rng, seed)) {
    fputs("FAIL: duplicate-selection RNG setup failed\n", stderr);
    return 1;
  }

  if (!error_injection_flip_random_odd_bits(
        data,
        sizeof data,
        13U,
        &rng
      )) {
    fputs("FAIL: duplicate random positions were not reselected\n", stderr);
    return 1;
  }

  if (
    rng.state != UINT32_C(0x436C41A3) ||
    data[0] != UINT8_C(0x2E) ||
    data[1] != UINT8_C(0xE9) ||
    data[2] != UINT8_C(0xAC)
  ) {
    fputs("FAIL: duplicate odd-bit reselection was not reproducible\n", stderr);
    return 1;
  }

  puts("PASS: reselect duplicate random odd-bit positions");
  return 0;
}

/**
 * @brief Checks reproducible placement of a contiguous random burst.
 *
 * Applies a seeded seven-bit burst to two zero bytes and checks the expected
 * mask and generator state.
 */
static int test_reproduce_random_burst(void)
{
  uint8_t data[] = {
    UINT8_C(0x00),
    UINT8_C(0x00)
  };
  error_injection_rng_t rng = {UINT32_C(0)};
  const uint32_t seed = UINT32_C(0x12345678);

  if (!error_injection_rng_seed(&rng, seed)) {
    fputs("FAIL: random burst RNG setup failed\n", stderr);
    return 1;
  }

  if (!error_injection_flip_random_burst(
        data,
        sizeof data,
        7U,
        &rng
      )) {
    fputs("FAIL: random burst was rejected\n", stderr);
    return 1;
  }

  if (
    rng.state != UINT32_C(0x75432777) ||
    data[0] != UINT8_C(0x0F) ||
    data[1] != UINT8_C(0xE0)
  ) {
    fputs("FAIL: random burst result was not reproducible\n", stderr);
    return 1;
  }

  puts("PASS: reproduce random burst");
  return 0;
}

/**
 * @brief Checks rejection of null data in the random burst operation.
 *
 * Seeds the generator, supplies NULL data, and requires failure without
 * advancing its state.
 */
static int test_reject_null_random_burst_data(void)
{
  error_injection_rng_t rng = {UINT32_C(0)};
  const uint32_t seed = UINT32_C(0x12345678);

  if (!error_injection_rng_seed(&rng, seed)) {
    fputs("FAIL: null random burst data RNG setup failed\n", stderr);
    return 1;
  }

  if (error_injection_flip_random_burst(
        NULL,
        1U,
        1U,
        &rng
      )) {
    fputs("FAIL: null random burst data was accepted\n", stderr);
    return 1;
  }

  if (rng.state != seed) {
    fputs("FAIL: null random burst data advanced RNG state\n", stderr);
    return 1;
  }

  puts("PASS: reject null random burst data");
  return 0;
}

/**
 * @brief Checks rejection of a missing RNG in the random burst operation.
 *
 * Passes a valid saved buffer with NULL generator state and requires failure
 * without changing its bytes.
 */
static int test_reject_null_random_burst_rng(void)
{
  uint8_t data[] = {UINT8_C(0x5A)};
  const uint8_t expected = data[0];

  if (error_injection_flip_random_burst(
        data,
        sizeof data,
        3U,
        NULL
      )) {
    fputs("FAIL: null random burst RNG was accepted\n", stderr);
    return 1;
  }

  if (data[0] != expected) {
    fputs("FAIL: null random burst RNG modified data\n", stderr);
    return 1;
  }

  puts("PASS: reject null random burst RNG");
  return 0;
}

/**
 * @brief Checks rejection of an empty random burst buffer.
 *
 * Passes zero buffer length with saved data and a seeded generator,
 * requiring failure with both unchanged.
 */
static int test_reject_zero_length_random_burst(void)
{
  uint8_t data[] = {UINT8_C(0x5A)};
  const uint8_t expected = data[0];
  error_injection_rng_t rng = {UINT32_C(0)};
  const uint32_t seed = UINT32_C(0x12345678);

  if (!error_injection_rng_seed(&rng, seed)) {
    fputs("FAIL: zero-length random burst RNG setup failed\n", stderr);
    return 1;
  }

  if (error_injection_flip_random_burst(
        data,
        0U,
        1U,
        &rng
      )) {
    fputs("FAIL: zero-length random burst was accepted\n", stderr);
    return 1;
  }

  if (data[0] != expected) {
    fputs("FAIL: zero-length random burst modified data\n", stderr);
    return 1;
  }

  if (rng.state != seed) {
    fputs("FAIL: zero-length random burst advanced RNG state\n", stderr);
    return 1;
  }

  puts("PASS: reject zero-length random burst");
  return 0;
}

/**
 * @brief Checks overflow-safe rejection of a random burst buffer length.
 *
 * Supplies SIZE_MAX divided by eight plus one as the byte count and checks
 * that failure preserves both data and RNG state.
 */
static int test_reject_overflowing_random_burst_length(void)
{
  uint8_t data[] = {UINT8_C(0x5A)};
  const uint8_t expected = data[0];
  error_injection_rng_t rng = {UINT32_C(0)};
  const uint32_t seed = UINT32_C(0x12345678);
  const size_t overflowing_length = SIZE_MAX / 8U + 1U;

  if (!error_injection_rng_seed(&rng, seed)) {
    fputs("FAIL: overflowing random burst RNG setup failed\n", stderr);
    return 1;
  }

  if (error_injection_flip_random_burst(
        data,
        overflowing_length,
        1U,
        &rng
      )) {
    fputs("FAIL: overflowing random burst length was accepted\n", stderr);
    return 1;
  }

  if (data[0] != expected) {
    fputs("FAIL: overflowing random burst length modified data\n", stderr);
    return 1;
  }

  if (rng.state != seed) {
    fputs("FAIL: overflowing random burst length advanced RNG state\n", stderr);
    return 1;
  }

  puts("PASS: reject overflowing random burst length");
  return 0;
}

/**
 * @brief Checks that a random burst must have positive length.
 *
 * Requests a zero-bit burst in a valid buffer and requires failure without
 * changing data or the seeded generator.
 */
static int test_reject_zero_random_burst_length(void)
{
  uint8_t data[] = {UINT8_C(0x5A)};
  const uint8_t expected = data[0];
  error_injection_rng_t rng = {UINT32_C(0)};
  const uint32_t seed = UINT32_C(0x12345678);

  if (!error_injection_rng_seed(&rng, seed)) {
    fputs("FAIL: zero random burst length RNG setup failed\n", stderr);
    return 1;
  }

  if (error_injection_flip_random_burst(
        data,
        sizeof data,
        0U,
        &rng
      )) {
    fputs("FAIL: zero random burst length was accepted\n", stderr);
    return 1;
  }

  if (data[0] != expected) {
    fputs("FAIL: zero random burst length modified data\n", stderr);
    return 1;
  }

  if (rng.state != seed) {
    fputs("FAIL: zero random burst length advanced RNG state\n", stderr);
    return 1;
  }

  puts("PASS: reject zero random burst length");
  return 0;
}

/**
 * @brief Checks that a random burst must fit entirely within its buffer.
 *
 * Requests nine consecutive bits in one byte and verifies rejection with
 * bytes and RNG state unchanged.
 */
static int test_reject_excessive_random_burst_length(void)
{
  uint8_t data[] = {UINT8_C(0x5A)};
  const uint8_t expected = data[0];
  error_injection_rng_t rng = {UINT32_C(0)};
  const uint32_t seed = UINT32_C(0x12345678);

  if (!error_injection_rng_seed(&rng, seed)) {
    fputs("FAIL: excessive random burst RNG setup failed\n", stderr);
    return 1;
  }

  if (error_injection_flip_random_burst(
        data,
        sizeof data,
        9U,
        &rng
      )) {
    fputs("FAIL: excessive random burst length was accepted\n", stderr);
    return 1;
  }

  if (data[0] != expected) {
    fputs("FAIL: excessive random burst length modified data\n", stderr);
    return 1;
  }

  if (rng.state != seed) {
    fputs("FAIL: excessive random burst length advanced RNG state\n", stderr);
    return 1;
  }

  puts("PASS: reject excessive random burst length");
  return 0;
}

/**
 * @brief Checks the single available start of a buffer-sized random burst.
 *
 * Requests all eight bits of one byte, requires its bitwise complement, and
 * checks that the RNG still advanced by the expected draw.
 */
static int test_flip_full_length_random_burst(void)
{
  uint8_t data[] = {UINT8_C(0x5A)};
  error_injection_rng_t rng = {UINT32_C(0)};
  const uint32_t seed = UINT32_C(0x12345678);

  if (!error_injection_rng_seed(&rng, seed)) {
    fputs("FAIL: full-length random burst RNG setup failed\n", stderr);
    return 1;
  }

  if (!error_injection_flip_random_burst(
        data,
        sizeof data,
        8U,
        &rng
      )) {
    fputs("FAIL: full-length random burst was rejected\n", stderr);
    return 1;
  }

  if (
    rng.state != UINT32_C(0x75432777) ||
    data[0] != UINT8_C(0xA5)
  ) {
    fputs("FAIL: full-length random burst result was incorrect\n", stderr);
    return 1;
  }

  puts("PASS: flip full-length random burst");
  return 0;
}

/**
 * @brief Checks the seeded frame-selection decision sequence.
 *
 * Compares twelve denominator-two decisions with a fixed boolean array and
 * verifies the final generator state.
 */
static int test_reproduce_random_frame_selection(void)
{
  static const bool expected[] = {
    true, false, false, true, true, true,
    true, false, false, false, false, false
  };
  error_injection_rng_t rng = {UINT32_C(0)};
  size_t index;

  if (!error_injection_rng_seed(&rng, UINT32_C(0x8C032FC1))) {
    fputs("FAIL: frame-selection RNG setup failed\n", stderr);
    return 1;
  }

  for (index = 0U; index < sizeof expected / sizeof expected[0]; index++) {
    bool selected = !expected[index];

    if (
      !error_injection_select_frame(&rng, UINT32_C(2), &selected) ||
      selected != expected[index]
    ) {
      fprintf(stderr, "FAIL: frame-selection decision %zu\n", index);
      return 1;
    }
  }

  if (rng.state != UINT32_C(0xEA0EB6CD)) {
    fputs("FAIL: frame-selection final RNG state\n", stderr);
    return 1;
  }

  puts("PASS: reproduce random frame selection");
  return 0;
}

/**
 * @brief Checks denominator-one selection without consuming randomness.
 *
 * Seeds the generator, requests selection with denominator one, and requires
 * true selection with the original state intact.
 */
static int test_select_every_frame_without_advancing_rng(void)
{
  error_injection_rng_t rng = {UINT32_C(0)};
  const uint32_t seed = UINT32_C(0x12345678);
  bool selected = false;

  if (
    !error_injection_rng_seed(&rng, seed) ||
    !error_injection_select_frame(&rng, UINT32_C(1), &selected) ||
    !selected ||
    rng.state != seed
  ) {
    fputs("FAIL: select every frame without advancing RNG\n", stderr);
    return 1;
  }

  puts("PASS: select every frame without advancing RNG");
  return 0;
}

/**
 * @brief Checks atomic rejection of invalid frame-selection arguments.
 *
 * Passes denominator zero and null pointers, requiring failure while the
 * selection output and generator state retain their saved values.
 */
static int test_reject_invalid_frame_selection(void)
{
  error_injection_rng_t rng = {UINT32_C(0)};
  const uint32_t seed = UINT32_C(0x12345678);
  bool selected = true;

  if (!error_injection_rng_seed(&rng, seed)) {
    fputs("FAIL: invalid frame-selection RNG setup failed\n", stderr);
    return 1;
  }

  if (
    error_injection_select_frame(&rng, UINT32_C(0), &selected) ||
    rng.state != seed ||
    !selected ||
    error_injection_select_frame(NULL, UINT32_C(2), &selected) ||
    !selected ||
    error_injection_select_frame(&rng, UINT32_C(2), NULL) ||
    rng.state != seed
  ) {
    fputs("FAIL: invalid frame selection changed output or RNG\n", stderr);
    return 1;
  }

  puts("PASS: reject invalid frame selection");
  return 0;
}

/**
 * @brief Checks that random two-bit choices cover parity and checksum
 * weights.
 *
 * Samples 10000 frame-sized buffers, locates exactly two nonadjacent changed
 * bits, and checks frequencies for matching parity and modulo-16 positions.
 */
static int test_two_bit_sampling_covers_checksum_weights(void)
{
  error_injection_rng_t rng = {UINT32_C(0x12345678)};
  size_t same_parity = 0U;
  size_t same_weight = 0U;
  for (size_t trial = 0U; trial < 10000U; trial++) {
    uint8_t data[1518] = {0};
    size_t positions[2] = {0};
    size_t count = 0U;
    if (!error_injection_flip_random_two_bits(data, sizeof data, &rng)) return 1;
    for (size_t bit = 0U; bit < sizeof data * 8U; bit++) {
      if ((data[bit / 8U] & (0x80U >> (bit % 8U))) != 0U) {
        if (count == 2U) return 1;
        positions[count++] = bit;
      }
    }
    if (count != 2U || positions[1] - positions[0] <= 1U) return 1;
    same_parity += positions[0] % 2U == positions[1] % 2U;
    same_weight += positions[0] % 16U == positions[1] % 16U;
  }
  if (same_parity < 4500U || same_parity > 5500U || same_weight < 400U) {
    fprintf(stderr, "FAIL: biased two-bit sampling: parity=%zu, weight=%zu\n", same_parity, same_weight);
    return 1;
  }
  puts("PASS: two-bit sampling covers parity and checksum weights");
  return 0;
}

/**
 * @brief Checks bounded mapping retries, maximum bounds, and invalid-input
 * atomicity.
 *
 * Pins a seed needing rejected products, verifies its accepted value and
 * final state, and exercises null/zero/oversized arguments plus the 2^32
 * bound where representable.
 */
static int test_bounded_rng_rejection_and_atomicity(void)
{
  error_injection_rng_t rng = {UINT32_C(0x12345678)};
  size_t value = 99U;
  if (error_injection_rng_bounded(&rng, 0U, &value) ||
      error_injection_rng_bounded(NULL, 4U, &value) ||
      error_injection_rng_bounded(&rng, 4U, NULL) ||
      rng.state != UINT32_C(0x12345678) || value != 99U) return 1;
  rng.state = UINT32_C(0x25DBFAC1);
  /* Low products 0x4B5C952C and 0x04DE0E9B are below 0x7FFFFFFF;
     the third candidate is the first accepted draw. */
  if (!error_injection_rng_bounded(&rng, (size_t)UINT32_C(0x80000001), &value) ||
      value != (size_t)UINT32_C(0x7155399F) ||
      rng.state != UINT32_C(0xE2AA733E)) return 1;
#if SIZE_MAX > UINT32_MAX
  const size_t full_range = (size_t)UINT64_C(0x100000000);
  if (error_injection_rng_bounded(&rng, full_range + 1U, &value) ||
      rng.state != UINT32_C(0xE2AA733E)) return 1;
  if (!error_injection_rng_bounded(&rng, full_range, &value) ||
      value != (size_t)UINT32_C(0xEA0F8185)) return 1;
#endif
  puts("PASS: bounded RNG handles rejection, limits, and atomic invalid input");
  return 0;
}

/**
 * @brief Checks rejection of buffers beyond the RNG's 2^32-bit range.
 *
 * On wide size_t platforms, calls every random mutation with a length just
 * over that limit and checks failure with unchanged bytes and RNG state.
 */
static int test_reject_rng_unaddressable_buffers(void)
{
#if SIZE_MAX > UINT32_MAX
  uint8_t data = UINT8_C(0xA5);
  error_injection_rng_t rng = {UINT32_C(0x12345678)};
  const size_t length = (size_t)(UINT64_C(0x100000000) / 8U + 1U);
  if (error_injection_flip_random_bit(&data, length, &rng) ||
      error_injection_flip_random_two_bits(&data, length, &rng) ||
      error_injection_flip_random_odd_bits(&data, length, 3U, &rng) ||
      error_injection_flip_random_burst(&data, length, 7U, &rng) ||
      data != UINT8_C(0xA5) || rng.state != UINT32_C(0x12345678)) return 1;
#endif
  puts("PASS: reject buffers beyond RNG range atomically");
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

  failures += test_two_bit_sampling_covers_checksum_weights();
  failures += test_bounded_rng_rejection_and_atomicity();
  failures += test_reject_rng_unaddressable_buffers();

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
  failures += test_reject_zero_length_random_two_bits();
  failures += test_reject_overflowing_random_two_bit_length();
  failures += test_reproduce_random_odd_bits();
  failures += test_reject_null_random_odd_bit_data();
  failures += test_reject_null_random_odd_bit_rng();
  failures += test_reject_zero_length_random_odd_bits();
  failures += test_reject_overflowing_random_odd_bit_length();
  failures += test_reject_zero_random_odd_bit_count();
  failures += test_reject_even_random_odd_bit_count();
  failures += test_reject_excessive_random_odd_bit_count();
  failures += test_reject_overflowing_random_odd_bit_allocation();
  failures += test_reselect_duplicate_random_odd_bits();
  failures += test_reproduce_random_burst();
  failures += test_reject_null_random_burst_data();
  failures += test_reject_null_random_burst_rng();
  failures += test_reject_zero_length_random_burst();
  failures += test_reject_overflowing_random_burst_length();
  failures += test_reject_zero_random_burst_length();
  failures += test_reject_excessive_random_burst_length();
  failures += test_flip_full_length_random_burst();
  failures += test_reproduce_random_frame_selection();
  failures += test_select_every_frame_without_advancing_rng();
  failures += test_reject_invalid_frame_selection();

  return failures == 0 ? 0 : 1;
}
