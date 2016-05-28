#include <string>
#include <algorithm>
#include <cmath>
#include <map>
#include <fstream>
#include <boost/program_options.hpp>
#include "CRC32.h"
#include "flasher.h"

using namespace std;

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

libusb_device_handle *usbInit(void) {
  libusb_device_handle *handle = NULL;
  libusb_init(NULL);
    // libusb_set_debug(NULL, 3);

  handle = libusb_open_device_with_vid_pid(NULL, 0x0483, 0x5710);
  if(handle == NULL) {
    throw Error::USBException(0);
  } else {
    cout << "Device: \t\tOK" << endl;
  }

  if(libusb_kernel_driver_active(handle, DEV_INTF)) {
    if(libusb_detach_kernel_driver(handle, DEV_INTF) != 0) {
      throw Error::USBException(1);
    }
  }

  if(libusb_claim_interface(handle, DEV_INTF) < 0) {
    throw Error::USBException(2);
  }
  
  return handle;
}

void usbDeInit(libusb_device_handle *handle) {
  if(handle != NULL)
    libusb_close(handle);
  libusb_exit(NULL);
}


void printBootInfo(BOOTInfoData *bootInfo) {
  int pages = (bootInfo->flash_size * 1024) / bootInfo->page_size;
  int pagesBootloader = 10240 / bootInfo->page_size;
  int app_addr = bootInfo->app_addr;
  
  cout << "Bootloader: \t\t";
  cout << bootInfo->bootloader_name;
  cout << " v" << (int)bootInfo->v_major << "." << (int)bootInfo->v_minor << "." << (int)bootInfo->v_fixn << endl;
  cout << "Flash size: \t\t" << bootInfo->flash_size << "Kb" << endl;
  cout << "Page size: \t\t" << bootInfo->page_size << "b" << endl;
  cout << "Pages: \t\t\t" << pages << " (" << pagesBootloader << " for bootloader) " << endl;
  cout << "Available pages: \t" << pages - 10 << ": 0x" << hex << app_addr << " - 0x" << hex << (app_addr + ((pages - 10) * 1024)) << endl;
  cout << "Application address: \t0x" << hex << app_addr << endl;
}

void getBootInfo(void) {
  try {
    libusb_device_handle *handle = usbInit();
    Flasher* flasher = new Flasher(handle);
    
    BOOTInfoData bootInfo = flasher->getBootInfo();
    
    printBootInfo(&bootInfo);

    usbDeInit(handle);
  } catch (Error::USBException &e) {
    cout << "Error: " << e.getError() << endl;
  } catch (Error::USBTransferException &e) {
    cout << "Error: " << e.getError() << endl;
  } catch (Error::FlasherException &e) {
    cout << "Error: " << e.getError() << endl;
  }
  
}

void write(string fname, bool dcheckSig) {
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
    if(!dcheckSig) {
      if(findVersionSignature(&fVersion, &file)) {
        cout << "Detected firmware: \t";
        cout << fVersion.firmware_name;
        cout << " v" << (int)fVersion.v_major << "." << (int)fVersion.v_minor << "." << (int)fVersion.v_fixn << endl;
      } else {
        cout << "This is not a AnastasiaFlasher firmware" << endl;
        return;
      }
    }
    
    handle = usbInit();
    
    CRC32* crc = new CRC32();
    Flasher* flasher = new Flasher(handle);

    BOOTInfoData bootInfo = flasher->getBootInfo();
    
    printBootInfo(&bootInfo);
    
    int bootloaderPages = 10240 / bootInfo.page_size;
    int firmwarePages = ((bootInfo.flash_size * 1024) / bootInfo.page_size) - bootloaderPages;
  
    int filePages = ceil((float)fSize / (float)bootInfo.page_size);
    
    cout << "File contains " << filePages << " pages" << endl;
    
    if (filePages > firmwarePages) {
      throw Error::FlasherException(4);
    }

    flasher->unlockFlash();

    int page;
    uint32_t addr = bootInfo.app_addr;

    for(page = 0; page < filePages; page++) {
      uint8_t buf[bootInfo.page_size] = { 0 };
      size_t r = 0;

      int n = bootInfo.page_size;
      if((n + file.tellg()) > fSize) {
        n = fSize - file.tellg();
      }
      r = file.read((char*)&buf[0], n).gcount();

      
      cout << (page + 1) << ": 0x" << hex << addr << " set page addr" << endl;
      
      flasher->setPageAddr(addr);
      flasher->erasePage();
      cout << (page + 1) << ": 0x" << hex << addr << " erase page" << endl;

      flasher->prepareDataSend();
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
  } catch (Error::FlasherException &e) {
    cout << "Error: " << e.getError() << endl;
  }

  usbDeInit(handle);
}

int main(int argc, char *argv[]) {
  namespace po = boost::program_options;

  string filename;
  string command;
  bool checkSig;
  
  po::options_description desc("Options"); 
  desc.add_options() 
    ("help,h", "Print help messages")
    ("version,v", "Display the version number")
    ("filename,f", po::value<string> (&filename), "Firmware file")
    ("dont-check,d", po::bool_switch(&checkSig), "Don't check the firmware signature")
    ("command", po::value<string> (&command), "Command: bootinfo | write");
      
  po::positional_options_description p; 
  p.add("command", 1); 
  
  po::variables_map vm;
  
  try {
    po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);
    po::notify(vm); 
    if (vm.count("help")) { 
      cout << "Anastasia Flasher. Usage: AnastasiaFlasher <command> [-f filename] [-v] [-d]" << endl;
      cout << desc << endl;
      return 0;
    }
    
    if (vm.count("version")) {
      cout << "Anastasia Flasher. Version: " << FLASHER_V_MAJOR << "." << FLASHER_V_MINOR << "." << FLASHER_V_FIXN << endl;
      return 0;
    }
    
    if(vm.count("command"))  { 
      if (command.compare("bootinfo") == 0) {
        getBootInfo();
      } else if (command.compare("write") == 0) {
        if (!vm.count("filename")) {
          cout << "Please specify filename to write" << endl;
          cout << desc << endl;
          return 0;
        } else {
          write(filename, checkSig);
        }
      }
      
    } else {
      cout << "Anastasia Flasher. Usage: AnastasiaFlasher <command> [-f filename] [-v] [-d]" << endl;
      cout << desc << endl; 
      return 0;
    }
    
  } catch(boost::program_options::required_option& e)  { 
    std::cerr << "ERROR: " << e.what() << std::endl << std::endl; 
    return 0; 
  } catch(boost::program_options::error& e) {
    std::cerr << "ERROR: " << e.what() << std::endl << std::endl; 
    return 0; 
  } 
 
  return 0;
}