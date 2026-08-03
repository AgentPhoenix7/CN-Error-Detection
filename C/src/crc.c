#include "crc.h"

const crc_parameters_t crc8_parameters = {
  .width = 8U,
  .polynomial = UINT32_C(0xD5)
};

const crc_parameters_t crc10_parameters = {
  .width = 10U,
  .polynomial = UINT32_C(0x233)
};

const crc_parameters_t crc16_parameters = {
  .width = 16U,
  .polynomial = UINT32_C(0x8005)
};

const crc_parameters_t crc32_parameters = {
  .width = 32U,
  .polynomial = UINT32_C(0x04C11DB7)
};