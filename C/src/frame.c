#include "frame.h"

#include "checksum.h"
#include "crc.h"

#include <string.h>

static const crc_parameters_t *frame_crc_parameters(
  frame_fcs_scheme_t scheme
)
{
  switch (scheme) {
    case FRAME_FCS_CRC8:
      return &crc8_parameters;
    case FRAME_FCS_CRC10:
      return &crc10_parameters;
    case FRAME_FCS_CRC16:
      return &crc16_parameters;
    case FRAME_FCS_CRC32:
      return &crc32_parameters;
    case FRAME_FCS_CHECKSUM16:
    default:
      return NULL;
  }
}

static void frame_write_uint16(uint8_t *output, uint16_t value)
{
  output[0] = (uint8_t)(value >> 8U);
  output[1] = (uint8_t)(value & UINT16_C(0x00FF));
}

static uint16_t frame_read_uint16(const uint8_t *input)
{
  return (uint16_t)(
    ((uint16_t)input[0] << 8U) |
    (uint16_t)input[1]
  );
}

static void frame_write_uint32(uint8_t *output, uint32_t value)
{
  output[0] = (uint8_t)(value >> 24U);
  output[1] = (uint8_t)(value >> 16U);
  output[2] = (uint8_t)(value >> 8U);
  output[3] = (uint8_t)value;
}

static uint32_t frame_read_uint32(const uint8_t *input)
{
  return
    ((uint32_t)input[0] << 24U) |
    ((uint32_t)input[1] << 16U) |
    ((uint32_t)input[2] << 8U) |
    (uint32_t)input[3];
}

static bool frame_padding_is_zero(
  const uint8_t *payload,
  size_t payload_length,
  size_t padded_length
)
{
  size_t index;

  for (index = payload_length; index < padded_length; index++) {
    if (payload[index] != 0U) {
      return false;
    }
  }

  return true;
}

bool frame_fcs_scheme_parse(
  const char *text,
  frame_fcs_scheme_t *scheme
)
{
  frame_fcs_scheme_t parsed_scheme;

  if (text == NULL || scheme == NULL) {
    return false;
  }

  if (strcmp(text, "checksum") == 0) {
    parsed_scheme = FRAME_FCS_CHECKSUM16;
  } else if (strcmp(text, "crc8") == 0) {
    parsed_scheme = FRAME_FCS_CRC8;
  } else if (strcmp(text, "crc10") == 0) {
    parsed_scheme = FRAME_FCS_CRC10;
  } else if (strcmp(text, "crc16") == 0) {
    parsed_scheme = FRAME_FCS_CRC16;
  } else if (strcmp(text, "crc32") == 0) {
    parsed_scheme = FRAME_FCS_CRC32;
  } else {
    return false;
  }

  *scheme = parsed_scheme;
  return true;
}

const char *frame_fcs_scheme_name(frame_fcs_scheme_t scheme)
{
  switch (scheme) {
    case FRAME_FCS_CHECKSUM16:
      return "checksum";
    case FRAME_FCS_CRC8:
      return "crc8";
    case FRAME_FCS_CRC10:
      return "crc10";
    case FRAME_FCS_CRC16:
      return "crc16";
    case FRAME_FCS_CRC32:
      return "crc32";
    default:
      return NULL;
  }
}

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

bool frame_header_parse(
  const uint8_t serialized_header[FRAME_HEADER_SIZE],
  frame_header_t *header
)
{
  frame_header_t parsed_header;

  if (serialized_header == NULL || header == NULL) {
    return false;
  }

  memcpy(
    parsed_header.source_mac,
    serialized_header,
    MAC_ADDRESS_SIZE
  );
  memcpy(
    parsed_header.destination_mac,
    serialized_header + MAC_ADDRESS_SIZE,
    MAC_ADDRESS_SIZE
  );
  parsed_header.payload_length = frame_read_uint16(
    serialized_header + (2U * MAC_ADDRESS_SIZE)
  );
  parsed_header.sequence_number = serialized_header[
    (2U * MAC_ADDRESS_SIZE) + LENGTH_FIELD_SIZE
  ];

  if (parsed_header.payload_length > MAX_PAYLOAD_SIZE) {
    return false;
  }

  *header = parsed_header;
  return true;
}

bool frame_fcs_size(
  frame_fcs_scheme_t scheme,
  size_t *fcs_size
)
{
  size_t selected_size;

  if (fcs_size == NULL) {
    return false;
  }

  switch (scheme) {
    case FRAME_FCS_CHECKSUM16:
      selected_size = CHECKSUM16_SIZE;
      break;
    case FRAME_FCS_CRC8:
      selected_size = CRC8_SIZE;
      break;
    case FRAME_FCS_CRC10:
      selected_size = CRC10_SIZE;
      break;
    case FRAME_FCS_CRC16:
      selected_size = CRC16_SIZE;
      break;
    case FRAME_FCS_CRC32:
      selected_size = CRC32_SIZE;
      break;
    default:
      return false;
  }

  *fcs_size = selected_size;
  return true;
}

bool frame_padded_payload_length(
  uint16_t payload_length,
  size_t *padded_length
)
{
  size_t selected_length;

  if (
    padded_length == NULL ||
    payload_length > MAX_PAYLOAD_SIZE
  ) {
    return false;
  }

  selected_length = payload_length;

  if (selected_length < MIN_PAYLOAD_SIZE) {
    selected_length = MIN_PAYLOAD_SIZE;
  }

  *padded_length = selected_length;
  return true;
}

