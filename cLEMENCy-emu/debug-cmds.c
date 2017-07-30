//
//  debug-cmds.h
//  cLEMENCy debugger commands
//
//  Created by Lightning on 2015/12/20.
//  Copyright (c) 2017 Lightning. All rights reserved.
//

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include "debug.h"
#include "debug-cmds.h"
#include "cpu.h"
#include "memory.h"
#include "exceptions.h"
#include "recordstate.h"
#include "util.h"

int GetRegNum(char *Data)
{
	int RegNum;

	//either find the register in the reg list or rXX
	//if failure then return -1
	if((Data[0] == 'r') || (Data[0] == 'R'))
	{
		if(IsNum(&Data[1]))
		{
			RegNum = atoi(&Data[1]);
			if((RegNum < 0) || (RegNum > 31))
				return -1;

			return RegNum;
		}
	}

	//check the special registers
	for(RegNum = 28; RegNum < 32; RegNum++)
	{
		if(strcasecmp(Data, RegisterStr[RegNum]) == 0)
			return RegNum;
	}

	//special value for flag register
	if(strcasecmp(Data, "fl") == 0)
		return 32;

	//default fail
	return -1;
}

char *GetParam(char **Params)
{
	char *CurChar;
	char *Begin;

	//if no params, no pointer
	if(((int)*Params == -1) || !*Params || (**Params == 0))
		return 0;

	//find a space, put a null in place and advance to the next param
	CurChar = *Params;
	Begin = *Params;

	while(*CurChar && (*CurChar != ' '))
		CurChar++;

	if(!*CurChar)
	{
		*Params = 0;
		return Begin;
	}

	//insert null and find next param after a space
	*CurChar = 0;
	CurChar++;

	while(*CurChar && (*CurChar == ' '))
		CurChar++;

	*Params = CurChar;
	return Begin;
}

long GetParamVal(char *Param, int *Failed)
{
	long CurVal;

	//attempt to handle as a register
	*Failed = 0;
	CurVal = GetRegNum(Param);
	if(CurVal == -1)
	{
		//not a register, if a hex string then interpret as such
		if(IsHex(Param))
			CurVal = strtoul(Param, 0, 16);
		else
		{
			//not a hex string, look up as a name and get it's location
			CurVal = GetMapLocation(Param);
			if(CurVal == -1)
			{
				printf("Error parsing %s\n", Param);
				*Failed = 1;
				return 0;
			}
		}
	}
	else if(CurVal == 32)
		CurVal = CPU.Registers.Flags;
	else
		CurVal = CPU.Registers.R[CurVal];
	return CurVal;
}

