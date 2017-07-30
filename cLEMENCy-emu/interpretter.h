//
//  interpretter.h
//  cLEMENCy - LEgitbs Middle ENdian Computer
//
//  Created by Lightning on 2017/11/21.
//  Copyright (c) 2017 Lightning. All rights reserved.
//

#ifndef __CLEMENCY__interpretter__
#define __CLEMENCY__interpretter__

#include <stdio.h>
#include "cpu.h"

void InitInterpretter();
int Interpretter_RunInstruction();
unsigned int CPU_Random();

#define MATH_INSTRUCTION_MACRO(CASENUM, OPERATION, FLAGTYPE)    \
case(CASENUM):                                                  \
    if(F)                                                       \
    {                                                           \
        OpcodeSize = 0;                                         \
        RaiseException(EXCEPTION_INVALID_INSTRUCTION, 0);       \
    }                                                           \
    else if(M)                                                  \
    {                                                           \
        rBM.l = (((unsigned long)CPU.Registers.R[rB]) << 27) | CPU.Registers.R[(rB+1) & 0x1f];  \
    	if(USEIMMEDIATE_MATH)						\
    		rCM.l = IMMEDIATE_MATH;					\
    	else								\
            rCM.l = (((unsigned long)CPU.Registers.R[rC]) << 27) | CPU.Registers.R[(rC+1) & 0x1f];  \
        rAM.l = rBM.l OPERATION rCM.l;                          \
        CPU.Registers.R[rA] = (rAM.l & 0x003FFFFFF8000000) >> 27;     \
        CPU.Registers.R[(rA + 1) & 0x1f] = (rAM.l & MEMORY_MASK);     \
        SET_FLAGS_MULTI_  ## FLAGTYPE(rAM.l, rBM.l, rCM.l);         \
    }                                       \
    else                                    \
    {                                       \
        rBS.i = CPU.Registers.R[rB];          \
    	if(USEIMMEDIATE_MATH)						\
    		rCS.i = IMMEDIATE_MATH;					\
    	else								\
		      rCS.i = CPU.Registers.R[rC];	        \
        rAS.i = rBS.i OPERATION rCS.i;              \
        CPU.Registers.R[rA] = rAS.i & MEMORY_MASK;     \
        SET_FLAGS_ ## FLAGTYPE(rAS.i, rBS.i, rCS.i); \
    }                                                \
    break;

#define MATH_INSTRUCTION_WITH_SIGNED_MACRO(CASENUM, OPERATION, FLAGTYPE)    \
case(CASENUM):                                                  \
    if(F)                                                       \
    {                                                           \
        OpcodeSize = 0;                                         \
        RaiseException(EXCEPTION_INVALID_INSTRUCTION, 0);       \
    }                                                           \
    else if(M)                                                  \
    {                                                           \
        rBM.l = (((unsigned long)CPU.Registers.R[rB]) << 27) | CPU.Registers.R[(rB+1) & 0x1f];  \
    	if(USEIMMEDIATE_MATH)						    \
        {                                               \
    		rCM.l = IMMEDIATE_MATH;                     \
            if(SIGNED)                                  \
                rCM.l = (((long)rCM.l << 57) >> 57);	\
        }                                               \
    	else								\
        	rCM.l = (((unsigned long)CPU.Registers.R[rC]) << 27) | CPU.Registers.R[(rC+1) & 0x1f];  \
        if(SIGNED)					\
	    {						\
    		rBM.l = (((long)rBM.l << 10) >> 10);	\
    		rCM.l = (((long)rCM.l << 10) >> 10);	\
            rAM.l = ((long)rBM.l) OPERATION ((long)rCM.l);    \
	    }                                               \
        else						\
            rAM.l = rBM.l OPERATION rCM.l;                          \
        CPU.Registers.R[rA] = (rAM.l & 0x003FFFFFF8000000) >> 27;     \
        CPU.Registers.R[(rA + 1) & 0x1f] = (rAM.l & MEMORY_MASK);     \
        SET_FLAGS_MULTI_  ## FLAGTYPE(rAM.l, rBM.l, rCM.l);         \
    }                                       \
    else                                    \
    {                                       \
        rBS.i = CPU.Registers.R[rB];          \
    	if(USEIMMEDIATE_MATH)						\
        {                                               \
    		rCS.i = IMMEDIATE_MATH;                     \
            if(SIGNED)                                  \
                rCS.i = (((int)rCS.i << 25) >> 25);	\
        }                                               \
    	else								\
    		rCS.i = CPU.Registers.R[rC];	        \
        if(SIGNED)					\
	    {						\
    		rBS.i = (((int)rBS.i << 5) >> 5);	\
    		rCS.i = (((int)rCS.i << 5) >> 5);	\
            rAS.i = ((int)rBS.i) OPERATION ((int)rCS.i);          \
        }						\
        else                                            \
            rAS.i = rBS.i OPERATION rCS.i;              \
        CPU.Registers.R[rA] = rAS.i & MEMORY_MASK;     \
        SET_FLAGS_ ## FLAGTYPE(rAS.i, rBS.i, rCS.i); \
    }                                                \
    break;

