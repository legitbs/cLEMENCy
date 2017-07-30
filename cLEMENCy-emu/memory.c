//
//  memory.c
//  cLEMENCy memory
//
//  Created by Lightning on 2015/11/21.
//  Copyright (c) 2017 Lightning. All rights reserved.
//

#include <sys/mman.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include "memory.h"
#include "exceptions.h"
#include "io.h"
#include "debug.h"
#include "cpu.h"
#include "network.h"
#include "clemency_nfo.h"

unsigned short *MemoryBlocks[MAX_MEMORY >> 20];

//someone is going to go through this and wonder what I was thinkning when 2 bits would store all combos
//This was originally planned for doing other things with the processor and allowed flexibility and
//instead of rewriting all the code last minute, it has been left as it is fully functional and doesn't
//impact speed enough
unsigned long _MemoryReadPermission[MAX_MEMORY / MEMORY_PAGE_SIZE / 64];
unsigned long _MemoryWritePermission[MAX_MEMORY / MEMORY_PAGE_SIZE / 64];
unsigned long _MemoryExecutePermission[MAX_MEMORY / MEMORY_PAGE_SIZE / 64];

void InitMemory()
{
	int i;

	memset(_MemoryReadPermission, 0, sizeof(_MemoryReadPermission));
	memset(_MemoryWritePermission, 0, sizeof(_MemoryWritePermission));
	memset(_MemoryExecutePermission, 0, sizeof(_MemoryExecutePermission));

	memset(MemoryBlocks, 0, sizeof(MemoryBlocks));

	//first page is executable
	SetMemoryProtection(0, MEMORY_PROTECTION_EXECUTE);

	//clock io
	for(i = IO_CLOCK; i <= IO_CLOCK_END; i+= 1024)
		SetMemoryProtection(i, MEMORY_PROTECTION_READWRITE);

	//flag io
	for(i = IO_FLAG; i <= IO_FLAG_END; i+= 1024)
		SetMemoryProtection(i, MEMORY_PROTECTION_READ);

	//network receive and receive size
	for(i = IO_NETWORK_RECV; i <= IO_NETWORK_RECV_SIZE_END; i+= 1024)
		SetMemoryProtection(i, MEMORY_PROTECTION_READWRITE);

	//network send and send size
	for(i = IO_NETWORK_SEND; i <= IO_NETWORK_SEND_SIZE_END; i+= 1024)
		SetMemoryProtection(i, MEMORY_PROTECTION_READWRITE);

	//interrupts, value is too high so mask it
	for(i = IO_INTERRUPTS & 0xfffffc00; i <= IO_INTERRUPTS_END; i+= 1024)
		SetMemoryProtection(i, MEMORY_PROTECTION_READWRITE);

	//processor id - handled by interrupts so don't touch
//	for(i = IO_PROCESSOR_ID; i <= IO_PROCESSOR_ID_END; i+= 1024)
//		SetMemoryProtection(i, MEMORY_PROTECTION_READWRITE);
	SetupClemencyNFO();
}

int Convert_8_to_9(unsigned short *OutData, unsigned char *InData, unsigned int DataLen)
{
    unsigned int CurPos, CurPos9;
    short CurData;
    int BitPos;
    unsigned short *CurBlock;
    int BlockID;

    //setup the location to write to, if it is 0 then it is a special flag to write to actual memory
    CurBlock = OutData;
    BlockID = 0;
    if(!OutData)
        CurBlock = MemoryBlocks[0];

    BitPos = 1;
    for(CurPos = 0, CurPos9 = 0; (CurPos+1) < DataLen;)
    {
        //get bits
        CurData = ((unsigned short)InData[CurPos] & ((1 << (9 - BitPos)) - 1)) << BitPos;
        CurPos++;

        //get the next block of bits
        CurData |= (unsigned short)InData[CurPos] >> (8 - BitPos);

        CurBlock[CurPos9] = CurData;
        CurPos9++;

        //if we hit a megabyte alignment and outdata is 0 then advance our block pointer
        if(!OutData && (CurPos9 % (1024*1024)) == 0)
        {
            CurPos9 = 0;
            BlockID++;
            CurBlock = MemoryBlocks[BlockID];
        }

        BitPos++;
        if(BitPos == 9)
        {
            CurPos++;
            BitPos = 1;
        }
    }

    return CurPos9;
}