long GetValue(char *Param, int *Failed)
{
	unsigned int CurPos;
	int FoundMath;
	int PrevMath;
	long CurVal;
	long TotalVal;
	char Maths[] = {'+','-','*','/','%','&','|','^',0};
	char *CurParamPos;
	int BracketCount;

	//take in a parameter and split it up doing any conversions needed and returning a final result
	//try register, if it fails then get normal offset
	//note, due to previous parsing, no spaces should exist in this
	//all values are also in hex

	//we need to find a delimiter, any math operation
	//we don't adhere to order, we adhere to first operation seen
	TotalVal = 0;
	*Failed = 0;
	PrevMath = 0;
	while(*Param != 0)
	{
		//If the current position we are on starts with [ then find ] and pass to a sub function
		//note, we need our matching ] so we have to count accordingly
		BracketCount = 0;
		CurVal = 0;
		if(*Param == '[')
		{
				//go find our closing bracket
				BracketCount = 0;
				CurParamPos = Param;
				while(*CurParamPos)
				{
					if(*CurParamPos == '[')
						BracketCount++;
					else if(*CurParamPos == ']')
					{
						BracketCount--;
						if(!BracketCount)
							break;
					}
					CurParamPos++;
				}

				//if we have a bracket count left or we hit a null byte then Error
				if(BracketCount || !*CurParamPos)
				{
					*Failed = 1;
					return 0;
				}

				//remove both brackets and process
				Param++;
				*CurParamPos = 0;
				CurParamPos++;

				//go get the bracket value
				CurVal = GetValue(Param, Failed);
				if(*Failed)
					return 0;

				//now deref it
				CurVal = _Read27(CurVal, 0);
				BracketCount = 1;

				//advance after the bracket
				Param = CurParamPos;
		}

		FoundMath = 0;
		for(CurPos = 0; CurPos < strlen(Param); CurPos++)
		{
			//search all the math symbols
			for(FoundMath = 1; Maths[FoundMath-1]; FoundMath++)
			{
				if(Param[CurPos] == Maths[FoundMath-1])
					break;
			}

			//if we found a math symbol then break
			if(Maths[FoundMath-1] != 0)
				break;
			else
				FoundMath = 0;
		}

		if(FoundMath)
		{
			//figure out what we have up to this value from beginning of the string
			Param[CurPos] = 0;
			if(!BracketCount)
			{
				CurVal = GetParamVal(Param, Failed);
				if(*Failed != 0)
					return 0;
			}

			//advance to the next entry
			Param = &Param[CurPos + 1];
		}
		else
		{
			//no math found, convert what we have
			if(!BracketCount)
			{
				CurVal = GetParamVal(Param, Failed);
				if(*Failed != 0)
					return 0;
			}

			//advance to the next entry
			Param = &Param[CurPos];
		}

		//we have a value, do proper math on it based on previous math operation
		if(PrevMath == 0)
			TotalVal = CurVal;
		else if(PrevMath == 1)
			TotalVal += CurVal;
		else if(PrevMath == 2)
			TotalVal -= CurVal;
		else if(PrevMath == 3)
			TotalVal *= CurVal;
		else if(PrevMath == 4)
			TotalVal /= CurVal;
		else if(PrevMath == 5)
			TotalVal %= CurVal;
		else if(PrevMath == 6)
			TotalVal &= CurVal;
		else if(PrevMath == 7)
			TotalVal |= CurVal;
		else if(PrevMath == 8)
			TotalVal ^= CurVal;

		PrevMath = FoundMath;
	};

	return TotalVal;
}

int cmdAddBreakpoint(char *Params)
{
	char *AddrStr;
	char *LenStr;
	char *FlagStr;
	char *Extra;

	unsigned int Addr;
	unsigned int Len;
	unsigned int Flags;
	int i;
	int Failed;

	//if print help then do so
	if((int)Params == -1)
	{
		AddrStr = 0;
		LenStr = 0;
		FlagStr = 0;
		Extra = (char *)1;
	}
	else
	{
		AddrStr = GetParam(&Params);
		LenStr = GetParam(&Params);
		FlagStr = GetParam(&Params);
		Extra = GetParam(&Params);
	}

	//get address and length
	if(AddrStr)
	{
		Addr = GetValue(AddrStr, &Failed);
		if(Failed)
			return 0;
	}

	if(!LenStr)
		Len = 1;
	else
	{
		Len = GetValue(LenStr, &Failed);
		if(Failed)
			return 0;
	}

	if(!FlagStr)
		Flags = DEBUG_EXECUTE;
	else
	{
		Flags = 0;
		for(i = 0; FlagStr[i]; i++)
		{
			//if we hit 4 entries we have an issue
			if(i == 3)
			{
				//force help to show up
				AddrStr = 0;
				break;
			}
			if((FlagStr[i] == 'r') || (FlagStr[i] == 'R'))
			{
				if(Flags & DEBUG_READ)
				{
					//force help to show up
					AddrStr = 0;
					break;
				}
				Flags |= DEBUG_READ;
			}
			else if((FlagStr[i] == 'w') || (FlagStr[i] == 'W'))
			{
				if(Flags & DEBUG_WRITE)
				{
					//force help to show up
					AddrStr = 0;
					break;
				}
				Flags |= DEBUG_WRITE;
			}
			else if((FlagStr[i] == 'x') || (FlagStr[i] == 'X'))
			{
				if(Flags & DEBUG_EXECUTE)
				{
					//force help to show up
					AddrStr = 0;
					break;
				}
				Flags |= DEBUG_EXECUTE;
			}
			else
			{
				//force help to show up
				AddrStr = 0;
				break;
			}
		}
	}

	//if no address or too many params then print the help menu for adding a breakpoint
	if(!AddrStr || Extra)
	{
		DebugOut("bp address [len] [flags]");
		DebugOut("flags can be a combination of R, W, and X");
		DebugOut("If len is not specified defaults to 1");
		DebugOut("If flags is not specified defaults to X");
		return 0;
	}
	else
	{
		Flags = AddBreakpoint(Addr, Len, Flags);
		printf("Added breakpoint %d\n", Flags);
	}

	return 0;
}