#define MATH_INSTRUCTION_DIV_MACRO(CASENUM, OPERATION, FLAGTYPE)    \
case(CASENUM):                                                  \
    if(F)                                                       \
    {                                                           \
        OpcodeSize = 0;                                         \
        RaiseException(EXCEPTION_INVALID_INSTRUCTION, 0);       \
    }                                                           \
    else if(M)                                                  \
    {                                                           \
        rBM.l = (((unsigned long)CPU.Registers.R[rB]) << 27) | CPU.Registers.R[(rB+1) & 0x1f];  \
    	if(USEIMMEDIATE_MATH)						\
    		rCM.l = IMMEDIATE_MATH;					\
    	else								\
        	rCM.l = (((unsigned long)CPU.Registers.R[rC]) << 27) | CPU.Registers.R[(rC+1) & 0x1f];  \
        if(rCM.l == 0)                                  \
        {                                               \
            RaiseException(EXCEPTION_DIVIDE_BY_0, 0);   \
            break;                                      \
        }                                               \
        if(SIGNED)					\
    	{						\
    		rBM.l = (((long)rBM.l << 10) >> 10);	\
    		rCM.l = (((long)rCM.l << 10) >> 10);	\
            rAM.l = ((long)rBM.l) OPERATION ((long)rCM.l);    \
	    }                                               \
        else						\
            rAM.l = rBM.l OPERATION rCM.l;                          \
        CPU.Registers.R[rA] = (rAM.l & 0x003FFFFFF8000000) >> 27;     \
        CPU.Registers.R[(rA + 1) & 0x1f] = (rAM.l & MEMORY_MASK);     \
        SET_FLAGS_MULTI_  ## FLAGTYPE(rAM.l, rBM.l, rCM.l);         \
    }                                       \
    else                                    \
    {                                       \
        rBS.i = CPU.Registers.R[rB];          \
    	if(USEIMMEDIATE_MATH)						\
    		rCS.i = IMMEDIATE_MATH;					\
    	else								\
    		rCS.i = CPU.Registers.R[rC];	        \
        if(rCS.i == 0)                                  \
        {                                               \
            RaiseException(EXCEPTION_DIVIDE_BY_0, 0);   \
            break;                                      \
        }                                               \
        if(SIGNED)					\
	    {						\
    		rBS.i = (((int)rBS.i << 5) >> 5);	\
    		rCS.i = (((int)rCS.i << 5) >> 5);	\
            rAS.i = ((int)rBS.i) OPERATION ((int)rCS.i);          \
    	}						\
        else                                            \
            rAS.i = rBS.i OPERATION rCS.i;              \
        CPU.Registers.R[rA] = rAS.i & MEMORY_MASK;     \
        SET_FLAGS_ ## FLAGTYPE(rAS.i, rBS.i, rCS.i); \
    }                                                \
    break;

