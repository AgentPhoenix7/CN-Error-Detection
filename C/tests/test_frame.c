#include "frame.h"

#include "checksum.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int test_serialize_header(void)
{
  const frame_header_t header = {
    .source_mac = {
      UINT8_C(0x00),
      UINT8_C(0x11),
      UINT8_C(0x22),
      UINT8_C(0x33),
      UINT8_C(0x44),
      UINT8_C(0x55)
    },
    .destination_mac = {
      UINT8_C(0xAA),
      UINT8_C(0xBB),
      UINT8_C(0xCC),
      UINT8_C(0xDD),
      UINT8_C(0xEE),
      UINT8_C(0xFF)
    },
    .payload_length = UINT16_C(0x0123),
    .sequence_number = UINT8_C(0x7E)
  };
  const uint8_t expected[FRAME_HEADER_SIZE] = {
    UINT8_C(0x00), UINT8_C(0x11), UINT8_C(0x22),
    UINT8_C(0x33), UINT8_C(0x44), UINT8_C(0x55),
    UINT8_C(0xAA), UINT8_C(0xBB), UINT8_C(0xCC),
    UINT8_C(0xDD), UINT8_C(0xEE), UINT8_C(0xFF),
    UINT8_C(0x01), UINT8_C(0x23),
    UINT8_C(0x7E)
  };
  uint8_t serialized_header[FRAME_HEADER_SIZE] = {UINT8_C(0)};
  size_t index;

  if (!frame_header_serialize(&header, serialized_header)) {
    fputs("FAIL: valid frame header was rejected\n", stderr);
    return 1;
  }

  for (index = 0U; index < FRAME_HEADER_SIZE; index++) {
    if (serialized_header[index] != expected[index]) {
      fprintf(
        stderr,
        "FAIL header byte %zu: expected 0x%02X, received 0x%02X\n",
        index,
        (unsigned int)expected[index],
        (unsigned int)serialized_header[index]
      );
      return 1;
    }
  }

  puts("PASS: serialize frame header");
  return 0;
}

static int test_serialize_maximum_payload_length(void)
{
  const frame_header_t header = {
    .source_mac = {UINT8_C(0)},
    .destination_mac = {UINT8_C(0)},
    .payload_length = (uint16_t)MAX_PAYLOAD_SIZE,
    .sequence_number = UINT8_C(0)
  };
  uint8_t serialized_header[FRAME_HEADER_SIZE] = {UINT8_C(0)};

  if (!frame_header_serialize(&header, serialized_header)) {
    fputs("FAIL: maximum payload length was rejected\n", stderr);
    return 1;
  }

  if (
    serialized_header[12U] != UINT8_C(0x05) ||
    serialized_header[13U] != UINT8_C(0xDB)
  ) {
    fputs(
      "FAIL: maximum payload length was not encoded in network byte order\n",
      stderr
    );
    return 1;
  }

  puts("PASS: serialize maximum payload length");
  return 0;
}

static int test_reject_null_header(void)
{
  uint8_t serialized_header[FRAME_HEADER_SIZE];
  const uint8_t sentinel = UINT8_C(0xA5);
  size_t index;

  memset(
    serialized_header,
    sentinel,
    sizeof serialized_header
  );

  if (frame_header_serialize(NULL, serialized_header)) {
    fputs("FAIL: null frame header was accepted\n", stderr);
    return 1;
  }

  for (index = 0U; index < FRAME_HEADER_SIZE; index++) {
    if (serialized_header[index] != sentinel) {
      fputs("FAIL: null frame header modified output\n", stderr);
      return 1;
    }
  }

  puts("PASS: reject null frame header");
  return 0;
}

static int test_reject_null_serialized_header(void)
{
  const frame_header_t header = {
    .source_mac = {UINT8_C(0)},
    .destination_mac = {UINT8_C(0)},
    .payload_length = UINT16_C(1),
    .sequence_number = UINT8_C(0)
  };

  if (frame_header_serialize(&header, NULL)) {
    fputs("FAIL: null serialized header was accepted\n", stderr);
    return 1;
  }

  puts("PASS: reject null serialized header");
  return 0;
}

