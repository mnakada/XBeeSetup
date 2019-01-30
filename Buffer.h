/*
 Buffer.h
 
 Copyright: Copyright (C) 2013 Mitsuru Nakada All Rights Reserved.
 License: GNU GPL v2
 */

#ifndef __Buffer_h__
#define __Buffer_h__

#include <stdio.h>
#include "Types.h"

class Buffer {

public:
  Buffer();
  int PutBuffer(const byte d);
  int AddBuffer(const byte *buf, int size);
  int GetBuffer();
  int PeekBuffer(int offset = 0);
  int GetLen();
  void Clean();

private:
  static const int BufferSize = (1 << 12);
  static const int BufferMask = BufferSize - 1;

  int WritePoint;
  int ReadPoint;
  byte Buf[BufferSize];
};

#endif // __Buffer_h__