int cmdDelBreakpoint(char *Params)
{
	char *IDStr = GetParam(&Params);
	char *Extra = GetParam(&Params);
	int ID;

	if(((int)Params == -1) || Extra || !*IDStr || !IsNum(IDStr))
	{
		printf("bd id\n");
		return 0;
	}

	ID = atoi(IDStr);
	RemoveBreakpoint(ID);

	return 0;
}

int cmdListBreakpoints(char *Params)
{
	DebugBreakpointStruct *CurBP;
	CurBP = BreakpointListID;
	if(!CurBP)
		printf("No breakpoints\n");
	else
		printf("ID:  Addr     Size  Flags  Commands\n");
	while(CurBP)
	{
		printf("%2d:  %07x  %4x    %c%c%c", CurBP->ID, CurBP->Location, CurBP->Len, ((CurBP->Type & DEBUG_READ)?'R':' '), ((CurBP->Type & DEBUG_WRITE)?'W':' '), ((CurBP->Type & DEBUG_EXECUTE)?'X':' '));
		if(CurBP->Commands)
			printf("    %s", CurBP->Commands);
		printf("\n");
		CurBP = CurBP->NextID;
	};
	return 0;
}

int cmdDumpMemoryString(char *Params)
{
	char *OffsetStr = GetParam(&Params);
	char *Extra = GetParam(&Params);
	int Offset;
	int Failed;

	if(((int)Params == -1) || !OffsetStr || Extra)
	{
		DebugOut("ds address");
		return 0;
	}

	//go get the offset or register to dump
	Offset = GetValue(OffsetStr, &Failed);
	if(Failed)
		return 0;

	PrintMemory(Offset, MAX_MEMORY, 0);
	return 0;
}

int cmdDumpMemoryByte(char *Params)
{
	char *OffsetStr = GetParam(&Params);
	char *LenStr = GetParam(&Params);
	char *Extra = GetParam(&Params);
	int Offset;
	int Len;
	int Failed;

	if(((int)Params == -1) || !OffsetStr || Extra || (LenStr && !IsHex(LenStr)))
	{
		DebugOut("db address [len]");
		DebugOut("If no len then defaults to 0x100");
		return 0;
	}

	//get the offset to dump
	Offset = GetValue(OffsetStr, &Failed);
	if(Failed)
		return 0;

	if(!LenStr)
		Len = 0x100;
	else
	{
		Len = GetValue(LenStr, &Failed);
		if(Failed)
			return 0;
	}

	PrintMemory(Offset, Len, 1);
	return 0;
}

int cmdDumpMemoryWord(char *Params)
{
	char *OffsetStr = GetParam(&Params);
	char *LenStr = GetParam(&Params);
	char *Extra = GetParam(&Params);
	int Offset;
	int Len;
	int Failed;

	if(((int)Params == -1) || !OffsetStr || Extra || (LenStr && !IsHex(LenStr)))
	{
		DebugOut("dw address [len]");
		DebugOut("If no len then defaults to 0x100");
		return 0;
	}

	Offset = GetValue(OffsetStr, &Failed);
	if(Failed)
		return 0;

	if(!LenStr)
		Len = 0x100;
	else
	{
		Len = GetValue(LenStr, &Failed);
		if(Failed)
			return 0;
	}

	PrintMemory(Offset, Len, 2);
	return 0;
}