#define MATH_INSTRUCTION_WITH_SIGNED_MACRO_DIV(CASENUM, OPERATION, FLAGTYPE)    \
case(CASENUM):                                                  \
    if(F)                                                       \
    {                                                           \
        OpcodeSize = 0;                                         \
        RaiseException(EXCEPTION_INVALID_INSTRUCTION, 0);       \
    }                                                           \
    else if(M)                                                  \
    {                                                           \
        rBM.l = (((unsigned long)CPU.Registers.R[rB]) << 27) | CPU.Registers.R[(rB+1) & 0x1f];  \
    	if(USEIMMEDIATE_MATH)						    \
        {                                               \
    		rCM.l = IMMEDIATE_MATH;                     \
            if(SIGNED)                                  \
                rCM.l = (((long)rCM.l << 57) >> 57);	\
        }                                               \
    	else								\
        	rCM.l = (((unsigned long)CPU.Registers.R[rC]) << 27) | CPU.Registers.R[(rC+1) & 0x1f];  \
        if(rCM.l == 0)                                  \
        {                                               \
            RaiseException(EXCEPTION_DIVIDE_BY_0, 0);   \
            break;                                      \
        }                                               \
        if(SIGNED)					\
	    {						\
    		rBM.l = (((long)rBM.l << 10) >> 10);	\
    		rCM.l = (((long)rCM.l << 10) >> 10);	\
            rAM.l = ((long)rBM.l) OPERATION ((long)rCM.l);    \
	    }                                               \
        else						\
            rAM.l = rBM.l OPERATION rCM.l;                          \
        CPU.Registers.R[rA] = (rAM.l & 0x003FFFFFF8000000) >> 27;     \
        CPU.Registers.R[(rA + 1) & 0x1f] = (rAM.l & MEMORY_MASK);     \
        SET_FLAGS_MULTI_  ## FLAGTYPE(rAM.l, rBM.l, rCM.l);         \
    }                                       \
    else                                    \
    {                                       \
        rBS.i = CPU.Registers.R[rB];          \
    	if(USEIMMEDIATE_MATH)						\
        {                                               \
    		rCS.i = IMMEDIATE_MATH;                     \
            if(SIGNED)                                  \
                rCS.i = (((int)rCS.i << 25) >> 25);	\
        }                                               \
    	else								\
    		rCS.i = CPU.Registers.R[rC];	        \
        if(rCS.i == 0)                                  \
        {                                               \
            RaiseException(EXCEPTION_DIVIDE_BY_0, 0);   \
            break;                                      \
        }                                               \
        if(SIGNED)					\
    	{						\
    		rBS.i = (((int)rBS.i << 5) >> 5);	\
    		rCS.i = (((int)rCS.i << 5) >> 5);	\
            rAS.i = ((int)rBS.i) OPERATION ((int)rCS.i);          \
    	}						\
        else                                            \
            rAS.i = rBS.i OPERATION rCS.i;              \
        CPU.Registers.R[rA] = rAS.i & MEMORY_MASK;     \
        SET_FLAGS_ ## FLAGTYPE(rAS.i, rBS.i, rCS.i); \
    }                                                \
    break;

#define MATH_INSTRUCTION_CARRY_MACRO(CASENUM, OPERATION, FLAGTYPE)        \
case(CASENUM):                                                  \
    if(F)                                                       \
    {                                                           \
        OpcodeSize = 0;                                         \
        RaiseException(EXCEPTION_INVALID_INSTRUCTION, 0);       \
    }                                                           \
    else if(M)                                                  \
    {                                                           \
        rBM.l = (((unsigned long)CPU.Registers.R[rB]) << 27) | CPU.Registers.R[(rB+1) & 0x1f];  \
	if(USEIMMEDIATE_MATH)						\
		rCM.l = IMMEDIATE_MATH;					\
	else								\
        	rCM.l = (((unsigned long)CPU.Registers.R[rC]) << 27) | CPU.Registers.R[(rC+1) & 0x1f];  \
            /* multi-reg */                                     \
            rAM.l = rBM.l OPERATION rCM.l OPERATION ((CPU_FLAGS & CPU_FLAG_CARRY) != 0);                              \
            CPU.Registers.R[rA] = (rAM.l & 0x003FFFFFF8000000) >> 27;     \
            CPU.Registers.R[(rA + 1) & 0x1f] = (rAM.l & MEMORY_MASK);     \
            SET_FLAGS_MULTI_ ## FLAGTYPE(rAM.l, rBM.l, rCM.l);          \
    }                                       \
    else                                    \
    {                                       \
        rBS.i = CPU.Registers.R[rB];          \
	if(USEIMMEDIATE_MATH)						\
		rCS.i = IMMEDIATE_MATH;					\
	else								\
		rCS.i = CPU.Registers.R[rC];	     \
        rAS.i = rBS.i OPERATION rCS.i OPERATION ((CPU_FLAGS & CPU_FLAG_CARRY) != 0); 		     \
        CPU.Registers.R[rA] = rAS.i & MEMORY_MASK;     \
        SET_FLAGS_ ## FLAGTYPE(rAS.i, rBS.i, rCS.i); \
    }                                                \
    break;

