/**
 * @file frame.h
 * @brief Declares Ethernet-like frame structures and operations.
 */

#ifndef FRAME_H
#define FRAME_H

#include "config.h"

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Represents the logical fields of a frame header.
 *
 * `payload_length` contains the number of valid, unpadded payload bytes in
 * host byte order. Serialization converts it to network byte order.
 */
typedef struct
{
  uint8_t source_mac[MAC_ADDRESS_SIZE];
  uint8_t destination_mac[MAC_ADDRESS_SIZE];
  uint16_t payload_length;
  uint8_t sequence_number;
} frame_header_t;

/**
 * @brief Serializes a logical header into its fixed-size wire representation.
 *
 * Fields are written in header order, and `payload_length` is converted from
 * host byte order to network byte order. No output is modified when either
 * pointer is `NULL` or `payload_length` exceeds `MAX_PAYLOAD_SIZE`.
 *
 * @param[in] header Logical header fields to serialize.
 * @param[out] serialized_header Buffer of `FRAME_HEADER_SIZE` bytes.
 * @return `true` when the header was serialized; otherwise `false`.
 */
bool frame_header_serialize(
  const frame_header_t *header,
  uint8_t serialized_header[FRAME_HEADER_SIZE]
);

#endif
