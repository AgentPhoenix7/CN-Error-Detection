#include "frame.h"

#include <string.h>

bool frame_header_serialize(
  const frame_header_t *header,
  uint8_t serialized_header[FRAME_HEADER_SIZE]
)
{
  size_t offset = 0U;

  if (
    header == NULL ||
    serialized_header == NULL ||
    header->payload_length > MAX_PAYLOAD_SIZE
  ) {
    return false;
  }

  memcpy(
    serialized_header + offset,
    header->source_mac,
    MAC_ADDRESS_SIZE
  );
  offset += MAC_ADDRESS_SIZE;

  memcpy(
    serialized_header + offset,
    header->destination_mac,
    MAC_ADDRESS_SIZE
  );
  offset += MAC_ADDRESS_SIZE;

  serialized_header[offset] =
    (uint8_t)(header->payload_length >> 8U);
  serialized_header[offset + 1U] =
    (uint8_t)(header->payload_length & UINT16_C(0x00FF));
  offset += LENGTH_FIELD_SIZE;

  serialized_header[offset] = header->sequence_number;

  return true;
}
