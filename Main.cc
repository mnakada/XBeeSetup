/*
 XBeeSetup Main.cc
 
 Copyright: Copyright (C) 2013,2014 Mitsuru Nakada All Rights Reserved.
 License: GNU GPL v2
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <ctype.h>
#include <sys/types.h>
#include <unistd.h>

#include "XBeeIF.h"
#include "Error.h"

int main(int argc, char **argv) {

  int error = 0;
  int config = 0;
  int device = 0;
  int debug = 0;
  int reset = 0;
  int forceUpdate = 0;
  XBeeIF XB;
  int setupCmds[16];
  int setupCmdNum = 0;

  for(int i = 1; i < argc; i++) {
    if(!strcmp(argv[i], "-f")) {
      forceUpdate = 1;
      continue;
    }
    if(!strcmp(argv[i], "-c")) {
      i++;
      config = i;
      continue;
    }
    if(!strcmp(argv[i], "-d")) {
      i++;
      device = i;
      continue;
    }
    if(!strcmp(argv[i], "-r")) {
      reset = 1;
      continue;
    }
    if(!strcmp(argv[i], "-s")) {
      i++;
      setupCmds[setupCmdNum++] = i;
      continue;
    }
    if(!strcmp(argv[i], "-debug")) {
      XB.EnableLog();
      continue;
    }
    error = Error;
  }

  if(error || !device) {
    fprintf(stderr, "usage : %s [-f] [-c <config file>] -d <device file> -s <setup1> -s <setup2> ...\n", argv[0]);
    fprintf(stderr, "   -f : force update\n");
    fprintf(stderr, "   -s : setup config (It's inserted before the setup item in the config file.)\n");
    return Error;
  }

  if(reset) return XB.Reset(argv[device]);

  int fd = XB.Initialize(argv[device]);
  if(fd < 0) {
    fprintf(stderr, "device open error : %s\n", argv[device]);
    return fd;
  }

  XBeeIF::DevInfo devInfo;
  error = XB.GetDeviceInfo(&devInfo);
  if(!error) {
    fprintf(stderr, "Baudrate : %d\n", XB.GetBaudRate());
    fprintf(stderr, "FirmwareVersion : %04x\n", devInfo.FWVer);
    fprintf(stderr, "HardwareVersion : %04x\n", devInfo.HWVer);
    fprintf(stderr, "BootVersion     : %04x\n", devInfo.BootVer);
    fprintf(stderr, "SerialNumber    : %08x-%08x\n", devInfo.SerialHi, devInfo.SerialLo);
    fprintf(stderr, "NodeIdentifier  : %s\n", devInfo.NodeID);
  }

  if(config || setupCmdNum) {
    XBeeIF::SetupTableSt setupTable[256];
    int setupTableSize = 0;
    char buf[256];
    for(int i = 0; i < setupCmdNum; i++) {
      if(setupTableSize > 255) {
        error = 1;
        break;
      }
      strcpy(buf, argv[setupCmds[i]]);
      char *p = strtok(buf, ": \t\r\n");
      if(!p) {
        fprintf(stderr, "setup parapeter error\n");
        return Error;
      }
      setupTable[setupTableSize].Cmd[0] = (unsigned char)toupper(p[0]);
      setupTable[setupTableSize].Cmd[1] = (unsigned char)toupper(p[1]);
      int c;
      for(c = 2; c < 256; c++) {
        p = strtok(NULL, ": \t\r\n");
        if(!p) break;
        setupTable[setupTableSize].Cmd[c] = strtoul(p, NULL, 16);
      }
      setupTable[setupTableSize++].Size = c;
    }
    char firmware[256] = "";
    FILE *fp = fopen(argv[config], "r");
    if(fp == NULL) return Error;
    int mode = -1;
    int line = 0;
    while(!feof(fp)) {
      char *q = fgets(buf, 255, fp);
      if(q == NULL) break;
      line++;
      char *p = strchr(buf, '#');
      if(p) *p = 0;
      p = strtok(buf, ": \t\r\n");
      if(!p) continue;

      if(!strcmp(p, "firmware")) {
        p = strtok(NULL, ": \t\r\n");
        if(p) {
          strncpy(firmware, p, 255);
          firmware[255] = 0;
          fprintf(stderr, "Firmware : %s\n", firmware);
        }
        continue;
      }

      if(!strcmp(p, "setup")) {
        mode = 1;
        continue;
      }

      if(mode == 1) { // setup:
        if(setupTableSize > 255) {
          error = 1;
          break;
        }
        setupTable[setupTableSize].Cmd[0] = (unsigned char)toupper(p[0]);
        setupTable[setupTableSize].Cmd[1] = (unsigned char)toupper(p[1]);
        int c;
        for(c = 2; c < 256; c++) {
          p = strtok(NULL, ": \t\r\n");
          if(!p) break;
          setupTable[setupTableSize].Cmd[c] = strtoul(p, NULL, 16);
        }
        setupTable[setupTableSize++].Size = c;
      }
    }
    fclose(fp);

    if(error) {
      fprintf(stderr, "Error : Config format error\n ---> line%d,mode%d: %s\n", line, mode, buf);
      return Error;
    }
    fprintf(stderr, "Read Config Done.\n");
    if(setupTableSize < 0) goto error;

    char path[256];
    strcpy(path, firmware);
    char *name = basename(path);
    char *dir = dirname(path);
    error = NoExec;
    if(!strcmp(name, "*")) {
      error = XB.Update(dir, devInfo.HWVer, devInfo.BootVer, devInfo.FWVer, forceUpdate);
    } else if(strlen(firmware)) {
      error = XB.SendFirmware(firmware, "Main", forceUpdate);
    }

    if(!error) {
      XB.Finalize();
      fd = XB.Initialize(argv[device]);
      if(fd < 0) error = Error;
    }
    if(error >= 0) {
      error = XB.SetupCommands(setupTable, setupTableSize);
      if(!error) fprintf(stderr, "Complete.\n");
    }
  }

error:
  return error;
}
