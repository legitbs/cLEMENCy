//
//  interpretter.c
//  cLEMENCy - LEgitbs Middle ENdian Computer
//
//  Created by Lightning on 2015/11/21.
//  Copyright (c) 2017 Lightning. All rights reserved.
//

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "interpretter.h"
#include "memory.h"
#include "WELL512a.h"
#include "exceptions.h"
#include "debug.h"
#include "clem-instructions.h"
#include "memory.h"

struct CPUStruct CPU;
DebugBreakpointStruct DBRK_BP;

union RegLongDouble
{
    unsigned long l;
    double d;
};

union RegIntFloat
{
    unsigned int i;
    float f;
};

void InitInterpretter()
{
    memset(&CPU, 0, sizeof(CPU));
    memset(&DBRK_BP, 0, sizeof(DBRK_BP));
    DBRK_BP.ID = -1;
}

unsigned int CPU_Random()
{
    static int RandInit = 0;
    if(!RandInit)
    {
        RandInit = open("/dev/urandom", O_RDONLY);
        if(RandInit == -1)
        {
            DebugOut("ERROR INITIALIZING CPU RANDOM!");
            exit(-1);
        }
        read(RandInit, RANDOM_STATE, sizeof(RANDOM_STATE));
        close(RandInit);
        RandInit = 1;
    }

    return WELLRNG512a() & 0x07ffffff;
}