bool frame_wire_size(
  uint16_t payload_length,
  frame_fcs_scheme_t scheme,
  size_t *frame_size
)
{
  size_t padded_length;
  size_t fcs_size;

  if (
    frame_size == NULL ||
    !frame_padded_payload_length(payload_length, &padded_length) ||
    !frame_fcs_size(scheme, &fcs_size)
  ) {
    return false;
  }

  *frame_size = FRAME_HEADER_SIZE + padded_length + fcs_size;
  return true;
}

bool frame_serialize(
  const frame_header_t *header,
  const uint8_t *payload,
  frame_fcs_scheme_t scheme,
  uint8_t *serialized_frame,
  size_t capacity,
  size_t *serialized_length
)
{
  const crc_parameters_t *parameters;
  size_t padded_length;
  size_t fcs_size;
  size_t frame_size;
  size_t protected_length;
  uint8_t *fcs;
  uint32_t crc;

  if (
    header == NULL ||
    serialized_frame == NULL ||
    serialized_length == NULL ||
    (header->payload_length > 0U && payload == NULL) ||
    !frame_padded_payload_length(header->payload_length, &padded_length) ||
    !frame_fcs_size(scheme, &fcs_size)
  ) {
    return false;
  }

  frame_size = FRAME_HEADER_SIZE + padded_length + fcs_size;

  if (capacity < frame_size) {
    return false;
  }

  (void)frame_header_serialize(header, serialized_frame);

  if (header->payload_length > 0U) {
    memcpy(
      serialized_frame + FRAME_HEADER_SIZE,
      payload,
      header->payload_length
    );
  }

  memset(
    serialized_frame + FRAME_HEADER_SIZE + header->payload_length,
    0,
    padded_length - header->payload_length
  );

  protected_length = FRAME_HEADER_SIZE + padded_length;
  fcs = serialized_frame + protected_length;

  if (scheme == FRAME_FCS_CHECKSUM16) {
    frame_write_uint16(
      fcs,
      checksum16_compute(serialized_frame, protected_length)
    );
  } else {
    parameters = frame_crc_parameters(scheme);
    crc = crc_compute(
      serialized_frame,
      protected_length,
      parameters
    );

    switch (scheme) {
      case FRAME_FCS_CRC8:
        fcs[0] = (uint8_t)crc;
        break;
      case FRAME_FCS_CRC10:
        frame_write_uint16(fcs, (uint16_t)(crc << 6U));
        break;
      case FRAME_FCS_CRC16:
        frame_write_uint16(fcs, (uint16_t)crc);
        break;
      case FRAME_FCS_CRC32:
        frame_write_uint32(fcs, crc);
        break;
      case FRAME_FCS_CHECKSUM16:
      default:
        return false;
    }
  }

  *serialized_length = frame_size;
  return true;
}

bool frame_verify_and_extract(
  const uint8_t *serialized_frame,
  size_t serialized_length,
  frame_fcs_scheme_t scheme,
  frame_header_t *header,
  uint8_t *payload,
  size_t payload_capacity,
  size_t *payload_length
)
{
  const crc_parameters_t *parameters;
  frame_header_t parsed_header;
  size_t expected_length;
  size_t padded_length;
  size_t protected_length;
  const uint8_t *fcs;
  uint16_t encoded_crc10;
  uint32_t received_crc;
  bool valid;

  if (
    serialized_frame == NULL ||
    header == NULL ||
    payload_length == NULL ||
    serialized_length < FRAME_HEADER_SIZE ||
    !frame_header_parse(serialized_frame, &parsed_header) ||
    !frame_padded_payload_length(
      parsed_header.payload_length,
      &padded_length
    ) ||
    !frame_wire_size(parsed_header.payload_length, scheme, &expected_length) ||
    serialized_length != expected_length ||
    parsed_header.payload_length > payload_capacity ||
    (parsed_header.payload_length > 0U && payload == NULL)
  ) {
    return false;
  }

  protected_length = FRAME_HEADER_SIZE + padded_length;
  fcs = serialized_frame + protected_length;

  if (!frame_padding_is_zero(
    serialized_frame + FRAME_HEADER_SIZE,
    parsed_header.payload_length,
    padded_length
  )) {
    return false;
  }

  if (scheme == FRAME_FCS_CHECKSUM16) {
    valid = checksum16_verify(
      serialized_frame,
      protected_length,
      frame_read_uint16(fcs)
    );
  } else {
    parameters = frame_crc_parameters(scheme);

    switch (scheme) {
      case FRAME_FCS_CRC8:
        received_crc = fcs[0];
        break;
      case FRAME_FCS_CRC10:
        encoded_crc10 = frame_read_uint16(fcs);

        if ((encoded_crc10 & UINT16_C(0x003F)) != 0U) {
          return false;
        }

        received_crc = encoded_crc10 >> 6U;
        break;
      case FRAME_FCS_CRC16:
        received_crc = frame_read_uint16(fcs);
        break;
      case FRAME_FCS_CRC32:
        received_crc = frame_read_uint32(fcs);
        break;
      case FRAME_FCS_CHECKSUM16:
      default:
        return false;
    }

    valid = crc_verify(
      serialized_frame,
      protected_length,
      received_crc,
      parameters
    );
  }

  if (!valid) {
    return false;
  }

  if (parsed_header.payload_length > 0U) {
    memcpy(
      payload,
      serialized_frame + FRAME_HEADER_SIZE,
      parsed_header.payload_length
    );
  }

  *header = parsed_header;
  *payload_length = parsed_header.payload_length;
  return true;
}
