#define _POSIX_C_SOURCE 200112L

#include "socket_utils.h"

#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

static bool socket_utils_format_port(
  uint16_t port,
  char output[6]
)
{
  int written;

  written = snprintf(output, 6U, "%u", (unsigned int)port);
  return written > 0 && written < 6;
}

int socket_utils_connect(const char *host, uint16_t port)
{
  struct addrinfo hints;
  struct addrinfo *addresses;
  struct addrinfo *address;
  char port_text[6];
  int socket_fd = -1;
  int result;

  if (host == NULL || !socket_utils_format_port(port, port_text)) {
    return -1;
  }

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  result = getaddrinfo(host, port_text, &hints, &addresses);

  if (result != 0) {
    return -1;
  }

  for (
    address = addresses;
    address != NULL;
    address = address->ai_next
  )
  {
    socket_fd = socket(
      address->ai_family,
      address->ai_socktype,
      address->ai_protocol
    );

    if (socket_fd < 0) {
      continue;
    }

    if (connect(socket_fd, address->ai_addr, address->ai_addrlen) == 0) {
      break;
    }

    (void)close(socket_fd);
    socket_fd = -1;
  }

  freeaddrinfo(addresses);
  return socket_fd;
}

int socket_utils_listen(uint16_t port, int backlog)
{
  struct addrinfo hints;
  struct addrinfo *addresses;
  struct addrinfo *address;
  char port_text[6];
  int listener = -1;
  int reuse_address = 1;
  int result;

  if (backlog <= 0 || !socket_utils_format_port(port, port_text)) {
    return -1;
  }

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  hints.ai_flags = AI_PASSIVE;

  result = getaddrinfo(NULL, port_text, &hints, &addresses);

  if (result != 0) {
    return -1;
  }

  for (
    address = addresses;
    address != NULL;
    address = address->ai_next
  )
  {
    listener = socket(
      address->ai_family,
      address->ai_socktype,
      address->ai_protocol
    );

    if (listener < 0) {
      continue;
    }

    (void)setsockopt(
      listener,
      SOL_SOCKET,
      SO_REUSEADDR,
      &reuse_address,
      sizeof reuse_address
    );

    if (
      bind(listener, address->ai_addr, address->ai_addrlen) == 0 &&
      listen(listener, backlog) == 0
    ) {
      break;
    }

    (void)close(listener);
    listener = -1;
  }

  freeaddrinfo(addresses);
  return listener;
}

int socket_utils_accept(int listener)
{
  int socket_fd;

  do {
    socket_fd = accept(listener, NULL, NULL);
  } while (socket_fd < 0 && errno == EINTR);

  return socket_fd;
}

bool socket_utils_send_all(
  int socket_fd,
  const uint8_t *data,
  size_t length
)
{
  size_t offset = 0U;
  int flags = 0;

#ifdef MSG_NOSIGNAL
  flags = MSG_NOSIGNAL;
#endif

  if (socket_fd < 0 || (data == NULL && length > 0U)) {
    return false;
  }

  while (offset < length) {
    ssize_t sent;

    sent = send(socket_fd, data + offset, length - offset, flags);

    if (sent < 0) {
      if (errno == EINTR) {
        continue;
      }

      return false;
    }

    if (sent == 0) {
      return false;
    }

    offset += (size_t)sent;
  }

  return true;
}

socket_receive_result_t socket_utils_receive_all(
  int socket_fd,
  uint8_t *data,
  size_t length
)
{
  size_t offset = 0U;

  if (socket_fd < 0 || (data == NULL && length > 0U)) {
    return SOCKET_RECEIVE_ERROR;
  }

  while (offset < length) {
    ssize_t received;

    received = recv(socket_fd, data + offset, length - offset, 0);

    if (received < 0) {
      if (errno == EINTR) {
        continue;
      }

      return SOCKET_RECEIVE_ERROR;
    }

    if (received == 0) {
      return offset == 0U
        ? SOCKET_RECEIVE_EOF
        : SOCKET_RECEIVE_ERROR;
    }

    offset += (size_t)received;
  }

  return SOCKET_RECEIVE_OK;
}
