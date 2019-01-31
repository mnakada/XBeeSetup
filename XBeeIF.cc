/*
 XBeeIF.cc

 Copyright: Copyright (C) 2013-2019 Mitsuru Nakada All Rights Reserved.
 License: GNU GPL v2
*/

#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <termios.h>
#include <libgen.h>
#include <sys/time.h>
#include <sys/stat.h>

#include "XBeeIF.h"
#include "Error.h"

const XBeeIF::BaudRateTableSt XBeeIF::BaudRateTable[] = {
  { B9600,   9600 },
  { B19200,  19200 },
  { B38400,  38400 },
  { B57600,  57600 },
  { B115200, 115200 },
};

const XBeeIF::SetupSt XBeeIF::Setup[] = {
  {
    0x1800, 0xf800,
    0x21a7, 0x0000,
    XBeeIF::Type_Coordinator,
    "XB24-ZB_21A7.ebl",
    NULL,
    "EM250",
    NULL,
  },
  {
    0x2000, 0xf000,
    0x4060, 0x0000,
    XBeeIF::Type_Coordinator,
    "xb24c-s2c_4060.ebl",
    NULL,
    "EM357",
    NULL,
  },
  {
    0x4000, 0xfc00,
    0x1005, 0x0166,
    XBeeIF::Type_Coordinator,
    "XB3-24Z_1005.gbl",
    "xb3-boot-rf_1.6.6.gbl",
    "Gecko",
    NULL,
  },
  {
    0x1800, 0xf800,
    0x23a7, 0x0000,
    XBeeIF::Type_Router,
    "XB24-ZB_23A7.ebl",
    NULL,
    "EM250",
    NULL,
  },
  {
    0x2000, 0xf000,
    0x4060, 0x0000,
    XBeeIF::Type_Router,
    "xb24c-s2c_4060.ebl",
    NULL,
    "EM357",
    NULL,
  },
  {
    0x4000, 0xfc00,
    0x1005, 0x0166,
    XBeeIF::Type_Router,
    "XB3-24Z_1005.gbl",
    "xb3-boot-rf_1.6.6.gbl",
    "Gecko",
    NULL,
  },
  {
    0x1800, 0xf800,
    0x29a7, 0x0000,
    XBeeIF::Type_Endpoint,
    "XB24-ZB_23A7.ebl",
    NULL,
    "EM250",
    NULL,
  },
  {
    0x2000, 0xf000,
    0x4060, 0x0000,
    XBeeIF::Type_Endpoint,
    "xb24c-s2c_4060.ebl",
    NULL,
    "EM357",
    NULL,
  },
  {
    0x4000, 0xfc00,
    0x1005, 0x0166,
    XBeeIF::Type_Endpoint,
    "XB3-24Z_1005.gbl",
    "xb3-boot-rf_1.6.6.gbl",
    "Gecko",
    NULL,
  },
};

XBeeIF::XBeeIF() {

  Fd = -1;
  BaudRate = 0;
  Mode = Mode_Unknown;
  Timeout = 100;
  LogEnable = 0;
  EscapedFlag = 0;
}

XBeeIF::~XBeeIF() {

  Finalize();
}

int XBeeIF::Initialize(const char *device) {

  if(LogEnable) fprintf(stderr, "Initialize %s\n", device);

  FrameID = 1;
  Mode = Mode_API2;
  Timeout = 200;
  Fd = open(device, O_RDWR | O_NONBLOCK | O_NOCTTY);
  if(Fd < 0) return Error;
  int error;
  error = SearchBaudrate();
  if(Mode == Mode_Boot) {
    LeaveBootMode();
    SearchBaudrate();
  }
  if(error) {
    Finalize();
    return error;
  }
  return Fd;
}

