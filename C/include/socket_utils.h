/**
 * @file socket_utils.h
 * @brief Declares reliable TCP socket helper operations.
 */

#ifndef SOCKET_UTILS_H
#define SOCKET_UTILS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Describes the outcome of an exact-length receive operation.
 */
typedef enum
{
  SOCKET_RECEIVE_OK,
  SOCKET_RECEIVE_EOF,
  SOCKET_RECEIVE_ERROR
} socket_receive_result_t;

/**
 * @brief Connects a TCP socket to a numeric or named host.
 *
 * Resolves candidate addresses with getaddrinfo, tries socket/connect for
 * each, closes failed descriptors, frees the address list, and returns the
 * successful descriptor or -1.
 *
 * @param[in] host Host name or address.
 * @param[in] port TCP port in host byte order.
 * @return Connected descriptor, or `-1` on failure.
 */
int socket_utils_connect(const char *host, uint16_t port);

/**
 * @brief Creates a TCP listening socket on all local interfaces.
 *
 * Resolves passive addresses, tries sockets with SO_REUSEADDR, binds and
 * listens with the supplied backlog, and closes unsuccessful candidates
 * before returning.
 *
 * @param[in] port TCP port in host byte order.
 * @param[in] backlog Pending-connection queue length.
 * @return Listening descriptor, or `-1` on failure.
 */
int socket_utils_listen(uint16_t port, int backlog);

/**
 * @brief Accepts one client, retrying when interrupted by a signal.
 *
 * Repeats accept while it fails with EINTR and returns the resulting
 * connected descriptor or failure code.
 *
 * @param[in] listener Listening descriptor.
 * @return Connected descriptor, or `-1` on failure.
 */
int socket_utils_accept(int listener);

/**
 * @brief Sends an entire byte buffer across a connected socket.
 *
 * Loops over partial send results, retries EINTR, suppresses SIGPIPE where
 * MSG_NOSIGNAL is available, and fails on other errors or a zero-byte send.
 *
 * @param[in] socket_fd Connected descriptor.
 * @param[in] data Bytes to send. It may be `NULL` only when `length` is zero.
 * @param[in] length Number of bytes to send.
 * @return `true` after every byte is sent; otherwise `false`.
 */
bool socket_utils_send_all(
  int socket_fd,
  const uint8_t *data,
  size_t length
);

/**
 * @brief Receives an exact number of bytes from a connected socket.
 *
 * EOF before any requested bytes returns `SOCKET_RECEIVE_EOF`; EOF after a
 * partial record returns `SOCKET_RECEIVE_ERROR`.
 *
 * Accumulates partial recv results, retries EINTR, returns clean EOF only
 * before any requested byte arrives, and treats an incomplete record as an
 * error.
 *
 * @param[in] socket_fd Connected descriptor.
 * @param[out] data Destination buffer. It may be `NULL` only when `length`
 *                  is zero.
 * @param[in] length Exact number of bytes to receive.
 * @return Receive outcome.
 */
socket_receive_result_t socket_utils_receive_all(
  int socket_fd,
  uint8_t *data,
  size_t length
);

#endif
