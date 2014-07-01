/*
 XBeeSetup.h
 
 Copyright: Copyright (C) 2013,2014 Mitsuru Nakada All Rights Reserved.
 License: GNU GPL v2
 */

#ifndef __XBeeSetup_h__
#define __XBeeSetup_h__

#include "XBee.h"

class XBeeSetup {
public:
  XBeeSetup();
  ~XBeeSetup();
  
  int ReadConfFile(const char *file);
  int Initialize(const char *device);
  void Finalize();
  int FWUpdate();
  int Setup();
  void SetForceUpdate() { ForceUpdate = 1; }
  
private:
  int CheckVer();

  char Firmware[256];
  struct SetupTableSt {
    int Size;
    unsigned char Cmd[256];
  };
  SetupTableSt SetupTable[256];
  int SetupTableMax;
  
  int FWVer;
  int HWVer;
  unsigned int SerialHi;
  unsigned int SerialLo;
  char NodeID[32];
  int ForceUpdate;
  int Timeout;
  
  XBee XB;
};

#endif // __XBeeSetup_h__