#define MATH_INSTRUCTION_INT_MACRO(CASENUM, OPERATION)          \
case(CASENUM):                                                  \
    if(M)                                                       \
    {                                                           \
        /* multi-reg */                                         \
        rBM.l = (((unsigned long)CPU.Registers.R[rB]) << 27) | CPU.Registers.R[(rB+1) & 0x1f];  \
	if(USEIMMEDIATE_MATH)						\
		rCM.l = IMMEDIATE_MATH;					\
	else								\
        	rCM.l = (((unsigned long)CPU.Registers.R[rC]) << 27) | CPU.Registers.R[(rC+1) & 0x1f];  \
        rAM.l = rBM.l OPERATION rCM.l;                              \
        CPU.Registers.R[rA] = (rAM.l & 0x003FFFFFF8000000) >> 27;     \
        CPU.Registers.R[(rA + 1) & 0x1f] = (rAM.l & MEMORY_MASK);     \
        SET_FLAGS_MULTI_ADD(rAM.l, rBM.l, rCM.l);                   \
    }                                                               \
    else                                                            \
    {                                           \
        rBS.i = CPU.Registers.R[rB];              \
	if(USEIMMEDIATE_MATH)						\
		rCS.i = IMMEDIATE_MATH;					\
	else								\
		rCS.i = CPU.Registers.R[rC];				\
        rAS.i = rBS.i OPERATION rCS.i;                      \
        CPU.Registers.R[rA] = rAS.i & MEMORY_MASK;            \
        SET_FLAGS_ADD(rAS.i, rBS.i, rCS.i);                 \
    }                                                       \
    break;

#define MATH_INSTRUCTION_INT_SHIFT_MACRO(OPERATION)    \
{                                                               \
    if(M)                                                       \
    {                                                           \
        /* multi-reg */                                         \
        rBM.l = (((unsigned long)CPU.Registers.R[rB]) << 27) | CPU.Registers.R[(rB+1) & 0x1f];  \
        rCM.l = (((unsigned long)CPU.Registers.R[rC]) << 27) | CPU.Registers.R[(rC+1) & 0x1f];  \
        rAM.l = rBM.l OPERATION rCM.l;                              \
        CPU.Registers.R[rA] = (rAM.l & 0x003FFFFFF8000000) >> 27;     \
        CPU.Registers.R[(rA + 1) & 0x1f] = (rAM.l & MEMORY_MASK);     \
        SET_FLAGS_MULTI_ADD(rAM.l, rBM.l, rBM.l);                   \
    }                                                               \
    else                                                            \
    {                                           \
        rBS.i = CPU.Registers.R[rB];              \
        rAS.i = rBS.i OPERATION CPU.Registers.R[rC];     \
        CPU.Registers.R[rA] = rAS.i & MEMORY_MASK;       \
        SET_FLAGS_ADD(rAS.i, rBS.i, rBS.i);     \
    }                                           \
}

#define MATH_INSTRUCTION_INT_IMM2_SHIFT_MACRO(OPERATION)        \
{                                                               \
    if(M)                                                       \
    {                                                           \
        /* multi-reg */                                         \
        rBM.l = (((unsigned long)CPU.Registers.R[rB]) << 27) | CPU.Registers.R[(rB+1) & 0x1f];  \
        rAM.l = rBM.l OPERATION IMMEDIATE_2;                        \
        CPU.Registers.R[rA] = (rAM.l & 0x003FFFFFF8000000) >> 27;     \
        CPU.Registers.R[(rA + 1) & 0x1f] = (rAM.l & MEMORY_MASK);     \
        SET_FLAGS_MULTI_ADD(rAM.l, rBM.l, rBM.l);                   \
    }                                                               \
    else                                                            \
    {                                                  \
        rBS.i = CPU.Registers.R[rB];                     \
        rAS.i = rBS.i OPERATION IMMEDIATE_2;           \
        CPU.Registers.R[rA] = rAS.i & MEMORY_MASK;       \
        SET_FLAGS_ADD(rAS.i, rBS.i, rBS.i);            \
    }                                                  \
}

