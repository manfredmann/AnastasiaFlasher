#ifndef FLASHER_H
#define FLASHER_H

#include <libusb-1.0/libusb.h>
#include <string.h>
#include <string>
#include <iostream>
#include "errors.h"

using namespace std;

#define DEV_INTF 0
#define EP_IN 0x81
#define EP_OUT 0x01

#define MAX_PACKET_SIZE 62

#define BOOT_GET_INFO 0x01
#define BOOT_FLASH_LOCK 0x04
#define BOOT_FLASH_UNLOCK 0x05
#define BOOT_FLASH_SET_PAGE_ADDR 0x12
#define BOOT_FLASH_ERASE_PAGE 0x06
#define BOOT_FLASH_WRITE_DATA_START 0x07
#define BOOT_FLASH_WRITE_PAGE 0x08
#define BOOT_FLASH_CHECK_CRC 0x09
#define BOOT_FLASH_READ_PAGE 0x10
#define BOOT_FLASH_READ_DATA 0x11

#define BOOT_RETURN_OK 0x01
#define BOOT_RETURN_OVEFLOW 0x02
#define BOOT_RETURN_CRC32_ERROR 0x03

#define BOOT_CMD 1
#define BOOT_DATA 2

#define FLASHER_V_MAJOR 0
#define FLASHER_V_MINOR 1
#define FLASHER_V_FIXN 0

#pragma pack(1)
typedef struct {
  unsigned char bootloader_name[32];
  uint8_t v_major;
  uint8_t v_minor;
  uint8_t v_fixn;
  uint16_t flash_size;
  uint16_t page_size;
  uint32_t app_addr;
} BOOTInfoData;

#pragma pack(1)
typedef struct  {
    char sig[5];
    char firmware_name[16];
    uint8_t v_major;
    uint8_t v_minor;
    uint8_t v_fixn;
} firmware_version;

#pragma pack(1)
typedef struct {
  uint8_t cmd;
  uint32_t param1;
  uint32_t param2;
} BOOTCommand;

#pragma pack(1) 
typedef struct {
  uint8_t data[MAX_PACKET_SIZE-1];
  uint8_t size;
} BOOTData;

class Flasher
{
public:
  Flasher(libusb_device_handle *handle);
  ~Flasher();
  BOOTInfoData getBootInfo();
  void unlockFlash();
  void lockFlash();
  void prepareDataSend();
  void setPageAddr(uint32_t addr);
  void erasePage();
  void sendPage(uint8_t *buf, int bufSize);
  void writePage();
  bool checkCRC(uint32_t addr, uint32_t crc32);
  
private:
  libusb_device_handle *handle;
  uint8_t *sendCommand(uint8_t cmd, uint32_t param1, uint32_t param2, uint8_t *a_mode, uint8_t *size);
  void sendData(BOOTData *data);

};

#endif // FLASHER_H
