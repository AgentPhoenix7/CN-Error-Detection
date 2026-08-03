/**
 * @file crc.h
 * @brief Declares generic Cyclic Redundancy Check operations.
 */

#ifndef CRC_H
#define CRC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Describes a CRC generator polynomial.
 */
typedef struct
{
  uint8_t width;        /**< Number of bits in the CRC remainder. */
  uint32_t polynomial; /**< Polynomial without its highest-order term. */
} crc_parameters_t;

/**
 * @brief CRC-8 parameters using polynomial 0xD5.
 */
extern const crc_parameters_t crc8_parameters;

/**
 * @brief CRC-10 parameters using polynomial 0x233.
 */
extern const crc_parameters_t crc10_parameters;

/**
 * @brief CRC-16 parameters using polynomial 0x8005.
 */
extern const crc_parameters_t crc16_parameters;

/**
 * @brief CRC-32 parameters using polynomial 0x04C11DB7.
 */
extern const crc_parameters_t crc32_parameters;

/**
 * @brief Calculates a CRC remainder for a byte array.
 *
 * @param[in] data Pointer to the input bytes. It may be `NULL` only when
 *                 `length` is zero.
 * @param[in] length Number of bytes in `data`.
 * @param[in] parameters CRC width and generator polynomial to use.
 * @return The calculated remainder in the lowest `parameters->width` bits.
 *
 * @pre `parameters` must not be `NULL`.
 * @pre `parameters->width` must be between 1 and 32.
 */
uint32_t crc_compute(
  const uint8_t *data,
  size_t length,
  const crc_parameters_t *parameters
);

#endif