#define MATH_INSTRUCTION_REGA_MACRO(OPERATION)                      \
{                                                                   \
    if(FTI_M)                                                       \
    {                                                               \
        /* multi-reg */                                             \
        rBM.l = (((unsigned long)CPU.Registers.R[FTI_rB]) << 27) | CPU.Registers.R[(FTI_rB+1) & 0x1f];  \
        rAM.l = OPERATION(rBM.l);                                   \
        CPU.Registers.R[FTI_rA] = (rAM.l & 0x003FFFFFF8000000) >> 27; \
        CPU.Registers.R[(FTI_rA + 1) & 0x1f] = (rAM.l & MEMORY_MASK); \
        SET_FLAGS_MULTI_ADD(rAM.l, rBM.l, rBM.l);                   \
    }                                                               \
    else                                                            \
    {                                                  \
        rBS.i = CPU.Registers.R[FTI_rB];                 \
        rAS.i = OPERATION(rBS.i);                      \
        CPU.Registers.R[FTI_rA] = rAS.i & MEMORY_MASK;   \
        SET_FLAGS_ADD(rAS.i, rBS.i, rBS.i);            \
    }                                                  \
}

#define LOAD_INSTRUCTION_PRE(LOADTYPE)                                     \
    rAS.i = LDSTR_rA;                                                      \
    rBS.i = CPU.Registers.R[LDSTR_rB];                                       \
    rCS.i = LDSTR_COUNT + 1;                                               \
    ImmediateVal = LDSTR_IMM;                                              \
    rBS.i = (rBS.i + ImmediateVal) & MEMORY_MASK;                          \
    Temp = CPU_PC;                                                         \
    if(LDSTR_ADJUST_RB == 2)                                               \
    {                                                                      \
        rBS.i -= rCS.i * (LOADTYPE / 9);                                   \
        rBS.i &= MEMORY_MASK;                                              \
        Temp2 = rBS.i;                                                     \
    }

#define LOAD_INSTRUCTION_POST(LOADTYPE)                                    \
    for(;rCS.i; rCS.i--)                                                   \
    {                                                                      \
        CPU.Registers.R[rAS.i] = Read##LOADTYPE(rBS.i);                      \
        rAS.i = (rAS.i + 1) & 0x3f;                                        \
        rBS.i = (rBS.i + (LOADTYPE / 9));                                  \
    };                                                                     \
    CPU_PC = Temp;                                                         \
    if(LDSTR_ADJUST_RB == 1)                                               \
        CPU.Registers.R[LDSTR_rB] = rBS.i & MEMORY_MASK;                   \
    else if(LDSTR_ADJUST_RB == 2)                                          \
        CPU.Registers.R[LDSTR_rB] = Temp2;

#ifdef NODEBUG
#define LOAD_INSTRUCTION(LOADTYPE) {                                       \
LOAD_INSTRUCTION_PRE(LOADTYPE);                                            \
LOAD_INSTRUCTION_POST(LOADTYPE); }
#else
#define LOAD_INSTRUCTION(LOADTYPE) {                                       \
LOAD_INSTRUCTION_PRE(LOADTYPE);                                            \
if(CPU.Debugging && (DebugState != DEBUG_STATE_BP_HIT))                    \
{                                                                          \
    CPU.DebugTripped = DebugCheckMemoryAccess(rBS.i, 3*rCS.i, DEBUG_READ); \
    if(CPU.DebugTripped)                                                   \
        break;                                                             \
}                                                                          \
LOAD_INSTRUCTION_POST(LOADTYPE); }
#endif

