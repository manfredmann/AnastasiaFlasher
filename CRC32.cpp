#include "CRC32.h"

CRC32::CRC32()
{
  this->make_crc_table();
}

CRC32::~CRC32()
{
}

void CRC32::make_crc_table() {
  uint32_t POLYNOMIAL = 0xEDB88320;
  uint32_t remainder;
  unsigned char b = 0;
  do {
    // Start with the data byte
    remainder = b;
    for (uint32_t bit = 8; bit > 0; --bit)
    {
      if (remainder & 1)
        remainder = (remainder >> 1) ^ POLYNOMIAL;
      else
        remainder = (remainder >> 1);
      }
      crcTable[(size_t)b] = remainder;
    } while(0 != ++b);
}

uint32_t CRC32::crc(unsigned char *p, size_t n) {
  uint32_t crc = 0xfffffffful;
  size_t i;
  for(i = 0; i < n; i++)
    crc = crcTable[*p++ ^ (crc&0xff)] ^ (crc>>8);
  return(~crc);
}
