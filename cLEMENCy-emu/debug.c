//
//  debug.c
//  cLEMENCy debug handler
//
//  Created by Lightning on 2015/12/24.
//  Copyright (c) 2017 Lightning. All rights reserved.
//

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

#include "cpu.h"
#include "memory.h"
#include "debug.h"
#include "debug-cmds.h"
#include "exceptions.h"

char *RegisterStr[32];
char *ExceptionStr[] = {"Memory read", "Memory write", "Memory execute", "Invalid instruction", "Floating point", "Divide by 0", "DBRK"};
int ExceptionTriggers;
char *DebugDisplayStr;

DebugBreakpointStruct *BreakpointListID;
DebugBreakpointStruct *BreakpointListAddr;

int DebugState;
unsigned int DebugGoAddr;
int DebugStepCount;
int DebugSingleStepBP;
int DebugOverNetwork;

typedef struct MapStruct
{
	char *Name;
	int StartAddr;
	int EndAddr;
	struct MapStruct *Next;
} MapStruct;

CPURegStruct LastRegVals;
MapStruct *MapList;

void InitDebug()
{
	int i;

	for(i = 0; i < 29; i++)
	{
		RegisterStr[i] = (char *)malloc(4);
		sprintf(RegisterStr[i], "R%02d", i);
	}

	RegisterStr[29] = "ST";
	RegisterStr[30] = "RA";
	RegisterStr[31] = "PC";

	BreakpointListAddr = 0;
	BreakpointListID = 0;

	DebugState = DEBUG_STATE_PAUSED;
	DebugStepCount = 0;
	DebugGoAddr = -1;
	DebugSingleStepBP = 0;

	MapList = 0;
	memset(&LastRegVals, 0, sizeof(LastRegVals));

	ExceptionTriggers = (1 << EXCEPTION_COUNT) - 1;
	DebugOverNetwork = 0;
	//rl_prep_terminal(0);
}

void LoadMapFile(char *Firmware)
{
	char *MapFile;
	struct stat StatBuf;
	FILE *fd;
	char Buffer[256];
	char Name[256];
	int ID, StartAddr, Len;
	MapStruct *NewMapEntry;
	MapStruct *CurEntry, *PrevEntry;

	//get a copy of the filename then append .map
	Len = strlen(Firmware);
	MapFile = malloc(Len + 5);
	strcpy(MapFile, Firmware);
	strcat(MapFile, ".map");

	//if it doesn't exist then stop trying
	if(stat(MapFile, &StatBuf) != 0)
	{
		printf("No map file found\n");
		free(MapFile);
		return;
	}

	printf("Parsing map file %s...\n", MapFile);
	fflush(stdout);

	fd = fopen(MapFile, "r");
	Buffer[sizeof(Buffer) - 1] = 0;
	while(fgets(Buffer, sizeof(Buffer) - 1, fd))
	{
		//we limited to 255 characters so this should be fine
		if(sscanf(Buffer, "%d: %255s @ %07x %d", &ID, Name, &StartAddr, &Len) != 4)
		{
			puts("Failed to parse map file\n");
			break;
		}

		//store the entry
		Name[sizeof(Name) - 1] = 0;
		NewMapEntry = malloc(sizeof(MapStruct));
		printf("Found %s\n", Name);
		NewMapEntry->Name = strdup(Name);
		NewMapEntry->StartAddr = StartAddr;
		if(Len)
			Len--;
		NewMapEntry->EndAddr = StartAddr + Len;
		NewMapEntry->Next = 0;

		//insert them in order
		CurEntry = MapList;
		PrevEntry = 0;
		while(CurEntry && (CurEntry->StartAddr < StartAddr))
		{
			PrevEntry = CurEntry;
			CurEntry = CurEntry->Next;
		}

		//if no previous then put at the beginning of the list
		if(!PrevEntry)
		{
			NewMapEntry->Next = MapList;
			MapList = NewMapEntry;
		}
		else
		{
			//insert
			NewMapEntry->Next = CurEntry;
			PrevEntry->Next = NewMapEntry;
		}
	};
	fclose(fd);

	free(MapFile);

	printf("Read %d entries\n", ID);
}

char *GetMapName(int Address)
{
	MapStruct *CurEntry;

	//go find an entry
	CurEntry = MapList;
	while(CurEntry && (CurEntry->EndAddr < Address))
		CurEntry = CurEntry->Next;

	//if we have an entry, see if it matches
	if(CurEntry && (CurEntry->StartAddr <= Address) && (CurEntry->EndAddr >= Address))
		return CurEntry->Name;

	//no name
	return 0;
}