int XBeeIF::SearchBaudrate() {

  if(LogEnable) fprintf(stderr, "SearchBaudrate\n");

  int error;
  BaudRate = 9600;
  if(SetBaudRate(BaudRate)) return Error;
  error = CheckMode();
  if(error) {
    BaudRate = 115200;
    if(SetBaudRate(BaudRate)) return Error;
    error = CheckMode();
  }
  if(error) {
    error = CheckBootMode();
    if(!error) Mode = Mode_Boot;
  }
  if(error) {
    BaudRate = 38400;
    if(SetBaudRate(BaudRate)) return Error;
    error = CheckMode();
  }
  if(error) {
    BaudRate = 19200;
    if(SetBaudRate(BaudRate)) return Error;
    error = CheckMode();
  }
  if(error) {
    BaudRate = 57600;
    if(SetBaudRate(BaudRate)) return Error;
    error = CheckMode();
  }
  if(error) return error;
  if(Mode == Mode_AT) fprintf(stderr, "AT mode\n");
  if(Mode == Mode_API) fprintf(stderr, "API mode\n");
  if(Mode == Mode_API2) fprintf(stderr, "API2 mode\n");
  if(Mode == Mode_Boot) fprintf(stderr, "Boot mode\n");
  return Success;
}

void XBeeIF::Finalize() {

  if(LogEnable) fprintf(stderr, "Finalize\n");

  if(Fd >= 0) {
    close(Fd);
    Fd = Error;
  }
}

int XBeeIF::SetBaudRate(int bps) {

  int baudrate = 0;
  for(int i = 0; i < sizeof(BaudRateTable) / sizeof(BaudRateTableSt); i++) {
    if(bps == BaudRateTable[i].BPS) baudrate = BaudRateTable[i].BaudRate;
  }
  if(!baudrate) return Error;

  if(LogEnable) fprintf(stderr, "SetBaudRate %d\n", bps);

  struct termios current_termios;
  tcgetattr(Fd, &current_termios);
  cfmakeraw(&current_termios);
  cfsetspeed(&current_termios, baudrate);
  current_termios.c_cflag &= ~(PARENB | PARODD);
  current_termios.c_cflag = (current_termios.c_cflag & ~CSIZE) | CS8;
  current_termios.c_cflag &= ~(CRTSCTS);
  current_termios.c_iflag &= ~(IXON | IXOFF | IXANY);
  current_termios.c_cflag &= ~(HUPCL);
  if(int error = tcsetattr(Fd, TCSAFLUSH, &current_termios)) {
    fprintf(stderr, "tcsetattr error %d\n", baudrate);
    close(Fd);
    Fd = Error;
    return Error;
  }
  return Success;
}

int XBeeIF::CheckMode() {

  fprintf(stderr, ".");
  int lastTimeout = Timeout;
  Timeout = 200;
  unsigned char retBuf[16];

  Mode = Mode_API;
  int size = SendATCommand(0, "AP", NULL, 0, retBuf, 16);
  if(size == 2) {
    if(retBuf[1] == 2) Mode = Mode_API2;
    Timeout = lastTimeout;
    return Success;
  }

  Mode = Mode_API2;
  size = SendATCommand(0, "AP", NULL, 0, retBuf, 16);
  if(size == 2) {
    if(retBuf[1] == 1) Mode = Mode_API;
    Timeout = lastTimeout;
    return Success;
  }

  Mode = Mode_Unknown;
  char buf[256];
  Timeout = 1500;
  SendText("\r");
  size = ReceiveText(buf, 256);
  if(size > 0) sleep(1);
  SendText("+++");
  size = ReceiveText(buf, 256);
  Timeout = lastTimeout;
  if((size == 2) && !strcmp(buf, "OK")) { // AT mode
    SendText("ATAP\r");
    size = ReceiveText(buf, 256);
    if(size < 0) return size;
    Mode = Mode_Unknown;
    if(!strncmp(buf, "0", 1)) Mode = Mode_AT;
    if(!strncmp(buf, "1", 1)) Mode = Mode_API;
    if(!strncmp(buf, "2", 1)) Mode = Mode_API2;
    if(!strncmp(buf, "ERROR", 5)) Mode = Mode_AT;
    SendText("ATCN\r");
    size = ReceiveText(buf, 256);
    if(size < 0) return size;
    if(strcmp(buf, "OK")) return Error;
    sleep(1);
    return Success;
  }
  return Error;
}

int XBeeIF::SendText(const char *buf) {
  
  int size = strlen(buf);
  if(LogEnable) fprintf(stderr, "XBeeIF::SendText : %s\n", buf);
  int num = write(Fd, buf, size);
  if(num != size) return Error;
  return Success;
}

