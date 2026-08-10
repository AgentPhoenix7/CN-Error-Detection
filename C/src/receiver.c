#define _POSIX_C_SOURCE 200809L

#include "config.h"
#include "frame.h"
#include "socket_utils.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <time.h>
#include <unistd.h>

typedef struct
{
  size_t frames_received;
  size_t frames_accepted;
  size_t frames_rejected;
  size_t payload_bytes_written;
  double elapsed_seconds;
  double user_cpu_seconds;
  double system_cpu_seconds;
  long max_rss_kb;
} receiver_metrics_t;

static void receiver_print_usage(const char *program)
{
  fprintf(
    stderr,
    "Usage: %s OUTPUT SCHEME [PORT] [CSV_PATH] [PATTERN_LABEL]\n"
    "  SCHEME: checksum | crc8 | crc10 | crc16 | crc32\n",
    program
  );
}

static bool receiver_parse_port(const char *text, uint16_t *port)
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

static double receiver_timespec_difference(
  const struct timespec *start,
  const struct timespec *end
)
{
  return
    (double)(end->tv_sec - start->tv_sec) +
    (double)(end->tv_nsec - start->tv_nsec) / 1000000000.0;
}

static double receiver_timeval_difference(
  const struct timeval *start,
  const struct timeval *end
)
{
  return
    (double)(end->tv_sec - start->tv_sec) +
    (double)(end->tv_usec - start->tv_usec) / 1000000.0;
}

static bool receiver_append_metrics(
  const char *path,
  frame_fcs_scheme_t scheme,
  const char *pattern_label,
  const receiver_metrics_t *metrics
)
{
  FILE *csv;
  long existing_size;
  double detection_rate;

  csv = fopen(path, "a+");

  if (csv == NULL) {
    return false;
  }

  if (fseek(csv, 0L, SEEK_END) != 0) {
    (void)fclose(csv);
    return false;
  }

  existing_size = ftell(csv);

  if (existing_size < 0L) {
    (void)fclose(csv);
    return false;
  }

  if (existing_size == 0L) {
    fputs(
      "scheme,error_pattern,frames_received,frames_accepted,"
      "frames_rejected,detection_rate,payload_bytes_written,"
      "elapsed_seconds,user_cpu_seconds,system_cpu_seconds,max_rss_kb\n",
      csv
    );
  }

  detection_rate = metrics->frames_received == 0U
    ? 0.0
    : (double)metrics->frames_rejected /
      (double)metrics->frames_received;

  if (fprintf(
    csv,
    "%s,%s,%zu,%zu,%zu,%.9f,%zu,%.9f,%.9f,%.9f,%ld\n",
    frame_fcs_scheme_name(scheme),
    pattern_label,
    metrics->frames_received,
    metrics->frames_accepted,
    metrics->frames_rejected,
    detection_rate,
    metrics->payload_bytes_written,
    metrics->elapsed_seconds,
    metrics->user_cpu_seconds,
    metrics->system_cpu_seconds,
    metrics->max_rss_kb
  ) < 0) {
    (void)fclose(csv);
    return false;
  }

  return fclose(csv) == 0;
}