int Convert_9_to_8(unsigned char *OutData, unsigned short *InData, unsigned int MaxOutputLen, unsigned int *InputSize)
{
	unsigned int cur_offset_8bits = 0;
	unsigned int cur_offset_9bits = 0;
	unsigned int cur_offset_8bit_single = 1;
	short CurByte;

	//loop as long as we have input and haven't hit the maximum of output
	while(*InputSize && ((cur_offset_8bits+1) < MaxOutputLen))
	{
		OutData[cur_offset_8bits] &= (0xff << (9-cur_offset_8bit_single));
		OutData[cur_offset_8bits] |= ((*InData >> cur_offset_8bit_single) & 0xff);

		//move to the next byte
		cur_offset_8bits++;

		//mask and write the rest of the 9 bits
		CurByte = *InData & ((1 << cur_offset_8bit_single) - 1);
		OutData[cur_offset_8bits] = (CurByte << (8 - cur_offset_8bit_single)) & 0xff;

		//adjust what bit to start with
		cur_offset_8bit_single = 9 - (8 - cur_offset_8bit_single);
		if(cur_offset_8bit_single == 9)
		{
			cur_offset_8bit_single = 1;
			cur_offset_8bits++;
		}

		*InputSize = *InputSize - 1;
		InData++;
		cur_offset_9bits++;
	}

	//return number of bytes used
	return cur_offset_8bits + (cur_offset_8bit_single != 1);
}

int LoadFile(char *Filename, unsigned int MaximumSize)
{
	int fd;
	unsigned int FileSize;
	unsigned char *FileData;
	unsigned int MemID;

#ifndef NODEBUG
	printf("Loading %s\n", Filename);
#endif
	fd = open(Filename, O_RDONLY);
	if(fd < 0)
	{
		DebugOut("Error, no firmware file found");
		return -1;
	}

	FileSize = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);

	//File can only fill up half of memory as the top half is DMA
	if(((FileSize * 8 / 9) > (MAX_MEMORY >> 1)) || (MaximumSize && ((FileSize * 8 / 9) > MaximumSize)))
	{
		close(fd);
		DebugOut("Error, firmware too large");
		return -1;
	}

	//read the file data in
	FileData = malloc(FileSize);
	read(fd, FileData, FileSize);
	close(fd);

	//allocate all of the blocks for the file to be read
	//no need to allocate extra as we want to limit memory footprint
	for(MemID = 0; (MemID*(1024*1024)) < FileSize; MemID++)
	{
		//allocate
		MemoryBlocks[MemID] = mmap(0, (1024*1024) * sizeof(short), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
		if(MemoryBlocks[MemID] == MAP_FAILED)
		{
			DebugOut("Error during Memory init: mmap failed");
			exit(-1);
		}

		//erase it
		memset(MemoryBlocks[MemID], 0, 1024*1024*sizeof(short));
	}

	//now split up the data into memory itself
	Convert_8_to_9(0, FileData, FileSize);

	free(FileData);
	return 0;
}