int XBeeIF::ReceiveText(char *buf, int size) {
  
  int p = 0;
  int d = 0;
  do {
    d = GetByte();
    if(d < 0) break;
    if((d == 0x0d) || (d == 0x0a)) break;
    buf[p++] = d;
  } while(p < size - 1);
  buf[p] = 0;
  if(LogEnable) fprintf(stderr, "XBeeIF::ReceiveText : %s\n", buf);
  if(p > 0) return p;
  return d;
}

int XBeeIF::SendATCommand(longword addrL, const char *cmd, const byte *data, int size, byte *retBuf, int retSize) {

  if(LogEnable) fprintf(stderr, "SendATCommand\n");

  unsigned char sendBuf[256];
  int cmdOffset;
  int cmpLen;
  int resOffset;
  int lastTimeout = Timeout;
  if(addrL) {
    sendBuf[0] = 0x17;
    sendBuf[2] = (ADDRH >> 24) & 0xff;
    sendBuf[3] = (ADDRH >> 16) & 0xff;
    sendBuf[4] = (ADDRH >> 8) & 0xff;
    sendBuf[5] = (ADDRH) & 0xff;
    sendBuf[6] = (addrL >> 24) & 0xff;
    sendBuf[7] = (addrL >> 16) & 0xff;
    sendBuf[8] = (addrL >> 8) & 0xff;
    sendBuf[9] = (addrL) & 0xff;
    sendBuf[10] = 0xff;
    sendBuf[11] = 0xfe;
    sendBuf[12] = 0x02; // apply changes
    cmpLen = 9;
    cmdOffset = 13;
    resOffset = 14;
    Timeout = 3000;
  } else {
    sendBuf[0] = 0x08;
    cmpLen = 1;
    cmdOffset = 2;
    resOffset = 4;
  }
  sendBuf[cmdOffset] = cmd[0];
  sendBuf[cmdOffset + 1] = cmd[1];
  for(int i = 0; i < size; i++) sendBuf[cmdOffset + 2 + i] = data[i];

  int retry = 0;
  int ret = 0;
  while(retry < 3) {
    retry++;
    sendBuf[1] = FrameID;
    int error = Send(sendBuf, cmdOffset + 2 + size);
    if(error < 0) continue;
    
    unsigned char receiveBuf[256];
    struct timeval timeout;
    gettimeofday(&timeout, NULL);
    timeout.tv_sec++;
    while(1) {
      ret = error = Receive(receiveBuf, 256, 1);
      if(error < 0) break;
      if((receiveBuf[0] == (sendBuf[0] | 0x80)) && !memcmp(receiveBuf + 1, sendBuf + 1, cmpLen)) 
        break;
      struct timeval now;
      gettimeofday(&now, NULL);
      if(timercmp(&now, &timeout, >)) {
        error = Error;
        break;
      }
    }
    FrameID++;
    if(FrameID > 255) FrameID = 1;
    if(error < 0) continue;
    
    if(retSize > ret - resOffset) retSize = ret - resOffset;
    if(retSize < 0) retSize = 0;
    if(retBuf) memcpy(retBuf, receiveBuf + resOffset, retSize);
    Timeout = lastTimeout;
    return ret  - resOffset;
  }
  Timeout = lastTimeout;
  return Error;
}

int XBeeIF::Send(const byte *buf, int size) {

  if(Fd < 0) return Error;
  if(size > 255) return Error;
  
  if(LogEnable) {
    fprintf(stderr, "XBeeIF::Send : ");
    for(int i = 0; i < size; i++) fprintf(stderr, "%02x ", buf[i]);
    fprintf(stderr, "\n");
  }

  byte sendBuf[520];
  int p = 0;
  sendBuf[p++] = 0x7e;
  AddBufferWithEsc(sendBuf, &p, size >> 8);
  AddBufferWithEsc(sendBuf, &p, size);
  byte sum = 0;
  for(int i = 0; i < size; i++) {
    sum += buf[i];
    AddBufferWithEsc(sendBuf, &p, buf[i]);
  }
  AddBufferWithEsc(sendBuf, &p, 0xff - sum);
  int num = write(Fd, sendBuf, p);
  if(num != p) return Error;
  return Success;
}

