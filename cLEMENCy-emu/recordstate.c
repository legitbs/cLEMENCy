//
//  recordstate.c
//  cLEMENCy recording of state
//
//  Created by Lightning on 2017/07/15.
//  Copyright (c) 2017 Lightning. All rights reserved.
//

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include "recordstate.h"
#include "cpu.h"
#include "debug.h"

//could be long but if you are wanting that many entries then I have to ask why
//as i'm not allowing full system snapshot
//I could also save space by storing the register number in the top 5 bits of the int value
//and only store modified registers but I do not wish to implement all the logic required
//this is just a "nice thing" to help in debugging and I only have a week
unsigned int StartIndex;
unsigned int EndIndex;
unsigned int MaxIndex;

//our circular list
RecordStateStruct *RecordStates;

void InitRecordState()
{
    StartIndex = 0;
    EndIndex = 0;
    MaxIndex = 0;
    RecordStates = 0;
}

void RecordBeforeState()
{
    int CurIndex;

    if(!RecordStates)
        return;

    //move end, if it lines up with start then move start
    CurIndex = EndIndex;
    EndIndex = (EndIndex + 1) % MaxIndex;
    if(EndIndex == StartIndex)
        StartIndex = (StartIndex + 1) % MaxIndex;

    //write the prior entry
    memcpy(&RecordStates[CurIndex].BeforeState, &CPU.Registers, sizeof(CPURegStruct));
}

void RecordAfterState()
{
    int CurIndex;

    if(!RecordStates)
        return;

    //state after, normally only 1 register changes and we could save space but
    //it doesn't capture loads and i'm not worried about capturing excessively long runs
    //so snapshop all registers
    CurIndex = (EndIndex - 1 + MaxIndex) % MaxIndex;
    memcpy(&RecordStates[CurIndex].AfterState, &CPU.Registers, sizeof(CPURegStruct));
}

void DisplayRecordStates(unsigned int Count)
{
    unsigned int CurIndex;

    if(!RecordStates)
    {
        DebugOut("No state recorded");
        return;
    }

    if(Count > MaxIndex)
        Count = MaxIndex;

    //determine how many max entries we have and adjust count if need be so we don't go past start
    //start less than end then look inbetween
    //due to how I am indexing, there is a gap of 1 once we have rolled around so avoid printing
    //if too many
    if((!StartIndex) && (EndIndex < Count))
        Count = EndIndex;
    else if((StartIndex > EndIndex) && (Count >= MaxIndex))
        Count = MaxIndex - 1;

    //print count entries from the end of our list
    //oh the fun of this. So (0 - 9) % 10 gives 1 in python, -9 in C if signed values, and 7 if unsigned
    //hence the addition of MaxIndex to fix it
    CurIndex = ((EndIndex - Count) + MaxIndex) % MaxIndex;

    //loop until we hit the end
    for(; Count > 0; Count--)
    {
        DisassembleInstructions(RecordStates[CurIndex].BeforeState.R[31], 1, &RecordStates[CurIndex]);
        CurIndex = (CurIndex + 1) % MaxIndex;
    };
}

int SetRecordCount(unsigned int Count)
{
    //if we have a state then remove it as i'm not reorganizing it
    if(RecordStates)
    {
        free(RecordStates);
        RecordStates = 0;
    }

    //reset everything
    StartIndex = 0;
    EndIndex = 0;
    MaxIndex = 0;

    //allocate it then report if it was successful
    if(Count > 0)
    {
        RecordStates = (RecordStateStruct *)malloc((long)Count * sizeof(RecordStateStruct));
        if(!RecordStates)
            return 0;

        //all good, set the size
        MaxIndex = Count;
    }

    return 1;
}

unsigned int GetRecordCount()
{
    return MaxIndex;
}