static int test_reject_oversized_payload_length(void)
{
  const frame_header_t header = {
    .source_mac = {UINT8_C(0)},
    .destination_mac = {UINT8_C(0)},
    .payload_length = (uint16_t)(MAX_PAYLOAD_SIZE + 1U),
    .sequence_number = UINT8_C(0)
  };
  uint8_t serialized_header[FRAME_HEADER_SIZE];
  const uint8_t sentinel = UINT8_C(0xA5);
  size_t index;

  memset(
    serialized_header,
    sentinel,
    sizeof serialized_header
  );

  if (frame_header_serialize(&header, serialized_header)) {
    fputs("FAIL: oversized payload length was accepted\n", stderr);
    return 1;
  }

  for (index = 0U; index < FRAME_HEADER_SIZE; index++) {
    if (serialized_header[index] != sentinel) {
      fputs("FAIL: oversized payload length modified output\n", stderr);
      return 1;
    }
  }

  puts("PASS: reject oversized payload length");
  return 0;
}

static int test_parse_frame_header(void)
{
  const uint8_t serialized_header[FRAME_HEADER_SIZE] = {
    UINT8_C(0x00), UINT8_C(0x11), UINT8_C(0x22),
    UINT8_C(0x33), UINT8_C(0x44), UINT8_C(0x55),
    UINT8_C(0xAA), UINT8_C(0xBB), UINT8_C(0xCC),
    UINT8_C(0xDD), UINT8_C(0xEE), UINT8_C(0xFF),
    UINT8_C(0x05), UINT8_C(0xDB), UINT8_C(0x7E)
  };
  frame_header_t header;

  if (!frame_header_parse(serialized_header, &header)) {
    fputs("FAIL: valid serialized header was not parsed\n", stderr);
    return 1;
  }

  if (
    memcmp(header.source_mac, serialized_header, MAC_ADDRESS_SIZE) != 0 ||
    memcmp(
      header.destination_mac,
      serialized_header + MAC_ADDRESS_SIZE,
      MAC_ADDRESS_SIZE
    ) != 0 ||
    header.payload_length != MAX_PAYLOAD_SIZE ||
    header.sequence_number != UINT8_C(0x7E)
  ) {
    fputs("FAIL: parsed header fields do not match wire bytes\n", stderr);
    return 1;
  }

  puts("PASS: parse frame header");
  return 0;
}

static int test_reject_invalid_parsed_header_atomically(void)
{
  uint8_t serialized_header[FRAME_HEADER_SIZE] = {UINT8_C(0)};
  frame_header_t header;
  frame_header_t original;

  memset(&header, UINT8_C(0xA5), sizeof header);
  original = header;
  serialized_header[12U] = UINT8_C(0x05);
  serialized_header[13U] = UINT8_C(0xDC);

  if (frame_header_parse(serialized_header, &header)) {
    fputs("FAIL: oversized parsed payload length was accepted\n", stderr);
    return 1;
  }

  if (memcmp(&header, &original, sizeof header) != 0) {
    fputs("FAIL: rejected parsed header modified output\n", stderr);
    return 1;
  }

  puts("PASS: reject invalid parsed header atomically");
  return 0;
}

static int test_frame_size_calculations(void)
{
  static const frame_fcs_scheme_t schemes[] = {
    FRAME_FCS_CHECKSUM16,
    FRAME_FCS_CRC8,
    FRAME_FCS_CRC10,
    FRAME_FCS_CRC16,
    FRAME_FCS_CRC32
  };
  static const size_t expected_fcs_sizes[] = {
    CHECKSUM16_SIZE,
    CRC8_SIZE,
    CRC10_SIZE,
    CRC16_SIZE,
    CRC32_SIZE
  };
  size_t padded_length;
  size_t fcs_size;
  size_t frame_size;
  size_t index;

  if (
    !frame_padded_payload_length(UINT16_C(0), &padded_length) ||
    padded_length != MIN_PAYLOAD_SIZE ||
    !frame_padded_payload_length(
      (uint16_t)MAX_PAYLOAD_SIZE,
      &padded_length
    ) ||
    padded_length != MAX_PAYLOAD_SIZE
  ) {
    fputs("FAIL: padded payload size calculation is incorrect\n", stderr);
    return 1;
  }

  for (index = 0U; index < sizeof schemes / sizeof schemes[0]; index++) {
    if (
      !frame_fcs_size(schemes[index], &fcs_size) ||
      fcs_size != expected_fcs_sizes[index] ||
      !frame_wire_size(UINT16_C(1), schemes[index], &frame_size) ||
      frame_size != FRAME_HEADER_SIZE + MIN_PAYLOAD_SIZE + fcs_size
    ) {
      fputs("FAIL: FCS or frame size calculation is incorrect\n", stderr);
      return 1;
    }
  }

  puts("PASS: calculate padded payload, FCS, and frame sizes");
  return 0;
}