void XBeeIF::AddBufferWithEsc(byte *buf, int *p, byte d) {

  if(Mode == Mode_API2) {
    if((d == 0x7e) || (d == 0x7d) || (d == 0x11) || (d == 0x13)) {
      d ^= 0x20;
      buf[(*p)++] = 0x7d;
    }
  }
  buf[(*p)++] = d;
}

int XBeeIF::Receive(byte *buf, int bufSize, int useTimeout) {

  while(1) {

    byte buf2[256];
    int size = 256;
    if(!useTimeout) {
      size = read(Fd, buf2, 256);
      if(size <= 0) break;
    }
    for(int i = 0; i < size; i++) {
      int d;
      if(!useTimeout) {
        d = buf2[i];
      } else {
        d = GetByte();
        if(d < 0) return d;
      }
      // API=2 escape char
      if(d == 0x7e) {
        ReceiveSeq = 0;
        EscapedFlag = 0;
      }
      if((Mode == Mode_API2) && (d == 0x7d)) {
        EscapedFlag = 0x20;
        continue;
      }
      d ^= EscapedFlag;
      EscapedFlag = 0;
      switch(ReceiveSeq) {
        case 0: // wait delimiter
          if(d == 0x7e) ReceiveSeq++;
          break;
        case 1: // Packet Length High
          ReceiveLength = d << 8;
          ReceiveSeq++;
          break;
        case 2: // Packet Length Low
          ReceiveLength |= d;
          ReceiveSum = 0;
          ReceivePtr = 0;
          while(InputBuffer.GetLen()) {
            InputBuffer.GetBuffer();
          }
          ReceiveSeq++;
          if(ReceiveLength > 150) ReceiveSeq = 0;
          break;
        case 3: // data
          ReceiveSum += d;
          ReceivePtr++;
          InputBuffer.PutBuffer(d);
          if(ReceivePtr >= ReceiveLength) ReceiveSeq++;
          break;
        case 4: // csum
          ReceiveSum += d;
          ReceiveSeq = 0;
          if(ReceiveSum == 0xff) {
            if(ReceiveLength > bufSize) {
              if(LogEnable) {
                fprintf(stderr, "XBeeIF::Receive Error : ");
                for(int i = 0; i < ReceiveLength; i++) fprintf(stderr, "%02x ", buf[i]);
                fprintf(stderr, "\n");
              }
              return Error;
            }
            for(int i = 0; i < ReceiveLength; i++) {
              buf[i] = InputBuffer.GetBuffer();
            }
            if(LogEnable) {
              fprintf(stderr, "XBeeIF::Receive : ");
              for(int i = 0; i < ReceiveLength; i++) fprintf(stderr, "%02x ", buf[i]);
              fprintf(stderr, "\n");
            }
            return ReceiveLength;
          }
        default:
          ReceiveSeq = 0;
          break;
      }
    }
  }
  if(LogEnable) fprintf(stderr, "XBeeIF::No Receive\n");
  return Error;
}

int XBeeIF::GetByte() {

  struct timeval now;
  struct timeval timeout;
  gettimeofday(&now, NULL);
  timeout.tv_sec = Timeout / 1000;
  timeout.tv_usec = (Timeout % 1000) * 1000;
  timeradd(&now, &timeout, &timeout);
  do {
    int c;
    int n = read(Fd, &c, 1);
    if(n > 0) return (c & 0xff);
    gettimeofday(&now, NULL);
    if(timercmp(&now, &timeout, >)) return Error;
  } while(1);
}

