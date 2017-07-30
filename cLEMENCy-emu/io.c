//
//  io.c
//  cLEMENCy IO and DMA
//
//  Created by Lightning on 2015/12/20.
//  Copyright (c) 2017 Lightning. All rights reserved.
//

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <time.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include "io.h"
#include "interrupts.h"
#include "cpu.h"
#include "clock.h"
#include "network.h"
#include "debug.h"
#include "memory.h"
#include "clemency_nfo.h"

unsigned char Flag_Buffer[256];
unsigned int FlagSize;
unsigned int TempVal;

unsigned short *SharedMemory;
unsigned short *NVRamMemory;

#define TIMER_COUNT 4

void ReadFlagFile()
{
    int fd;

    fd = open("flag", O_RDONLY);
    if(fd < 0)
    {
        DebugOut("Error during flag read");
        exit(-1);
    }

    FlagSize = read(fd, Flag_Buffer, sizeof(Flag_Buffer));
    close(fd);
}

void InitIO()
{
    int fd;

    FlagSize = 0;

    //make sure our buffers are empty
    memset(Flag_Buffer, 0, sizeof(Flag_Buffer));
    TempVal = 0;
    SharedMemory = 0;
    NVRamMemory = 0;

    //make sure we can read the flag file
    fd = open("flag", O_RDONLY);
    if(fd < 0)
    {
        DebugOut("Error during IO init: unable to read flag file");
        exit(-1);
    }
}