int cmdDumpMemoryTri(char *Params)
{
	char *OffsetStr = GetParam(&Params);
	char *LenStr = GetParam(&Params);
	char *Extra = GetParam(&Params);
	int Offset;
	int Len;
	int Failed;

	if(((int)Params == -1) || !OffsetStr || Extra || (LenStr && !IsHex(LenStr)))
	{
		DebugOut("dt address [len]");
		DebugOut("If no len then defaults to 0x100");
		return 0;
	}

	Offset = GetValue(OffsetStr, &Failed);
	if(Failed)
		return 0;

	if(!LenStr)
		Len = 0x100;
	else
	{
		Len = GetValue(LenStr, &Failed);
		if(Failed)
			return 0;
	}

	PrintMemory(Offset, Len, 3);
	return 0;
}

int cmdGo(char *Params)
{
	//see if we have an address to step to
	char *GoAddrStr = GetParam(&Params);
	char *Extra = GetParam(&Params);
	int GoAddr;
	int Failed;

	//make sure valid
	if(((int)Params == -1) || Extra)
	{
		DebugOut("g [address]");
		DebugOut("Continue running, if address specified then run until the address");
		return 0;
	}

	//figure out what address to run to
	if(GoAddrStr)
	{
		GoAddr = GetValue(GoAddrStr, &Failed);
		if(Failed)
			return 0;
	}
	else
		GoAddr = -1;

	//continue
	DebugState = DEBUG_STATE_RUNNING;
	DebugGoAddr = GoAddr;
	return 1;
}

int cmdStep(char *Params)
{
	//see how many instructions we want to step
	char *StepStr = GetParam(&Params);
	char *Extra = GetParam(&Params);
	int StepCount;

	if(((int)Params == -1) || Extra || (StepStr && !IsNum(StepStr)))
	{
		DebugOut("t [count]");
		DebugOut("If no count then defaults to 1");
		return 0;
	}

	//if we have a step count then convert it
	if(StepStr)
		StepCount = atoi(StepStr);
	else
		StepCount = 1;

	//continue
	DebugState = DEBUG_STATE_STEP;
	DebugStepCount = StepCount;
	return 1;
}

int cmdRunTillNextInstruction(char *Params)
{
	//see if we have an address to step to
	char *Extra = GetParam(&Params);

	//make sure valid
	if(Extra)
	{
		DebugOut("p");
		DebugOut("Continue running until the next instruction is executed. Useful to step over calls");
		return 0;
	}

	//continue until the next instruction will run
	DebugState = DEBUG_STATE_RUNNING;
	DebugGoAddr = CPU_PC + SizeOfOpcode(CPU_PC);
	return 1;
}

int cmdExit(char *Params)
{
	//exit
	CPU.Running = 0;
	return 1;
}

int cmdRegisters(char *Params)
{
	char *RegStr = GetParam(&Params);
	char *ValStr = GetParam(&Params);
	char *Extra = GetParam(&Params);
	int Reg;
	int Val;
	int Failed;

	if(((int)Params == -1) || Extra || (RegStr && (GetRegNum(RegStr) == -1)))
	{
		DebugOut("r reg [newval]");
		return 0;
	}

	//if we have a register then check if it is proper otherwise print a message of usage
	if(RegStr)
		Reg = GetRegNum(RegStr);
	else
		Reg = -1;

	//if we have a value then set the register otherwise print the register
	if(ValStr)
	{
		Val = GetValue(ValStr, &Failed);
		if(Failed)
			return 0;
		if(Reg == 32)
			CPU.Registers.Flags = Val & 0x7ffffff;
		else
			CPU.Registers.R[Reg] = Val & 0x7ffffff;
	}
	else
		PrintRegisters(DEBUG_REG_NORMAL, Reg);

	return 0;
}

int cmdUnassemble(char *Params)
{
	char *AddrStr = GetParam(&Params);
	char *InstCountStr = GetParam(&Params);
	char *Extra = GetParam(&Params);
	unsigned int Addr;
	int InstCount;
	int Failed;

	if(!AddrStr)
		Addr = CPU_PC;
	else
	{
		Addr = GetValue(AddrStr, &Failed);
		if(Failed)
			return 0;
	}

	//if no instruction count then go to default of 20
	if(!InstCountStr)
		InstCount = 20;
	else
	{
		InstCount = GetValue(InstCountStr, &Failed);
		if(Failed)
			return 0;
	}

	if(((int)Params == -1) || Extra)
	{
		DebugOut("u [address] [count]");
		DebugOut("address defaults to current CPU address");
		DebugOut("count defaults to 20");
		return 0;
	}

	DisassembleInstructions(Addr, InstCount, 0);
	return 0;
}