int XBeeIF::ReadDeviceInfo() {

  if(LogEnable) fprintf(stderr, "ReadDeviceInfo\n");

  memset(&DeviceInfo, 0, sizeof(DeviceInfo));
  if(Mode == Mode_AT) {
    char buf[256];
    int lastTimeout = Timeout;
    Timeout = 1500;
    SendText("\r");
    int size = ReceiveText(buf, 256);
    if(size > 0) sleep(1);
    SendText("+++");
    size = ReceiveText(buf, 256);
    Timeout = lastTimeout;
    if((size != 2) || strcmp(buf, "OK")) return Error;
    SendText("ATVR\r");
    size = ReceiveText(buf, 256);
    if(size < 4) return Error;
    DeviceInfo.FWVer = strtoul(buf, NULL, 16);
    SendText("ATHV\r");
    size = ReceiveText(buf, 256);
    if(size < 4) return Error;
    DeviceInfo.HWVer = strtoul(buf, NULL, 16);
    SendText("ATVH\r");
    size = ReceiveText(buf, 256);
    if((size < 3) || !strcmp(buf, "ERROR")) {
      DeviceInfo.BootVer = 0;
    } else {
      DeviceInfo.BootVer = strtoul(buf, NULL, 16);
    }
    SendText("ATSH\r");
    size = ReceiveText(buf, 256);
    if(size < 0) return Error;
    DeviceInfo.SerialHi = strtoul(buf, NULL, 16);
    SendText("ATSL\r");
    size = ReceiveText(buf, 256);
    if(size < 0) return Error;
    DeviceInfo.SerialLo = strtoul(buf, NULL, 16);
    SendText("ATID\r");
    size = ReceiveText(buf, 256);
    if(size < 0) return Error;
    if(size >= 16) {
      for(int i = 0; i < 8; i++) {
        char buf2[4];
        memcpy(buf2, buf + i * 2, 2);
        buf2[2] = 0;
        DeviceInfo.PANID[i] = strtoul(buf2, NULL, 16);
      }
    } else {
      for(int i = 0; i < 8; i++) {
        DeviceInfo.PANID[i] = 0;
      }
    }
    SendText("ATNI\r");
    size = ReceiveText(buf, 256);
    if(size < 0) return Error;
    strcpy(DeviceInfo.NodeID, buf);
    return Success;
  } else if((Mode == Mode_API) || (Mode == Mode_API2)) {
    unsigned char res[256];
    int size = SendATCommand(0, "VR", NULL, 0, res, 256);
    if(size < 3) return Error;
    DeviceInfo.FWVer = (res[1] << 8) | res[2];
    size = SendATCommand(0, "HV", NULL, 0, res, 256);
    if(size < 3) return Error;
    DeviceInfo.HWVer = (res[1] << 8) | res[2];
    size = SendATCommand(0, "VH", NULL, 0, res, 256);
    if(size < 3) {
      DeviceInfo.BootVer = 0;
    } else {
      DeviceInfo.BootVer = (res[1] << 8) | res[2];
    }
    size = SendATCommand(0, "SH", NULL, 0, res, 256);
    if(size < 5) return Error;
    DeviceInfo.SerialHi = (res[1] << 24) | (res[2] << 16) | (res[3] << 8) | res[4];
    size = SendATCommand(0, "SL", NULL, 0, res, 256);
    if(size < 5) return Error;
    DeviceInfo.SerialLo = (res[1] << 24) | (res[2] << 16) | (res[3] << 8) | res[4];
    size = SendATCommand(0, "ID", NULL, 0, res, 256);
    if(size < 9) return Error;
    memcpy(DeviceInfo.PANID, res + 1, 8);
    size = SendATCommand(0, "NI", NULL, 0, res, 256);
    if(size < 1) return Error;
    res[size] = 0;
    strcpy(DeviceInfo.NodeID, (char *)res + 1);
    return Success;
  }
  return Error;
}

int XBeeIF::GetDeviceInfo(XBeeIF::DevInfo *devInfo) {

  if(LogEnable) fprintf(stderr, "GetDeviceInfo\n");

  if(ReadDeviceInfo()) return Error;
  if(devInfo) {
    memcpy(devInfo, &DeviceInfo, sizeof(DeviceInfo));
    if(!DeviceInfo.SerialHi) return Error;
    return Success;
  }
  return Error;
}

int XBeeIF::EnterBootMode() {

  if(LogEnable) fprintf(stderr, "EnterBootMode\n");

  if(Mode == Mode_Boot) return Success;
  if(Mode == Mode_AT) {
    char buf[256];
    SendText("AT%P00002469\r");
    int size = ReceiveText(buf, 256);
    if(size < 0) return size;
  } else if((Mode == Mode_API) || (Mode == Mode_API2)) {
    unsigned char res[256];
    int size = SendATCommand(0, "%P", (byte *)"00002469", 8);
    if(size < 0) return size;
  }
  if(int error = SetBaudRate(115200)) return error;
  sleep(3);
  return Success;
}

