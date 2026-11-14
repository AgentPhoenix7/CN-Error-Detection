#include "socket_utils.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

/**
 * @brief Checks an exact binary transfer through the socket helpers.
 *
 * Creates a connected socket pair, sends the fixed byte vector, receives its
 * full length, compares bytes, and closes both descriptors.
 */
static int test_send_and_receive_all(void)
{
  const uint8_t sent[] = {
    UINT8_C(0x00), UINT8_C(0x11), UINT8_C(0x22),
    UINT8_C(0x33), UINT8_C(0x44)
  };
  uint8_t received[sizeof sent] = {UINT8_C(0)};
  int sockets[2];
  int failed = 0;

  if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) != 0) {
    perror("FAIL: socketpair");
    return 1;
  }

  if (!socket_utils_send_all(sockets[0], sent, sizeof sent)) {
    perror("FAIL: send all");
    failed = 1;
  } else if (socket_utils_receive_all(
      sockets[1],
      received,
      sizeof received
    ) != SOCKET_RECEIVE_OK) {
    perror("FAIL: receive all");
    failed = 1;
  } else if (memcmp(sent, received, sizeof sent) != 0) {
    fputs("FAIL: exact socket send/receive changed bytes\n", stderr);
    failed = 1;
  }

  (void)close(sockets[0]);
  (void)close(sockets[1]);

  if (failed != 0) {
    return failed;
  }

  puts("PASS: send and receive all bytes");
  return 0;
}

/**
 * @brief Checks that EOF before any record bytes is reported distinctly.
 *
 * Closes one endpoint of a socket pair before receiving and requires
 * SOCKET_RECEIVE_EOF from the remaining endpoint.
 */
static int test_receive_clean_eof(void)
{
  uint8_t output;
  int sockets[2];
  int failed = 0;

  if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) != 0) {
    perror("FAIL: socketpair");
    return 1;
  }

  (void)close(sockets[0]);

  if (
    socket_utils_receive_all(
      sockets[1],
      &output,
      sizeof output
    ) != SOCKET_RECEIVE_EOF
  ) {
    fputs("FAIL: clean socket EOF was not distinguished\n", stderr);
    failed = 1;
  }

  (void)close(sockets[1]);

  if (failed != 0) {
    return failed;
  }

  puts("PASS: distinguish clean socket EOF");
  return 0;
}

/**
 * @brief Checks that a truncated socket record is treated as an error.
 *
 * Sends two bytes, shuts down the writer, requests three bytes, and requires
 * SOCKET_RECEIVE_ERROR rather than clean EOF.
 */
static int test_reject_partial_record_eof(void)
{
  const uint8_t sent[] = {UINT8_C(0xAA), UINT8_C(0xBB)};
  uint8_t received[3] = {UINT8_C(0)};
  int sockets[2];
  int failed = 0;

  if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) != 0) {
    perror("FAIL: socketpair");
    return 1;
  }

  if (!socket_utils_send_all(sockets[0], sent, sizeof sent)) {
    perror("FAIL: partial-record send");
    failed = 1;
  } else if (shutdown(sockets[0], SHUT_WR) != 0) {
    perror("FAIL: partial-record shutdown");
    failed = 1;
  } else if (socket_utils_receive_all(
      sockets[1],
      received,
      sizeof received
    ) != SOCKET_RECEIVE_ERROR) {
    fputs("FAIL: partial record EOF was not rejected\n", stderr);
    failed = 1;
  }

  (void)close(sockets[0]);
  (void)close(sockets[1]);

  if (failed != 0) {
    return failed;
  }

  puts("PASS: reject partial socket record");
  return 0;
}

/**
 * @brief Checks that empty socket operations accept null buffers.
 *
 * Uses a connected socket pair to send and receive zero bytes with NULL data
 * and requires both operations to succeed.
 */
static int test_zero_length_operations(void)
{
  int sockets[2];
  int failed = 0;

  if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) != 0) {
    perror("FAIL: socketpair");
    return 1;
  }

  if (
    !socket_utils_send_all(sockets[0], NULL, 0U) ||
    socket_utils_receive_all(sockets[1], NULL, 0U) != SOCKET_RECEIVE_OK
  ) {
    fputs("FAIL: zero-length socket operation was rejected\n", stderr);
    failed = 1;
  }

  (void)close(sockets[0]);
  (void)close(sockets[1]);

  if (failed != 0) {
    return failed;
  }

  puts("PASS: allow zero-length socket operations");
  return 0;
}

/**
 * @brief Checks argument validation before socket operations.
 *
 * Passes invalid descriptors, null nonempty buffers, a null host, and a zero
 * backlog to the helpers and requires their documented failure results.
 */
static int test_reject_invalid_socket_inputs(void)
{
  uint8_t byte = UINT8_C(0);

  if (
    socket_utils_send_all(-1, &byte, sizeof byte) ||
    socket_utils_send_all(0, NULL, sizeof byte) ||
    socket_utils_receive_all(-1, &byte, sizeof byte) !=
      SOCKET_RECEIVE_ERROR ||
    socket_utils_receive_all(0, NULL, sizeof byte) !=
      SOCKET_RECEIVE_ERROR ||
    socket_utils_connect(NULL, UINT16_C(4321)) != -1 ||
    socket_utils_listen(UINT16_C(4321), 0) != -1
  ) {
    fputs("FAIL: invalid socket utility input was accepted\n", stderr);
    return 1;
  }

  puts("PASS: reject invalid socket utility inputs");
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

  failures += test_send_and_receive_all();
  failures += test_receive_clean_eof();
  failures += test_reject_partial_record_eof();
  failures += test_zero_length_operations();
  failures += test_reject_invalid_socket_inputs();

  return failures == 0 ? 0 : 1;
}