static int test_reject_invalid_frame_size_inputs(void)
{
  size_t output = 123U;

  if (
    frame_padded_payload_length(
      (uint16_t)(MAX_PAYLOAD_SIZE + 1U),
      &output
    ) ||
    output != 123U ||
    frame_fcs_size((frame_fcs_scheme_t)99, &output) ||
    output != 123U ||
    frame_wire_size(
      UINT16_C(1),
      (frame_fcs_scheme_t)99,
      &output
    ) ||
    output != 123U
  ) {
    fputs("FAIL: invalid frame size input modified output\n", stderr);
    return 1;
  }

  puts("PASS: reject invalid frame size inputs atomically");
  return 0;
}

static int test_serialize_fcs_wire_formats(void)
{
  static const frame_fcs_scheme_t schemes[] = {
    FRAME_FCS_CHECKSUM16,
    FRAME_FCS_CRC8,
    FRAME_FCS_CRC10,
    FRAME_FCS_CRC16,
    FRAME_FCS_CRC32
  };
  static const uint8_t expected_fcs[][MAX_FCS_SIZE] = {
    {UINT8_C(0xCA), UINT8_C(0x94), UINT8_C(0), UINT8_C(0)},
    {UINT8_C(0x7E), UINT8_C(0), UINT8_C(0), UINT8_C(0)},
    {UINT8_C(0xB6), UINT8_C(0x40), UINT8_C(0), UINT8_C(0)},
    {UINT8_C(0x23), UINT8_C(0xFE), UINT8_C(0), UINT8_C(0)},
    {UINT8_C(0x10), UINT8_C(0xB3), UINT8_C(0x27), UINT8_C(0x8F)}
  };
  const frame_header_t header = {
    .source_mac = {UINT8_C(0)},
    .destination_mac = {UINT8_C(0)},
    .payload_length = UINT16_C(3),
    .sequence_number = UINT8_C(1)
  };
  const uint8_t payload[] = {
    UINT8_C(0x12), UINT8_C(0x34), UINT8_C(0x56)
  };
  uint8_t serialized_frame[MAX_FRAME_SIZE];
  size_t serialized_length;
  size_t fcs_size;
  size_t fcs_offset;
  size_t index;

  fcs_offset = FRAME_HEADER_SIZE + MIN_PAYLOAD_SIZE;

  for (index = 0U; index < sizeof schemes / sizeof schemes[0]; index++) {
    if (
      !frame_serialize(
        &header,
        payload,
        schemes[index],
        serialized_frame,
        sizeof serialized_frame,
        &serialized_length
      ) ||
      !frame_fcs_size(schemes[index], &fcs_size) ||
      serialized_length != fcs_offset + fcs_size ||
      memcmp(
        serialized_frame + fcs_offset,
        expected_fcs[index],
        fcs_size
      ) != 0
    ) {
      fprintf(stderr, "FAIL: FCS wire format for scheme %zu\n", index);
      return 1;
    }
  }

  puts("PASS: serialize all FCS wire formats");
  return 0;
}

static int test_verify_all_frame_schemes(void)
{
  static const frame_fcs_scheme_t schemes[] = {
    FRAME_FCS_CHECKSUM16,
    FRAME_FCS_CRC8,
    FRAME_FCS_CRC10,
    FRAME_FCS_CRC16,
    FRAME_FCS_CRC32
  };
  const frame_header_t header = {
    .source_mac = {UINT8_C(1)},
    .destination_mac = {UINT8_C(2)},
    .payload_length = UINT16_C(3),
    .sequence_number = UINT8_C(4)
  };
  const uint8_t payload[] = {
    UINT8_C(0xDE), UINT8_C(0xAD), UINT8_C(0x00)
  };
  uint8_t serialized_frame[MAX_FRAME_SIZE];
  uint8_t extracted[MAX_PAYLOAD_SIZE];
  frame_header_t parsed_header;
  size_t serialized_length;
  size_t extracted_length;
  size_t index;

  for (index = 0U; index < sizeof schemes / sizeof schemes[0]; index++) {
    if (
      !frame_serialize(
        &header,
        payload,
        schemes[index],
        serialized_frame,
        sizeof serialized_frame,
        &serialized_length
      ) ||
      !frame_verify_and_extract(
        serialized_frame,
        serialized_length,
        schemes[index],
        &parsed_header,
        extracted,
        sizeof extracted,
        &extracted_length
      ) ||
      extracted_length != sizeof payload ||
      memcmp(extracted, payload, sizeof payload) != 0 ||
      parsed_header.sequence_number != header.sequence_number
    ) {
      fprintf(stderr, "FAIL: verify valid frame scheme %zu\n", index);
      return 1;
    }

    serialized_frame[FRAME_HEADER_SIZE] ^= UINT8_C(0x80);

    if (frame_verify_and_extract(
      serialized_frame,
      serialized_length,
      schemes[index],
      &parsed_header,
      extracted,
      sizeof extracted,
      &extracted_length
    )) {
      fprintf(stderr, "FAIL: corrupted frame scheme %zu was accepted\n", index);
      return 1;
    }
  }

  puts("PASS: verify and reject corruption for all frame schemes");
  return 0;
}