int GetMapLocation(char *Name)
{
	MapStruct *CurEntry;

	//cycle through all entries, return the start address if it exists
	CurEntry = MapList;
	while(CurEntry && strcasecmp(CurEntry->Name, Name))
		CurEntry = CurEntry->Next;

	if(CurEntry)
		return CurEntry->StartAddr;

	//failure if we can't find it
	return -1;
}

void DebugOut(char *Msg)
{
#ifndef NODEBUG
	write(1, Msg, strlen(Msg));
	write(1, "\n", 1);
#endif
}

void PrintRegisters(int RegType, unsigned int RegNum)
{
	int i;

	//if register isn't valid then exit
	if(((int)RegNum != -1) && RegNum > 32)
		return;

	if(RegType & DEBUG_REG_NORMAL)
	{
		if(RegNum == (unsigned int)-1)
		{
			//print all registers
			for(i = 0; i < 32; i++)
			{
				if(CPU.Registers.R[i] != LastRegVals.R[i])
					printf("\x1b[33;1m");
				printf("%3s: %07x\t", RegisterStr[i], CPU.Registers.R[i]);
				if(CPU.Registers.R[i] != LastRegVals.R[i])
					printf("\x1b[39;0m\x1b[39;49m");
				if((i+1) % 4 == 0)
					printf("\n");
			}
		}

		//if printing all registers or the flag register then print it
		if((RegNum == (unsigned int)-1) || (RegNum == 32))
		{
			//print flag register
			if((RegNum == (unsigned int)-1) && (CPU.Registers.Flags != LastRegVals.Flags))
				printf("\x1b[33;1m");
			printf(" FL: %07x", CPU.Registers.Flags);
			if(CPU_FLAGS & CPU_FLAG_SIGNED)
				printf(" SF");
			else
				printf("   ");
			if(CPU_FLAGS & CPU_FLAG_OVERFLOW)
				printf(" OF");
			else
				printf("   ");
			if(CPU_FLAGS & CPU_FLAG_CARRY)
				printf(" CF");
			else
				printf("   ");
			if(CPU_FLAGS & CPU_FLAG_ZERO)
				printf(" ZF");
			else
				printf("   ");
			printf("\n");
			if((RegNum == (unsigned int)-1) && (CPU.Registers.Flags != LastRegVals.Flags))
				printf("\x1b[39;0m\x1b[39;49m");
			printf("\n");
		}
		else
			printf("%3s: %07x\n", RegisterStr[RegNum], CPU.Registers.R[RegNum]);
	}
	//code for floating point removed
}

void PrintMemory(int Offset, int Len, int Type)
{
	int i;
	DebugBreakpointStruct *BP;
	char *MapName;
	short CurVal;
	char CharLineData[0x19];

	//type is 1 to 3 for byte, word, tribyte
	//type 0 is character by character until null

	MapName = GetMapName(Offset);
	if(MapName)
		printf("%s  ", MapName);

	printf("%07x: ", Offset);

	//print the memory requested
	memset(CharLineData, 0, sizeof(CharLineData));
	for(i = 0; i < Len;)
	{
		//if we have a breakpoint then see what color to set
		BP = (DebugBreakpointStruct *)DebugCheckMemoryAccess(Offset, Type, DEBUG_READ | DEBUG_WRITE | DEBUG_EXECUTE);
		if(BP)
		{
			if(BP->Type & DEBUG_EXECUTE)
				printf("\x1b[31;1m");
			else if(BP->Type & DEBUG_READ)
				printf("\x1b[34;1m");
			else if(BP->Type & DEBUG_WRITE)
				printf("\x1b[36;1m");
		}

		if(Type == 0)
		{
			CurVal = _Read9(Offset, 0);
			if(((CurVal < 0x20) && (CurVal != 9) && (CurVal != 0xa)) || (CurVal > 0x7f))
				break;
			printf("%c", CurVal);
			i++;
			Offset++;
		}
		else if(Type == 1)
		{
			CurVal = _Read9(Offset, 0);
			printf("%03x ", CurVal);
			if((CurVal >= 0x20) && (CurVal <= 0x7f))
				CharLineData[i % 0x18] = CurVal;
			else
				CharLineData[i % 0x18] = '.';
			Offset++;
		}
		else if(Type == 2)
		{
			printf("%05x ", _Read18(Offset, 0));
			Offset += 2;
		}
		else
		{
			printf("%07x ", _Read27(Offset, 0));
			Offset += 3;
		}

		if(BP)
			printf("\x1b[39;0m\x1b[39;49m");

		i += Type;
		if(i < Len)
		{
			if((i % 0x18) == 0)		//8 entries total per line assuming 3 bytes per entry
			{
				if(Type == 1)
				{
					printf("  %s", CharLineData);
					memset(CharLineData, 0, sizeof(CharLineData));
				}
				printf("\n");
				MapName = GetMapName(Offset);
				if(MapName)
					printf("%s  ", MapName);
				printf("%07x: ", Offset);
			}
			else if((i % 0xC) == 0)	//4 per side at 3 byte entries
				printf("- ");
		}
	}

	printf("\n");
}

