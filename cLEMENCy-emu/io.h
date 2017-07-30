//
//  io.h
//  cLEMENCy IO and DMA
//
//  Created by Lightning on 2015/12/20.
//  Copyright (c) 2017 Lightning. All rights reserved.
//

#ifndef __CLEMENCY__io__
#define __CLEMENCY__io__

void InitIO();
void InitSharedMemory(char *BaseName);
void InitNVRamMemory(char *BaseName, int fd);
void ReadFlagFile();

unsigned short IO_Read9(unsigned int Location);
unsigned int IO_Read18(unsigned int Location);
unsigned int IO_Read27(unsigned int Location);

void IO_Write9(unsigned int Location, unsigned short Val);
void IO_Write18(unsigned int Location, unsigned int Val);
void IO_Write27(unsigned int Location, unsigned int Val);

#define IO_MASK				0x0fff0000

#define IO_CLOCK			0x04000000
#define IO_CLOCK_END			0x0400001D

#define IO_FLAG				0x04010000
#define IO_FLAG_END			0x04010FFF

#define IO_NETWORK_RECV			0x05000000
#define IO_NETWORK_RECV_END		0x05001FFF

#define IO_NETWORK_RECV_SIZE		0x05002000
#define IO_NETWORK_RECV_SIZE_END	0x05002002

#define IO_NETWORK_SEND			0x05010000
#define IO_NETWORK_SEND_END		0x05011FFF
#define IO_NETWORK_SEND_SIZE		0x05012000
#define IO_NETWORK_SEND_SIZE_END	0x05012003

#define IO_SHARED_MEMORY		0x06000000
#define IO_SHARED_MEMORY_END		0x067FFFFF

#define IO_NVRAM_MEMORY			0x06800000
#define IO_NVRAM_MEMORY_END		0x06FFFFFF

#define IO_INTERRUPTS			0x07FFFF00
#define IO_INTERRUPTS_END		0x07FFFF1B
#define IO_INTERRUPTS_MASK      0x07FFFF00

#define IO_PROCESSOR_ID			0x7FFFFF80
#define IO_PROCESSOR_ID_END		0x7FFFFFFF

#endif
