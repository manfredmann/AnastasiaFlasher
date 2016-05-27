#include <string>
#include <iostream>

using namespace std;

namespace Error {
  class USBTransferException {
  public:
    USBTransferException(int error) {
      //this->msg = std::string(libusb_error_name(error));
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
  
  
  class FlasherException {
  public:
    FlasherException(int error) {
      this->errCode = error;
      switch(error) {
        case 0: {
          this->msg = "Can't get info from bootloader";
          break;
        }
        case 1: {
          this->msg = "The bootloader has not returned BOOT_RETURN_OK";
          break;
        }
        case 2: {
          this->msg = "The bootloader has returned BOOT_DATA instead of BOOT_CMD";
          break;
        }
        case 3: {
          this->msg = "The bootloader has returned BOOT_CMD instead of BOOT_DATA";
        }
      }
    }
    std::string getError() {
      return this->msg;
    }
  private:
    std::string msg;
    int errCode;
  };
}