int XBeeIF::CheckBootMode() {

  if(LogEnable) fprintf(stderr, "CheckBootMode\n");

  int lastTimeout = Timeout;
  Timeout = 100;
  int retryCount = 0;
  int blFlag = 0;
  while(GetByte() > 0);
  BootType[0] = 0;
  do {
    SendText("\r");
    do {
      char buf[256];
      int size = ReceiveText(buf, 255);
      if(size < 0) break;
      if(strstr(buf, " Bootloader")) {
        strncpy(BootType, buf, 5);
        BootType[5] = 0;
      }
      if(!strncmp(buf, "BL >", 4)) blFlag = 1;
    } while(1);
  } while(!blFlag && (retryCount++ < 4));
  if(!blFlag) return Error;
  Mode = Mode_Boot;
  if(LogEnable) fprintf(stderr, "BootMode : %s\n", BootType);
  return Success;
}

int XBeeIF::LeaveBootMode() {

  if(LogEnable) fprintf(stderr, "LeaveBootMode\n");

  if(int error = CheckBootMode()) return error;
  SendText("2\r");
  sleep(4);
  if(int error = SetBaudRate(BaudRate)) return error;
  return Success;
}

int XBeeIF::CheckUpdate(word hwver, word bootver, word swver) {

  if(LogEnable) fprintf(stderr, "CheckUpdate\n");

  int setupTable = -1;
  for(int i = 0; i < sizeof(Setup) / sizeof(SetupSt); i++) {
    if(((hwver & Setup[i].HWMask) == Setup[i].HWVer) && (Type == Setup[i].Type))  {
      setupTable = i;
      break;
    }
  }
  if(setupTable < 0) return Error;

  if((Setup[setupTable].SWVer != swver) || (Setup[setupTable].BootVer != bootver)) return 1;
  return 0;
}

int XBeeIF::Update(const char *installPath, word hwver, word bootver, word swver, int force) {

  if(LogEnable) fprintf(stderr, "Update HW:%04x BOOT:%04x FW:%04x\n", hwver, bootver, swver);

  int setupTable = -1;
  for(int i = 0; i < sizeof(Setup) / sizeof(SetupSt); i++) {
    if(((hwver & Setup[i].HWMask) == Setup[i].HWVer) && (Type == Setup[i].Type))  {
      setupTable = i;
      break;
    }
  }
  if((setupTable < 0) && (Mode == Mode_Boot)) {
    for(int i = 0; i < sizeof(Setup) / sizeof(SetupSt); i++) {
      if(!strncmp(Setup[i].BootType, BootType, 5) && (Setup[i].Type == Type))  {
        setupTable = i;
        break;
      }
    }
  }
  if(setupTable < 0) return Error;

  int lastMode = Mode;
  int modifyFlag = 0;
  if((force || (Setup[setupTable].BootVer != bootver)) && Setup[setupTable].BootFile) {
    char file[256];
    strcpy(file, installPath);
    if(file[strlen(file) - 1] != '/') strcat(file, "/");
    strcat(file, Setup[setupTable].BootFile);

    if(SendFirmware(file, "Boot", force)) return Error;
    modifyFlag = 1;
    SearchBaudrate();
  }

  if(force || (Setup[setupTable].SWVer != swver)) {
    char file[256];
    strcpy(file, installPath);
    if(file[strlen(file) - 1] != '/') strcat(file, "/");
    strcat(file, Setup[setupTable].Firmware);

    int error = SendFirmware(file, "Main", modifyFlag | force);
    if(error) return Error;
    modifyFlag = 1;
  } else if(Mode == Mode_Boot) {
    if(LeaveBootMode()) return Error;
  }
  Mode = lastMode;
  if(modifyFlag) return Success;
  return NoExec;
}