int Interpretter_RunInstruction()
{
	union RegLongDouble rAM, rBM, rCM;
	union RegIntFloat rAS, rBS, rCS;

	unsigned int ImmediateVal;
	unsigned long Opcode = ReadForExecute27(CPU_PC);
	unsigned int Temp, Temp2;
	unsigned int OpcodeSize = 3;

	//if we tripped on accessing the instruction then exit
	//note, we only trip if the breakpoint is at the beginning of the instruction or exception triggered
#ifndef NODEBUG
	if(CPU.Debugging && CPU.DebugTripped)
		return 0;
#endif

    //if the execute failed then exit
    if(!CPU.Running)
        return 0;

	switch(OPCODE)
	{
		MATH_INSTRUCTION_MACRO(CLEM_AD, +, ADD);
		MATH_INSTRUCTION_MACRO(CLEM_SB, -, SUB);
		MATH_INSTRUCTION_WITH_SIGNED_MACRO(CLEM_MU, *, ADD_MUL);
		MATH_INSTRUCTION_DIV_MACRO(CLEM_DV, /, SUB_DIV);
		MATH_INSTRUCTION_WITH_SIGNED_MACRO_DIV(CLEM_MD, %, SUB_DIV);
		MATH_INSTRUCTION_INT_MACRO(CLEM_AN, &);
		MATH_INSTRUCTION_INT_MACRO(CLEM_OR, |);
		MATH_INSTRUCTION_INT_MACRO(CLEM_XR, ^);
		MATH_INSTRUCTION_CARRY_MACRO(CLEM_ADC, +, ADD);
		MATH_INSTRUCTION_CARRY_MACRO(CLEM_SBC, -, SUB);

		case CLEM_SL:
			if(!D)
				MATH_INSTRUCTION_INT_SHIFT_MACRO(<<)
			else
				MATH_INSTRUCTION_INT_SHIFT_MACRO(>>)
			break;

		case CLEM_SA:    //arithmetic shift right
			if(!D)
			{
                OpcodeSize = 0;
				RaiseException(EXCEPTION_INVALID_INSTRUCTION, 0);
				break;
			}

			if(M)
			{
				/* multi-reg */
				rBM.l = (((unsigned long)CPU.Registers.R[rB]) << 27) | CPU.Registers.R[(rB+1) & 0x1f];
				rCM.l = (((unsigned long)CPU.Registers.R[rC]) << 27) | CPU.Registers.R[(rC+1) & 0x1f];
				rAM.l = ((((long)rBM.l) << 10) >> 10) >> rCM.l;
				CPU.Registers.R[rA] = (rAM.l & 0x003FFFFFF8000000) >> 27;
				CPU.Registers.R[(rA + 1) & 0x1f] = (rAM.l & MEMORY_MASK);
				SET_FLAGS_MULTI_ADD(rAM.l, rBM.l, rBM.l);
			}
			else
			{
				rBS.i = CPU.Registers.R[rB];
				CPU.Registers.R[rA] = ((((int)rBS.i << 5) >> 5) >> CPU.Registers.R[rC]) & MEMORY_MASK;
				SET_FLAGS_ADD(CPU.Registers.R[rA], rBS.i, rBS.i);
			}
			break;

		case CLEM_RL:    //rotate left
			if(!D)
			{
				if(M)
				{
					/* multi-reg */
					rBM.l = (((unsigned long)CPU.Registers.R[rB]) << 27) | CPU.Registers.R[(rB+1) & 0x1f];
					rCM.l = (((unsigned long)CPU.Registers.R[rC]) << 27) | CPU.Registers.R[(rC+1) & 0x1f] % 54;
					rAM.l = (rBM.l << rCM.l) | (rBM.l >> (54-rCM.l));
					CPU.Registers.R[rA] = (rAM.l & 0x003FFFFFF8000000) >> 27;
					CPU.Registers.R[(rA + 1) & 0x1f] = (rAM.l & MEMORY_MASK);
					SET_FLAGS_MULTI_ADD(rAM.l, rBM.l, rBM.l);
				}
				else
				{
					rBS.i = CPU.Registers.R[rB];
					rCS.i = CPU.Registers.R[rC] % 27;
					CPU.Registers.R[rA] = ((rBS.i << rCS.i) | ( rBS.i >> (27-rCS.i))) & MEMORY_MASK;
					SET_FLAGS_ADD(CPU.Registers.R[rA], rBS.i, rBS.i);
				}
			}
			else    //rotate right
			{
				if(M)
				{
					/* multi-reg */
					rBM.l = (((unsigned long)CPU.Registers.R[rB]) << 27) | CPU.Registers.R[(rB+1) & 0x1f];
					rCM.l = (((unsigned long)CPU.Registers.R[rC]) << 27) | CPU.Registers.R[(rC+1) & 0x1f] % 54;
					rAM.l = (rBM.l >> rCM.l) | (rBM.l << (54-rCM.l));
					CPU.Registers.R[rA] = (rAM.l & 0x003FFFFFF8000000) >> 27;
					CPU.Registers.R[(rA + 1) & 0x1f] = (rAM.l & MEMORY_MASK);
					SET_FLAGS_MULTI_ADD(rAM.l, rBM.l, rBM.l);
				}
				else
				{
					rBS.i = CPU.Registers.R[rB];
					rCS.i = CPU.Registers.R[rC] % 27;
					CPU.Registers.R[rA] = ((rBS.i >> rCS.i) | ( rBS.i << (27-rCS.i))) & MEMORY_MASK;
					SET_FLAGS_ADD(CPU.Registers.R[rA], rBS.i, rBS.i);
				}
			}
			break;

		case CLEM_SLI:
			if(!D)    //SLI
				MATH_INSTRUCTION_INT_IMM2_SHIFT_MACRO(<<)
			else      //SRI
				MATH_INSTRUCTION_INT_IMM2_SHIFT_MACRO(>>)
			break;

		case CLEM_SAI:    //shift arithmetic right immediate
			if(!D)
			{
                OpcodeSize = 0;
				RaiseException(EXCEPTION_INVALID_INSTRUCTION, 0);
				break;
			}

			if(M)
			{
				/* multi-reg */
				rBM.l = (((unsigned long)CPU.Registers.R[rB]) << 27) | CPU.Registers.R[(rB+1) & 0x1f];
				rAM.l = ((((long)rBM.l) << 10) >> 10) >> IMMEDIATE_2;
				CPU.Registers.R[rA] = (rAM.l & 0x003FFFFFF8000000) >> 27;
				CPU.Registers.R[(rA + 1) & 0x1f] = (rAM.l & MEMORY_MASK);
				SET_FLAGS_MULTI_ADD(rAM.l, rBM.l, rBM.l);
			}
			else
			{
				rBS.i = CPU.Registers.R[rB];
				CPU.Registers.R[rA] = ((((int)rBS.i << 5) >> 5) >> IMMEDIATE_2) & MEMORY_MASK;
				SET_FLAGS_ADD(CPU.Registers.R[rA], rBS.i, rBS.i);
			}
			break;

		case CLEM_RLI:    //rotate left immediate
			if(!D)
			{
				if(M)
				{
					/* multi-reg */
					ImmediateVal = IMMEDIATE_2 % 54;
					rBM.l = (((unsigned long)CPU.Registers.R[rB]) << 27) | CPU.Registers.R[(rB+1) & 0x1f];
					rAM.l = (rBM.l << ImmediateVal) | (rBM.l >> (54-ImmediateVal));
					CPU.Registers.R[rA] = (rAM.l & 0x003FFFFFF8000000) >> 27;
					CPU.Registers.R[(rA + 1) & 0x1f] = (rAM.l & MEMORY_MASK);
					SET_FLAGS_MULTI_ADD(rAM.l, rBM.l, rBM.l);
				}
				else
				{
					ImmediateVal = IMMEDIATE_2 % 27;
					rBS.i = CPU.Registers.R[rB];
					rCS.i = CPU.Registers.R[rC] % 27;
					CPU.Registers.R[rA] = ((rBS.i << ImmediateVal) | (rBS.i >> (27-ImmediateVal))) & MEMORY_MASK;
					SET_FLAGS_ADD(CPU.Registers.R[rA], rBS.i, rBS.i);
				}
			}
			else    //rotate right immediate
			{
				if(M)
				{
					/* multi-reg */
					ImmediateVal = IMMEDIATE_2 % 54;
					rBM.l = (((unsigned long)CPU.Registers.R[rB]) << 27) | CPU.Registers.R[(rB+1) & 0x1f];
					rAM.l = (rBM.l >> ImmediateVal) | (rBM.l << (54-ImmediateVal));
					CPU.Registers.R[rA] = (rAM.l & 0x003FFFFFF8000000) >> 27;
					CPU.Registers.R[(rA + 1) & 0x1f] = (rAM.l & MEMORY_MASK);
					SET_FLAGS_MULTI_ADD(CPU.Registers.R[rA], rBM.l, rBM.l);
				}
				else
				{
					ImmediateVal = IMMEDIATE_2 % 27;
					rBS.i = CPU.Registers.R[rB];
					rCS.i = CPU.Registers.R[rC] % 27;
					CPU.Registers.R[rA] = ((rBS.i >> ImmediateVal) | (rBS.i << (27-ImmediateVal))) & MEMORY_MASK;
					SET_FLAGS_ADD(CPU.Registers.R[rA], rBS.i, rBS.i);
				}
			}
			break;

		case CLEM_DMT:	 //direct memory transfer
			rAS.i = CPU.Registers.R[rA];
			rBS.i = CPU.Registers.R[rB];
			rCS.i = CPU.Registers.R[rC];
			Memory_MemCpy(rAS.i, rBS.i, rCS.i);
				break;

		case CLEM_MH:    //move high
			CPU.Registers.R[rA_8] = ((IMMEDIATE_8 << 10) | (CPU.Registers.R[rA_8] & 0x3ff)) & MEMORY_MASK;
			break;

		case CLEM_ML:    //move low zero extend
			CPU.Registers.R[rA_8] = IMMEDIATE_8;
			break;

		case CLEM_MS:    //move low sign extend
			CPU.Registers.R[rA_8] = (unsigned int)((signed int)(IMMEDIATE_8 << (32-17)) >> (32-17)) & MEMORY_MASK;
			break;

		case CLEM_RE:    //branch return, interrupt return, etc
			switch(SUBOPCODE)
			{
				case 0:
					OpcodeSize = 2;
					if(SUBOPCODE_5BIT == CLEM_RE_SUB2)          //branch return
					{
						CPU_PC = CPU_RETADDR;
						CPU.BranchHit = 1;
					}
					else if(SUBOPCODE_5BIT == CLEM_IR_SUB2)     //interrupt return
						ReturnFromInterrupt();
					else if(SUBOPCODE_5BIT == CLEM_WT_SUB2)    //wait
						CPU.Wait = 1;
					else if(SUBOPCODE_5BIT == CLEM_HT_SUB2)    //halt
					{
#ifndef NODEBUG
						printf("HT instruction encountered at %07x\n", CPU_PC);
#endif
						CPU.Running = 0;
					}
					else if(SUBOPCODE_5BIT == CLEM_EI_SUB2)    //enable interrupts
					{
						if(RESERVED_16)
						{
                            OpcodeSize = 0;
							RaiseException(EXCEPTION_INVALID_INSTRUCTION, 0);
							break;
						}

						EnableInterrupts(CPU.Registers.R[SIGNEXTEND_rA]);
					}
					else if(SUBOPCODE_5BIT == CLEM_DI_SUB2)    //disable interrupts
					{
						if(RESERVED_16)
						{
                            OpcodeSize = 0;
							RaiseException(EXCEPTION_INVALID_INSTRUCTION, 0);
							break;
						}

						DisableInterrupts(CPU.Registers.R[SIGNEXTEND_rA]);
					}
					else if(SUBOPCODE_5BIT == CLEM_SES_SUB2)    //sign extend single
					{
						OpcodeSize = 3;
						if(RESERVED_13)
						{
                            OpcodeSize = 0;
							RaiseException(EXCEPTION_INVALID_INSTRUCTION, 0);
							break;
						}

						CPU.Registers.R[SIGNEXTEND_rA] = (((int)CPU.Registers.R[SIGNEXTEND_rB] << 23) >> 23) & 0x7ffffff;
					}
					else if(SUBOPCODE_5BIT == CLEM_SEW_SUB2)    //sign extend word
					{
						OpcodeSize = 3;
						if(RESERVED_13)
						{
                            OpcodeSize = 0;
							RaiseException(EXCEPTION_INVALID_INSTRUCTION, 0);
							break;
						}

						CPU.Registers.R[SIGNEXTEND_rA] = (((int)CPU.Registers.R[SIGNEXTEND_rB] << 14) >> 14) & 0x7ffffff;
					}
					else if(SUBOPCODE_5BIT == CLEM_ZES_SUB2)    //zero extend single
					{
						OpcodeSize = 3;
						if(RESERVED_13)
						{
                            OpcodeSize = 0;
							RaiseException(EXCEPTION_INVALID_INSTRUCTION, 0);
							break;
						}

						CPU.Registers.R[SIGNEXTEND_rA] = CPU.Registers.R[SIGNEXTEND_rB] & 0x00001ff;
					}
					else if(SUBOPCODE_5BIT == CLEM_ZEW_SUB2)    //zero extend word
					{
						OpcodeSize = 3;
						if(RESERVED_13)
						{
                            OpcodeSize = 0;
							RaiseException(EXCEPTION_INVALID_INSTRUCTION, 0);
							break;
						}

						CPU.Registers.R[SIGNEXTEND_rA] = CPU.Registers.R[SIGNEXTEND_rB] & 0x003ffff;
					}
					else if(SUBOPCODE_5BIT == CLEM_SF_SUB2)    //set flag register
					{
						if(RESERVED_16)
						{
                            OpcodeSize = 0;
							RaiseException(EXCEPTION_INVALID_INSTRUCTION, 0);
							break;
						}

						CPU_FLAGS = CPU.Registers.R[SIGNEXTEND_rA] & 0x1fff;
					}
					else if(SUBOPCODE_5BIT == CLEM_RF_SUB2)    //read flag register
					{
						if(RESERVED_16)
						{
                            OpcodeSize = 0;
							RaiseException(EXCEPTION_INVALID_INSTRUCTION, 0);
							break;
						}

						CPU.Registers.R[SIGNEXTEND_rA] = CPU_FLAGS;
					}
					else
                    {
                        OpcodeSize = 0;
						RaiseException(EXCEPTION_INVALID_INSTRUCTION, 0);
                    }
					break;

				case 1:
                    OpcodeSize = 0;
					RaiseException(EXCEPTION_INVALID_INSTRUCTION, 0);
					break;

				case 2:
					if(RESERVED_10)
					{
                        OpcodeSize = 0;
						RaiseException(EXCEPTION_INVALID_INSTRUCTION, 0);
						break;
					}

					if(SMP_WRITE == CLEM_SMP_SUB2)         //set memory protection
					{
						rAS.i = CPU.Registers.R[rA];
						rBS.i = CPU.Registers.R[rB];

						//error if not page aligned
						if(rAS.i & (MEMORY_PAGE_SIZE - 1))
						{
							CLEAR_ZERO();
							CPU.Registers.R[rA] = 0;
							break;
						}

						//error if too many pages
						if((rAS.i + (rBS.i * MEMORY_PAGE_SIZE)) > MEMORY_MASK)
						{
							CLEAR_ZERO();
							CPU.Registers.R[rA] = 1;
							break;
						}

						for(; rBS.i; rBS.i--)
						{
							SetMemoryProtection(rAS.i, SMP_FLAGS);
							rAS.i = (rAS.i + MEMORY_PAGE_SIZE) & MEMORY_MASK;
						};
						SET_ZERO();
					}
					else				   //CLEM_RMP read memory protection
					{
						//return up to 13 pages worth (2 bits per page)
						rAS.i = CPU.Registers.R[rA];
						rBS.i = CPU.Registers.R[rB];

						//error if not page aligned
						if(rAS.i & (MEMORY_PAGE_SIZE - 1))
						{
							CLEAR_ZERO();
							CPU.Registers.R[rA] = 0;
							break;
						}

						//error if too many pages
						if(rBS.i > 13)
						{
							CLEAR_ZERO();
							CPU.Registers.R[rA] = 1;
							break;
						}

						rCS.i = 0;
						for(rCS.i = 0; rBS.i; rBS.i--)
						{
							rCS.i = (rCS.i << 2) | ReadMemoryProtection(rAS.i);
							rAS.i += MEMORY_PAGE_SIZE;
							rAS.i &= MEMORY_MASK;
						}
						CPU.Registers.R[rA] = rCS.i & MEMORY_MASK;
						SET_ZERO();
					}
					break;

				case 3:
					if(RESERVED_4)
					{
                        OpcodeSize = 0;
						RaiseException(EXCEPTION_INVALID_INSTRUCTION, 0);
						break;
					}

					if(SUBOPCODE_4 == CLEM_NG_SUB2)
						MATH_INSTRUCTION_REGA_MACRO(-)
					else if(SUBOPCODE_4 == CLEM_NT_SUB2)
						MATH_INSTRUCTION_REGA_MACRO(!)
					else if(SUBOPCODE_4 == CLEM_BF_SUB2)
						MATH_INSTRUCTION_REGA_MACRO(~)
					else	//CLEM_RND_SUB
					{
						if(FTI_M)
						{
							CPU.Registers.R[FTI_rA] = CPU_Random();
							CPU.Registers.R[(FTI_rA + 1) & 0x1f] = CPU_Random();
						}
						else
							CPU.Registers.R[FTI_rA] = CPU_Random();
					}
					break;
			}
			break;

		case CLEM_LDT:    //load
			OpcodeSize = 6;
			Opcode = (Opcode << 27) | ReadForExecute27(CPU_PC + 3);
			if(LDSTR_ADJUST_RB == 3 || LDSTR_SUBOPCODE == 3)
            {
                OpcodeSize = 0;
				RaiseException(EXCEPTION_INVALID_INSTRUCTION, 0);
            }
			else if(LDSTR_SUBOPCODE == CLEM_LDS_SUB)
				LOAD_INSTRUCTION(9)
			else if(LDSTR_SUBOPCODE == CLEM_LDW_SUB)
				LOAD_INSTRUCTION(18)
			else //if(SUBOPCODE == 2)	//CLEM_LDT_SUB
				LOAD_INSTRUCTION(27)
			break;

		case CLEM_STT:    //store
			OpcodeSize = 6;
			Opcode = (Opcode << 27) | ReadForExecute27(CPU_PC + 3);
			if(LDSTR_ADJUST_RB == 3 || LDSTR_SUBOPCODE == 3)
            {
                OpcodeSize = 0;
				RaiseException(EXCEPTION_INVALID_INSTRUCTION, 0);
            }
			else if(LDSTR_SUBOPCODE == CLEM_STS_SUB)
				STORE_INSTRUCTION(9)
			else if(LDSTR_SUBOPCODE == CLEM_STW_SUB)
				STORE_INSTRUCTION(18)
			else //if(SUBOPCODE == 2)	//CLEM_STT_SUB
				STORE_INSTRUCTION(27)
			break;

		case CLEM_CM:    //compare
			//if the register version then 2 bytes for the opcode
			if(!COMPARE_USEIMMEDIATE)
				OpcodeSize = 2;

			if(F)
			{
                OpcodeSize = 0;
				RaiseException(EXCEPTION_INVALID_INSTRUCTION, 0);
				break;
			}

			if(M)
			{
				rAM.l = (((unsigned long)CPU.Registers.R[COMPARE_rA]) << 27) | CPU.Registers.R[(COMPARE_rA+1) & 0x1f];
				if(COMPARE_USEIMMEDIATE)
				{
					//sign extend but not all the way
					rBM.l = COMPARE_IMMEDIATE;
					if(rBM.l & 0x2000)
						rBM.l |= 0xffffffffffe000;
				}
				else
					rBM.l = (((unsigned long)CPU.Registers.R[COMPARE_rB]) << 27) | CPU.Registers.R[(COMPARE_rB+1) & 0x1f];

				/* multi-reg */
				SET_FLAGS_MULTI_SUB(rAM.l - rBM.l, rAM.l, rBM.l);
			}
			else
			{
				//normal register
				rAS.i = CPU.Registers.R[COMPARE_rA];
				if(COMPARE_USEIMMEDIATE)
				{
					//sign extend the value put in but not all the way
					rBS.i = COMPARE_IMMEDIATE;
					if(rBS.i & 0x2000)
						rBS.i |= 0x7ffe000;
				}
				else
					rBS.i = CPU.Registers.R[COMPARE_rB];
				SET_FLAGS_SUB(rAS.i - rBS.i, rAS.i, rBS.i);
			}
			break;

		case CLEM_BR:   //branch register conditional
		case CLEM_CR:   //call register conditional
			OpcodeSize = 2;
			ImmediateVal = CPU.Registers.R[BRANCH_rA] - CPU_PC;

			//We fall through here

		case CLEM_B:    //branch conditional
		case CLEM_C:    //call conditional

			if((OPCODE == CLEM_B) || (OPCODE == CLEM_C))
				ImmediateVal = (((int)BRANCH_REL_OFFSET) << (32-17)) >> (32-17);

			//store off our PC, at the end check if we branched
			//if so then update the return address register
			rBS.i = CPU_PC;

			//check our branches
			switch(BRANCH_COND)
			{
				case BRANCH_COND_NOT_EQUAL:
					if(!(CPU_FLAGS & CPU_FLAG_ZERO))
					{
						CPU_PC = (CPU_PC + ImmediateVal) & MEMORY_MASK;
						CPU.BranchHit = 1;
					}
					break;

				case BRANCH_COND_EQUAL:
					if(CPU_FLAGS & CPU_FLAG_ZERO)
					{
						CPU_PC = (CPU_PC + ImmediateVal) & MEMORY_MASK;
						CPU.BranchHit = 1;
					}
					break;

				case BRANCH_COND_LESS_THAN:
					//CARRY == 1 && EQUAL = 0
					if((CPU_FLAGS & CPU_FLAG_CARRY) && !(CPU_FLAGS & CPU_FLAG_ZERO))
					{
						CPU_PC = (CPU_PC + ImmediateVal) & MEMORY_MASK;
						CPU.BranchHit = 1;
					}
					break;

				case BRANCH_COND_LESS_THAN_OR_EQUAL:
					//CARRY == 1 || EQUAL == 1
					if((CPU_FLAGS & CPU_FLAG_ZERO) || (CPU_FLAGS & CPU_FLAG_CARRY))
					{
						CPU_PC = (CPU_PC + ImmediateVal) & MEMORY_MASK;
						CPU.BranchHit = 1;
					}
					break;

				case BRANCH_COND_GREATER_THAN:
					//CARRY == 0 && EQUAL == 0
					if(!(CPU_FLAGS & CPU_FLAG_ZERO) && !(CPU_FLAGS & CPU_FLAG_CARRY))
					{
						CPU_PC = (CPU_PC + ImmediateVal) & MEMORY_MASK;
						CPU.BranchHit = 1;
					}
					break;

				case BRANCH_COND_GREATER_THAN_OR_EQUAL:
					//CARRY == 0 || EQUAL == 1
					if(!(CPU_FLAGS & CPU_FLAG_CARRY) || (CPU_FLAGS & CPU_FLAG_ZERO))
					{
						CPU_PC = (CPU_PC + ImmediateVal) & MEMORY_MASK;
						CPU.BranchHit = 1;
					}
					break;

				case BRANCH_COND_NOT_OVERFLOW:
					//not overflow
					if(!(CPU_FLAGS & CPU_FLAG_OVERFLOW))
					{
						CPU_PC = (CPU_PC + ImmediateVal) & MEMORY_MASK;
						CPU.BranchHit = 1;
					}
					break;

				case BRANCH_COND_OVERFLOW:
					//overflow
					if(CPU_FLAGS & CPU_FLAG_OVERFLOW)
					{
						CPU_PC = (CPU_PC + ImmediateVal) & MEMORY_MASK;
						CPU.BranchHit = 1;
					}
					break;

				case BRANCH_COND_NOT_SIGNED:
					//not signed
					if(!(CPU_FLAGS & CPU_FLAG_SIGNED))
					{
						CPU_PC = (CPU_PC + ImmediateVal) & MEMORY_MASK;
						CPU.BranchHit = 1;
					}
					break;

				case BRANCH_COND_SIGNED:
					//signed
					if(CPU_FLAGS & CPU_FLAG_SIGNED)
					{
						CPU_PC = (CPU_PC + ImmediateVal) & MEMORY_MASK;
						CPU.BranchHit = 1;
					}
					break;

				case BRANCH_COND_SIGNED_LESS_THAN:
					//SIGNED != OVERFLOW
					if(((CPU_FLAGS & CPU_FLAG_SIGNED) >> 1) != (CPU_FLAGS & CPU_FLAG_OVERFLOW))
					{
						CPU_PC = (CPU_PC + ImmediateVal) & MEMORY_MASK;
						CPU.BranchHit = 1;
					}
					break;

				case BRANCH_COND_SIGNED_LESS_THAN_OR_EQUAL:
					//EQ || SIGNED != OVERFLOW
					if((CPU_FLAGS & CPU_FLAG_ZERO) || (((CPU_FLAGS & CPU_FLAG_SIGNED) >> 1) != (CPU_FLAGS & CPU_FLAG_OVERFLOW)))
					{
						CPU_PC = (CPU_PC + ImmediateVal) & MEMORY_MASK;
						CPU.BranchHit = 1;
					}
					break;

				case BRANCH_COND_SIGNED_GREATER_THAN:
					//EQ == 0 && SIGNED == OVERFLOW
					if(!(CPU_FLAGS & CPU_FLAG_ZERO) && (((CPU_FLAGS & CPU_FLAG_SIGNED) >> 1) == (CPU_FLAGS & CPU_FLAG_OVERFLOW)))
					{
						CPU_PC = (CPU_PC + ImmediateVal) & MEMORY_MASK;
						CPU.BranchHit = 1;
					}
					break;

				case BRANCH_COND_SIGNED_GREATER_THAN_OR_EQUAL:
					//SIGNED == OVERFLOW
					if(((CPU_FLAGS & CPU_FLAG_SIGNED) >> 1) == (CPU_FLAGS & CPU_FLAG_OVERFLOW))
					{
						CPU_PC = (CPU_PC + ImmediateVal) & MEMORY_MASK;
						CPU.BranchHit = 1;
					}
					break;

				case BRANCH_COND_ALWAYS:
					CPU_PC = (CPU_PC + ImmediateVal) & MEMORY_MASK;
					CPU.BranchHit = 1;
					break;

				default:
                    OpcodeSize = 0;
					RaiseException(EXCEPTION_INVALID_INSTRUCTION, 0);
					break;
			}

			//if we branched and a call then store off our return address
			if(CPU.BranchHit && BRANCH_CALL)
				CPU_RETADDR = rBS.i + OpcodeSize;
			break;

		case CLEM_BRA:    //branch/call relative/absolute
			OpcodeSize = 4;
			CPU.BranchHit = 1;
			if((SUBOPCODE == CLEM_CAR_SUB) || (SUBOPCODE == CLEM_CAA_SUB))
				CPU_RETADDR = CPU_PC + 4;

			ImmediateVal = ReadForExecute27(CPU_PC + 1);

			//now recombine and get the proper immediate val
			//Opcode: A B C
			//Memory: B A C D
			//ImmVal: C A D
			//Need: Imm = B C D
			ImmediateVal = ((Opcode & 0x3ffff) << 9) | (ImmediateVal & 0x1ff);
			ImmediateVal = (ImmediateVal << (32 - 27)) >> (32 - 27);

			if((SUBOPCODE == CLEM_BRA_SUB) || (SUBOPCODE == CLEM_CAA_SUB))
				CPU_PC = ImmediateVal & MEMORY_MASK;
			else
				CPU_PC = (CPU_PC + ImmediateVal) & MEMORY_MASK;
			break;

#ifndef NODEBUG
		case CLEM_DBRK:
			//if no debugger then fail
			if(!CPU.Debugging)
			{
                OpcodeSize = 0;
				RaiseException(EXCEPTION_INVALID_INSTRUCTION, 0);
				break;
			}

			//setup our custom breakpoint
			OpcodeSize = 2;
			if(ExceptionTriggers & (1 << EXCEPTION_DBRK))
				CPU.DebugTripped = &DBRK_BP;
			break;
#endif
		default:
            OpcodeSize = 0;
			RaiseException(EXCEPTION_INVALID_INSTRUCTION, 0);
			break;
    };
    return OpcodeSize;
}
