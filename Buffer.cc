/*
 Buffer.cc
 
 Copyright: Copyright (C) 2013 Mitsuru Nakada All Rights Reserved.
 License: GNU GPL v2
 */

#include "Buffer.h"

Buffer::Buffer() {

  Clean();
}

int Buffer::PutBuffer(const byte d) {

  int nextp = (WritePoint + 1) & BufferMask;
  if(nextp == ReadPoint) return -1;
  Buf[WritePoint] = d;
  WritePoint = nextp;
  return 0;
}

int Buffer::AddBuffer(const byte *buf, int size) {

  for(int i = 0; i < size; i++) {
    if(PutBuffer(buf[i])) return -1;
  }
  return 0;
}

int Buffer::GetBuffer() {

  if(ReadPoint == WritePoint) return -1;
  int d = Buf[ReadPoint];
  ReadPoint = (ReadPoint + 1) & BufferMask;
  return d;
}

int Buffer::PeekBuffer(int offset) {

  if(offset >= GetLen()) return -1;
  return Buf[(ReadPoint + offset) & BufferMask];
}

int Buffer::GetLen() {

  return (WritePoint + BufferSize - ReadPoint) & BufferMask;
}

void Buffer::Clean() {

  ReadPoint = 0;
  WritePoint = 0;
}
  
