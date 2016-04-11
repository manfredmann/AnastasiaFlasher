#include "CRC32.h"

CRC32::CRC32()
{
  this->make_crc_table();
}

CRC32::~CRC32()
{
}

void CRC32::make_crc_table() {
  unsigned long POLYNOMIAL = 0xEDB88320;
  unsigned long remainder;
  unsigned char b = 0;
  do {
    // Start with the data byte
    remainder = b;
    for (unsigned long bit = 8; bit > 0; --bit)
    {
      if (remainder & 1)
        remainder = (remainder >> 1) ^ POLYNOMIAL;
      else
        remainder = (remainder >> 1);
      }
      crcTable[(size_t)b] = remainder;
    } while(0 != ++b);
}

unsigned long CRC32::crc(unsigned char *p, size_t n) {
  unsigned long crc = 0xfffffffful;
  size_t i;
  for(i = 0; i < n; i++)
    crc = crcTable[*p++ ^ (crc&0xff)] ^ (crc>>8);
  return(~crc);
}