int cmdPrintMenu(char *Params)
{
	int i;
	char *CmdStr = GetParam(&Params);
	char *Extra = GetParam(&Params);
	DebugFunc Func;

	//if we have a parameter try to find it
	if(!Extra && CmdStr)
	{
		for(i = 0; DebugCmds[i].Help; i++)
		{
			//if we have a match then run it and break
			if(strcmp(DebugCmds[i].Cmd, CmdStr) == 0)
			{
				//call the function indicating a help menu with -1
				Func = DebugCmds[i].Func;
				Func((char *)-1);
				return 0;
			}
		}
	}

	//print the menu out
	for(i = 0; DebugCmds[i].Help; i++)
		printf("%s\t%s\n", DebugCmds[i].Cmd, DebugCmds[i].Help);

	return 0;
}

unsigned int GetMemProtectBlockSize(int StartAddr, unsigned int Count)
{
	//cycle through a block until either count or end of it
	//return how many blocks we did
	unsigned int CurCount;
	unsigned int Location;
	int Flags;

	//get the mask for the current block
	Location = (StartAddr & -1023);
	if(GetMemoryProtectBit(Execute, Location))
		Flags = MEMORY_PROTECTION_EXECUTE;
	else if(GetMemoryProtectBit(Write, Location))
		Flags = MEMORY_PROTECTION_READWRITE;
	else if(GetMemoryProtectBit(Read, Location))
		Flags = MEMORY_PROTECTION_READ;
	else
		Flags = 0;

	for(CurCount = 0; CurCount < Count; CurCount++)
	{
		//if outside of actual memory then stop looking
		if((StartAddr + (1024*CurCount)) >= MAX_MEMORY)
			break;

		Location = (StartAddr & -1023) + (CurCount * 1024);

		//if a flag is set and doesn't match flags then return the number of matching blocks
		if(GetMemoryProtectBit(Execute, Location))
		{
			if(Flags != MEMORY_PROTECTION_EXECUTE)
				break;
		}
		else if(GetMemoryProtectBit(Write, Location))
		{
			if(Flags != MEMORY_PROTECTION_READWRITE)
				break;
		}
		else if(GetMemoryProtectBit(Read, Location))
		{
			if(Flags != MEMORY_PROTECTION_READ)
				break;
		}
		else if(Flags)	//no flags set, if a flag was set then stop
			break;
	}

	//print the entry
	printf("%07x - %07x  %05x  %c%c%c\n", StartAddr, StartAddr + (CurCount * 1024) - 1, CurCount,
	((Flags)?'R':'-'),
	((Flags == MEMORY_PROTECTION_READWRITE)?'W':'-'),
	((Flags == MEMORY_PROTECTION_EXECUTE)?'X':'-'));
	return CurCount;
}

