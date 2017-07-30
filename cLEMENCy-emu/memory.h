//
//  memory.h
//  cLEMENCy memory
//
//  Created by Lightning on 2015/11/21.
//  Copyright (c) 2017 Lightning. All rights reserved.
//

#ifndef __CLEMENCY__memory__
#define __CLEMENCY__memory__

#define MAX_MEMORY (1<<27)
#define MEMORY_MASK (MAX_MEMORY-1)
#define MEMORY_PAGE_SHIFT 10
#define MEMORY_PAGE_SIZE (1 << MEMORY_PAGE_SHIFT)
#define IO_MEMORY_MASK 0x4000000

#define MEMORY_PROTECTION_NONE 0
#define MEMORY_PROTECTION_READ 1
#define MEMORY_PROTECTION_READWRITE 2
#define MEMORY_PROTECTION_EXECUTE 3

//yes, all of this could have been in one array but it is rare that we access multiple permissions
extern unsigned long _MemoryReadPermission[];
extern unsigned long _MemoryWritePermission[];
extern unsigned long _MemoryExecutePermission[];

void InitMemory();
int LoadFile(char *Filename, unsigned int MaxSize);

#define Read9(Location) _Read9(Location, 1)
#define Read18(Location) _Read18(Location, 1)
#define Read27(Location) _Read27(Location, 1)

unsigned short _Read9(unsigned int Location, int DoCheck);
unsigned int _Read18(unsigned int Location, int DoCheck);
unsigned int _Read27(unsigned int Location, int DoCheck);
unsigned int ReadForExecute27(unsigned int Location);

#define Write9(Location, Val) _Write9(Location, Val, 1)
#define Write18(Location, Val) _Write18(Location, Val, 1)
#define Write27(Location, Val) _Write27(Location, Val, 1)

void _Write9(unsigned int Location, unsigned short Val, int DoCheck);
void _Write18(unsigned int Location, unsigned int Val, int DoCheck);
void _Write27(unsigned int Location, unsigned int Val, int DoCheck);

#define GetMemoryPageNum(MemAddress) (((MemAddress) & MEMORY_MASK) >> MEMORY_PAGE_SHIFT)
#define MemoryProtectBits(MemAddress) (GetMemoryPageNum(MemAddress) >> 6)	//divide by 64 due to being a long
#define GetMemoryProtectBit(MemType, MemAddress)	(_Memory ## MemType ## Permission[MemoryProtectBits(MemAddress)] & ((unsigned long)1 << (GetMemoryPageNum(MemAddress) & 0x3f)))
#define ClearMemoryProtectBit(MemType, MemAddress)	_Memory ## MemType ## Permission[MemoryProtectBits(MemAddress)] &= (~((unsigned long)1 << (GetMemoryPageNum(MemAddress) & 0x3f)))
#define SetMemoryProtectBit(MemType, MemAddress)	_Memory ## MemType ## Permission[MemoryProtectBits(MemAddress)] |= ((unsigned long)1 << (GetMemoryPageNum(MemAddress) & 0x3f))

unsigned int ReadMemoryProtection(unsigned int Location);
void SetMemoryProtection(unsigned int Location, unsigned int Protection);

int Convert_8_to_9(unsigned short *OutData, unsigned char *InData, unsigned int DataLen);
int Convert_9_to_8(unsigned char *OutData, unsigned short *InData, unsigned int MaxOutputLen, unsigned int *InputSize);

unsigned int Memory_MemCpy(unsigned int OutLocation, unsigned int InLocation, unsigned int Size);

#endif
