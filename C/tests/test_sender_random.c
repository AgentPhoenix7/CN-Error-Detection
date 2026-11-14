#include "config.h"
#include "sender_errors.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/**
 * @brief Checks that rejected random dispatch preserves data and RNG state.
 *
 * Exercises null data, zero length, a seed selecting an oversized burst, and
 * a null RNG, comparing the generator and buffer against saved values.
 */
static int test_rejected_dispatch_preserves_state(void)
{
  uint8_t data[64];
  uint8_t original[64];
  memset(data, 0xA5, sizeof data);
  memcpy(original, data, sizeof data);
  error_injection_rng_t rng = {UINT32_C(0x12345678)};
  if (sender_errors_apply(NULL, sizeof data, SENDER_ERROR_PATTERN_RANDOM, &rng) ||
      rng.state != UINT32_C(0x12345678) ||
      sender_errors_apply(data, 0U, SENDER_ERROR_PATTERN_RANDOM, &rng) ||
      rng.state != UINT32_C(0x12345678)) {
    fputs("FAIL: rejected random dispatch advanced RNG state\n", stderr);
    return 1;
  }
  /* This seed chooses the 16-bit burst, which cannot fit in one byte. */
  rng.state = UINT32_C(0x75432777);
  if (sender_errors_apply(data, 1U, SENDER_ERROR_PATTERN_RANDOM, &rng) ||
      rng.state != UINT32_C(0x75432777) || memcmp(data, original, sizeof data) != 0 ||
      sender_errors_apply(data, sizeof data, SENDER_ERROR_PATTERN_RANDOM, NULL) ||
      memcmp(data, original, sizeof data) != 0) {
    fputs("FAIL: rejected random operation changed data or RNG\n", stderr);
    return 1;
  }
  puts("PASS: rejected sender dispatch preserves data and RNG");
  return 0;
}

/**
 * @brief Checks random-mode coverage and rejected-dispatch atomicity.
 *
 * Runs the rejection checks, applies 1000 seeded random patterns, counts
 * changed bits to identify each mode, and requires all four mode counts
 * within the expected coverage range.
 */
int main(void)
{
  if (test_rejected_dispatch_preserves_state() != 0) return 1;
  error_injection_rng_t rng = {UINT32_C(0x12345678)};
  size_t counts[4] = {0U};
  for (size_t trial = 0U; trial < 1000U; trial++) {
    uint8_t data[MAX_FRAME_SIZE] = {0};
    size_t changed = 0U;
    if (!sender_errors_apply(data, sizeof data, SENDER_ERROR_PATTERN_RANDOM, &rng)) return 1;
    for (size_t bit = 0U; bit < sizeof data * 8U; bit++) {
      changed += (data[bit / 8U] & (0x80U >> (bit % 8U))) != 0U;
    }
    switch (changed) {
      case 1U: counts[0]++; break;
      case 2U: counts[1]++; break;
      case 3U: counts[2]++; break;
      case 16U: counts[3]++; break;
      default: return 1;
    }
  }
  for (size_t pattern = 0U; pattern < 4U; pattern++) {
    if (counts[pattern] < 150U || counts[pattern] > 350U) {
      fprintf(stderr, "FAIL: random pattern %zu selected %zu/1000 times\n", pattern, counts[pattern]);
      return 1;
    }
  }
  puts("PASS: sender random mode covers all four error patterns");
  return 0;
}
