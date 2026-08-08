/**
 * @file frame.h
 * @brief Declares Ethernet-like frame structures and operations.
 */

#ifndef FRAME_H
#define FRAME_H

#include "config.h"

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

#endif
