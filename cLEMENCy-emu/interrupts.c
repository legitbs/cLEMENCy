//
//  interrupts.c
//  cLEMENCy interrupt handler
//
//  Created by Lightning on 2015/12/20.
//  Copyright (c) 2017 Lightning. All rights reserved.
//

#include <string.h>
#include "interrupts.h"
#include "cpu.h"
#include "memory.h"
#include "exceptions.h"

int InterruptStackDirection;

void InitInterrupts()
{
    //turn off all interrupts
    memset(CPU.Interrupts, 0, sizeof(CPU.Interrupts));
    CPU.Registers.Flags &= ~INTERRUPT_MASK;
    CPU.ExceptionTriggered = 0;
    InterruptStackDirection = 0;
}

unsigned int GetInterrupt(unsigned int ID)
{
    //make sure ID is valid
    if(ID >= INTERRUPT_COUNT)
        return 0;

    return CPU.Interrupts[ID];
}

void SetInterrupt(unsigned int ID, unsigned int Location)
{
    //make sure ID is valid
    if(ID >= INTERRUPT_COUNT)
        return;

    CPU.Interrupts[ID] = Location & 0x7ffffff;
}

void FireInterrupt(unsigned int ID)
{
    //store everything to the stack
    int Reg;
    int TempStack;

    //if ID is invalid then ignore
    if(ID >= INTERRUPT_COUNT)
        return;

    //make sure wait is turned off
    CPU.Wait = 0;

    //if the interrupt is not set or interrupts disabled then ignore
    if((CPU.Interrupts[ID] == 0) || ((CPU.Registers.Flags & (1 << (ID+4))) == 0))
        return;

    //store all registers
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
            TempStack = (TempStack + 3) & MEMORY_MASK;
        }
        Write27(TempStack, CPU_FLAGS & 0xf);
        TempStack = (TempStack + 3) & MEMORY_MASK;
    }
    CPU_STACK = TempStack;
    CPU.InsideInterruptInit = 0;

    //adjust PC and indicate a branch was hit
    //we check for interrupts after any PC adjustments so what
    //is stored will be the next PC to trigger
    CPU_PC = CPU.Interrupts[ID];
    CPU.BranchHit = 1;
}

void ReturnFromInterrupt()
{
    //need to restore all registers from the stack
    int Reg;
    int TempStack;

    TempStack = CPU_STACK;
    if(InterruptStackDirection == 1)
    {
        TempStack = (TempStack - 3) & MEMORY_MASK;
        CPU_FLAGS &= ~0xf;
        CPU_FLAGS |= (Read27(TempStack) & 0xf);

        for(Reg = 31; Reg >= 0; Reg--)
        {
            TempStack = (TempStack - 3) & MEMORY_MASK;
            CPU.Registers.R[Reg] = Read27(TempStack);
        }
    }
    else
    {
        for(Reg = 0; Reg < 32; Reg++)
        {
            CPU.Registers.R[Reg] = Read27(TempStack);
            TempStack = (TempStack + 3) & MEMORY_MASK;
        }

        CPU_FLAGS &= ~0xf;
        CPU_FLAGS |= (Read27(TempStack) & 0xf);
    }

    //indicate a branch was hit so the PC isn't modified and undo any exceptions
    CPU.BranchHit = 1;
    CPU.ExceptionTriggered = 0;
}

void inline EnableInterrupts(unsigned int IDs)
{
    CPU.Registers.Flags = (CPU.Registers.Flags & 0x7ffe00f) | ((IDs << 4) & INTERRUPT_MASK);
}

void inline DisableInterrupts(unsigned int IDs)
{
   CPU.Registers.Flags = (CPU.Registers.Flags & 0x7ffe00f) | ((~IDs << 4) & INTERRUPT_MASK);
}
