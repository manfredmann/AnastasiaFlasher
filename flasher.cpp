#include "flasher.h"

Flasher::Flasher(libusb_device_handle *handle) {
  this->handle = handle;
}

Flasher::~Flasher(){
}

uint8_t *Flasher::sendCommand(uint8_t cmd, uint32_t param1, uint32_t param2, uint8_t *a_mode, uint8_t *size) {
  BOOTCommand command;
  command.cmd = cmd;
  command.param1 = param1;
  command.param2 = param2;
  
  uint8_t tbuf[sizeof(command)+1];
  uint8_t rbuf[63];
  
  tbuf[0] = BOOT_CMD;
  memcpy(&tbuf[1], &command, sizeof(command));
  int returned;
  int len;
  returned = libusb_interrupt_transfer(handle, EP_OUT, &tbuf[0], sizeof(tbuf), &len, 200);

  if (returned != LIBUSB_SUCCESS) {
    throw Error::USBTransferException(returned);
  }
  
  returned = libusb_interrupt_transfer(handle, EP_IN, &rbuf[0], sizeof(rbuf), &len, 200);
  
  if (returned != LIBUSB_SUCCESS) {
    throw Error::USBTransferException(returned);
  }
  
  (*a_mode) = rbuf[0];
  uint8_t *answer = new uint8_t[len - 1];
  memcpy(answer, &rbuf[1], len - 1);
  (*size) = len - 1;
  return answer;
}

void Flasher::sendData(BOOTData *data) {
  uint8_t tbuf[sizeof(BOOTData)+1];
  uint8_t rbuf[63];
  
  tbuf[0] = BOOT_DATA;
  memcpy(&tbuf[1], data, sizeof(BOOTData));
  
  int returned;
  int len;
  returned = libusb_interrupt_transfer(handle, EP_OUT, &tbuf[0], sizeof(tbuf), &len, 200);

  if (returned != LIBUSB_SUCCESS) {
    throw Error::USBTransferException(returned);
  }
  
  returned = libusb_interrupt_transfer(handle, EP_IN, &rbuf[0], sizeof(rbuf), &len, 200);
  
  if (returned != LIBUSB_SUCCESS) {
    throw Error::USBTransferException(returned);
  }
  
  if (rbuf[0] != BOOT_CMD && (len - 1) != sizeof(BOOTCommand)) {
    throw Error::FlasherException(2);
  } else {
    BOOTCommand command;
    memcpy(&command, &rbuf[1], len - 1);
    if (command.cmd != BOOT_RETURN_OK) {
      throw Error::FlasherException(1);
    }
  }
}

BOOTInfoData Flasher::getBootInfo() {
  BOOTInfoData bootInfo;
  BOOTData data;
  
  uint8_t mode;
  uint8_t size;
  uint8_t *answer = this->sendCommand(BOOT_GET_INFO, 0x00, 0x00, &mode, &size);
  if (mode == BOOT_DATA && size == (MAX_PACKET_SIZE)) {
    memcpy(&data, answer, sizeof(data));
    memcpy(&bootInfo, &data.data, data.size);
    return bootInfo;
  } else {
    throw Error::FlasherException(0);
  }
}

void Flasher::unlockFlash() {
  BOOTCommand command;
  uint8_t mode;
  uint8_t size;
  
  uint8_t *answer = this->sendCommand(BOOT_FLASH_UNLOCK, 0x00, 0x00, &mode, &size);
  if (mode == BOOT_CMD && size == sizeof(command)) {
    
    memcpy(&command, answer, sizeof(command));
    if (command.cmd != BOOT_RETURN_OK) {
      throw Error::FlasherException(1);
    }
  } else {
    throw Error::FlasherException(2);
  }
}

void Flasher::lockFlash() {
  BOOTCommand command;
  uint8_t mode;
  uint8_t size;
  
  uint8_t *answer = this->sendCommand(BOOT_FLASH_LOCK, 0x00, 0x00, &mode, &size);
  if (mode == BOOT_CMD && size == sizeof(command)) {
    
    memcpy(&command, answer, sizeof(command));
    if (command.cmd != BOOT_RETURN_OK) {
      throw Error::FlasherException(1);
    }
  } else {
    throw Error::FlasherException(2);
  }
}

void Flasher::prepareDataSend() {
  BOOTCommand command;
  uint8_t mode;
  uint8_t size;
  
  uint8_t *answer = this->sendCommand(BOOT_FLASH_WRITE_DATA_START, 0x00, 0x00, &mode, &size);
  if (mode == BOOT_CMD && size == sizeof(command)) {
    
    memcpy(&command, answer, sizeof(command));
    if (command.cmd != BOOT_RETURN_OK) {
      throw Error::FlasherException(1);
    }
  } else {
    throw Error::FlasherException(2);
  }
}

void Flasher::setPageAddr(uint32_t addr) {
  BOOTCommand command;
  uint8_t mode;
  uint8_t size;
  
  uint8_t *answer = this->sendCommand(BOOT_FLASH_SET_PAGE_ADDR, addr, 0x00, &mode, &size);
  if (mode == BOOT_CMD && size == sizeof(command)) {
    
    memcpy(&command, answer, sizeof(command));
    if (command.cmd != BOOT_RETURN_OK) {
      throw Error::FlasherException(1);
    }
  } else {
    throw Error::FlasherException(2);
  }
}

void Flasher::erasePage() {
  BOOTCommand command;
  uint8_t mode;
  uint8_t size;
  
  uint8_t *answer = this->sendCommand(BOOT_FLASH_ERASE_PAGE, 0x00, 0x00, &mode, &size);
  if (mode == BOOT_CMD && size == sizeof(command)) {
    
    memcpy(&command, answer, sizeof(command));
    if (command.cmd != BOOT_RETURN_OK) {
      throw Error::FlasherException(1);
    }
  } else {
    throw Error::FlasherException(2);
  }
}

void Flasher::sendPage(uint8_t *buf, int bufSize) {
  int j = 0;
  int r = bufSize;
  
  BOOTData data;
  
  while ( j < r) {
    uint8_t size;
    memset(data.data, 0, MAX_PACKET_SIZE - 1);

    if (r - j < (MAX_PACKET_SIZE - 1)) {
      size = (r - j);
    } else {
      size = (MAX_PACKET_SIZE - 1);
    }

    memcpy(&data.data[0], &buf[j], size);
    j += size;
    data.size = size;
    
    this->sendData(&data);
  }
}

void Flasher::writePage() {
  BOOTCommand command;
  uint8_t mode;
  uint8_t size;
  
  uint8_t *answer = this->sendCommand(BOOT_FLASH_WRITE_PAGE, 0x00, 0x00, &mode, &size);
  if (mode == BOOT_CMD && size == sizeof(command)) {
    
    memcpy(&command, answer, sizeof(command));
    if (command.cmd != BOOT_RETURN_OK) {
      throw Error::FlasherException(1);
    }
  } else {
    throw Error::FlasherException(2);
  }
}

bool Flasher::checkCRC(uint32_t addr, uint32_t crc32) {
  BOOTCommand command;
  uint8_t mode;
  uint8_t size;
  
  uint8_t *answer = this->sendCommand(BOOT_FLASH_CHECK_CRC, crc32, addr, &mode, &size);
  if (mode == BOOT_CMD && size == sizeof(command)) {
    
    memcpy(&command, answer, sizeof(command));
    if (command.cmd == BOOT_RETURN_CRC32_ERROR) {
      return false;
    } else {
      return true;
    }
  } else {
    throw Error::FlasherException(2);
  }
}