#ifndef FLASHER_H
#define FLASHER_H

#include <libusb-1.0/libusb.h>
#include <string.h>
#include <string>

#define DEV_INTF 0
#define EP_IN 0x81
#define EP_OUT 0x01

#define PACKET_SIZE 62

#define BOOT_GET_INFO 0x01
#define BOOT_READY 0x02
#define BOOT_FLASH_LOCK 0x04
#define BOOT_FLASH_UNLOCK 0x05
#define BOOT_FLASH_ERASE_PAGE 0x06
#define BOOT_FLASH_WRITE_DATA 0x07
#define BOOT_FLASH_WRITE_PAGE 0x08
#define BOOT_FLASH_CHECK_CRC 0x09

#define BOOT_RETURN_OK 0x01
#define BOOT_RETURN_OVEFLOW 0x02
#define BOOT_RETURN_CRC32_ERROR 0x03

#define APP_ADDRESS 0x08004000

#pragma pack(1)
typedef struct {
  uint8_t cmd;
  uint8_t params;
  uint8_t data[PACKET_SIZE - 2];
} BOOTPacket;

#pragma pack(1)
typedef struct {
  unsigned char bootloader_name[32];
  uint8_t v_major;
  uint8_t v_minor;
  uint8_t v_fixn;
  uint16_t flash_size;
  uint16_t page_size;
} BOOTInfoData;

#pragma pack(1)
typedef struct {
  uint32_t addr;
} BOOTEraseData;

#pragma pack(1)
typedef struct {
  uint32_t crc32;
  uint32_t addr;
} BOOTCRC32Data;

#pragma pack(1)
typedef struct  {
    char sig[5];
    char firmware_name[16];
    uint8_t v_major;
    uint8_t v_minor;
    uint8_t v_fixn;
} firmware_version;

class Flasher
{
public:
  Flasher(libusb_device_handle *handle);
  ~Flasher();
  BOOTInfoData getBootInfo();
  void unlockFlash();
  void lockFlash();
  void erasePage(uint32_t addr);
  void sendPage(uint8_t *buf, int bufSize);
  void writePage();
  bool checkCRC(uint32_t addr, uint32_t crc32);
  
private:
  libusb_device_handle *handle;
  BOOTPacket sendPacket(BOOTPacket *packet);
  BOOTPacket sendCommand(uint8_t cmd, uint8_t params, uint8_t *data, size_t size = 0);
  void sendData(uint8_t *data, uint8_t size);

};

namespace Error {
  class USBTransferException {
  public:
    USBTransferException(int error) {
      this->msg = std::string(libusb_error_name(error));
      switch(error) {
        case LIBUSB_ERROR_IO: {
          this->msg = "USB Input/output error";
          break;
        }
        case LIBUSB_ERROR_INVALID_PARAM: {
          this->msg = "USB Invalid parameter";
          break;
        }
        case LIBUSB_ERROR_ACCESS: {
          this->msg = "USB Access denied (insufficient permissions)";
          break;
        }
        case LIBUSB_ERROR_NOT_FOUND: {
          this->msg = "USB Entity not found";
          break;
        }
        case LIBUSB_ERROR_BUSY: {
          this->msg = "USB Resource busy";
          break;
        }
        case LIBUSB_ERROR_INTERRUPTED: {
          this->msg = "USB System call interrupted (perhaps due to signal)";
          break;
        }
        case LIBUSB_ERROR_NO_MEM: {
          this->msg = "USB Insufficient memory";
          break;
        }
        case LIBUSB_ERROR_NOT_SUPPORTED: {
          this->msg = "USB Operation not supported or unimplemented on this platform";
          break;
        }
        case LIBUSB_ERROR_TIMEOUT: {
          this->msg = "USB transfer timed out";
          break;
        }
        case LIBUSB_ERROR_PIPE: {
          this->msg = "USB endpoint halted";
          break;
        }
        case LIBUSB_ERROR_OVERFLOW: {
          this->msg = "USB device offered more data";
          break;
        }
        case LIBUSB_ERROR_NO_DEVICE: {
          this->msg = "USB device has been disconnected";
          break;
        }
        default: {
          this->msg = "USB Unresolved error";
        }
      }
    }
    std::string getError() {
      return this->msg;
    }
  private:
    std::string msg;
  };
  
  class USBException {
  public:
    USBException(int error) {
      this->errCode = error;
      switch(error) {
        case 0: {
          this->msg = "Couldn't find device";
          break;
        }
        case 1: {
          this->msg = "Couldn't detach kernel driver";
          break;
        }
        case 2: {
          this->msg = "Claim interface error";
          break;
        }
        default: {
          this->msg = "USB Unresolved error";
        }
      }
    }
    std::string getError() {
      return this->msg;
    }
    int getErrorCode() {
      return this->errCode;
    }
  private:
    std::string msg;
    int errCode;
  };
}

#endif // FLASHER_H
