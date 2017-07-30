//
//  cpu.h
//  cLEMENCy CPU header
//
//  Created by Lightning on 2015/12/20.
//  Copyright (c) 2017 Lightning. All rights reserved.
//

#ifndef __CLEMENCY_CPU__
#define __CLEMENCY_CPU__

#include <setjmp.h>
#include "interrupts.h"

typedef struct CPURegStruct
{
	unsigned int R[32];
	unsigned int Flags;
} CPURegStruct;

struct CPUStruct
{
	CPURegStruct Registers;
	int Wait;
	int Running;
	unsigned int Interrupts[INTERRUPT_COUNT];
	unsigned int InsideInterruptInit;
	unsigned long ExceptionTriggered;
	int BranchHit;
	unsigned long TickCount;
#ifndef NODEBUG
	int Debugging;
	void *DebugTripped;
#endif
} CPUStruct;

extern struct CPUStruct CPU;

//all of these assume 27 bit opcode, so 18bit has to be shifted
#define OPCODE			((Opcode >> 22) & 0x1F)
#define SUBOPCODE		((Opcode >> 20) & 0x03)
#define SUBOPCODE_5BIT		((Opcode >> 15) & 0x1F)
#define SUBOPCODE_4		((Opcode >>  6) & 0x03)
#define M			((Opcode >> 21) & 0x01)
#define F			((Opcode >> 20) & 0x01)
#define D			((Opcode >> 20) & 0x01)
#define rA			((Opcode >> 15) & 0x1F)
#define rA_8			((Opcode >> 17) & 0x1F)
#define rB			((Opcode >> 10) & 0x1F)
#define rC			((Opcode >>  5) & 0x1F)
#define IMMEDIATE_MATH		((Opcode >>  3) & 0x7F)
#define SIGNED			((Opcode >>  2) & 0x01)
#define USEIMMEDIATE_MATH	((Opcode >>  1) & 0x01)
#define UPDATE_FLAGS		((Opcode >>  0) & 0x01)
#define RESERVED_0		((Opcode >>  9) & 0x3F)
#define RESERVED_1		((Opcode >>  1) & 0x07)
#define RESERVED_2		((Opcode >>  1) & 0x03)
#define RESERVED_3		((Opcode >>  0) & 0x07)
#define RESERVED_4		((Opcode >>  1) & 0x1F)
#define RESERVED_7		((Opcode >>  9) & 0x03)
#define RESERVED_9		((Opcode >>  9) & 0x07)
#define RESERVED_10		((Opcode >>  0) & 0x7F)
#define RESERVED_12		((Opcode >>  0) & 0xFF)
#define RESERVED_13		((Opcode >>  0) & 0x1F)
#define RESERVED_14		((Opcode >>  1) & 0x0F)
#define RESERVED_16		((Opcode >>  9) & 0x01)

#define IMMEDIATE_2		((Opcode >>  3) & 0x7F)
#define IMMEDIATE_8		((Opcode >>  0) & 0x1FFFF)

#define SIGNEXTEND_rA		((Opcode >> 10) & 0x1F)
#define SIGNEXTEND_rB		((Opcode >>  5) & 0x1F)
#define SIGNEXTEND_RESERVED	((Opcode >>  0) & 0x1F)

#define FTI_M			((Opcode >> 19) & 0x01)
#define FTI_F			((Opcode >> 18) & 0x01)
#define FTI_rA			((Opcode >> 13) & 0x1F)
#define FTI_rB 			((Opcode >>  8) & 0x1F)

#define SMP_WRITE		((Opcode >>  9) & 0x01)
#define SMP_FLAGS   		((Opcode >>  7) & 0x03)


#define LDSTR_OPCODE		((Opcode >> 49) & 0x1F)
#define LDSTR_SUBOPCODE		((Opcode >> 47) & 0x03)
#define LDSTR_rA		((Opcode >> 42) & 0x1F)
#define LDSTR_rB		((Opcode >> 37) & 0x1F)
#define LDSTR_COUNT		((Opcode >> 32) & 0x1F)
#define LDSTR_ADJUST_RB 	((Opcode >> 30) & 0x03)
#define LDSTR_IMM		((Opcode >>  3) & 0x7FFFFFF)
#define LDSTR_RESERVED		((Opcode >>  0) & 0x03)

#define BRANCH_CALL     	((Opcode >> 21) & 0x01)
#define BRANCH_COND     	((Opcode >> 17) & 0x0F)
#define BRANCH_rA       	((Opcode >> 12) & 0x1F)
#define BRANCH_REL_OFFSET   	((Opcode >>  0) & 0x1FFFF)
#define BRANCH_ABSOLUTE 	((Opcode >> 20) & 0x01)

#define COMPARE_USEIMMEDIATE	((Opcode >> 19) & 0x01)
#define COMPARE_rA		((Opcode >> 14) & 0x1F)
#define COMPARE_rB		((Opcode >>  9) & 0x1F)
#define COMPARE_IMMEDIATE	((Opcode >>  0) & 0x3FFF)

#define CPU_FLAGS CPU.Registers.Flags
#define CPU_STACK CPU.Registers.R[29]
#define CPU_RETADDR CPU.Registers.R[30]
#define CPU_PC CPU.Registers.R[31]

#define CPU_FLAG_ZERO                 0x01
#define CPU_FLAG_CARRY                0x02
#define CPU_FLAG_OVERFLOW             0x04
#define CPU_FLAG_SIGNED               0x08

#define SET_ZERO()	CPU_FLAGS |= CPU_FLAG_ZERO
#define CLEAR_ZERO()	CPU_FLAGS &= ~CPU_FLAG_ZERO

#endif