int cmdMemoryProtection(char *Params)
{
	char *StartAddrStr = GetParam(&Params);
	char *NumSectionsStr = GetParam(&Params);
	char *MemFlagStr = GetParam(&Params);
	char *Extra = GetParam(&Params);
	unsigned int StartAddr;
	unsigned int NumSections;
	int MemFlag;
	int Failed;
	unsigned int i;

	if(((int)Params == -1) || Extra)
	{
		DebugOut("mp [start_address] [count] [new_flags]");
		DebugOut("Print memory from starting address");
		DebugOut("If no count then go until end of current memory area");
		DebugOut("If no start address then print all blocks");
		DebugOut("If new flags then change memory accordingly. R, W, X, N, or numeric");
		return 0;
	}

	//get the params
	if(!StartAddrStr)
		StartAddr = -1;
	else
	{
		StartAddr = GetValue(StartAddrStr, &Failed);
		if(Failed)
			return 0;
	}

	if(!NumSectionsStr)
		NumSections = (unsigned int)-1;
	else
	{
		NumSections = GetValue(NumSectionsStr, &Failed);
		if(Failed)
			return 0;

		//make sure the number of sections isn't too large
		if(NumSections > ((MAX_MEMORY - StartAddr) / 1024))
		{
			DebugOut("Number of sections invalid");
			return 0;
		}
	}

	if(!MemFlagStr)
		MemFlag = -1;
	else
	{
		//if the value is numeric then convert otherwise it is characters
		if(IsNum(MemFlagStr))
			MemFlag = atoi(MemFlagStr) & 0x3;
		else
		{
			//go figure out which flag they want
			if(((MemFlagStr[0] == 'r') || (MemFlagStr[0] == 'R')) && !MemFlagStr[1])
				MemFlag = MEMORY_PROTECTION_READ;
			else if(((MemFlagStr[0] == 'w') || (MemFlagStr[0] == 'W')) && !MemFlagStr[1])
				MemFlag = MEMORY_PROTECTION_READWRITE;
			else if(((MemFlagStr[0] == 'x') || (MemFlagStr[0] == 'X')) && !MemFlagStr[1])
				MemFlag = MEMORY_PROTECTION_EXECUTE;
			else if(((MemFlagStr[0] == 'n') || (MemFlagStr[0] == 'N')) && !MemFlagStr[1])
				MemFlag = MEMORY_PROTECTION_NONE;
			else
			{
				DebugOut("Invalid flag, expecting R, W, X, or N");
				return 0;
			}
		}
	}

	//see if we need to loop over all or not
	if(MemFlag == -1)
	{
		if(StartAddr == (unsigned int)-1)
		{
			for(StartAddr = 0; StartAddr < MAX_MEMORY; StartAddr += (1024*NumSections))
				NumSections = GetMemProtectBlockSize(StartAddr, -1);

		}
		else
			NumSections = GetMemProtectBlockSize(StartAddr, NumSections);

	}
	else
	{
		for(i = 0; i < NumSections; i++)
			SetMemoryProtection((StartAddr + (i * 1024)) & 0xfffffc00, MemFlag);
	}
	return 0;
}

int cmdExceptionReset(char *Params)
{
	char *Extra = GetParam(&Params);

	if((int)Params == -1 || Extra)
	{
		DebugOut("er");
		DebugOut("Reset the internal exception flag. This will not modify any stack or register values");
		return 0;
	}

	CPU.ExceptionTriggered = 0;
	return 0;
}

int cmdExceptions(char *Params)
{
	char *ExceptionIDStr = GetParam(&Params);
	char *Extra = GetParam(&Params);
	unsigned int ExceptionID;
	char TabChar;
	char TabChar2;

	if(((int)Params == -1) || Extra || (ExceptionIDStr && !IsNum(ExceptionIDStr)))
	{
		DebugOut("e [id]");
		DebugOut("Print exceptions and if they will currently trigger the debugger");
		DebugOut("Specifying an ID will flip the exception between on and off");
		return 0;
	}

	//get the params
	if(!ExceptionIDStr)
	{
		for(ExceptionID = 0; ExceptionID < EXCEPTION_COUNT; ExceptionID++)
		{
			TabChar = '\t';
			TabChar2 = 0;
			if(ExceptionID == 3)
				TabChar = 0;
			else if(ExceptionID == 6)
				TabChar2 = '\t';
			printf("%d\t%s\t%c%c%s\n", ExceptionID + 1, ExceptionStr[ExceptionID], TabChar, TabChar2, (ExceptionTriggers & (1 << ExceptionID)) ? "on" : "off");
		}
	}
	else
	{
		ExceptionID = atoi(ExceptionIDStr) - 1;
		if(ExceptionID >= EXCEPTION_COUNT)
			printf("Invalid exception id\n");
		else
		{
			ExceptionTriggers ^= (1 << ExceptionID);
			printf("%s %s\n", ExceptionStr[ExceptionID], (ExceptionTriggers & (1 << ExceptionID)) ? "on" : "off");
		}
	}

	return 0;
}

