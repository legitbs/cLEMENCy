//
//  recordstate.h
//  cLEMENCy recording of state
//
//  Created by Lightning on 2017/07/15.
//  Copyright (c) 2017 Lightning. All rights reserved.
//

#ifndef __CLEMENCY__recordstate__
#define __CLEMENCY__recordstate__

#include "cpu.h"

typedef struct RecordStateStruct
{
    CPURegStruct BeforeState;
    CPURegStruct AfterState;
} RecordStateStruct;

void InitRecordState();
void RecordBeforeState();
void RecordAfterState();
void DisplayRecordStates(unsigned int Count);
int SetRecordCount(unsigned int MaxEntries);
unsigned int GetRecordCount();

#endif
