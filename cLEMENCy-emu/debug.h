//
//  debug.h
//  cLEMENCy debug handler
//
//  Created by Lightning on 2015/12/24.
//  Copyright (c) 2017 Lightning. All rights reserved.
//

#ifndef __CLEMENCY__debug__
#define __CLEMENCY__debug__

#include "recordstate.h"

typedef struct DebugBreakpointStruct
{
	unsigned int Location;
	int Type;
	unsigned int Len;
	unsigned int ID;
	char *Commands;
	struct DebugBreakpointStruct *NextAddr;	//ordered by address
	struct DebugBreakpointStruct *NextID;	//ordered by ID
} DebugBreakpointStruct;

typedef struct DebugExceptionStruct
{
	int ExceptionType;
	int Value;
	int RegistersStored;
	unsigned int PC;
	int PrevDebugState;
} DebugExceptionStruct;

extern DebugBreakpointStruct *BreakpointListID;
extern DebugBreakpointStruct *BreakpointListAddr;

extern int ExceptionTriggers;
extern int DebugState;
extern int DebugStepCount;
extern unsigned int DebugGoAddr;
extern int DebugOverNetwork;

extern char *RegisterStr[];
extern char *ExceptionStr[];
extern char *DebugDisplayStr;

#ifdef NODEBUG
#define DebugOut(a) do{}while(0)
#else
void DebugOut(char *Msg);
#endif

void InitDebug();
void LoadMapFile(char *Firmware);
char *GetMapName(int Address);
int GetMapLocation(char *Entry);
void PrintRegisters(int RegType, unsigned int RegNum);
void PrintMemory(int Offset, int Len, int Type);
int SizeOfOpcode(int Location);
void DisassembleInstructions(int Location, int InstCount, RecordStateStruct *State);
void HandleDebugger();
void *DebugCheckMemoryAccess(unsigned int Location, unsigned int Len, int Type);
int AddBreakpoint(unsigned int Location, unsigned int Len, int Type);
int RemoveBreakpoint(unsigned int ID);
int HandleDebugCommandStr(char *RunCommand);
int HandleMultipleDebugCommandStr(char *RunCommand);

#define DEBUG_READ 1
#define DEBUG_WRITE 2
#define DEBUG_EXECUTE 4

#define DEBUG_REG_NORMAL 1
#define DEBUG_REG_FLOAT 2

#define DEBUG_STATE_PAUSED	0
#define DEBUG_STATE_RUNNING	1
#define DEBUG_STATE_STEP	2
#define DEBUG_STATE_CTRLC	3
#define DEBUG_STATE_EXCEPTION	4
#define DEBUG_STATE_BP_HIT	5

#endif