static int test_reject_crc10_padding_bits(void)
{
  const frame_header_t header = {
    .source_mac = {UINT8_C(0)},
    .destination_mac = {UINT8_C(0)},
    .payload_length = UINT16_C(1),
    .sequence_number = UINT8_C(0)
  };
  const uint8_t input = UINT8_C(0xA5);
  uint8_t serialized_frame[MAX_FRAME_SIZE];
  uint8_t output;
  frame_header_t parsed_header;
  size_t serialized_length;
  size_t output_length;

  if (!frame_serialize(
    &header,
    &input,
    FRAME_FCS_CRC10,
    serialized_frame,
    sizeof serialized_frame,
    &serialized_length
  )) {
    fputs("FAIL: CRC-10 frame setup failed\n", stderr);
    return 1;
  }

  serialized_frame[serialized_length - 1U] |= UINT8_C(0x01);

  if (frame_verify_and_extract(
    serialized_frame,
    serialized_length,
    FRAME_FCS_CRC10,
    &parsed_header,
    &output,
    sizeof output,
    &output_length
  )) {
    fputs("FAIL: nonzero CRC-10 padding bits were accepted\n", stderr);
    return 1;
  }

  puts("PASS: reject nonzero CRC-10 padding bits");
  return 0;
}

static int test_reject_small_serialization_buffer_atomically(void)
{
  const frame_header_t header = {
    .source_mac = {UINT8_C(0)},
    .destination_mac = {UINT8_C(0)},
    .payload_length = UINT16_C(1),
    .sequence_number = UINT8_C(0)
  };
  const uint8_t input = UINT8_C(0x5A);
  uint8_t serialized_frame[MAX_FRAME_SIZE];
  uint8_t original[MAX_FRAME_SIZE];
  size_t serialized_length = 123U;

  memset(serialized_frame, UINT8_C(0xA5), sizeof serialized_frame);
  memcpy(original, serialized_frame, sizeof original);

  if (
    frame_serialize(
      &header,
      &input,
      FRAME_FCS_CRC32,
      serialized_frame,
      MIN_FRAME_SIZE - 1U,
      &serialized_length
    ) ||
    serialized_length != 123U ||
    memcmp(serialized_frame, original, sizeof original) != 0
  ) {
    fputs("FAIL: small serialization buffer modified output\n", stderr);
    return 1;
  }

  puts("PASS: reject small serialization buffer atomically");
  return 0;
}

static int test_reject_invalid_received_length_atomically(void)
{
  const frame_header_t header = {
    .source_mac = {UINT8_C(0)},
    .destination_mac = {UINT8_C(0)},
    .payload_length = UINT16_C(1),
    .sequence_number = UINT8_C(0)
  };
  const uint8_t input = UINT8_C(0x5A);
  uint8_t serialized_frame[MAX_FRAME_SIZE];
  uint8_t output = UINT8_C(0xA5);
  frame_header_t parsed_header;
  frame_header_t original_header;
  size_t serialized_length;
  size_t output_length = 123U;

  memset(&parsed_header, UINT8_C(0xA5), sizeof parsed_header);
  original_header = parsed_header;

  if (!frame_serialize(
    &header,
    &input,
    FRAME_FCS_CHECKSUM16,
    serialized_frame,
    sizeof serialized_frame,
    &serialized_length
  )) {
    fputs("FAIL: received-length test setup failed\n", stderr);
    return 1;
  }

  if (
    frame_verify_and_extract(
      serialized_frame,
      serialized_length - 1U,
      FRAME_FCS_CHECKSUM16,
      &parsed_header,
      &output,
      sizeof output,
      &output_length
    ) ||
    output != UINT8_C(0xA5) ||
    output_length != 123U ||
    memcmp(&parsed_header, &original_header, sizeof parsed_header) != 0
  ) {
    fputs("FAIL: invalid received length modified parsed outputs\n", stderr);
    return 1;
  }

  puts("PASS: reject invalid received length atomically");
  return 0;
}

