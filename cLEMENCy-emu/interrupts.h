//
//  interrupts.h
//  cLEMENCy interrupt handler
//
//  Created by Lightning on 2015/12/20.
//  Copyright (c) 2017 Lightning. All rights reserved.
//

#ifndef __CLEMENCY__interrupts__
#define __CLEMENCY__interrupts__

#define INTERRUPT_MASK	0x1FF0
#define EXCEPTION_MASK	0x0070

#define INTERRUPT_TIMER1              0
#define INTERRUPT_TIMER2              1
#define INTERRUPT_TIMER3              2
#define INTERRUPT_TIMER4              3
#define INTERRUPT_INVALID_INSTRUCTION 4
#define INTERRUPT_DIVIDE_BY_0         5
#define INTERRUPT_MEMORY_EXCEPTION    6
#define INTERRUPT_DATA_RECIEVED       7
#define INTERRUPT_DATA_SENT           8
#define INTERRUPT_COUNT               9

extern int InterruptStackDirection;

void InitInterrupts();

unsigned int GetInterrupt(unsigned int ID);
void SetInterrupt(unsigned int ID, unsigned int Location);

void FireInterrupt(unsigned int ID);
void ReturnFromInterrupt();

void EnableInterrupts(unsigned int IDs);
void DisableInterrupts(unsigned int IDs);

#endif
