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
  uint8_t width;
  uint32_t polynomial;
 } crc_parameters_t;
 
 #endif