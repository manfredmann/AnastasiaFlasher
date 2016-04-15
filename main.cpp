#define _GLIBCXX_USE_CXX11_ABI 0

#include <string>
#include <algorithm>
#include <map>
#include <fstream>
#include <tclap/CmdLine.h>
#include "CRC32.h"
#include "flasher.h"

using namespace std;
using namespace TCLAP;

bool findVersionSignature(firmware_version *ver, ifstream *file) {
  char buf[5];
  int pos = 0;
  size_t r;
  bool bingo = false;
  
  file->seekg(0, file->beg);

  try {
    while((r = file->read(&buf[0], 5).gcount()) == 5) {
      string bufStr(buf);
      string sig("FIRM");
      
      if(bufStr.compare(sig) == 0) {
        file->seekg(pos, file->beg);
        file->read((char *) ver, sizeof(firmware_version));
        bingo = true;
        break;
      } else {
        pos++;
        file->seekg(pos, file->beg);
      }
    }
    file->seekg(0, file->beg);
  } catch(ifstream::failure& e) {
    if (file->rdstate() & ifstream::eofbit) {
      
    } else
      cout << "Caught an exception: " << e.what() << endl;
  }
  return bingo;
}

void flash(string fname, bool checkSig) {
  ifstream file;
  firmware_version fVersion;
  int fSize;
  file.exceptions(ifstream::failbit | ifstream::badbit | ifstream::eofbit);
  try {
    file.open(fname, ifstream::in | ifstream::binary);
    file.seekg (0, file.end);
    fSize = file.tellg();
    file.seekg (0, file.beg);
  }
  
  catch (ios_base::failure &e) {
    cout << "Couldn't open file \"" << fname << "\"" << endl; 
    return;
  }
  
  libusb_device_handle *handle = NULL;
  
  try {
    if(checkSig) {
      if(findVersionSignature(&fVersion, &file)) {
        cout << "Detected firmware: ";
        cout << fVersion.firmware_name;
        cout << " v" << (int)fVersion.v_major << "." << (int)fVersion.v_minor << "." << (int)fVersion.v_fixn << endl;
      } else {
        cout << "This is not a AnastasiaFlasher firmware" << endl;
        return;
      }
    }

    libusb_init(NULL);
    // libusb_set_debug(NULL, 3);

    handle = libusb_open_device_with_vid_pid(NULL, 0x0483, 0x5710);
    if(handle == NULL) {
      throw Error::USBException(0);
    } else {
      cout << "Device: OK" << endl;
    }

    if(libusb_kernel_driver_active(handle, DEV_INTF)) {
      if(libusb_detach_kernel_driver(handle, DEV_INTF) != 0) {
        throw Error::USBException(1);
      }
    }

    if(libusb_claim_interface(handle, DEV_INTF) < 0) {
      throw Error::USBException(2);
    }

    uint8_t data[PACKET_SIZE - 2];
    memset(data, 0, PACKET_SIZE - 2);

    CRC32* crc = new CRC32();
    Flasher* flasher = new Flasher(handle);

    BOOTInfoData bootInfo = flasher->getBootInfo();

    cout << "Bootloader: ";
    cout << bootInfo.bootloader_name;
    cout << " v" << (int)bootInfo.v_major << "." << (int)bootInfo.v_minor << "." << (int)bootInfo.v_fixn << endl;
    cout << "Flash size: \t" << bootInfo.flash_size << "Kb" << endl;
    cout << "Page size: \t" << bootInfo.page_size << "b" << endl;
    cout << "Pages: \t\t" << (bootInfo.flash_size * 1024) / bootInfo.page_size << endl;

    int pages = ceil((float)fSize / (float)bootInfo.page_size);

    cout << "File contains " << pages << " pages" << endl;
    flasher->unlockFlash();

    int page;
    uint32_t addr = APP_ADDRESS;

    for(page = 0; page < pages; page++) {
      uint8_t buf[bootInfo.page_size] = { 0 };
      size_t r = 0;

      int n = 1024;
      if((1024 + file.tellg()) > fSize) {
        n = fSize - file.tellg();
      }
      r = file.read((char*)&buf[0], n).gcount();

      flasher->erasePage(addr);
      cout << (page + 1) << ": 0x" << hex << addr << " erase page" << endl;

      flasher->sendPage(&buf[0], r);

      cout << (page + 1) << ": 0x" << hex << addr << " write page" << endl;
      flasher->writePage();

      cout << (page + 1) << ": 0x" << hex << addr << " check CRC32 ";
      uint32_t crc32 = crc->crc(&buf[0], 1024);
      if(flasher->checkCRC(addr, crc32)) {
        cout << "OK" << endl;
      } else {
        cout << "Fail" << endl;
      }

      addr += bootInfo.page_size;
    }

    flasher->lockFlash();
    file.close();
  } catch (ios_base::failure &e) {
    cout << "Caught an exception: " << e.what() << endl;
  } catch (Error::USBException &e) {
    cout << "Error: " << e.getError() << endl;
  } catch (Error::USBTransferException &e) {
    cout << "Error: " << e.getError() << endl;
  }
  
  if(handle != NULL)
    libusb_close(handle);
  libusb_exit(NULL);
}

int main(int argc, char *argv[]) {
  string fname;
  string command;
  bool checkSig;
  try {
    CmdLine cmd("Flasher for Anastasia Bootloader", ' ', "0.1.0");


    vector<string> allowed;
		allowed.push_back("flash");
		allowed.push_back("erase");
		allowed.push_back("reboot");
		allowed.push_back("dump");
		ValuesConstraint<string> allowedVals(allowed);

    UnlabeledValueArg<string> cmdArg("command", "command", true, "", "command", false, (Visitor *) &allowedVals);
    ValueArg<string> fileArg("f", "file", "File to flash", false, "main.bin", "string");
    SwitchArg checkSigArg("d", "dont-check", "Don't check firmware signature", true);
    cmd.add(cmdArg);
    cmd.add(fileArg);
    cmd.add(checkSigArg);

    cmd.parse(argc, argv);
    
    fname = fileArg.getValue();
    command = cmdArg.getValue();
    checkSig = checkSigArg.getValue();
  } catch (TCLAP::ArgException &e) {
    cerr << "error: " << e.error() << " for arg " << e.argId() << endl;
    return 0;
  }
  
  #define CMD_FLASH 1
  #define CMD_DUMP 2
  #define CMP_ERASE 3
  #define CMD_REBOOT 4
  
  map <string, int> allowedCommands;
  
  allowedCommands["flash"] = CMD_FLASH;
  allowedCommands["dump"] = CMD_DUMP;
  allowedCommands["erase"] = CMP_ERASE;
  allowedCommands["reboot"] = CMD_REBOOT;
  
  
  switch(allowedCommands[command]) {
    case CMD_FLASH: {
      flash(fname, checkSig);
      break;
    }
    case CMD_DUMP: {
      break;
    }
    case CMP_ERASE: {
      break;
    }
    case CMD_REBOOT: {
      break;
    }
    default: {
      cout << "Unsupported command" << endl;
    }
  }
  
  return 0;
}