#define STORE_INSTRUCTION_PRE(STORETYPE)                                   \
    rAS.i = LDSTR_rA;                                                      \
    rBS.i = CPU.Registers.R[LDSTR_rB];                                     \
    rCS.i = LDSTR_COUNT + 1;                                               \
    ImmediateVal = LDSTR_IMM;                                              \
    rBS.i = (rBS.i + ImmediateVal) & MEMORY_MASK;                          \
    if(LDSTR_ADJUST_RB == 2)                                               \
    {                                                                      \
        rBS.i -= rCS.i * (STORETYPE / 9);                                  \
        rBS.i &= MEMORY_MASK;                                              \
        Temp = rBS.i;                                                      \
    }

#define STORE_INSTRUCTION_POST(STORETYPE)                                  \
    for(;rCS.i; rCS.i--)                                                   \
    {                                                                      \
        Write##STORETYPE(rBS.i, CPU.Registers.R[rAS.i]);      \
        rAS.i = (rAS.i + 1) & 0x3f;                                        \
        rBS.i = (rBS.i + (STORETYPE / 9));                                 \
    };                                                                     \
    if(LDSTR_ADJUST_RB == 1)                                               \
        CPU.Registers.R[LDSTR_rB] = rBS.i;                                   \
    else if(LDSTR_ADJUST_RB == 2)                                          \
        CPU.Registers.R[LDSTR_rB] = Temp;

#ifdef NODEBUG
#define STORE_INSTRUCTION(STORETYPE) {                                      \
STORE_INSTRUCTION_PRE(STORETYPE);                                           \
STORE_INSTRUCTION_POST(STORETYPE); }
#else
#define STORE_INSTRUCTION(STORETYPE) {                                      \
STORE_INSTRUCTION_PRE(STORETYPE);                                           \
if(CPU.Debugging && (DebugState != DEBUG_STATE_BP_HIT))                     \
{                                                                           \
    CPU.DebugTripped = DebugCheckMemoryAccess(rBS.i, 3*rCS.i, DEBUG_WRITE); \
    if(CPU.DebugTripped)                                                    \
        break;                                                              \
}                                                                           \
STORE_INSTRUCTION_POST(STORETYPE); }
#endif

#define SET_FLAGS_ADD(RESULT, INPUT1, INPUT2)          \
CPU_FLAGS &= 0x7fffff0;                                \
if((RESULT == 0))                                      \
    CPU_FLAGS |= CPU_FLAG_ZERO;                        \
else if(RESULT & 0x04000000)                           \
    CPU_FLAGS |= CPU_FLAG_SIGNED;                      \
if((RESULT & 0x08000000))                              \
    CPU_FLAGS |= CPU_FLAG_CARRY;                       \
if(((INPUT1 & 0x04000000) == (INPUT2 & 0x04000000)) && ((INPUT2 & 0x04000000) != (RESULT & 0x04000000)))      \
    CPU_FLAGS |= CPU_FLAG_OVERFLOW;

#define SET_FLAGS_ADD_MUL(RESULT, INPUT1, INPUT2)      \
CPU_FLAGS &= 0x7fffff0;                                \
if((RESULT == 0))                                      \
    CPU_FLAGS |= CPU_FLAG_ZERO;                        \
else if(RESULT & 0x04000000)                           \
    CPU_FLAGS |= CPU_FLAG_SIGNED;                      \
if(((INPUT1 & 0x04000000) == (INPUT2 & 0x04000000)) && ((INPUT2 & 0x04000000) != (RESULT & 0x04000000)))      \
{                                                      \
    CPU_FLAGS |= CPU_FLAG_OVERFLOW;                    \
    CPU_FLAGS |= CPU_FLAG_CARRY;                       \
}

#define SET_FLAGS_MULTI_ADD(RESULT, INPUT1, INPUT2)    \
CPU_FLAGS &= 0x7fffff0;                                \
if((RESULT == 0))                                      \
    CPU_FLAGS |= CPU_FLAG_ZERO;                        \
else if(RESULT & 0x20000000000000)                     \
    CPU_FLAGS |= CPU_FLAG_SIGNED;                      \
if((RESULT & 0x40000000000000))                        \
    CPU_FLAGS |= CPU_FLAG_CARRY;                       \
