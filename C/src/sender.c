#define _POSIX_C_SOURCE 200809L

#include "config.h"
#include "error_injection.h"
#include "sender_errors.h"
#include "frame.h"
#include "socket_utils.h"
#include "process_metrics.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <time.h>
#include <unistd.h>

#define SELECTION_SEED_XOR UINT32_C(0x9E3779B9)

/**
 * @brief Prints the sender's supported command-line arguments.
 *
 * Formats the executable name, available schemes, error modes, and
 * selection-denominator explanation to standard error.
 */
static void sender_print_usage(const char *program)
{
  fprintf(
    stderr,
    "Usage: %s INPUT SCHEME PATTERN [HOST] [PORT] [SEED] "
    "[ERROR_INTERVAL]\n"
    "  SCHEME: checksum | crc8 | crc10 | crc16 | crc32\n"
    "  PATTERN: none | single | two | odd | burst | random\n"
    "  ERROR_INTERVAL: select each frame with seeded probability 1/N "
    "(default: 2)\n",
    program
  );
}

/**
 * @brief Parses a nonzero decimal TCP port safely.
 *
 * Uses strtoul in base ten, checks errno, trailing characters and uint16_t
 * bounds, then assigns the port only on success.
 */
static bool sender_parse_port(const char *text, uint16_t *port)
{
  char *end;
  unsigned long value;

  if (text == NULL || port == NULL || text[0] == '\0') {
    return false;
  }

  errno = 0;
  value = strtoul(text, &end, 10);

  if (
    errno != 0 ||
    *end != '\0' ||
    value == 0UL ||
    value > UINT16_MAX
  ) {
    return false;
  }

  *port = (uint16_t)value;
  return true;
}

/**
 * @brief Parses a uint32_t seed with standard numeric base prefixes.
 *
 * Uses strtoul with base zero, rejects conversion errors, trailing
 * characters and out-of-range values, and stores the validated seed.
 */
static bool sender_parse_seed(const char *text, uint32_t *seed)
{
  char *end;
  unsigned long value;

  if (text == NULL || seed == NULL || text[0] == '\0') {
    return false;
  }

  errno = 0;
  value = strtoul(text, &end, 0);

  if (errno != 0 || *end != '\0' || value > UINT32_MAX) {
    return false;
  }

  *seed = (uint32_t)value;
  return true;
}

/**
 * @brief Parses the positive uint32_t frame-selection denominator.
 *
 * Uses decimal strtoul, checks conversion errors and the full value's
 * bounds, and stores the result only after validating it.
 */
static bool sender_parse_error_interval(
  const char *text,
  uint32_t *error_interval
)
{
  char *end;
  unsigned long value;

  if (text == NULL || error_interval == NULL || text[0] == '\0') {
    return false;
  }

  errno = 0;
  value = strtoul(text, &end, 10);

  if (errno != 0 || *end != '\0' || value == 0UL || value > UINT32_MAX) {
    return false;
  }

  *error_interval = (uint32_t)value;
  return true;
}

/**
 * @brief Converts an error-mode name into the sender's pattern enum.
 *
 * Compares the text against none, single, two, odd, burst, and random, then
 * commits the matched temporary enum to the caller.
 */
static bool sender_parse_error_pattern(
  const char *text,
  sender_error_pattern_t *pattern
)
{
  sender_error_pattern_t parsed_pattern;

  if (text == NULL || pattern == NULL) {
    return false;
  }

  if (strcmp(text, "none") == 0) {
    parsed_pattern = SENDER_ERROR_PATTERN_NONE;
  } else if (strcmp(text, "single") == 0) {
    parsed_pattern = SENDER_ERROR_PATTERN_SINGLE;
  } else if (strcmp(text, "two") == 0) {
    parsed_pattern = SENDER_ERROR_PATTERN_TWO;
  } else if (strcmp(text, "odd") == 0) {
    parsed_pattern = SENDER_ERROR_PATTERN_ODD;
  } else if (strcmp(text, "burst") == 0) {
    parsed_pattern = SENDER_ERROR_PATTERN_BURST;
  } else if (strcmp(text, "random") == 0) {
    parsed_pattern = SENDER_ERROR_PATTERN_RANDOM;
  } else {
    return false;
  }

  *pattern = parsed_pattern;
  return true;
}