void lowerstr(char *str)
{
	while(*str)
	{
		if((*str >= 'A') && (*str <= 'Z'))
			*str =  *str | 0x40;
		str++;
	};
}

void PrintExceptionInfo()
{
	DebugExceptionStruct *DebugExcept;
	char *FuncName;

	//if no data to parse then just exist
	if(!CPU.DebugTripped)
	{
		printf("Unable to locate exception information\n");
		return;
	}

	DebugExcept = (DebugExceptionStruct *)CPU.DebugTripped;
	printf("\x1b[31;1m%s exception at %07x", ExceptionStr[DebugExcept->ExceptionType], DebugExcept->PC);
	FuncName = GetMapName(DebugExcept->PC);
	if(FuncName)
		printf(" in function %s", FuncName);
	printf("\x1b[0m\n");

	//if an exception handler is setup then report where we will go to
	if(DebugExcept->PC != CPU_PC)
		printf("Exception execution starting at %07x\n", CPU_PC);

	//disassemble the failing instruction
	DisassembleInstructions(DebugExcept->PC, 1, 0);

	//show the next instruction to execute as single step will already cause it to trigger
	if(DebugExcept->PC != CPU_PC)
		DisassembleInstructions(CPU_PC, 1, 0);

	free(DebugExcept);
	CPU.DebugTripped = 0;
}

void PrintBPInfo()
{
	DebugBreakpointStruct *BP;
	BP = (DebugBreakpointStruct *)CPU.DebugTripped;

	if(!BP)
	{
		printf("Unable to locate breakpoint information\n");
		return;
	}

	//-1 is the dbrk instruction
	if(BP->ID == (unsigned int)-1)
	{
		printf("DBRK hit at %x\n", CPU_PC);
		CPU_PC += 2;
	}
	else
	{
		printf("Breakpoint %d hit at %x\n", BP->ID, CPU_PC);

		if(BP->Commands)
			HandleMultipleDebugCommandStr(BP->Commands);
	}
	CPU.DebugTripped = 0;
}

int HandleMultipleDebugCommandStr(char *RunCommand)
{
		char *CmdBuffer;
		char *NextCommand;
		char *Semicolon;
		char *CurCommand;
		int Ret;

		//copy the string
		CmdBuffer = strdup(RunCommand);
		CurCommand = CmdBuffer;

		//cycle until we hit the end
		Ret = 0;
		while(CurCommand[0])
		{
			//go find the beginning of the next command
			Semicolon = CurCommand;
			NextCommand = 0;
			while(Semicolon[0])
			{
				//if a semicolon then set it to null and walk past it
				if(Semicolon[0] == ';')
				{
					//advance past the semicolon
					NextCommand = Semicolon + 1;
					Semicolon[0] = 0;

					//walk backwards and remove white space
					Semicolon--;
					while(Semicolon[0] == ' ')
					{
						Semicolon[0] = 0;
						Semicolon--;
					}

					//walk past white space
					NextCommand++;
					while(NextCommand[0] == ' ')
						NextCommand++;
					break;
				}

				Semicolon++;
			}

			//if we didn't find the semicolon then just set to the end
			if(!NextCommand)
				NextCommand = Semicolon;

			//run the command we have
			printf("\n\x1b[34;1m%s\x1b[0m\n", CurCommand);
			Ret = HandleDebugCommandStr(CurCommand);

			//advance forward to the next one and process
			CurCommand = NextCommand;
		}

		DebugOut("");
		free(CmdBuffer);

		return Ret;
}

int HandleDebugCommandStr(char *RunCommand)
{
	int i;
	int DebugDone = 0;
	DebugFunc Func;
	char *NextParam;
	char *CmdBuffer;

	//make a copy so we can modify it
	CmdBuffer = strdup(RunCommand);

	//parse up the command
	NextParam = CmdBuffer;
	while(*NextParam && (*NextParam != ' '))
		NextParam++;

	//if found space then remove the space and advance
	if(*NextParam)
	{
		*NextParam = 0;
		NextParam++;
		while(*NextParam && (*NextParam == ' '))
			NextParam++;
	}

	//lowercase the string then start looking at commands
	//this should only lowercase the first entry due to the null additions above
	lowerstr(CmdBuffer);
	for(i = 0; DebugCmds[i].Cmd; i++)
	{
		//if we have a match then run it and break
		if(strcmp(DebugCmds[i].Cmd, CmdBuffer) == 0)
		{
			//if the command wants us to process then exit
			Func = DebugCmds[i].Func;
			DebugDone = Func(NextParam);
			break;
		}
	}

	if(!DebugCmds[i].Cmd)
		printf("Invalid command\n");

	free(CmdBuffer);
	return DebugDone;
}

