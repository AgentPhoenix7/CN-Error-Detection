/**
 * @file frame.h
 * @brief Declares Ethernet-like frame structures and operations.
 */

#ifndef FRAME_H
#define FRAME_H

#include "config.h"

#include <stdbool.h>
#include <stddef.h>
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
 * @brief Identifies the error-detection scheme used by a frame.
 */
typedef enum
{
  FRAME_FCS_CHECKSUM16,
  FRAME_FCS_CRC8,
  FRAME_FCS_CRC10,
  FRAME_FCS_CRC16,
  FRAME_FCS_CRC32
} frame_fcs_scheme_t;

/**
 * @brief Parses a command-line scheme name.
 *
 * Accepted names are `checksum`, `crc8`, `crc10`, `crc16`, and `crc32`.
 *
 * @param[in] text Scheme name.
 * @param[out] scheme Parsed scheme, unchanged on failure.
 * @return `true` when the name is supported; otherwise `false`.
 */
bool frame_fcs_scheme_parse(
  const char *text,
  frame_fcs_scheme_t *scheme
);

/**
 * @brief Returns the stable command-line name of a scheme.
 *
 * @param[in] scheme Error-detection scheme.
 * @return Static name string, or `NULL` for an unsupported value.
 */
const char *frame_fcs_scheme_name(frame_fcs_scheme_t scheme);

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

/**
 * @brief Parses a fixed-size wire header into its logical representation.
 *
 * The output is unchanged when either pointer is `NULL` or the encoded
 * payload length exceeds `MAX_PAYLOAD_SIZE`.
 *
 * @param[in] serialized_header Buffer containing `FRAME_HEADER_SIZE` bytes.
 * @param[out] header Parsed logical header.
 * @return `true` when the header is valid; otherwise `false`.
 */
bool frame_header_parse(
  const uint8_t serialized_header[FRAME_HEADER_SIZE],
  frame_header_t *header
);

/**
 * @brief Returns the serialized FCS size for a supported scheme.
 *
 * @param[in] scheme Error-detection scheme.
 * @param[out] fcs_size Serialized FCS size in bytes.
 * @return `true` for a supported scheme; otherwise `false`.
 */
bool frame_fcs_size(
  frame_fcs_scheme_t scheme,
  size_t *fcs_size
);

/**
 * @brief Calculates the transmitted payload size after zero padding.
 *
 * @param[in] payload_length Actual unpadded payload length.
 * @param[out] padded_length Transmitted payload length.
 * @return `true` when `payload_length` is valid; otherwise `false`.
 */
bool frame_padded_payload_length(
  uint16_t payload_length,
  size_t *padded_length
);

/**
 * @brief Calculates the complete serialized frame size.
 *
 * @param[in] payload_length Actual unpadded payload length.
 * @param[in] scheme Error-detection scheme.
 * @param[out] frame_size Complete header, padded payload, and FCS size.
 * @return `true` when every input is valid; otherwise `false`.
 */
bool frame_wire_size(
  uint16_t payload_length,
  frame_fcs_scheme_t scheme,
  size_t *frame_size
);

/**
 * @brief Serializes a complete protected frame.
 *
 * The FCS covers the serialized header and complete padded payload. CRC-10 is
 * left-aligned in its two-byte field. Output arguments are unchanged when an
 * input is invalid or the output capacity is insufficient.
 *
 * @param[in] header Logical header whose length matches the payload.
 * @param[in] payload Actual unpadded payload bytes.
 * @param[in] scheme Error-detection scheme.
 * @param[out] serialized_frame Destination buffer.
 * @param[in] capacity Size of `serialized_frame` in bytes.
 * @param[out] serialized_length Number of bytes written.
 * @return `true` when the frame was serialized; otherwise `false`.
 */
bool frame_serialize(
  const frame_header_t *header,
  const uint8_t *payload,
  frame_fcs_scheme_t scheme,
  uint8_t *serialized_frame,
  size_t capacity,
  size_t *serialized_length
);

/**
 * @brief Verifies and extracts a complete serialized frame.
 *
 * Parsed outputs are unchanged unless the frame has a valid structure and
 * FCS. CRC-10 fields with nonzero padding bits are rejected.
 *
 * @param[in] serialized_frame Complete serialized frame.
 * @param[in] serialized_length Number of received bytes.
 * @param[in] scheme Error-detection scheme.
 * @param[out] header Parsed logical header.
 * @param[out] payload Buffer for actual unpadded payload bytes. It may be
 *                     `NULL` only for a zero-length payload.
 * @param[in] payload_capacity Size of `payload` in bytes.
 * @param[out] payload_length Number of extracted payload bytes.
 * @return `true` when the frame is structurally valid and its FCS matches;
 *         otherwise `false`.
 */
bool frame_verify_and_extract(
  const uint8_t *serialized_frame,
  size_t serialized_length,
  frame_fcs_scheme_t scheme,
  frame_header_t *header,
  uint8_t *payload,
  size_t payload_capacity,
  size_t *payload_length
);

#endif