int XBeeIF::SendFirmware(const char *file, const char *name, int force) {

  if(LogEnable) fprintf(stderr, "SendFirmware %s %s\n", file, name);

  int lastTimeout = Timeout;
  int retryCount;
  int lastMode = Mode;

  struct stat st;
  int error = stat(file, &st);
  if((error < 0) || !st.st_size) {
    fprintf(stderr, "XBee %s firmware file error : %s\n", name, file);
    return Error;
  }
  int blockMax = (int)(st.st_size + 127) / 128;

  if(strcmp(name, "Boot")) {
    char path[256];
    strcpy(path, file);
    char *p = strtok(basename(path), "_.");
    p = strtok(NULL, "_.");
    if(!p) {
      fprintf(stderr, "firmware filename error\n");
      return Error;
    }
    unsigned int fw = strtoul(p, NULL, 16);
    if(!force && (fw == DeviceInfo.FWVer)) {
      fprintf(stderr, "The firmware has been updated already.\n");
      return NoExec;
    }
    fprintf(stderr, "DeviceFW : %04x updateFW : %04x\n", DeviceInfo.FWVer, fw);
  }

  FILE *fp = fopen(file, "r");
  if(fp == NULL) return Error;

  if(int error = EnterBootMode()) goto error;
  if(int error = CheckBootMode()) goto error;
  Mode = Mode_Boot;

  Timeout = 100;
  SendText("1");
  while(GetByte() > 0);

  retryCount = 0;
  do {
    int d = GetByte();
    if(d == NAKC) break;
    if(retryCount++ > 32) goto error;
  } while(1);

  Timeout = 1000;
  retryCount = 0;
  unsigned char buf[256];
  for(int blockNum = 1; blockNum <= blockMax;) {
    static int lastMsgBlk = -1;
    if((blockNum * 20 / blockMax) != lastMsgBlk) {
      fprintf(stderr, "XBee %s update : %d / %d\r", name, blockNum, blockMax);
      lastMsgBlk = (blockNum * 20 / blockMax);
    }

    buf[0] = SOH;
    buf[1] = blockNum;
    buf[2] = (blockNum ^ 0xff) & 0xff;
    int size = fread(buf + 3, 1, 128, fp);
    if(size == 0) break;
    if(size < 128) {
      for(int i = size; i < 128; i++) buf[3 + i] = 0;
    }
    unsigned short crc = CalcCRC(buf + 3, 128, 0);
    buf[131] = crc >> 8;
    buf[132] = crc;
    int num = write(Fd, buf, 133);
    if(num != 133) goto error;

    int d = GetByte();
    if(d == ACK) {
      blockNum++;
      retryCount = 0;
    } else if(d == CAN) {
      fprintf(stderr, "\nCAN accept error\n");
      goto error;
    } else {
      while(GetByte() >= 0);
      fseek(fp, (blockNum - 1) * 128, SEEK_SET);
      fprintf(stderr, "\n %02x retry\n", d);
      if(retryCount++ > 5) {
        buf[0] = CAN;
        write(Fd, buf, 1);
        fprintf(stderr, "\nNAK %02x retry error\n", d);
        goto error;
      }
    }
  }
  fprintf(stderr, "\n");
  buf[0] = EOT;
  write(Fd, buf, 1);

error:
  if(Mode == Mode_Boot) error |= LeaveBootMode();
  Mode = lastMode;
  Timeout = lastTimeout;
  fclose(fp);
  return error;
}