if(((INPUT1 & 0x20000000000000) == (INPUT2 & 0x20000000000000)) && ((INPUT2 & 0x20000000000000) != (RESULT & 0x20000000000000)))      \
    CPU_FLAGS |= CPU_FLAG_OVERFLOW;

#define SET_FLAGS_MULTI_ADD_MUL(RESULT, INPUT1, INPUT2)    \
CPU_FLAGS &= 0x7fffff0;                                \
if((RESULT == 0))                                      \
    CPU_FLAGS |= CPU_FLAG_ZERO;                        \
else if(RESULT & 0x20000000000000)                     \
    CPU_FLAGS |= CPU_FLAG_SIGNED;                      \
if(((INPUT1 & 0x20000000000000) == (INPUT2 & 0x20000000000000)) && ((INPUT2 & 0x20000000000000) != (RESULT & 0x20000000000000)))      \
{                                                      \
    CPU_FLAGS |= CPU_FLAG_OVERFLOW;                    \
    CPU_FLAGS |= CPU_FLAG_CARRY;                       \
}

#define SET_FLAGS_SUB(RESULT, INPUT1, INPUT2)          \
CPU_FLAGS &= 0x7fffff0;                                \
if((RESULT == 0))                                      \
    CPU_FLAGS |= CPU_FLAG_ZERO;                        \
else if(RESULT & 0x04000000)                           \
    CPU_FLAGS |= CPU_FLAG_SIGNED;                      \
if((RESULT & 0x08000000))                              \
    CPU_FLAGS |= CPU_FLAG_CARRY;                       \
if(((INPUT1 & 0x04000000) != (INPUT2 & 0x04000000)) && ((INPUT2 & 0x04000000) == (RESULT & 0x04000000)))      \
    CPU_FLAGS |= CPU_FLAG_OVERFLOW;

#define SET_FLAGS_SUB_DIV(RESULT, INPUT1, INPUT2)      \
CPU_FLAGS &= 0x7fffff0;                                \
if((RESULT == 0))                                      \
    CPU_FLAGS |= CPU_FLAG_ZERO;                        \
else if(RESULT & 0x04000000)                           \
    CPU_FLAGS |= CPU_FLAG_SIGNED;                      \
if(((INPUT1 & 0x04000000) != (INPUT2 & 0x04000000)) && ((INPUT2 & 0x04000000) == (RESULT & 0x04000000)))      \
{                                                      \
    CPU_FLAGS |= CPU_FLAG_OVERFLOW;                    \
    CPU_FLAGS |= CPU_FLAG_CARRY;                       \
}

#define SET_FLAGS_MULTI_SUB(RESULT, INPUT1, INPUT2)    \
CPU_FLAGS &= 0x7fffff0;                                \
if((RESULT == 0))                                      \
    CPU_FLAGS |= CPU_FLAG_ZERO;                        \
else if(RESULT & 0x20000000000000)                     \
    CPU_FLAGS |= CPU_FLAG_SIGNED;                      \
if((RESULT & 0x40000000000000))                        \
    CPU_FLAGS |= CPU_FLAG_CARRY;                       \
if(((INPUT1 & 0x20000000000000) != (INPUT2 & 0x20000000000000)) && ((INPUT2 & 0x20000000000000) == (RESULT & 0x20000000000000)))      \
    CPU_FLAGS |= CPU_FLAG_OVERFLOW;

#define SET_FLAGS_MULTI_SUB_DIV(RESULT, INPUT1, INPUT2)    \
CPU_FLAGS &= 0x7fffff0;                                \
if((RESULT == 0))                                      \
    CPU_FLAGS |= CPU_FLAG_ZERO;                        \
else if(RESULT & 0x20000000000000)                     \
    CPU_FLAGS |= CPU_FLAG_SIGNED;                      \
if(((INPUT1 & 0x20000000000000) != (INPUT2 & 0x20000000000000)) && ((INPUT2 & 0x20000000000000) == (RESULT & 0x20000000000000)))      \
{                                                      \
    CPU_FLAGS |= CPU_FLAG_OVERFLOW;                    \
    CPU_FLAGS |= CPU_FLAG_CARRY;                       \
}

#endif
