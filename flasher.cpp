#include "flasher.h"

Flasher::Flasher(libusb_device_handle *handle) {
  this->handle = handle;
}

Flasher::~Flasher(){
}

BOOTPacket Flasher::sendPacket(BOOTPacket *packet) {
  uint8_t tbuf[PACKET_SIZE] = {0};
  uint8_t rbuf[PACKET_SIZE] = {0};

  memcpy(&tbuf[0], packet, PACKET_SIZE);
  int returned;
  int len;
  returned = libusb_interrupt_transfer(handle, EP_OUT, tbuf, PACKET_SIZE, &len, 200);
  
  if (returned != LIBUSB_SUCCESS) {
    throw Error::USBTransferException(returned);
  }
  
  returned = libusb_interrupt_transfer(handle, EP_IN, rbuf, PACKET_SIZE, &len, 200);
  
  if (returned != LIBUSB_SUCCESS) {
    throw Error::USBTransferException(returned);
  }
  
  BOOTPacket answer;
  memcpy(&answer, &rbuf[0], PACKET_SIZE);
  return answer;
}

BOOTPacket Flasher::sendCommand(uint8_t cmd, uint8_t params, uint8_t *data, size_t size) {
  BOOTPacket packet;
  packet.cmd = cmd;
  packet.params = params;
  if (size == 0)
    memcpy(&packet.data, data, PACKET_SIZE - 2);
  else
    memcpy(&packet.data, data, size);

  return sendPacket(&packet);
}

BOOTInfoData Flasher::getBootInfo() {
  BOOTInfoData bootInfo;
  
  uint8_t data[PACKET_SIZE - 2];
  memset(data, 0, PACKET_SIZE - 2);
  BOOTPacket answer = sendCommand(BOOT_GET_INFO, 0x00, &data[0]);

  memcpy(&bootInfo, &answer.data, sizeof(bootInfo));
  return bootInfo;
}

void Flasher::unlockFlash() {
  uint8_t data[PACKET_SIZE - 2];
  memset(data, 0, PACKET_SIZE - 2);
  
  sendCommand(BOOT_FLASH_UNLOCK, 0x00, &data[0]);
}

void Flasher::lockFlash() {
  uint8_t data[PACKET_SIZE - 2];
  memset(data, 0, PACKET_SIZE - 2);
  
  sendCommand(BOOT_FLASH_LOCK, 0x00, &data[0]);
}

void Flasher::erasePage(uint32_t addr) {
  BOOTEraseData eData;
  eData.addr = addr;

  uint8_t data[PACKET_SIZE - 2];
  memset(data, 0, PACKET_SIZE - 2);
  
  memcpy(&data[0], &eData, sizeof(eData));
  sendCommand(BOOT_FLASH_ERASE_PAGE, 0x00, &data[0]);
}

void Flasher::sendData(uint8_t *data, uint8_t size) {
  sendCommand(BOOT_FLASH_WRITE_DATA, size, data);
}

void Flasher::sendPage(uint8_t *buf, int bufSize) {
  int j = 0;
  int r = bufSize;
  
  uint8_t data[PACKET_SIZE - 2];
  
  while ( j < r) {
    uint8_t size;
    memset(data, 0, PACKET_SIZE - 2);

    if (r - j < (PACKET_SIZE - 2)) {
      size = (r - j);
    } else {
      size = (PACKET_SIZE - 2);
    }

    memcpy(&data[0], &buf[j], size);
    j += size;

    sendData(&data[0], size);
  }
}

void Flasher::writePage() {
  uint8_t data[PACKET_SIZE - 2];
  
  memset(data, 0, PACKET_SIZE - 2);
  sendCommand(BOOT_FLASH_WRITE_PAGE, 0x00, &data[0]);
}

bool Flasher::checkCRC(uint32_t addr, uint32_t crc32) {
  BOOTCRC32Data crc32Data;
  crc32Data.crc32 = crc32;
  crc32Data.addr = addr;

  BOOTPacket answer = sendCommand(BOOT_FLASH_CHECK_CRC, 0x00, (uint8_t *) &crc32Data, sizeof(crc32Data));

  if (answer.params == BOOT_RETURN_OK)
    return true;
  else
    return false;
}