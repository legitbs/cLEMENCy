//
//  exceptions.c
//  cLEMENCy exception handler
//
//  Created by Lightning on 2015/12/20.
//  Copyright (c) 2017 Lightning. All rights reserved.
//

#include <stdlib.h>
#include <signal.h>
#include <setjmp.h>
#include <unistd.h>
#include <stdio.h>
#include <readline/readline.h>
#include "exceptions.h"
#include "cpu.h"
#include "interrupts.h"
#include "memory.h"
#include "debug.h"

jmp_buf ExceptionJumpBuf;

static void ExceptionFloatingPoint(int sig)
{
	//our floating point exception handler
	RaiseException(EXCEPTION_FLOATING_POINT, 0);

	//allow execution to continue
	siglongjmp(ExceptionJumpBuf, 1);
}

static void ExceptionCtrlC(int sig)
{
#ifndef NODEBUG
	DebugState = DEBUG_STATE_CTRLC;

	if(!CPU.Debugging)
#endif
		_exit(0);

	//allow execution to continue
	siglongjmp(ExceptionJumpBuf, 1);
}

static void ExceptionSigPipe(int sig)
{
	//allow execution to continue
	//siglongjmp(ExceptionJumpBuf, 1);
	CPU.Running = 0;
	siglongjmp(ExceptionJumpBuf, 1);
}

void InitExceptions()
{
	struct sigaction sa;

	//capture floating point errors
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	sa.sa_handler = ExceptionFloatingPoint;
	sigaction(SIGFPE, &sa, 0);

	//setup the default jump location to be the exception handler
	if(sigsetjmp(ExceptionJumpBuf, 1))
	{
		//DebugOut("Exception with no handler");
		exit(-1);
	}

	//capture ctrl-c
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	sa.sa_handler = ExceptionCtrlC;
	sigaction(SIGINT, &sa, 0);

	//capture sigpipe
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	sa.sa_handler = ExceptionSigPipe;
	sigaction(SIGPIPE, &sa, 0);
}

void RaiseException(enum ExceptionEnum ExceptionType, unsigned int Value)
{
	int Reg;
	unsigned int TempStack;

	#ifndef NODEBUG
		//tell the debugger of the exception that is occurring
		struct DebugExceptionStruct *DebugExcept;

		DebugExcept = malloc(sizeof(DebugExceptionStruct));
		DebugExcept->ExceptionType = ExceptionType;
		DebugExcept->Value = Value;
		DebugExcept->RegistersStored = 0;
		DebugExcept->PC = CPU_PC;
		DebugExcept->PrevDebugState = DebugState;
		CPU.DebugTripped = (void *)DebugExcept;
		DebugState = DEBUG_STATE_EXCEPTION;
	#endif

	//if we have a memory write exception and it is during the interrupt init then we need to bail
	if((ExceptionType == EXCEPTION_MEMORY_WRITE) && CPU.InsideInterruptInit)
	{
	#ifndef NODEBUG
		printf("Exception: memory write error during interrupt firing\n");
		if(!CPU.Debugging)
	#endif
			CPU.Running = 0;
			return;
	}

	//if no interrupt then exit, don't check for invalid instruction though
	//as we want to halt but only after telling the debugger
	if(((ExceptionType == EXCEPTION_DIVIDE_BY_0) || (ExceptionType == EXCEPTION_FLOATING_POINT)) &&
	   ((!CPU.Interrupts[INTERRUPT_DIVIDE_BY_0]) || (!(CPU_FLAGS >> 4) & INTERRUPT_DIVIDE_BY_0)))
		return;
	else if((ExceptionType <= EXCEPTION_MEMORY_EXECUTE) && ((!CPU.Interrupts[INTERRUPT_MEMORY_EXCEPTION]) || (!(CPU_FLAGS >> 4) & INTERRUPT_MEMORY_EXCEPTION)))
	{
		//if it is not execute then we are fine otherwise we need to die
		if(ExceptionType != EXCEPTION_MEMORY_EXECUTE)
			return;

	#ifndef NODEBUG
		printf("Exception: memory execute at %07x\n", CPU_PC);
		if(!CPU.Debugging)
	#endif
			CPU.Running = 0;
		return;
	}
	else if((ExceptionType == EXCEPTION_INVALID_INSTRUCTION) && ((!CPU.Interrupts[INTERRUPT_INVALID_INSTRUCTION]) || !((CPU_FLAGS >> 4) & INTERRUPT_INVALID_INSTRUCTION)))
		{
			//if we have an invalid instruction then halt the processor if disabled or not handler
	#ifndef NODEBUG
			printf("Exception: invalid instruction at %07x\n", CPU_PC);

			//only turn off the cpu if we aren't debugging otherwise the emulator
			//will shutdown before anything can be investigated
			if(!CPU.Debugging)
	#endif
				CPU.Running = 0;
			return;
		}

	//check and see if we have triggered an exception. If so we also must make
	//sure we haven't triggered an exception for the same tick in-case there is a condition
	//that allows multiple exceptions within the same instruction.
	//If that occurs then only the latest will be known
	if(CPU.ExceptionTriggered && (CPU.ExceptionTriggered != CPU.TickCount))
	{
		//we are inside of an exception and triggered again, just die
#ifndef NODEBUG
		printf("Exception triggered inside of exception\n");
#endif
		CPU.Running = 0;
		return;
	}
	else
		CPU.ExceptionTriggered = CPU.TickCount;	//exception triggered on this tick

	//store all registers, there is an edge case here which i'm ok with
	//if the write fails due to memory protections then we keep moving ahead
	//this will result in the iret (assuming the area is readable) 0'ing out registers
    CPU.InsideInterruptInit = 1;
    TempStack = CPU_STACK;
    if(InterruptStackDirection == 0)
    {
        TempStack = (TempStack - 3) & MEMORY_MASK;
        Write27(TempStack, CPU_FLAGS & 0xf);
        for(Reg = 31; Reg >= 0; Reg--)
        {
            TempStack = (TempStack - 3) & MEMORY_MASK;
            Write27(TempStack, CPU.Registers.R[Reg]);
        }
    }
    else
    {
        for(Reg = 0; Reg < 32; Reg++)
        {
            Write27(TempStack, CPU.Registers.R[Reg]);
            TempStack = (CPU_STACK + 3) & MEMORY_MASK;
        }
        Write27(TempStack, CPU_FLAGS & 0xf);
        TempStack = (CPU_STACK + 3) & MEMORY_MASK;
    }
    CPU_STACK = TempStack;
    CPU.InsideInterruptInit = 0;


	//R0 is always PC that failed
	//R1 is the exception type
	//R2 is any value passed in
	CPU.Registers.R[0] = CPU_PC;
	CPU.Registers.R[1] = ExceptionType;
	CPU.Registers.R[2] = Value;

	if(ExceptionType == EXCEPTION_INVALID_INSTRUCTION)
		CPU_PC = CPU.Interrupts[INTERRUPT_INVALID_INSTRUCTION];
	else if((ExceptionType == EXCEPTION_DIVIDE_BY_0) || (ExceptionType == EXCEPTION_FLOATING_POINT))
		CPU_PC = CPU.Interrupts[INTERRUPT_DIVIDE_BY_0];
	else
		CPU_PC = CPU.Interrupts[INTERRUPT_MEMORY_EXCEPTION];

#ifndef NODEBUG
	DebugExcept->RegistersStored = 1;
#endif

	//make sure it is known that we branched
	CPU.BranchHit = 3;
}
