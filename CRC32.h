#ifndef CRC32_H
#define CRC32_H

#include <iomanip>
#include <sstream>

class CRC32
{
public:
  CRC32();
  ~CRC32();
  uint32_t crc(unsigned char *p, size_t n);
  
private:
  uint32_t crcTable[256];
  void make_crc_table();

};

#endif // CRC32_H