/**
 * @brief Converts the difference between monotonic timestamps to seconds.
 *
 * Subtracts seconds and adds the nanosecond difference divided by one
 * billion as a double.
 */
static double sender_timespec_difference(
  const struct timespec *start,
  const struct timespec *end
)
{
  return
    (double)(end->tv_sec - start->tv_sec) +
    (double)(end->tv_nsec - start->tv_nsec) / 1000000000.0;
}

/**
 * @brief Converts a pair of resource-usage timestamps to elapsed CPU
 * seconds.
 *
 * Subtracts seconds and adds the microsecond difference divided by one
 * million as a double.
 */
static double sender_timeval_difference(
  const struct timeval *start,
  const struct timeval *end
)
{
  return
    (double)(end->tv_sec - start->tv_sec) +
    (double)(end->tv_usec - start->tv_usec) / 1000000.0;
}

/**
 * @brief Sends a simulated frame with its TCP record-length prefix.
 *
 * Checks the length fits uint16_t, writes its two big-endian prefix bytes,
 * and sends prefix and frame completely; zero length sends the end marker.
 */
static bool sender_send_record(
  int socket_fd,
  const uint8_t *frame,
  size_t frame_length
)
{
  uint8_t length_prefix[2];

  if (frame_length > UINT16_MAX) {
    return false;
  }

  length_prefix[0] = (uint8_t)(frame_length >> 8U);
  length_prefix[1] = (uint8_t)frame_length;

  return
    socket_utils_send_all(socket_fd, length_prefix, sizeof length_prefix) &&
    socket_utils_send_all(socket_fd, frame, frame_length);
}

/**
 * @brief Transfers an input file as optionally corrupted protected frames.
 *
 * Parses the CLI, connects TCP, chunks by the selected FCS capacity,
 * serializes sequenced frames, selects and applies seeded errors, sends
 * length-prefixed records and an end marker, then reports metrics and closes
 * resources.
 */