int main(int argc, char *argv[])
{
  const char *csv_path = NULL;
  const char *pattern_label = "unspecified";
  uint16_t port = DEFAULT_SERVER_PORT;
  frame_fcs_scheme_t scheme;
  uint8_t length_prefix[2];
  uint8_t serialized_frame[MAX_FRAME_SIZE];
  uint8_t payload[MAX_PAYLOAD_SIZE];
  frame_header_t header;
  receiver_metrics_t metrics = {0};
  struct timespec start_time;
  struct timespec end_time;
  struct rusage start_usage;
  struct rusage end_usage;
  size_t serialized_length;
  size_t payload_length;
  socket_receive_result_t receive_result;
  int listener = -1;
  int client = -1;
  FILE *output = NULL;
  int status = EXIT_FAILURE;

  if (argc < 3 || argc > 6) {
    receiver_print_usage(argv[0]);
    return EXIT_FAILURE;
  }

  if (
    !frame_fcs_scheme_parse(argv[2], &scheme) ||
    (argc >= 4 && !receiver_parse_port(argv[3], &port))
  ) {
    receiver_print_usage(argv[0]);
    return EXIT_FAILURE;
  }

  if (argc >= 5) {
    csv_path = argv[4];
  }

  if (argc >= 6) {
    pattern_label = argv[5];
  }

  output = fopen(argv[1], "wb");

  if (output == NULL) {
    perror("receiver: open output");
    goto cleanup;
  }

  listener = socket_utils_listen(port, 1);

  if (listener < 0) {
    fprintf(stderr, "receiver: could not listen on port %u\n", port);
    goto cleanup;
  }

  printf("Listening on port %u for %s frames...\n", port, argv[2]);
  fflush(stdout);
  client = socket_utils_accept(listener);

  if (client < 0) {
    perror("receiver: accept");
    goto cleanup;
  }

  if (
    clock_gettime(CLOCK_MONOTONIC, &start_time) != 0 ||
    getrusage(RUSAGE_SELF, &start_usage) != 0
  ) {
    perror("receiver: start metrics");
    goto cleanup;
  }

  for (
    ;
    ;
  )
  {
    receive_result = socket_utils_receive_all(
      client,
      length_prefix,
      sizeof length_prefix
    );

    if (receive_result != SOCKET_RECEIVE_OK) {
      fputs("receiver: connection ended before the end marker\n", stderr);
      goto cleanup;
    }

    serialized_length =
      ((size_t)length_prefix[0] << 8U) |
      (size_t)length_prefix[1];

    if (serialized_length == 0U) {
      break;
    }

    if (serialized_length > sizeof serialized_frame) {
      fputs("receiver: frame record exceeds maximum size\n", stderr);
      goto cleanup;
    }

    if (socket_utils_receive_all(
      client,
      serialized_frame,
      serialized_length
    ) != SOCKET_RECEIVE_OK) {
      fputs("receiver: incomplete frame record\n", stderr);
      goto cleanup;
    }

    metrics.frames_received++;

    if (frame_verify_and_extract(
      serialized_frame,
      serialized_length,
      scheme,
      &header,
      payload,
      sizeof payload,
      &payload_length
    )) {
      if (fwrite(payload, 1U, payload_length, output) != payload_length) {
        perror("receiver: write output");
        goto cleanup;
      }

      metrics.frames_accepted++;
      metrics.payload_bytes_written += payload_length;
    } else {
      metrics.frames_rejected++;
    }
  }

  if (
    clock_gettime(CLOCK_MONOTONIC, &end_time) != 0 ||
    getrusage(RUSAGE_SELF, &end_usage) != 0
  ) {
    perror("receiver: finish metrics");
    goto cleanup;
  }

  metrics.elapsed_seconds = receiver_timespec_difference(
    &start_time,
    &end_time
  );
  metrics.user_cpu_seconds = receiver_timeval_difference(
    &start_usage.ru_utime,
    &end_usage.ru_utime
  );
  metrics.system_cpu_seconds = receiver_timeval_difference(
    &start_usage.ru_stime,
    &end_usage.ru_stime
  );
  metrics.max_rss_kb = end_usage.ru_maxrss;

  if (
    csv_path != NULL &&
    !receiver_append_metrics(
      csv_path,
      scheme,
      pattern_label,
      &metrics
    )
  ) {
    perror("receiver: append CSV metrics");
    goto cleanup;
  }

  printf(
    "Received %zu frame(s): %zu accepted, %zu rejected; "
    "wrote %zu payload byte(s) in %.6f s.\n",
    metrics.frames_received,
    metrics.frames_accepted,
    metrics.frames_rejected,
    metrics.payload_bytes_written,
    metrics.elapsed_seconds
  );
  status = EXIT_SUCCESS;

cleanup:
  if (client >= 0) {
    (void)close(client);
  }

  if (listener >= 0) {
    (void)close(listener);
  }

  if (output != NULL && fclose(output) != 0) {
    status = EXIT_FAILURE;
  }

  return status;
}