int XBeeIF::SetupCommands(SetupTableSt *setupTable, int setupTableNum) {

  if(LogEnable) fprintf(stderr, "SetupCommands\n");

  int lastTimeout = Timeout;
  Timeout = 5000;
  int bdFlag = 0;
  int apFlag = 0;
  if(Mode == Mode_AT) {
    char buf[256];
    SendText("+++");
    int size = ReceiveText(buf, 256);
    if((size != 2) || strcmp(buf, "OK")) return Error;
  }

  for(int i = 0; i < setupTableNum; i++) {
    if(!strncmp((char *)setupTable[i].Cmd, "BD", 2)) {
      if(setupTable[i].Cmd[2] > 7) {
        fprintf(stderr, "Error : %c%c %02x\n", setupTable[i].Cmd[0], setupTable[i].Cmd[1], setupTable[i].Cmd[2]);
        Timeout = lastTimeout;
        return Error;
      }
      bdFlag = ((setupTable[i].Cmd[2] < 6) ? 1200 : 900) << setupTable[i].Cmd[2];
    }
    if(!strncmp((char *)setupTable[i].Cmd, "AP", 2)) {
      if(setupTable[i].Size > 2) apFlag = setupTable[i].Cmd[2] + 1;
    }

    char sendBuf[256];
    sprintf(sendBuf, "AT%c%c: ", setupTable[i].Cmd[0], setupTable[i].Cmd[1]);
    for(int c = 2; c < setupTable[i].Size; c++) {
      snprintf(sendBuf + strlen(sendBuf), 255 - strlen(sendBuf), "%02X ", setupTable[i].Cmd[c]);
    }
    fprintf(stderr, "%s", sendBuf + 2);
    sendBuf[4] = ' ';
    snprintf(sendBuf + strlen(sendBuf) - 1, 255 - strlen(sendBuf) + 1, "\r\n");

    if(Mode == Mode_AT) {
      SendText(sendBuf);
      char recvBuf[256];
      int size = ReceiveText(recvBuf, 256);
      if(size < 0) {
        fprintf(stderr, "Error ReceiveText: %s %d\n", sendBuf, size);
        Timeout = lastTimeout;
        return size;
      }
      fprintf(stderr, " -> %s\n", recvBuf);
      if(bdFlag && !strncmp((char *)setupTable[i].Cmd, "AC", 2) && !strcmp(recvBuf, "OK")) {
        SetBaudRate(bdFlag);
        sleep(1);
      }
      if(apFlag && !strncmp((char *)setupTable[i].Cmd, "AC", 2) && !strcmp(recvBuf, "OK")) {
        Mode = apFlag;
      }

    } else if((Mode == Mode_API) || (Mode == Mode_API2)) {
      byte res[256];
      int size = SendATCommand(0, (char *)setupTable[i].Cmd, setupTable[i].Cmd + 2, setupTable[i].Size - 2, res, 256);
      if(size < 0) {
        fprintf(stderr, " -> Timeout Error\n");
        Timeout = lastTimeout;
        return size;
      }
      if(res[0]) {
        switch(res[0]) {
          case 1:
            fprintf(stderr, " -> error\n");
            break;
          case 2:
            fprintf(stderr, " -> Invalid Command\n");
            break;
          case 3:
            fprintf(stderr, " -> Invalid Parameter\n");
            break;
          case 4:
            fprintf(stderr, " -> Tx Failure\n");
            break;
          default:
            fprintf(stderr, " -> Unknown Error %02x\n", res[0]);
            break;
        }
        Timeout = lastTimeout;
        return Error;
      }
      fprintf(stderr, " -> OK\n");
      if(bdFlag) SetBaudRate(bdFlag);
      if(apFlag) {
        Mode = apFlag;
        if(Mode == Mode_AT) {
          Timeout = 200;
          SendText("\r");
          sleep(2);
          while(GetByte() >= 0);
          Timeout = 5000;
          char buf[256];
          SendText("+++");
          int size = ReceiveText(buf, 256);
          if((size != 2) || strcmp(buf, "OK")) return Error;
        }
      }
    } else {
      return Error;
    }
    bdFlag = 0;
    apFlag = 0;
  }
  Timeout = lastTimeout;
  return Success;
}

int XBeeIF::Reset(const char *device) {

  FrameID = 1;
  Mode = Mode_API2;
  Timeout = 200;
  Fd = open(device, O_RDWR | O_NONBLOCK | O_NOCTTY);
  if(Fd < 0) return Error;
  int error;

  BaudRate = 9600;
  if(SetBaudRate(BaudRate)) {
    fprintf(stderr, "SetBaudrate error\n");
  }

  fprintf(stderr, "Press the break button for 10seconds.\n");

  char buf[256];
  while(1) {
    int size = ReceiveText(buf, 255);
    if(size > 0) fprintf(stderr, "[%s]\n", buf);
    if(size > 0) break;
  }

  fprintf(stderr, "Release the break button and press any key.\n");
  fgetc(stdin);

  Timeout = 500;
  fprintf(stderr, "Send: ATBD 5\n");
  SendText("ATBD 5\r");
  int size = ReceiveText(buf, 255);
  if(size < 0) {
    fprintf(stderr, "-> Timeout Error\n");
    Finalize();
    return size;
  }
  fprintf(stderr, "Recv: %s\n", buf);

  fprintf(stderr, "Send: ATWR\n");
  SendText("ATWR\r");
  size = ReceiveText(buf, 255);
  if(size < 0) {
    fprintf(stderr, "-> Timeout Error\n");
    Finalize();
    return size;
  }
  fprintf(stderr, "Recv: %s\n", buf);
  Finalize();
  return Success;
}