void HandleDebugger()
{
	char *CmdBuffer = 0; //[256];
	int i;
	int DebugDone = 0;
	HIST_ENTRY *HistEntry;
	DebugExceptionStruct *ExceptData;

	//if we are doing debugging over network then do a quick check to see if the ctrl-c came across
	//this will also chew up any in data they send if we are running
	i = 0;
	if(DebugOverNetwork && !CPU.DebugTripped && (recv(0, &i, 1, MSG_DONTWAIT) == 1))
	{
		if(i == 3)
			DebugState = DEBUG_STATE_CTRLC;
	}

	//if we triggered an exception AND the trigger is not set then change our state and keep going
	if(DebugState == DEBUG_STATE_EXCEPTION)
	{
		ExceptData = (DebugExceptionStruct *)CPU.DebugTripped;
		if(!(ExceptionTriggers & (1 << ExceptData->ExceptionType)))
		{
			//trigger is not set, revert to prior which could be go or single step
			CPU.DebugTripped = 0;
			DebugState = ExceptData->PrevDebugState;
			free(ExceptData);
		}
		DebugSingleStepBP = 0;
	}

	//if debug was not tripped from breakpoint then check if we should continue
	if(!CPU.DebugTripped)
	{
		//if we were doing a breakpoint then continue normally
		if(DebugState == DEBUG_STATE_BP_HIT)
		{
			DebugState = DebugSingleStepBP;
			DebugSingleStepBP = 0;
		}

		//if we are running then continue assuming the address doesn't match
		if(DebugState == DEBUG_STATE_RUNNING)
		{
			if((DebugGoAddr == (unsigned int)-1) || (DebugGoAddr != CPU_PC))
				return;
		}

		//if stepping, keep returning for however many instructions
		if(DebugState == DEBUG_STATE_STEP)
		{
			//only adjust the step count if we are not waiting
			if(!CPU.Wait)
				DebugStepCount--;
			if(DebugStepCount > 0)
				return;
		}
	}
	else if(DebugState != DEBUG_STATE_EXCEPTION)
	{
		DebugSingleStepBP = 1;		//only way we can trip debug besides exception is breakpoint
		PrintBPInfo();
	}

	if(CPU.ExceptionTriggered)
		printf("\x1b[35;1mIn exception\x1b[0m\n");

	//if in wait mode then alert them to it
	if(CPU.Wait)
		printf("\x1b[35;1mIn Wait\x1b[0m\n");

	PrintRegisters(DEBUG_REG_NORMAL, -1);

	//if an exception then print it's details out
	if(DebugState == DEBUG_STATE_EXCEPTION)
		PrintExceptionInfo();
	else
		DisassembleInstructions(CPU_PC, 1, 0);

	//if we have a display string then handle it
	if(DebugDisplayStr)
		HandleMultipleDebugCommandStr(DebugDisplayStr);

	while(!DebugDone)
	{
		//if we have a string then free it
		if(CmdBuffer)
			free(CmdBuffer);
		CmdBuffer = readline("> ");

		//if CmdBuffer is null then the debugger was exited, tell the emulator to stop running
		if(!CmdBuffer)
		{
			CPU.Running = 0;
			break;
		}

		//if no command then get the last command
		if(strlen(CmdBuffer) == 0)
		{
			HistEntry = previous_history();
			if(HistEntry == 0)
				continue;
			i = strlen(HistEntry->line);
			free(CmdBuffer);
			CmdBuffer = malloc(i + 1);
			strcpy(CmdBuffer, HistEntry->line);
		}
		else
			add_history(CmdBuffer);

		DebugDone = HandleDebugCommandStr(CmdBuffer);
	};

	//if we had a command then free it
	if(CmdBuffer)
		free(CmdBuffer);

	//get a copy of the registers before returning control
	memcpy(&LastRegVals, &CPU.Registers, sizeof(LastRegVals));

	//if we hit a breakpoint then we need to single step an instruction then run normally
	if(DebugSingleStepBP)
	{
		DebugSingleStepBP = DebugState;
		DebugState = DEBUG_STATE_BP_HIT;
	}

	//if in wait mode then alert them to it
	if(CPU.Wait)
		printf("\x1b[35;1mReturning to wait mode\x1b[0m\n");
}