int main(int argc, char *argv[])
{
  static const uint8_t source_mac[MAC_ADDRESS_SIZE] = {
    UINT8_C(0x02), UINT8_C(0x00), UINT8_C(0x00),
    UINT8_C(0x00), UINT8_C(0x00), UINT8_C(0x01)
  };
  static const uint8_t destination_mac[MAC_ADDRESS_SIZE] = {
    UINT8_C(0x02), UINT8_C(0x00), UINT8_C(0x00),
    UINT8_C(0x00), UINT8_C(0x00), UINT8_C(0x02)
  };
  const char *host = DEFAULT_SERVER_ADDRESS;
  uint16_t port = DEFAULT_SERVER_PORT;
  uint32_t seed = UINT32_C(0x12345678);
  frame_fcs_scheme_t scheme;
  sender_error_pattern_t pattern;
  error_injection_rng_t selection_rng;
  error_injection_rng_t error_rng;
  frame_header_t header;
  uint8_t payload[MAX_PAYLOAD_SIZE];
  uint8_t serialized_frame[MAX_FRAME_SIZE];
  uint8_t sequence_number = UINT8_C(0);
  size_t fcs_size;
  size_t payload_capacity;
  size_t payload_length;
  size_t serialized_length;
  size_t frame_count = 0U;
  size_t frames_injected = 0U;
  uint32_t error_interval = UINT32_C(2);
  bool inject_frame;
  struct timespec start_time;
  struct timespec end_time;
  struct rusage start_usage;
  struct rusage end_usage;
  double elapsed_seconds;
  double user_cpu_seconds;
  double system_cpu_seconds;
  long peak_rss_kb;
  int socket_fd = -1;
  FILE *input = NULL;
  int status = EXIT_FAILURE;

  if (argc < 4 || argc > 8) {
    sender_print_usage(argv[0]);
    return EXIT_FAILURE;
  }

  if (
    !frame_fcs_scheme_parse(argv[2], &scheme) ||
    !sender_parse_error_pattern(argv[3], &pattern) ||
    (argc >= 6 && !sender_parse_port(argv[5], &port)) ||
    (argc >= 7 && !sender_parse_seed(argv[6], &seed)) ||
    (argc >= 8 && !sender_parse_error_interval(argv[7], &error_interval))
  ) {
    sender_print_usage(argv[0]);
    return EXIT_FAILURE;
  }

  if (argc >= 5) {
    host = argv[4];
  }

  if (!frame_fcs_size(scheme, &fcs_size)) {
    fputs("sender: invalid FCS scheme\n", stderr);
    return EXIT_FAILURE;
  }

  payload_capacity = MAX_FRAME_SIZE - FRAME_HEADER_SIZE - fcs_size;

  input = fopen(argv[1], "rb");

  if (input == NULL) {
    perror("sender: open input");
    goto cleanup;
  }

  socket_fd = socket_utils_connect(host, port);

  if (socket_fd < 0) {
    fprintf(stderr, "sender: could not connect to %s:%u\n", host, port);
    goto cleanup;
  }

  if (
    clock_gettime(CLOCK_MONOTONIC, &start_time) != 0 ||
    getrusage(RUSAGE_SELF, &start_usage) != 0
  ) {
    perror("sender: start metrics");
    goto cleanup;
  }

  (void)error_injection_rng_seed(
    &selection_rng,
    seed ^ SELECTION_SEED_XOR
  );
  (void)error_injection_rng_seed(&error_rng, seed);
  memcpy(header.source_mac, source_mac, sizeof source_mac);
  memcpy(header.destination_mac, destination_mac, sizeof destination_mac);

  while ((payload_length = fread(
    payload,
    1U,
    payload_capacity,
    input
  )) > 0U) {
    header.payload_length = (uint16_t)payload_length;
    header.sequence_number = sequence_number;

    if (!frame_serialize(
        &header,
        payload,
        scheme,
        serialized_frame,
        sizeof serialized_frame,
        &serialized_length
      )) {
      fputs("sender: failed to serialize a frame\n", stderr);
      goto cleanup;
    }

    inject_frame = false;

    if (
      pattern != SENDER_ERROR_PATTERN_NONE &&
      !error_injection_select_frame(
        &selection_rng,
        error_interval,
        &inject_frame
      )
    ) {
      fputs("sender: failed to select an error-injection frame\n", stderr);
      goto cleanup;
    }

    if (inject_frame) {
      if (!sender_errors_apply(
        serialized_frame,
        serialized_length,
        pattern,
        &error_rng
      )) {
        fputs("sender: failed to inject an error\n", stderr);
        goto cleanup;
      }

      frames_injected++;
    }

    if (!sender_send_record(socket_fd, serialized_frame, serialized_length)) {
      fputs("sender: failed to transmit a frame\n", stderr);
      goto cleanup;
    }

    frame_count++;
    sequence_number++;
  }

  if (ferror(input)) {
    perror("sender: read input");
    goto cleanup;
  }

  if (!sender_send_record(socket_fd, NULL, 0U)) {
    fputs("sender: failed to transmit end marker\n", stderr);
    goto cleanup;
  }

  if (
    clock_gettime(CLOCK_MONOTONIC, &end_time) != 0 ||
    getrusage(RUSAGE_SELF, &end_usage) != 0 ||
    !process_metrics_peak_rss_kb(&peak_rss_kb)
  ) {
    perror("sender: finish metrics");
    goto cleanup;
  }

  elapsed_seconds = sender_timespec_difference(&start_time, &end_time);
  user_cpu_seconds = sender_timeval_difference(
    &start_usage.ru_utime,
    &end_usage.ru_utime
  );
  system_cpu_seconds = sender_timeval_difference(
    &start_usage.ru_stime,
    &end_usage.ru_stime
  );

  printf(
    "Sent %zu frame(s): %zu clean, %zu error-injected using %s/%s "
    "(seed 0x%08X, selection probability 1/%u).\n",
    frame_count,
    frame_count - frames_injected,
    frames_injected,
    frame_fcs_scheme_name(scheme),
    argv[3],
    (unsigned int)seed,
    (unsigned int)error_interval
  );
  printf(
    "SENDER_METRICS,%zu,%zu,%zu,%.9f,%.9f,%.9f,%ld\n",
    frame_count,
    frames_injected,
    frame_count - frames_injected,
    elapsed_seconds,
    user_cpu_seconds,
    system_cpu_seconds,
    peak_rss_kb
  );
  status = EXIT_SUCCESS;

cleanup:
  if (socket_fd >= 0) {
    (void)close(socket_fd);
  }

  if (input != NULL) {
    (void)fclose(input);
  }

  return status;
}
