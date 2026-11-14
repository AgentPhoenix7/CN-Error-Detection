#include "sender_errors.h"

/**
 * @brief Applies the sender's chosen corruption pattern to a serialized
 * frame.
 *
 * Treats none as a no-op, chooses a random mode when requested, dispatches
 * single/two/three/burst mutations using a local RNG copy, and commits state
 * only after success.
 */
bool sender_errors_apply(
  uint8_t *frame,
  const size_t frame_length,
  sender_error_pattern_t pattern,
  error_injection_rng_t *rng
)
{
  if (pattern == SENDER_ERROR_PATTERN_NONE) {
    return true;
  }
  if (rng == NULL) {
    return false;
  }

  error_injection_rng_t pending_rng = *rng;
  bool applied;

  if (pattern == SENDER_ERROR_PATTERN_RANDOM) {
    size_t choice;
    if (!error_injection_rng_bounded(&pending_rng, 4U, &choice)) {
      return false;
    }
    pattern = (sender_error_pattern_t)(
      SENDER_ERROR_PATTERN_SINGLE + choice
    );
  }

  switch (pattern) {
    case SENDER_ERROR_PATTERN_NONE:
      return true;
    case SENDER_ERROR_PATTERN_SINGLE:
      applied = error_injection_flip_random_bit(frame, frame_length, &pending_rng);
      break;
    case SENDER_ERROR_PATTERN_TWO:
      applied = error_injection_flip_random_two_bits(frame, frame_length, &pending_rng);
      break;
    case SENDER_ERROR_PATTERN_ODD:
      applied = error_injection_flip_random_odd_bits(
        frame,
        frame_length,
        3U,
        &pending_rng
      );
      break;
    case SENDER_ERROR_PATTERN_BURST:
      applied = error_injection_flip_random_burst(
        frame,
        frame_length,
        16U,
        &pending_rng
      );
      break;
    case SENDER_ERROR_PATTERN_RANDOM:
    default:
      return false;
  }

  if (applied) {
    *rng = pending_rng;
  }
  return applied;
}
