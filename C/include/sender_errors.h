/**
 * @file sender_errors.h
 * @brief Declares the sender's error-pattern dispatcher.
 */
#ifndef SENDER_ERRORS_H
#define SENDER_ERRORS_H

#include "error_injection.h"

/** @brief Identifies the corruption pattern selected by the sender. */
typedef enum
{
  SENDER_ERROR_PATTERN_NONE,
  SENDER_ERROR_PATTERN_SINGLE,
  SENDER_ERROR_PATTERN_TWO,
  SENDER_ERROR_PATTERN_ODD,
  SENDER_ERROR_PATTERN_BURST,
  SENDER_ERROR_PATTERN_RANDOM
} sender_error_pattern_t;

/**
 * @brief Applies the selected sender error pattern to a serialized frame.
 *
 * Odd errors flip three distinct bits; bursts flip 16 consecutive bits.
 * Random mode chooses one of the four corrupting patterns using the supplied
 * generator. None is a no-op and does not consume RNG state.
 * Rejected operations leave both frame bytes and caller RNG state unchanged.
 *
 * Treats none as a no-op, chooses a random mode when requested, dispatches
 * single/two/three/burst mutations using a local RNG copy, and commits state
 * only after success.
 *
 * @param[in,out] frame Serialized frame to corrupt.
 * @param[in] frame_length Frame size in bytes.
 * @param[in] pattern Selected error pattern.
 * @param[in,out] rng Initialized mutation RNG.
 * @return Whether the selected operation succeeded.
 */
bool sender_errors_apply(
  uint8_t *frame,
  size_t frame_length,
  sender_error_pattern_t pattern,
  error_injection_rng_t *rng
);

#endif