void InitSharedMemory(char *BaseName)
{
    char SharedMemoryFile[256];
    int fd;
    int i;

    //init the shared memory area

    if((strlen(BaseName) + 8) >= 256)
    {
        DebugOut("Shared memory name too long");
        exit(-1);
    }

    //create <BaseName>.shared
    strcpy(SharedMemoryFile, BaseName);
    strcat(SharedMemoryFile, ".shared");

    //if we have a file then open it
    fd = open(SharedMemoryFile, O_RDWR | O_CREAT | O_NONBLOCK, 0777);

    //make sure the file is 16MB in size
    i = lseek(fd, 0, SEEK_END);

    //allocate enough space if the file is too small
    if(i < 0x1000000)
    {
        //seek to position, write a byte then close it to force size update
        lseek(fd, 0x1000000 - 1, SEEK_SET);
        write(fd, "\x00", 1);
        close(fd);

        //re-open the file
        fd = open(SharedMemoryFile, O_RDWR | O_CREAT | O_NONBLOCK, 0777);
    }

    //reset to the beginning
    lseek(fd, 0, SEEK_SET);

    //map it
    SharedMemory = mmap(0, 0x800000 * sizeof(short), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(SharedMemory == MAP_FAILED)
    {
        DebugOut("Error during shared memory init: Unable to map shared memory");
        exit(-1);
    }

    //shared memory region
    for(i = IO_SHARED_MEMORY; i <= IO_SHARED_MEMORY_END; i+= 1024)
        SetMemoryProtection(i, MEMORY_PROTECTION_READWRITE);
}

void InitNVRamMemory(char *BaseName, int fd)
{
    char NVRamMemoryFile[256];
    struct sockaddr_in inaddr;
    unsigned int i;

    //init the shared memory area

    if((strlen(BaseName) + 11) >= 256)
    {
        DebugOut("NVRam memory name too long");
        exit(-1);
    }

    //create <BaseName>.nvram.xx

    //figure out the ID to add, either 0 if local or part of the ip
    i = sizeof(struct sockaddr_in);
    if(getpeername(fd, (struct sockaddr *)&inaddr, &i) < 0)
        fd = 999;
    else
        fd = (inaddr.sin_addr.s_addr >> 16) & 0xff;

    sprintf(NVRamMemoryFile, "%s.nvram.%d", BaseName, fd);

    //if we have a file then open it
    fd = open(NVRamMemoryFile, O_RDWR | O_CREAT | O_NONBLOCK, 0777);

    //make sure the file is 16MB in size
    i = lseek(fd, 0, SEEK_END);

    //allocate enough space if the file is too small
    //reminder, we are allocating shorts so 8mb virtual memory, 16mb physical
    if(i < 0x1000000)
    {
        //seek to position, write a byte then close it to force size update
        lseek(fd, 0x1000000 - 1, SEEK_SET);
        write(fd, "\x00", 1);
        close(fd);

        //re-open the file
        fd = open(NVRamMemoryFile, O_RDWR | O_CREAT | O_NONBLOCK, 0777);
    }

    //reset to the beginning
    lseek(fd, 0, SEEK_SET);

    //map it
    NVRamMemory = mmap(0, 0x800000 * sizeof(short), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(NVRamMemory == MAP_FAILED)
    {
        DebugOut("Error during NVRam memory init: Unable to map memory");
        exit(-1);
    }

    //nvram memory region
    for(i = IO_NVRAM_MEMORY; i <= IO_NVRAM_MEMORY_END; i+= 1024)
        SetMemoryProtection(i, MEMORY_PROTECTION_READWRITE);
}

unsigned short ConvertVal9(unsigned int InVal, int Byte)
{
	if(Byte == 0)
		return (InVal >> 9) & 0x1ff;
	else if(Byte == 1)
		return (InVal >> 18) & 0x1ff;
	else
		return InVal & 0x1ff;
}

unsigned short IO_Read9(unsigned int Location)
{
    unsigned int Val;
    int ID;
    unsigned long TimeInSec;

    if((Location & IO_MASK) == IO_NETWORK_RECV)
    {
        if((Location & ~IO_MASK) <= 0x1fff)
            return Network_In_Buffer[Location & 0x1fff];
        else if((Location & ~IO_MASK) <= 0x2002)
            return ConvertVal9(Network_In_Buffer_Size, (Location & ~IO_MASK) - 0x2000);
    }
    else if((Location & IO_MASK) == IO_NETWORK_SEND)
    {
        if((Location & ~IO_MASK) <= 0x1fff)
            return Network_Out_Buffer[Location & 0x1fff];
        else if((Location & ~IO_MASK) <= 0x2002)
            return ConvertVal9(Network_Out_Buffer_Size, (Location & ~IO_MASK) - 0x2000);
    }
    else if((Location & IO_MASK) == IO_FLAG)
    {
        if(Location & 0x0000FF00)
            return 0;
        else
        {
            if(!FlagSize)
                ReadFlagFile();
            return Flag_Buffer[Location & 0x0000ff];
        }
    }
    else if((Location & IO_INTERRUPTS_MASK) == IO_INTERRUPTS)
    {
        //get the value of the clock timer or actual ticks
        //i'm avoiding division hence multiple if statements
        Location &= 0xff;

    	if(Location == 0xf0)
    		return InterruptStackDirection;

    	//if processor info
    	if(Location >= 0x80)
    	{
    		Location &= 0x7f;
    		char *ProcessorID = "LegitBS cLEMENCy";

    		//major/minor/rev - 1.0.0
    		unsigned int ProcessorVersion = ((unsigned int)(1) << 18) | ((unsigned int)(0) << 9) | ((unsigned int)(0) << 0);

    		if(Location < strlen(ProcessorID))
    			return (unsigned short)ProcessorID[Location];
    		else if(Location < 0x20)
    			return 0;
    		else if(Location < 0x23)
    			return ConvertVal9(ProcessorVersion, Location - 0x20);
    		else if(Location < 0x26)
    			return ConvertVal9(5, Location - 0x26);	//54-bit math and interrupt can flip directions
    	}

        if(Location >= (INTERRUPT_COUNT*3))
            return 0;

        ID = Location / 3;
        Location = Location % 3;
        Val = GetInterrupt(ID);

        //return the right byte
        return ConvertVal9(Val, Location);
    }
    else if((Location & IO_MASK) == IO_CLOCK)
    {
        //get the value of the clock timer or actual ticks
        Location &= 0xffff;
        if(Location >= (((TIMER_COUNT*2)*3) + 9))
            return 0;

        //figure out the ID for the timer being requested, 6 bytes per timer
        ID = Location / 6;
        Location = Location % 6;

        //if the ID is the number of timers then they want to read the time in seconds
        if(ID == TIMER_COUNT)
        {
            TimeInSec = GetClock_TimeSinceAug02();
            if(Location > 2)
                Val = (unsigned int)(TimeInSec & 0x7ffffff);
            else
                Val = (unsigned int)((TimeInSec >> 27) & 0x7ffffff);
        }
        else if(ID == (TIMER_COUNT + 1))
            Val = CPU.TickCount & 0x07ffffff;    //cpu tick count sits after time in seconds
        else if(Location > 2)
        {
            Val = GetClockTimer_TimeLeft(ID);
            Location -= 3;
        }
        else
            Val = GetClockTimer(ID);

        //return the right byte
        return ConvertVal9(Val, Location);
    }
    else if(((Location & IO_MASK) >= IO_SHARED_MEMORY) && ((Location & IO_MASK) < (IO_SHARED_MEMORY_END+1)))
    {
        //if no allocation then just return 0
        if(!SharedMemory)
            return 0;

        //return the right entry
        return SharedMemory[Location & 0x7FFFFF];
    }
    else if(((Location & IO_MASK) >= IO_NVRAM_MEMORY) && ((Location & IO_MASK) < (IO_NVRAM_MEMORY_END+1)))
    {
        //if no allocation then just return 0
        if(!NVRamMemory)
            return 0;

        //return the right entry
        return NVRamMemory[Location & 0x7FFFFF];
    }
    return ClemencyNFO_Read9(Location);
}

unsigned int IO_Read18(unsigned int Location)
{
    return ((unsigned int)IO_Read9(Location) | ((unsigned int)IO_Read9(Location + 1) << 9));
}

unsigned int IO_Read27(unsigned int Location)
{
    return (((unsigned int)IO_Read9(Location)) << 9) | (((unsigned int)IO_Read9(Location + 1)) << 18) | (unsigned int)IO_Read9(Location + 2);
}

void IO_Write9_Internal(unsigned int Location, unsigned short Val)
{
    //our internal write function as we need to dma check after each full write
    //and we re-use this function for the 16 and 24bit writes

    //we don't allow flag data to be rewritten
    if((Location & IO_MASK) == IO_FLAG)
        return;
    else if((Location & IO_MASK) == IO_NETWORK_RECV)
    {
        if((Location & 0xffff) <= 0x1fff)
            Network_In_Buffer[Location & 0x1fff] = Val;
        else if((Location & 0xffff) == 0x2000)
            Network_In_Buffer_Size = (Network_In_Buffer_Size & 0x7fc01ff) | (Val << 9);
        else if((Location & 0xffff) == 0x2001)
            Network_In_Buffer_Size = (Network_In_Buffer_Size & 0x003ffff) | (Val << 18);
        else if((Location & 0xffff) == 0x2002)
            Network_In_Buffer_Size = (Network_In_Buffer_Size & 0x7fffe00) | (Val << 0);
    }
    else if((Location & IO_MASK) == IO_NETWORK_SEND)
    {
        if((Location & 0xffff) <= 0x1fff)
            Network_Out_Buffer[Location & 0x1fff] = Val;
        else if((Location & 0xffff) == 0x2000)
            Network_Out_Buffer_Size = (Network_Out_Buffer_Size & 0x7fc01ff) | (Val << 9);
        else if((Location & 0xffff) == 0x2001)
            Network_Out_Buffer_Size = (Network_Out_Buffer_Size & 0x003ffff) | (Val << 18);
        else if((Location & 0xffff) == 0x2002)
            Network_Out_Buffer_Size = (Network_Out_Buffer_Size & 0x7fffe00) | (Val << 0);
    }
    else if((Location & IO_INTERRUPTS_MASK) == IO_INTERRUPTS)
    {
        Location &= 0xff;
        if(Location < (INTERRUPT_COUNT*3))
        {
            //adjust location to find out what byte to modify
            Location = Location % 3;

            if(Location == 0)
                TempVal = ((unsigned int)Val << 9) | (TempVal & 0x7fc01ff);
            else if(Location == 1)
                TempVal = ((unsigned int)Val << 18) | (TempVal & 0x03ffff);
            else //2
                TempVal = (TempVal & 0x7fffe00) | Val;
        }
    	else if(Location == 0xf0)
                InterruptStackDirection = Val & 1;
    }
    else if((Location & IO_MASK) == IO_CLOCK)
    {
        Location &= 0xffff;
        if(Location < (((TIMER_COUNT*2)*3) + 6))
        {
            //adjust location to find out what byte to modify
            Location = Location % 3;

            if(Location == 0)
                TempVal = ((unsigned int)Val << 9) | (TempVal & 0x7fc01ff);
            else if(Location == 1)
                TempVal = ((unsigned int)Val << 18) | (TempVal & 0x03ffff);
            else //2
                TempVal = (TempVal & 0x7fffe00) | Val;
        }
    }
    else if(((Location & IO_MASK) >= IO_SHARED_MEMORY) && ((Location & IO_MASK) < (IO_SHARED_MEMORY_END+1)))
    {
        //if no allocation then just return
        if(!SharedMemory)
            return;

        //write the right entry
        SharedMemory[Location & 0x7FFFFF] = Val & 0x1ff;

        //make sure it is synced
        msync(&SharedMemory[Location & 0x7FFFFF], 2, MS_ASYNC);
    }
    else if(((Location & IO_MASK) >= IO_NVRAM_MEMORY) && ((Location & IO_MASK) < (IO_NVRAM_MEMORY_END+1)))
    {
        //if no allocation then just return 0
        if(!NVRamMemory)
            return;

        //write the right entry
        NVRamMemory[Location & 0x7FFFFF] = Val & 0x1ff;

        //make sure it is synced
        msync(&NVRamMemory[Location & 0x7FFFFF], 2, MS_ASYNC);
    }
}

void IO_Check_DMA(unsigned int Location)
{
    //check the write location to see if we need to do internal updates, send data, etc
    int ID;
    unsigned long TimeSince;

    if((Location & IO_INTERRUPTS_MASK) == IO_INTERRUPTS)
    {
        Location &= 0xff;
        if(Location < (INTERRUPT_COUNT*3))
        {
            //figure out our ID
            ID = Location / 3;
            SetInterrupt(ID, TempVal);
        }
    }
    else if((Location & IO_MASK) == IO_CLOCK)
    {
        Location &= 0xffff;
        if(Location < (((TIMER_COUNT*2)*3) + 6))
        {
            //figure out our ID
            ID = Location / 6;
            Location = Location % 6;

            //if the ID is the number of timers then set the time
            if(ID == TIMER_COUNT)
            {
                TimeSince = GetClock_TimeSinceAug02();
                if(Location > 2)
                    TimeSince = (TimeSince & 0x003ffffff8000000) | TempVal;
                else
                    TimeSince = ((unsigned long)TempVal << 27) | (TimeSince & 0x7ffffff);
                SetClock_TimeSinceAug02(TimeSince);
            }
            else if(Location > 2)
                SetClockTimer_TimeLeft(ID, TempVal);
            else
                SetClockTimer(ID, TempVal);
        }
    }
}

void IO_Write9(unsigned int Location, unsigned short Val)
{
    TempVal = 0;
    IO_Write9_Internal(Location, Val);
    IO_Check_DMA(Location);
}

void IO_Write18(unsigned int Location, unsigned int Val)
{
    TempVal = 0;
    IO_Write9_Internal(Location, (unsigned short)(Val & 0x1ff));
    IO_Write9_Internal(Location + 1, (unsigned short)((Val >> 9) & 0x1ff));
    IO_Check_DMA(Location);
}

void IO_Write27(unsigned int Location, unsigned int Val)
{
    TempVal = 0;
    IO_Write9_Internal(Location, (unsigned short)((Val >> 9) & 0x1ff));
    IO_Write9_Internal(Location + 1, (unsigned short)((Val >> 18) & 0x1ff));
    IO_Write9_Internal(Location + 2, (unsigned short)(Val & 0x1ff));
    IO_Check_DMA(Location);
}
