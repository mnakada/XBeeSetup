/*
 XBeeSetup Main.cc
 
 Copyright: Copyright (C) 2013,2014 Mitsuru Nakada All Rights Reserved.
 License: GNU GPL v2
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "XBeeSetup.h"
#include "Error.h"

int main(int argc, char **argv) {

  int error = 0;
  int config = 0;
  int device = 0;
  int debug = 0;
  XBeeSetup XBeeSetup;
  for(int i = 1; i < argc; i++) {
    if(!strcmp(argv[i], "-f")) {
      XBeeSetup.SetForceUpdate();
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
    if(!strcmp(argv[i], "-debug")) {
      debug = 1;
      continue;
    }
    error = Error;
  }

  if(error || !config || !device) {
    fprintf(stderr, "usage : %s [-f] -c <config file> -d <device file>\n", argv[0]);
    return Error;
  }

  if(debug) XBeeSetup.EnableLog();
  
  error = XBeeSetup.ReadConfFile(argv[config]);
  if(error < 0) return error;

  error = XBeeSetup.Initialize(argv[device]);
  if(error < 0) return error;
  
  error = XBeeSetup.FWUpdate();
  if(error < 0) {
    XBeeSetup.Finalize();
    return error;
  }
  
  if(!error) XBeeSetup.Initialize(argv[device]);
  if(error < 0) {
    XBeeSetup.Finalize();
    return error;
  }
 
  error = XBeeSetup.Setup();
  XBeeSetup.Finalize();
  if(error < 0) return error;
  
  if(!error) fprintf(stderr, "Complete.\n");
  return Success;
}
