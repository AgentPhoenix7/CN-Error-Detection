#include "config.h"
#include "error_injection.h"
#include "frame.h"
#include "socket_utils.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef enum
{
  ERROR_PATTERN_NONE,
  ERROR_PATTERN_SINGLE,
  ERROR_PATTERN_TWO,
  ERROR_PATTERN_ODD,
  ERROR_PATTERN_BURST,
  ERROR_PATTERN_RANDOM
} error_pattern_t;

static void sender_print_usage(const char *program)
{
  fprintf(
    stderr,
    "Usage: %s INPUT SCHEME PATTERN [HOST] [PORT] [SEED]\n"
    "  SCHEME: checksum | crc8 | crc10 | crc16 | crc32\n"
    "  PATTERN: none | single | two | odd | burst | random\n",
    program
  );
}

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

static bool sender_parse_error_pattern(
  const char *text,
  error_pattern_t *pattern
)
{
  error_pattern_t parsed_pattern;

  if (text == NULL || pattern == NULL) {
    return false;
  }

  if (strcmp(text, "none") == 0) {
    parsed_pattern = ERROR_PATTERN_NONE;
  } else if (strcmp(text, "single") == 0) {
    parsed_pattern = ERROR_PATTERN_SINGLE;
  } else if (strcmp(text, "two") == 0) {
    parsed_pattern = ERROR_PATTERN_TWO;
  } else if (strcmp(text, "odd") == 0) {
    parsed_pattern = ERROR_PATTERN_ODD;
  } else if (strcmp(text, "burst") == 0) {
    parsed_pattern = ERROR_PATTERN_BURST;
  } else if (strcmp(text, "random") == 0) {
    parsed_pattern = ERROR_PATTERN_RANDOM;
  } else {
    return false;
  }

  *pattern = parsed_pattern;
  return true;
}

static uint32_t sender_rng_next(error_injection_rng_t *rng)
{
  rng->state =
    rng->state * UINT32_C(1664525) +
    UINT32_C(1013904223);
  return rng->state;
}

static bool sender_apply_error(
  uint8_t *frame,
  size_t frame_length,
  error_pattern_t pattern,
  error_injection_rng_t *rng
)
{
  if (pattern == ERROR_PATTERN_RANDOM) {
    pattern = (error_pattern_t)(
      ERROR_PATTERN_SINGLE + sender_rng_next(rng) % 4U
    );
  }

  switch (pattern) {
    case ERROR_PATTERN_NONE:
      return true;
    case ERROR_PATTERN_SINGLE:
      return error_injection_flip_random_bit(frame, frame_length, rng);
    case ERROR_PATTERN_TWO:
      return error_injection_flip_random_two_bits(frame, frame_length, rng);
    case ERROR_PATTERN_ODD:
      return error_injection_flip_random_odd_bits(
        frame,
        frame_length,
        3U,
        rng
      );
    case ERROR_PATTERN_BURST:
      return error_injection_flip_random_burst(
        frame,
        frame_length,
        16U,
        rng
      );
    case ERROR_PATTERN_RANDOM:
    default:
      return false;
  }
}

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
  error_pattern_t pattern;
  error_injection_rng_t rng;
  frame_header_t header;
  uint8_t payload[MAX_PAYLOAD_SIZE];
  uint8_t serialized_frame[MAX_FRAME_SIZE];
  uint8_t sequence_number = UINT8_C(0);
  size_t fcs_size;
  size_t payload_capacity;
  size_t payload_length;
  size_t serialized_length;
  size_t frame_count = 0U;
  int socket_fd = -1;
  FILE *input = NULL;
  int status = EXIT_FAILURE;

  if (argc < 4 || argc > 7) {
    sender_print_usage(argv[0]);
    return EXIT_FAILURE;
  }

  if (
    !frame_fcs_scheme_parse(argv[2], &scheme) ||
    !sender_parse_error_pattern(argv[3], &pattern) ||
    (argc >= 6 && !sender_parse_port(argv[5], &port)) ||
    (argc >= 7 && !sender_parse_seed(argv[6], &seed))
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

  (void)error_injection_rng_seed(&rng, seed);
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

    if (
      !frame_serialize(
        &header,
        payload,
        scheme,
        serialized_frame,
        sizeof serialized_frame,
        &serialized_length
      ) ||
      !sender_apply_error(
        serialized_frame,
        serialized_length,
        pattern,
        &rng
      ) ||
      !sender_send_record(socket_fd, serialized_frame, serialized_length)
    ) {
      fputs("sender: failed to prepare or transmit a frame\n", stderr);
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

  printf(
    "Sent %zu frame(s) using %s with %s errors (seed 0x%08X).\n",
    frame_count,
    frame_fcs_scheme_name(scheme),
    argv[3],
    (unsigned int)seed
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
