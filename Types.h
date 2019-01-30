/*
 Types.h

 Copyright: Copyright (C) 2013,2014 Mitsuru Nakada All Rights Reserved.
 License: GNU GPL v2
 */


#ifndef __Types_h__
#define __Types_h__

typedef unsigned char   byte;
typedef unsigned short  word;
typedef unsigned int   longword;

#ifndef NULL
#define NULL    0
#endif

static word SwapWord(word d) { return ((d >> 8) & 0x00ff) | ((d << 8) & 0xff00);}
static longword SwapLongword(longword d) { return ((d >> 24) & 0x000000ff) | ((d >> 8) & 0x0000ff00) | ((d << 8) & 0x00ff0000) | ((d << 24) & 0xff000000);}

#ifdef __BIG_ENDIAN__
#  define CpuToBeWord(d)	(d)
#  define CpuToBeLongword(d)	(d)
#  define BeToCpuWord(d)	(d)
#  define BeToCpuLongword(d)	(d)
#  define CpuToLeWord(d)	SwapWord(d)
#  define CpuToLeLongword(d)	SwapLongword(d)
#  define LeToCpuWord(d)	SwapWord(d)
#  define LeToCpuLongword(d)	SwapLongword(d)
#else
#  define CpuToBeWord(d)	SwapWord(d)
#  define CpuToBeLongword(d)	SwapLongword(d)
#  define BeToCpuWord(d)	SwapWord(d)
#  define BeToCpuLongword(d)	SwapLongword(d)
#  define CpuToLeWord(d)	(d)
#  define CpuToLeLongword(d)	(d)
#  define LeToCpuWord(d)	(d)
#  define LeToCpuLongword(d)	(d)
#endif

#endif // __Types_h__