void *DebugCheckMemoryAccess(unsigned int Location, unsigned int Len, int Type)
{
	//check a list of breakpoints for the specified access
	DebugBreakpointStruct *CurBP = BreakpointListAddr;

	//if the location + len > maximum memory then we need to do 2 lookups as our breakpoints don't wrap
	if((Location + Len) > MAX_MEMORY)
	{
		//figure out the length we wrap and check it first
		int TempLen;
		void *Ret;

		//number of bytes to check at 0
		TempLen = (Location + Len) - MAX_MEMORY;
		Ret = DebugCheckMemoryAccess(0, TempLen, Type);
		if(Ret)
			return Ret;

		//didn't match, check near the top after adjusting Len;
		Len -= TempLen;
	}

	//check all breakpoints
	while(CurBP)
	{
		//if current entry is too high then stop looking as the list is sorted
		if(CurBP->Location >= (Location + Len))
			break;

		//proper type?
		if(CurBP->Type & Type)
		{
			//type matches, check location, if it matches return this BP entry
			if((CurBP->Location <= Location) && ((CurBP->Location + CurBP->Len) > Location))
				return CurBP;

			//beginning of memory access didn't trip, check end of memory access for a trip
			if((CurBP->Location < (Location + Len)) && ((CurBP->Location + CurBP->Len) > Location))
				return CurBP;
		}

		//try again
		CurBP = CurBP->NextAddr;
	}

	return 0;
}

int AddBreakpoint(unsigned int Location, unsigned int Len, int Type)
{
	DebugBreakpointStruct *NewBP;
	DebugBreakpointStruct *CurBP;
	DebugBreakpointStruct *PrevBP;

	//create new entry
	NewBP = (DebugBreakpointStruct *)malloc(sizeof(DebugBreakpointStruct));
	memset(NewBP, 0, sizeof(DebugBreakpointStruct));

	//fill in the entries
	Location &= MEMORY_MASK;
	NewBP->Location = Location;
	NewBP->Len = Len;
	NewBP->Type = Type;

	//get next ID
	CurBP = BreakpointListID;
	PrevBP = 0;
	while(CurBP)
	{
		PrevBP = CurBP;
		CurBP = CurBP->NextID;
	}

	//add to the ID list
	if(!PrevBP)
	{
		BreakpointListID = NewBP;
		NewBP->ID = 1;
	}
	else
	{
		NewBP->ID = PrevBP->ID + 1;
		PrevBP->NextID = NewBP;
	}

	//go find where in the address list to insert
	CurBP = BreakpointListAddr;
	PrevBP = 0;
	while(CurBP)
	{
		//if we are past our location stop looking
		if(CurBP->Location > Location)
			break;

		PrevBP = CurBP;
		CurBP = CurBP->NextAddr;
	};

	//we have a previous entry then update, otherwise add to the beginning
	if(!PrevBP)
	{
		//first entry in the list
		BreakpointListAddr = NewBP;
		NewBP->NextAddr = CurBP;
	}
	else
	{
		//insert between previous and current
		PrevBP->NextAddr = NewBP;
		NewBP->NextAddr = CurBP;
	}

	return NewBP->ID;
}

int RemoveBreakpoint(unsigned int ID)
{
	DebugBreakpointStruct *CurBP;
	DebugBreakpointStruct *PrevBP;

	//find the entry
	CurBP = BreakpointListID;
	PrevBP = 0;
	while(CurBP)
	{
		if(CurBP->ID == ID)
			break;

		PrevBP = CurBP;
		CurBP = CurBP->NextID;
	};

	//if not found then fail
	if(!CurBP)
	{
		DebugOut("Invalid Breakpoint ID");
		return -1;
	}

	//if a previous ID then update it's list otherwise update the main list
	if(PrevBP)
		PrevBP->NextID = CurBP->NextID;
	else
		BreakpointListID = CurBP->NextID;

	//find the entry in the address list
	CurBP = BreakpointListAddr;
	PrevBP = 0;
	while(CurBP)
	{
		if(CurBP->ID == ID)
			break;

		PrevBP = CurBP;
		CurBP = CurBP->NextAddr;
	};

	//if first in the address list then use the next address
	if(PrevBP)
		PrevBP->NextAddr = CurBP->NextAddr;
	else
		BreakpointListAddr = CurBP->NextAddr;

	//now free memory
	if(CurBP->Commands)
		free(CurBP->Commands);
	free(CurBP);
	return 0;
}
