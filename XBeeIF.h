/*
 XBeeIF.h
 
 Copyright: Copyright (C) 2013-2019 Mitsuru Nakada All Rights Reserved.
 License: GNU GPL v2
 */

#ifndef __XBeeIF_h__
#define __XBeeIF_h__

#include "Error.h"
#include "Types.h"
#include "Buffer.h"
#include "CRC.h"

class XBeeIF : private CRC {
public:
  XBeeIF();
  ~XBeeIF();

  struct DevInfo;
  struct SetupTableSt;

  int Initialize(const char *uart);
  void Finalize();
  
  int Send(const byte *buf, int size);
  int Receive(byte *buf, int bufSize, int useTimeout = 0);

  int CheckUpdate(word hwver, word bootver, word swver);
  int Update(const char *file, word hwver, word bootver, word swver, int force);
  int SendFirmware(const char *file, const char *name, int force);

  int GetDeviceInfo(DevInfo *devInfo);

  int GetBaudRate() { return BaudRate; };
  int GetMode() { return Mode; };

  void EnableLog() {LogEnable = 1;};
  void DisableLog() {LogEnable = 0;};
  void SetTimeout(int t) {Timeout = t;};
  int SetupCommands(SetupTableSt *setupTable, int setupTableNum);

  int SendText(const char *buf);
  int ReceiveText(char *buf, int size);
  int SetBaudRate(int bps);
  int Reset(const char *device);
  
  enum Mode {
    Mode_Unknown = 0,
    Mode_AT,
    Mode_API,
    Mode_API2,
    Mode_Boot,
  };

  enum Type {
    Type_Coordinator = 0,
    Type_Router,
    Type_Endpoint,
  };

  struct DevInfo {
    word FWVer;
    word HWVer;
    word BootVer;
    longword SerialHi;
    longword SerialLo;
    byte PANID[8];
    char NodeID[32];
  };

  struct SetupTableSt {
    int Size;
    byte Cmd[256];
  };

private:
  int SendATCommand(longword addrL, const char *cmd, const byte *data = NULL, int size = 0, byte *retBuf = NULL, int retSize = 0);

  void AddBufferWithEsc(unsigned char *buf, int *p, unsigned char d);
  int GetByte();
  int ReadDeviceInfo();

  int SearchBaudrate();
  int CheckMode();
  int CheckBootMode();
  int EnterBootMode();
  int LeaveBootMode();


  static const unsigned int ADDRH = 0x0013a200;
  int FrameID;
  
  int Fd;
  int BaudRate;
  int Mode;
  int Timeout;
  int LogEnable;
  unsigned char EscapedFlag;
  int ReceiveSeq;
  int ReceivePtr;
  int ReceiveLength;
  byte ReceiveSum;
  Buffer InputBuffer;
  char BootType[16];
  int Type;

  DevInfo DeviceInfo;
  char InstallPath[256];

  static const unsigned char SOH = 0x01;
  static const unsigned char STX = 0x02;
  static const unsigned char EOT = 0x04;
  static const unsigned char ACK = 0x06;
  static const unsigned char NAK = 0x15;
  static const unsigned char CAN = 0x18;
  static const unsigned char NAKC = 0x43;

  struct BaudRateTableSt {
    int  BaudRate;
    int  BPS;
  };
  static const BaudRateTableSt BaudRateTable[];

  struct SetupSt {
    word HWVer;
    word HWMask;
    word SWVer;
    word BootVer;
    int  Type; // 0: coordinator / 1: router
    const char *Firmware;
    const char *BootFile;
    const char *BootType;
    const char **Commands;
  };
  static const SetupSt Setup[];
};

#endif // __XBeeIF_h__

