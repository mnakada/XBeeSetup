/*
 XBeeSetup_ReadConfig.cc
 
 Copyright: Copyright (C) 2013,2014 Mitsuru Nakada All Rights Reserved.
 License: GNU GPL v2
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "XBeeSetup.h"
#include "Error.h"

int XBeeSetup::ReadConfFile(const char *file) {

  FILE *fp = fopen(file, "r");
  if(fp == NULL) return Error;
  int mode = -1;
  int error = 0;
  char buf[256];
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
        strncpy(Firmware, p, 255);
        Firmware[255] = 0;
      }
      continue;
    }
    
    if(!strcmp(p, "setup")) {
      mode = 1;
      continue;
    }

    if(mode == 1) { // setup:
      if(SetupTableMax > 255) {
        error = 1;
        break;
      }
      SetupTable[SetupTableMax].Cmd[0] = (unsigned char)toupper(p[0]);
      SetupTable[SetupTableMax].Cmd[1] = (unsigned char)toupper(p[1]);
      int c;
      for(c = 2; c < 256; c++) {
        p = strtok(NULL, ": \t\r\n");
        if(!p) break;
        SetupTable[SetupTableMax].Cmd[c] = strtoul(p, NULL, 16);
      }
      SetupTable[SetupTableMax++].Size = c;
    }
  }
  fclose(fp);

  if(error) {
    fprintf(stderr, "Error : Config format error\n ---> line%d,mode%d: %s\n", line, mode, buf);
    return Error;
  }
  fprintf(stderr, "Read Config Done.\n");
  return Success;
}

