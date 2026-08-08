#include "frame.h"

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

int main(void)
{
  int failures = 0;

  failures += test_serialize_header();
  failures += test_reject_null_header();
  failures += test_reject_null_serialized_header();

  return failures == 0 ? 0 : 1;
}