static int test_parse_fcs_scheme_names(void)
{
  static const char *const names[] = {
    "checksum", "crc8", "crc10", "crc16", "crc32"
  };
  size_t index;

  for (index = 0U; index < sizeof names / sizeof names[0]; index++) {
    frame_fcs_scheme_t scheme;

    if (
      !frame_fcs_scheme_parse(names[index], &scheme) ||
      strcmp(frame_fcs_scheme_name(scheme), names[index]) != 0
    ) {
      fprintf(stderr, "FAIL: parse FCS scheme name %s\n", names[index]);
      return 1;
    }
  }

  puts("PASS: parse all FCS scheme names");
  return 0;
}

static int test_zero_length_payload_frame(void)
{
  const frame_header_t header = {
    .source_mac = {UINT8_C(0)},
    .destination_mac = {UINT8_C(0)},
    .payload_length = UINT16_C(0),
    .sequence_number = UINT8_C(0)
  };
  uint8_t serialized_frame[MAX_FRAME_SIZE];
  frame_header_t parsed_header;
  size_t serialized_length;
  size_t payload_length;

  if (
    !frame_serialize(
      &header,
      NULL,
      FRAME_FCS_CRC32,
      serialized_frame,
      sizeof serialized_frame,
      &serialized_length
    ) ||
    !frame_verify_and_extract(
      serialized_frame,
      serialized_length,
      FRAME_FCS_CRC32,
      &parsed_header,
      NULL,
      0U,
      &payload_length
    ) ||
    payload_length != 0U ||
    serialized_length != FRAME_HEADER_SIZE + MIN_PAYLOAD_SIZE + CRC32_SIZE
  ) {
    fputs("FAIL: zero-length payload frame did not round-trip\n", stderr);
    return 1;
  }

  puts("PASS: serialize and verify zero-length payload frame");
  return 0;
}

static int test_reject_nonzero_payload_padding(void)
{
  const frame_header_t header = {
    .source_mac = {UINT8_C(0)},
    .destination_mac = {UINT8_C(0)},
    .payload_length = UINT16_C(1),
    .sequence_number = UINT8_C(0)
  };
  const uint8_t input = UINT8_C(0x5A);
  uint8_t serialized_frame[MAX_FRAME_SIZE];
  uint8_t output;
  frame_header_t parsed_header;
  size_t serialized_length;
  size_t output_length;
  size_t protected_length = FRAME_HEADER_SIZE + MIN_PAYLOAD_SIZE;
  uint16_t checksum;

  if (!frame_serialize(
    &header,
    &input,
    FRAME_FCS_CHECKSUM16,
    serialized_frame,
    sizeof serialized_frame,
    &serialized_length
  )) {
    fputs("FAIL: padding test setup failed\n", stderr);
    return 1;
  }

  serialized_frame[FRAME_HEADER_SIZE + 1U] = UINT8_C(0x01);
  checksum = checksum16_compute(serialized_frame, protected_length);
  serialized_frame[protected_length] = (uint8_t)(checksum >> 8U);
  serialized_frame[protected_length + 1U] = (uint8_t)checksum;

  if (frame_verify_and_extract(
    serialized_frame,
    serialized_length,
    FRAME_FCS_CHECKSUM16,
    &parsed_header,
    &output,
    sizeof output,
    &output_length
  )) {
    fputs("FAIL: nonzero payload padding was accepted\n", stderr);
    return 1;
  }

  puts("PASS: reject nonzero payload padding");
  return 0;
}

int main(void)
{
  int failures = 0;

  failures += test_serialize_header();
  failures += test_reject_null_header();
  failures += test_reject_null_serialized_header();
  failures += test_reject_oversized_payload_length();
  failures += test_serialize_maximum_payload_length();
  failures += test_parse_frame_header();
  failures += test_reject_invalid_parsed_header_atomically();
  failures += test_frame_size_calculations();
  failures += test_reject_invalid_frame_size_inputs();
  failures += test_serialize_fcs_wire_formats();
  failures += test_verify_all_frame_schemes();
  failures += test_reject_crc10_padding_bits();
  failures += test_reject_small_serialization_buffer_atomically();
  failures += test_reject_invalid_received_length_atomically();
  failures += test_parse_fcs_scheme_names();
  failures += test_zero_length_payload_frame();
  failures += test_reject_nonzero_payload_padding();

  return failures == 0 ? 0 : 1;
}
