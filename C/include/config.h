#ifndef CONFIG_H
#define CONFIG_H

/* Complete simulated frame sizes */
#define MIN_FRAME_SIZE 64U
#define MAX_FRAME_SIZE 1518U

/* Header fields */
#define MAC_ADDRESS_SIZE 6U
#define LENGTH_FIELD_SIZE 2U
#define SEQUENCE_FIELD_SIZE 1U

#define FRAME_HEADER_SIZE ((2U * MAC_ADDRESS_SIZE) + LENGTH_FIELD_SIZE + SEQUENCE_FIELD_SIZE)

/* Frame Check Sequence sizes */
#define CHECKSUM16_SIZE 2U
#define CRC8_SIZE 1U
#define CRC10_SIZE 2U
#define CRC16_SIZE 2U
#define CRC32_SIZE 4U

#define MIN_FCS_SIZE CRC8_SIZE
#define MAX_FCS_SIZE CRC32_SIZE

/*
 * Using one common padded-payload size for every scheme:
 * 
 * Minimum: 64 - 15 - 1 = 48 bytes
 * Maximum: 1518 - 15 - 4 = 1499 bytes
 */
#define MIN_PAYLOAD_SIZE (MIN_FRAME_SIZE - FRAME_HEADER_SIZE - MIN_FCS_SIZE)
#define MAX_PAYLOAD_SIZE (MAX_FRAME_SIZE - FRAME_HEADER_SIZE - MAX_FCS_SIZE)

/* Default TCP configuration */
#define DEFAULT_SERVER_ADDRESS "127.0.0.1"
#define DEFAULT_SERVER_PORT 4321U

/*
 * Compile-time checks. Compilation fails if an accidental change
 * makes the frame-size calculations invalid.
 */
_Static_assert(FRAME_HEADER_SIZE == 15U, "The simulated frame header must be 15 bytes");
_Static_assert(MIN_PAYLOAD_SIZE  >= 46U, "The payload must meet the Ethernet minimum");
_Static_assert(FRAME_HEADER_SIZE + MIN_PAYLOAD_SIZE + MIN_FCS_SIZE == MIN_FRAME_SIZE, "Minimum frame-size calculation is incorrect");
_Static_assert(FRAME_HEADER_SIZE + MAX_PAYLOAD_SIZE + MAX_FCS_SIZE == MAX_FRAME_SIZE, "Maximum frame-size calculation is incorrect");
#endif /* CONFIG_H */