int cmdWriteByte(char *Params)
{
	char *AddrStr = GetParam(&Params);
	char *ValStr = GetParam(&Params);
	int Addr;
	int Val;
	int Failed;

	if(((int)Params == -1) || !AddrStr || !ValStr)
	{
		DebugOut("wb address newval");
		DebugOut("multiple space delimited values can be provided for more bytes to write");
		DebugOut("memory protections are ignored");
		return 0;
	}

	//Get the address to start with
	Addr = GetValue(AddrStr, &Failed);
	if(Failed)
		return 0;

	while(ValStr)
	{
		Val = GetValue(ValStr, &Failed) & 0x1ff;
		if(Failed)
			return 0;

		//write the byte
		_Write9(Addr, Val, 0);
		Addr = (Addr + 1) & MAX_MEMORY;

		//get next entry
		ValStr = GetParam(&Params);
	};

	return 0;
}

int cmdWriteWord(char *Params)
{
	char *AddrStr = GetParam(&Params);
	char *ValStr = GetParam(&Params);
	int Addr;
	int Val;
	int Failed;

	if(((int)Params == -1) || !AddrStr || !ValStr)
	{
		DebugOut("ww address newval");
		DebugOut("multiple space delimited values can be provided for more words to write");
		DebugOut("memory protections are ignored");
		return 0;
	}

	//Get the address to start with
	Addr = GetValue(AddrStr, &Failed);
	if(Failed)
		return 0;

	while(ValStr)
	{
		Val = GetValue(ValStr, &Failed) & 0x3ffff;
		if(Failed)
			return 0;

		//write the byte
		_Write18(Addr, Val, 0);
		Addr = (Addr + 2) & MAX_MEMORY;

		//get next entry
		ValStr = GetParam(&Params);
	};

	return 0;
}

int cmdWriteTri(char *Params)
{
	char *AddrStr = GetParam(&Params);
	char *ValStr = GetParam(&Params);
	int Addr;
	int Val;
	int Failed;

	if(((int)Params == -1) || !AddrStr || !ValStr)
	{
		DebugOut("wt address newval");
		DebugOut("multiple space delimited values can be provided for more tris to write");
		DebugOut("memory protections are ignored");
		return 0;
	}

	//Get the address to start with
	Addr = GetValue(AddrStr, &Failed);
	if(Failed)
		return 0;

	while(ValStr)
	{
		Val = GetValue(ValStr, &Failed) & 0x7ffffff;
		if(Failed)
			return 0;

		//write the byte
		_Write27(Addr, Val, 0);
		Addr = (Addr + 3) & MAX_MEMORY;

		//get next entry
		ValStr = GetParam(&Params);
	};

	return 0;
}

int cmdSetDisplay(char *Params)
{
	if((int)Params == -1)
	{
		DebugOut("dis commands");
		DebugOut("multiple commands can be seperated with a semicolon");
		DebugOut("- is used to clear the list of commands");
		return 0;
	}

	//if no new value then erase what we have
	if(strlen(Params) == 0)
	{
		if(!DebugDisplayStr)
			DebugOut("no commands set");
		else
			DebugOut(DebugDisplayStr);
	}
	else if((Params[0] == '-') && (Params[1] == 0))
	{
		if(DebugDisplayStr)
			free(DebugDisplayStr);
		DebugDisplayStr = 0;
		DebugOut("display list cleared");
	}
	else
	{
		if(DebugDisplayStr)
			free(DebugDisplayStr);

		DebugDisplayStr = strdup(Params);	//setup dispaly strin
	}
	return 0;
}

int cmdBreakpointCommand(char *Params)
{
	char *IDStr = GetParam(&Params);
	unsigned int ID;
	DebugBreakpointStruct *CurBP;
	DebugBreakpointStruct *PrevBP;

	if(((int)Params == -1) || !IDStr || !IsNum(IDStr))
	{
		DebugOut("bpc id commands");
		DebugOut("multiple commands can be seperated with a semicolon");
		DebugOut("- is used to clear the list of commands");
		return 0;
	}

	//get the id
	ID = atoi(IDStr);

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

	//if no new value then erase what we have
	if(strlen(Params) == 0)
	{
		if(!CurBP->Commands)
			DebugOut("no commands set");
		else
			DebugOut(CurBP->Commands);
	}
	else if((Params[0] == '-') && (Params[1] == 0))
	{
		if(CurBP->Commands)
			free(CurBP->Commands);
		CurBP->Commands = 0;
		DebugOut("command list cleared");
	}
	else
	{
		if(CurBP->Commands)
			free(CurBP->Commands);

		CurBP->Commands = strdup(Params);	//setup dispaly strin
	}
	return 0;
}

