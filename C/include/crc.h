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

#endif
