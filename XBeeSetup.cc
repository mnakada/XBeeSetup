/*
 XBeeSetup.cc
 
 Copyright: Copyright (C) 2013,2014 Mitsuru Nakada All Rights Reserved.
 License: GNU GPL v2
 */

#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "XBeeSetup.h"
#include "Error.h"

XBeeSetup::XBeeSetup() {

  Firmware[0] = 0;
  SetupTableMax = 0;
  HWVer = 0;
  FWVer = 0;
  ForceUpdate = 0;
}

XBeeSetup::~XBeeSetup() {

}

int XBeeSetup::Initialize(const char *device) {

  if(int error = XB.Initialize(device)) return error;
  
  int mode = XB.GetMode();
  if(mode == Mode_Boot) {
    fprintf(stderr, "Boot mode\n");
    return Success;
  }
  if(mode == Mode_AT) {
    char buf[256];
    fprintf(stderr, "AT mode\n");
    XB.SendText("ATVR\r");
    int size = XB.ReceiveText(buf, 256);
    if(size < 0) return size;
    FWVer = strtoul(buf, NULL, 16);
    XB.SendText("ATHV\r");
    size = XB.ReceiveText(buf, 256);
    if(size < 0) return size;
    HWVer = strtoul(buf, NULL, 16);
    XB.SendText("ATSH\r");
    size = XB.ReceiveText(buf, 256);
    if(size < 0) return size;
    SerialHi = strtoul(buf, NULL, 16);
    XB.SendText("ATSL\r");
    size = XB.ReceiveText(buf, 256);
    if(size < 0) return size;
    SerialLo = strtoul(buf, NULL, 16);
    XB.SendText("ATNI\r");
    size = XB.ReceiveText(buf, 256);
    if(size < 0) return size;
    strcpy(NodeID, buf);
  }
  if((mode == Mode_API) || (mode == Mode_API2)) {
    if(mode == Mode_API) fprintf(stderr, "API mode\n");
    if(mode == Mode_API2) fprintf(stderr, "API2 mode\n");
    unsigned char res[256];
    int size = XB.SendATCommand(0, "VR", NULL, 0, res, 256);
    if(size < 0) return size;
    FWVer = (res[1] << 8) | res[2];
    size = XB.SendATCommand(0, "HV", NULL, 0, res, 256);
    if(size < 0) return size;
    HWVer = (res[1] << 8) | res[2];
    size = XB.SendATCommand(0, "SH", NULL, 0, res, 256);
    if(size < 0) return size;
    SerialHi = (res[1] << 24) | (res[2] << 16) | (res[3] << 8) | res[4];
    size = XB.SendATCommand(0, "SL", NULL, 0, res, 256);
    if(size < 0) return size;
    SerialLo = (res[1] << 24) | (res[2] << 16) | (res[3] << 8) | res[4];
    size = XB.SendATCommand(0, "NI", NULL, 0, res, 256);
    if(size < 0) return size;
    res[size] = 0;
    strcpy(NodeID, (char *)res + 1);
  }
  
  fprintf(stderr, "Baudrate : %d\n", XB.GetBaudRate());
  fprintf(stderr, "FirmwareVersion : %04x\n", FWVer);
  fprintf(stderr, "HardwareVersion : %04x\n", HWVer);
  fprintf(stderr, "SerialNumber : %08x-%08x\n", SerialHi, SerialLo);
  fprintf(stderr, "NodeIdentifier : %s\n", NodeID);
  return Success;
}

void XBeeSetup::Finalize() {

  XB.Finalize();
}

int XBeeSetup::CheckVer() {

  if(XB.GetMode() == Mode_Boot) return Success;
  
  if(((HWVer & 0xff00) != 0x1900) && ((HWVer & 0xff00) != 0x1A00) && ((HWVer & 0xff00) != 0x2e00)) {
    fprintf(stderr, "Device Error : HWVer %04x != 0x19xx/0x1axx\n", HWVer);
    return Error;
  }
  char buf[256];
  strcpy(buf, Firmware);
  char *p = strtok(buf, "_.");
  p = strtok(NULL, "_.");
  unsigned int fw = strtoul(p, NULL, 16);
  if(!ForceUpdate && (fw == FWVer)) {
    fprintf(stderr, "The firmware has been updated already.\n");
    return NoExec;
  }
  fprintf(stderr, "DeviceFW : %04x updateFW : %04x\n", FWVer, fw);
  return Success;
}

int XBeeSetup::FWUpdate() {

  if(int error = CheckVer()) return error;
  
  if(int error = XB.EnterBootMode()) return error;
  if(int error = XB.SendFirmware(Firmware)) return error;
  if(int error = XB.LeaveBootMode()) return error;
  fprintf(stderr, "\r\n\n");
  return Success;
}

int XBeeSetup::Setup() {
  
  int bdFlag = 0;
  for(int i = 0; i < SetupTableMax; i++) {
    char buf[256];
    sprintf(buf, "AT%c%c: ", SetupTable[i].Cmd[0], SetupTable[i].Cmd[1]);
    for(int j = 2; j < SetupTable[i].Size; j++) {
      sprintf(buf + 5 + (j - 2) * 3, "%02x ", SetupTable[i].Cmd[j]);
    }
    buf[5 + (SetupTable[i].Size - 2) * 3 - 1] = 0;
    fprintf(stderr, "%s", buf+2);
    buf[4] = ' ';
    buf[5 + (SetupTable[i].Size - 2) * 3 - 1] = 0x0d;
    buf[5 + (SetupTable[i].Size - 2) * 3] = 0x0a;
    buf[5 + (SetupTable[i].Size - 2) * 3 + 1] = 0;
    if(!strncmp((const char *)SetupTable[i].Cmd, "BD", 2)) {
      if(SetupTable[i].Cmd[2] > 7) {
        fprintf(stderr, " -> BaudRate Error %d\n", SetupTable[i].Cmd[2]);
        return Error;
      }
      bdFlag = ((SetupTable[i].Cmd[2] < 6) ? 1200 : 900) << SetupTable[i].Cmd[2];
    }
    int mode = XB.GetMode();
    if(mode == Mode_AT) {
      XB.SendText(buf);
      int size = XB.ReceiveText(buf, 256);
      if(size < 0) {
        fprintf(stderr, "-> Timeout Error\n");
        return size;
      }
      fprintf(stderr, " -> %s\n", buf);
      if(bdFlag && !strncmp((const char *)SetupTable[i].Cmd, "AC", 2) && !strcmp(buf, "OK")) {
        XB.SetBaudRate(bdFlag);
      }
    } else if((mode == Mode_API) || (mode == Mode_API2)) {
      char cmd[256];
      unsigned char res[256];
      cmd[0] = SetupTable[i].Cmd[0];
      cmd[1] = SetupTable[i].Cmd[1];
      cmd[2] = 0;
      int size = XB.SendATCommand(0, cmd, SetupTable[i].Cmd + 2, SetupTable[i].Size - 2, res, 256);
      if(size < 0) {
        fprintf(stderr, " -> Timeout Error\n");
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
        return Error;
      } else {
        fprintf(stderr, " -> OK\n");
        if(bdFlag) XB.SetBaudRate(bdFlag);
      }
    }
  }
  return Success;
}