#ifndef TEAMBUILD
int cmdHistorySize(char *Params)
{
	char *CountStr = GetParam(&Params);
	char *Extra = GetParam(&Params);
	int Count;

	Count = 0;
	if(CountStr && IsNum(CountStr))
		Count = atoi(CountStr);

	if(((int)Params == -1) || Extra || (CountStr && (!IsNum(CountStr) || (Count < 0))))
	{
		DebugOut("hs count");
		DebugOut("sets how many entries to store, changing the value removes all history");
		return 0;
	}

	//if no count then tell how many
	if(!CountStr)
	{
		printf("record states: %d\n", GetRecordCount());
		return 0;
	}

	//set it up
	if(!SetRecordCount(Count))
		DebugOut("failed to set backtrace count");

	return 0;
}

int cmdHistory(char *Params)
{
	char *CountStr = GetParam(&Params);
	char *Extra = GetParam(&Params);
	int Count;

	Count = 0;
	if(CountStr && IsNum(CountStr))
		Count = atoi(CountStr);

	if(((int)Params == -1) || Extra || (CountStr && (!IsNum(CountStr) || (Count < 0))))
	{
		DebugOut("h count");
		DebugOut("display execution history up to count");
		return 0;
	}

	//if no value then get default
	if(!CountStr)
		Count = GetRecordCount();

	DisplayRecordStates(Count);
	return 0;
}
#endif

int cmdPrintAbout(char *Param)
{
	//print out a quick list of people
	printf("           cLEMENCy Emulator\n\n");
	printf("Author:\n  Lightning\n\n");
	printf("Team:\n");
	printf("  Deadwood       Fuzyll         Gynophage\n");
	printf("  HJ             Jymbolia       Selir\n");
	printf("  Sirgoon        Vito           Thing2\n");
	printf("Support Staff:\n  Aedegon9       Jetboy\n\n");
	printf("Emotional Support:\n  Phoenix Kiana  ASK\n");
	return 0;
}

DebugCmdsStruct DebugCmds[] =
{
	{"bp", cmdAddBreakpoint, "Add a breakpoint"},
	{"bd", cmdDelBreakpoint, "Delete a breakpoint"},
	{"bl", cmdListBreakpoints, "List breakpoints"},
	{"bpc", cmdBreakpointCommand, "Add a command to a breakpoint"},
	{"dis", cmdSetDisplay, "Set commands to run when debug pauses execution"},
	{"db", cmdDumpMemoryByte, "Dump memory in bytes"},
	{"dw", cmdDumpMemoryWord, "Dump memory in words"},
	{"dt", cmdDumpMemoryTri, "Dump memory in tris"},
	{"ds", cmdDumpMemoryString, "Dump memory as ascii string"},
	{"e", cmdExceptions, "View or set exceptions to trigger on"},
	{"er", cmdExceptionReset, "Exception Reset"},
	{"g", cmdGo, "Go, continue execution"},
#ifndef TEAMBUILD
	{"h", cmdHistory, "Display execution history"},
	{"hs", cmdHistorySize, "Set execution history size"},
#endif
	{"mp", cmdMemoryProtection, "Display or set memory protections"},
	{"p", cmdRunTillNextInstruction, "Run till next instruction"},
	{"r", cmdRegisters, "Print or set registers"},
	{"t", cmdStep, "Single step"},
	{"u", cmdUnassemble, "Unassemble"},
	{"wb", cmdWriteByte, "Write bytes"},
	{"ww", cmdWriteWord, "Write words"},
	{"wt", cmdWriteTri, "Write tris"},
	{"q", cmdExit, "Exit"},
	{"?", cmdPrintMenu, "Print info about a command"},
	{"about", cmdPrintAbout, 0},
	{0, 0, 0}
};