unsigned short *FindMemBlock(unsigned int Location, unsigned int *Size)
{
	unsigned int Offset;
	unsigned int Index;
	unsigned short *NewBlock;

	//calculate the offset into the block
	Index = (Location & MEMORY_MASK) >> 20;
	Offset = Location & 0xfffff;

	//if we don't have an entry then create a new area
	if(!MemoryBlocks[Index])
	{
		//allocate and setup
		NewBlock = mmap(0, (1024*1024) * sizeof(short), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
		memset(NewBlock, 0, (1024*1024) * sizeof(short));
		MemoryBlocks[Index] = NewBlock;
		return &NewBlock[Offset];
	}

	//if we have a size and it is in a new memory block then change size
	//otherwise the pointer is good for more than 1 read of data
	if(Size && ((Offset + *Size) > 0xfffff))
		*Size = 0;

	//return the pointer to the data
	return &(MemoryBlocks[Index])[Offset];
}

unsigned short _Read9(unsigned int Location, int DoCheck)
{
    Location &= MEMORY_MASK;

    if(DoCheck && (GetMemoryProtectBit(Read, Location) == 0))
    {
        RaiseException(EXCEPTION_MEMORY_READ, Location);
        return 0;
    }

    if(Location & IO_MEMORY_MASK)
        return IO_Read9(Location);

    return *FindMemBlock(Location, 0);
}

unsigned int _Read18(unsigned int Location, int DoCheck)
{
    unsigned short *Ptr;
    unsigned int Size;

    Location &= MEMORY_MASK;

	//chck memory protections
    if(DoCheck && (GetMemoryProtectBit(Read, Location) == 0))
    {
        RaiseException(EXCEPTION_MEMORY_READ, Location);
        return 0;
    }
    if((Location & 0x00fc00) != ((Location + 1) & 0x00fc00))
    {
        if(DoCheck && (GetMemoryProtectBit(Read, Location+1) == 0))
        {
            RaiseException(EXCEPTION_MEMORY_READ, Location);
            return 0;
        }
    }

    if(Location & IO_MEMORY_MASK)
        return IO_Read18(Location);

	//get the data
    Size = 2;
    Ptr = FindMemBlock(Location, &Size);
    if(Size)
        return ((unsigned int)*(Ptr + 1) << 9) |
               ((unsigned int)*Ptr);
    else
        return ((unsigned int)*FindMemBlock((Location+1) & MEMORY_MASK, 0) << 9) |
               ((unsigned int)*Ptr);
}

unsigned int _Read27(unsigned int Location, int DoCheck)
{
    unsigned short *Ptr;
    unsigned int Size;

    Location &= MEMORY_MASK;

    if(DoCheck && (GetMemoryProtectBit(Read, Location) == 0))
    {
        RaiseException(EXCEPTION_MEMORY_READ, Location);
        return 0;
    }
    if((Location & 0x00fc00) != ((Location + 2) & 0x00fc00))
    {
        if(DoCheck && (GetMemoryProtectBit(Read, Location+2) == 0))
        {
            RaiseException(EXCEPTION_MEMORY_READ, Location);
            return 0;
        }
    }

    if(Location & IO_MEMORY_MASK)
        return IO_Read27(Location);

    Size = 3;
    Ptr = FindMemBlock(Location, &Size);
    if(Size)
        return (((unsigned int)*Ptr) << 9) |
               (((unsigned int)*(Ptr + 1)) << 18) |
               (((unsigned int)*(Ptr + 2)) << 0);
    else
        return (((unsigned int)*Ptr) << 9) |
               (((unsigned int)*FindMemBlock((Location + 1) & MEMORY_MASK, 0)) << 18) |
               (((unsigned int)*FindMemBlock((Location + 2) & MEMORY_MASK, 0)) << 0);
}

unsigned int ReadForExecute27(unsigned int Location)
{
    unsigned short *Ptr;
    unsigned int Size;

    Location &= MEMORY_MASK;

    //if the processor is being debugged then check for memory access checks
    //if tripped then just exit so we avoid exceptions, etc
#ifndef NODEBUG
    if(CPU.Debugging && (DebugState != DEBUG_STATE_BP_HIT))
    {
        //1 is valid, we only want to trip if we are hitting the beginning of the instruction
        CPU.DebugTripped = DebugCheckMemoryAccess(Location, 1, DEBUG_EXECUTE);
        if(CPU.DebugTripped)
            return 0;
    }
#endif

    if(GetMemoryProtectBit(Execute, Location) == 0)
    {
        RaiseException(EXCEPTION_MEMORY_EXECUTE, Location);
        return 0;
    }
    if((Location & 0x00fc00) != ((Location + 2) & 0x00fc00))
    {
        if(GetMemoryProtectBit(Execute, Location+2) == 0)

        {
            RaiseException(EXCEPTION_MEMORY_EXECUTE, Location);
            return 0;
        }
    }
    if(Location & IO_MEMORY_MASK)
        return IO_Read27(Location);

    Size = 3;
    Ptr = FindMemBlock(Location, &Size);
    if(Size)
        return (((unsigned int)*Ptr) << 9) |
               (((unsigned int)*(Ptr + 1)) << 18) |
               (((unsigned int)*(Ptr + 2)) << 0);
    else
        return (((unsigned int)*Ptr) << 9) |
               (((unsigned int)*FindMemBlock((Location + 1) & MEMORY_MASK, 0)) << 18) |
               (((unsigned int)*FindMemBlock((Location + 2) & MEMORY_MASK, 0)) << 0);
}

void _Write9(unsigned int Location, unsigned short Val, int DoCheck)
{
    unsigned short *Addr;

    Location &= MEMORY_MASK;

    if(DoCheck && (GetMemoryProtectBit(Write, Location) == 0))
    {
        RaiseException(EXCEPTION_MEMORY_WRITE, Location);
        return;
    }

    if(Location & IO_MEMORY_MASK)
    {
        IO_Write9(Location, Val);
        return;
    }

    Addr = FindMemBlock(Location & MEMORY_MASK, 0);
    *Addr = Val & 0x1ff;
}

void _Write18(unsigned int Location, unsigned int Val, int DoCheck)
{
    unsigned short *Addr;
    unsigned int Size;

    Location &= MEMORY_MASK;

    if(DoCheck && (GetMemoryProtectBit(Write, Location) == 0))
    {
        RaiseException(EXCEPTION_MEMORY_WRITE, Location);
        return;
    }
    if((Location & 0x00fc00) != ((Location + 1) & 0x00fc00))
    {
        if(DoCheck && (GetMemoryProtectBit(Write, Location+1) == 0))
        {
            RaiseException(EXCEPTION_MEMORY_WRITE, Location);
            return;
        }
    }

    if(Location & IO_MEMORY_MASK)
    {
        IO_Write18(Location, Val);
        return;
    }

    Size = 2;
    Addr = FindMemBlock(Location & MEMORY_MASK, &Size);
    *Addr = Val & 0x1ff;

    if(Size)
        *(Addr + 1) = (Val >> 9) & 0x1ff;
    else
    {
        Addr = FindMemBlock((Location + 1) & MEMORY_MASK, 0);
        *(Addr + 1) = (Val >> 9) & 0x1ff;
    }
}

void _Write27(unsigned int Location, unsigned int Val, int DoCheck)
{
    unsigned short *Addr;
    unsigned int Size;

    Location &= MEMORY_MASK;

    if(DoCheck && (GetMemoryProtectBit(Write, Location) == 0))
    {
        RaiseException(EXCEPTION_MEMORY_WRITE, Location);
        return;
    }
    if((Location & 0x00fc00) != ((Location + 2) & 0x00fc00))
    {
        if(DoCheck && (GetMemoryProtectBit(Write, Location+2) == 0))

        {
            RaiseException(EXCEPTION_MEMORY_WRITE, Location);
            return;
        }
    }

    if(Location & IO_MEMORY_MASK)
    {
        IO_Write27(Location, Val);
        return;
    }

    Size = 3;
    Addr = FindMemBlock(Location & MEMORY_MASK, &Size);
    *Addr = (Val >> 9) & 0x1ff;

    if(Size)
    {
        *(Addr + 1) = (Val >> 18) & 0x1ff;
        *(Addr + 2) = Val & 0x1ff;
    }
    else
    {
        Addr = FindMemBlock((Location + 1) & MEMORY_MASK, 0);
        *Addr = (Val >> 18) & 0x1ff;

        Addr = FindMemBlock((Location + 2) & MEMORY_MASK, 0);
        *Addr = Val & 0x1ff;
    }
}

unsigned int ReadMemoryProtection(unsigned int Location)
{
    Location &= MEMORY_MASK;
    if(Location & (MEMORY_PAGE_SIZE - 1))
        return -1;

    if(GetMemoryProtectBit(Execute, Location))
        return MEMORY_PROTECTION_EXECUTE;
    else if(GetMemoryProtectBit(Write, Location))
        return MEMORY_PROTECTION_READWRITE;
    else if(GetMemoryProtectBit(Read, Location))
        return MEMORY_PROTECTION_READ;

    return MEMORY_PROTECTION_NONE;
}

void SetMemoryProtection(unsigned int Location, unsigned int Protection)
{
	Location &= MEMORY_MASK;
	if(Location & (MEMORY_PAGE_SIZE - 1))
		return;

	//if the location is DMA area then the flag area are always readable
	//everything else can be mucked with, could set the receive buffer executable for instance
	if(((Location & IO_MASK) == IO_FLAG) && (Protection == MEMORY_PROTECTION_NONE))
		Protection = MEMORY_PROTECTION_READ;

	//setup NFO protection
	Protection = ClemencyNFO_Protection(Location, Protection);

	//set the protection
	switch(Protection)
	{
		case MEMORY_PROTECTION_NONE:
			ClearMemoryProtectBit(Read, Location);
			ClearMemoryProtectBit(Write, Location);
			ClearMemoryProtectBit(Execute, Location);
			break;

		case MEMORY_PROTECTION_READ:
			SetMemoryProtectBit(Read, Location);
			ClearMemoryProtectBit(Write, Location);
			ClearMemoryProtectBit(Execute, Location);
			break;

		case MEMORY_PROTECTION_READWRITE:
			SetMemoryProtectBit(Read, Location);
			SetMemoryProtectBit(Write, Location);
			ClearMemoryProtectBit(Execute, Location);
			break;

		case MEMORY_PROTECTION_EXECUTE:
			SetMemoryProtectBit(Read, Location);
			ClearMemoryProtectBit(Write, Location);
			SetMemoryProtectBit(Execute, Location);
			break;
	}

	return;
}

unsigned int Memory_MemCpy(unsigned int OutLocation, unsigned int InLocation, unsigned int Size)
{
	unsigned short *PtrIn, *PtrOut;
	unsigned int CurSizePos;
	unsigned int CurInLocation;
	unsigned int CurOutLocation;
	unsigned int DataLeft;

	InLocation &= MEMORY_MASK;
	OutLocation &= MEMORY_MASK;

	//if we are going past the end of memory then just fail
	if((InLocation + Size) > MEMORY_MASK)
	{
		RaiseException(EXCEPTION_MEMORY_READ, MEMORY_MASK);
		return 0;
	}
	if((OutLocation + Size) > MEMORY_MASK)
	{
		RaiseException(EXCEPTION_MEMORY_WRITE, MEMORY_MASK);
		return 0;
	}

	//check each of our pages
	CurInLocation = InLocation & ~(MEMORY_PAGE_SIZE - 1);
	CurOutLocation = OutLocation & ~(MEMORY_PAGE_SIZE - 1);
	while(CurInLocation < (InLocation + Size))
	{
		if(GetMemoryProtectBit(Read, CurInLocation) == 0)
		{
			RaiseException(EXCEPTION_MEMORY_READ, CurInLocation);
			return 0;
		}
		if(GetMemoryProtectBit(Write, CurOutLocation) == 0)
		{
			RaiseException(EXCEPTION_MEMORY_WRITE, CurOutLocation);
			return 0;
		}
		CurInLocation += MEMORY_PAGE_SIZE;
		CurOutLocation += MEMORY_PAGE_SIZE;
	}

	//good to copy from one to the other, if we are doing it from IO then handle special
	if((InLocation & IO_MEMORY_MASK) || (OutLocation & IO_MEMORY_MASK))
	{
		//special case, if we are doing to/from network then try to transfer from the buffer itself
		if(((InLocation & IO_MASK) == IO_NETWORK_RECV) && (InLocation <= IO_NETWORK_RECV_END) && !(OutLocation & IO_MEMORY_MASK))
		{
			//part of the actual buffer, transfer what we can
			if((InLocation + Size) > IO_NETWORK_RECV_END)
			{
				CurSizePos = IO_NETWORK_RECV_END - InLocation;
				Size -= CurSizePos;
			}
			else
			{
				CurSizePos = Size;
				Size = 0;
			}

			//transfer all the data we can handle while handling memory block boundaries
			while(CurSizePos)
			{
				//calculate the offset so we can then see how much we can copy from block to block
				//CurOutLocation is how much to copy without going past the end of a memory block
				CurInLocation = InLocation & 0x1fff;
				CurOutLocation = 0x100000 - (OutLocation & 0xfffff);
				DataLeft = CurOutLocation;

				//if we don't have enough to fill up a memory block then just get size
				//otherwise stay at the memory block size that is left
				if(CurSizePos < DataLeft)
					DataLeft = CurSizePos;

				//get our pointer and copy
				PtrOut = FindMemBlock(OutLocation, &DataLeft);
				memcpy(PtrOut, &Network_In_Buffer[CurInLocation], DataLeft * 2);

				//adjust everyone
				CurSizePos -= DataLeft;
				InLocation += DataLeft;
				OutLocation += DataLeft;
			};

			//if no size left then bail
			if(!Size)
				return 0;
		}
		else if(((OutLocation & IO_MASK) == IO_NETWORK_SEND) && (OutLocation <= IO_NETWORK_SEND_END) && !(InLocation & IO_MEMORY_MASK))
		{
			//part of the actual buffer, transfer what we can
			if((OutLocation + Size) > IO_NETWORK_SEND_END)
			{
				CurSizePos = IO_NETWORK_SEND_END - OutLocation;
				Size -= CurSizePos;
			}
			else
			{
				CurSizePos = Size;
				Size = 0;
			}

			//transfer all the data we can handle
			while(CurSizePos)
			{
				//calculate the offset so we can then see how much we can copy from block to block
				CurInLocation = 0x100000 - (InLocation & 0xfffff);
				CurOutLocation = OutLocation & 0x1fff;
				DataLeft = CurInLocation;

				//if we don't have enough to fill up a memory block then just get size
				//otherwise stay at the memory block size that is left
				if(CurSizePos < DataLeft)
					DataLeft = CurSizePos;

				//get our pointers and copy
				PtrIn = FindMemBlock(InLocation, &DataLeft);
				memcpy(&Network_Out_Buffer[CurOutLocation], PtrIn, DataLeft * 2);

				//adjust everyone
				CurSizePos -= DataLeft;
				InLocation += DataLeft;
				OutLocation += DataLeft;
			};

			//if no size left then bail
			if(!Size)
				return 0;
		}

		//not reading RECV and not writing to SEND buffers so just handle normal

		//if aligned then do 3 bytes at a time
		if(((InLocation % 3) == 0) && ((OutLocation % 3) == 0))
		{
			while(Size >= 3)
			{
				DataLeft = _Read27(InLocation, 0);
				_Write27(OutLocation, DataLeft, 0);
				InLocation += 3;
				OutLocation += 3;
				Size -= 3;
			}
		}

		//finish copying byte by byte to avoid issues
		while(Size)
		{
			DataLeft = _Read9(InLocation, 0);
			_Write9(OutLocation, DataLeft, 0);
			InLocation++;
			OutLocation++;
			Size--;
		};

		return 0;
	}

	//not IO copy, copy block to block
	while(Size)
	{
		//calculate the offset so we can then see how much we can copy from block to block
		CurInLocation = 0x100000 - (InLocation & 0xfffff);
		CurOutLocation = 0x100000 - (OutLocation & 0xfffff);
		if(CurInLocation < CurOutLocation)
			DataLeft = CurInLocation;
		else
			DataLeft = CurOutLocation;

		//if we don't have enough to fill up a memory block then just get size
		//otherwise stay at the memory block size that is left
		if(Size < DataLeft)
			DataLeft = Size;

		//get our pointers and copy
		PtrIn = FindMemBlock(InLocation, &DataLeft);
		PtrOut = FindMemBlock(OutLocation, &DataLeft);
		memcpy(PtrOut, PtrIn, DataLeft * 2);

		//adjust everyone
		Size -= DataLeft;
		InLocation += DataLeft;
		OutLocation += DataLeft;
	}

	return 0;
}
