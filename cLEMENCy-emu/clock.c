//
//  clock.c
//  cLEMENCy real time clock
//
//  Created by Lightning on 2015/12/22.
//  Copyright (c) 2017 Lightning. All rights reserved.
//
#include <time.h>
#include <string.h>
#include "clock.h"
#include "interrupts.h"

typedef struct CLEM_TimerStruct
{
    unsigned int Milliseconds;
    unsigned long NextEvent;
} CLEM_TimerStruct;

CLEM_TimerStruct Timers[CLOCK_COUNT];
long TimeSinceAug02_Adjust;

void InitClock()
{
    memset(Timers, 0, sizeof(Timers));
    TimeSinceAug02_Adjust = 0;
}

unsigned long GetMS()
{
    struct timespec CurTime;

    //get the current time
    clock_gettime(CLOCK_MONOTONIC_RAW, &CurTime);

    //calculate a current millisecond
    return (CurTime.tv_nsec / 1000000) + (CurTime.tv_sec * 1000);
}

unsigned int GetClockTimer(unsigned int ID)
{
    //make sure ID is valid
    if(ID >= CLOCK_COUNT)
        return 0;

    //return the number of milliseconds this clock is set to
    return Timers[ID].Milliseconds;
}

unsigned int GetClockTimer_TimeLeft(unsigned int ID)
{
    unsigned long CurMS;

    //make sure ID is valid
    if(ID >= CLOCK_COUNT)
        return 0;

    //return the number of milliseconds left for this clock
    if(Timers[ID].Milliseconds == 0)
        return 0;

    //return number of milliseconds to next event
    //check to make sure the next event isn't in the past, if so return 0 as we will be firing shortly
    CurMS = GetMS();
    if(Timers[ID].NextEvent < CurMS)
        return 0;
    else
        return Timers[ID].NextEvent - CurMS;
}

void SetClockTimer(unsigned int ID, unsigned int Milliseconds)
{
    //make sure ID is valid
    if(ID >= CLOCK_COUNT)
        return;

    //set the clock event and the milliseconds for when to fire
    Timers[ID].Milliseconds = Milliseconds;
    if(Milliseconds)
        SetClockTimer_TimeLeft(ID, Milliseconds);
}

void SetClockTimer_TimeLeft(unsigned int ID, unsigned int Milliseconds)
{
    //make sure ID is valid
    if(ID >= CLOCK_COUNT)
        return;

    //adjust how much time is left to fire the interrupt
    Timers[ID].NextEvent = GetMS() + Milliseconds;
}

unsigned long GetClock_TimeSinceAug02()
{
    time_t CurTime;

    //get actual seconds Since Jan 01, 1970 00:00 UTC
    //then subtract the value required to get to Aug 2nd
    CurTime = time(0) - 1375459200;

    //now adjust for what they have in adjustment
    return CurTime + TimeSinceAug02_Adjust;
}

void SetClock_TimeSinceAug02(unsigned long TimeSince)
{
    time_t CurTime;

    //get actual seconds Since Jan 01, 1970 00:00 UTC
    CurTime = time(0) - 1375459200;

    //now we have a time that is number of seconds since Aug 2, 2013 9:00 PST that we want

    //they have adjusted the value though so take what they want and subtract it from
    //what we have so we know how much to add to get the same value when recalculating time
    TimeSinceAug02_Adjust = TimeSince - CurTime;
}

void Timers_Check(unsigned int Forced)
{
    //cycle through all timers and fire them if need be, we keep calling the interrupt code to fire
    //as it will handle pushing to the stack/etc and allow queuing up interrupts too
    unsigned long CurMS;
    unsigned int ID;
    static int Counter = 0;

    //check every 1000 insturctions or when forced due to wait
    Counter++;
    if(!Forced && (Counter < 1000))
        return;

    Counter = 0;

    CurMS = GetMS();
    for(ID = 0; ID < CLOCK_COUNT; ID++)
    {
        //if the timer is active and time has passed or matches then fire it
        if(Timers[ID].Milliseconds && (CurMS >= Timers[ID].NextEvent))
        {
            //adjust to the next millisecond time we need
            Timers[ID].NextEvent = CurMS + Timers[ID].Milliseconds;

            //fire interrupt
            FireInterrupt(INTERRUPT_TIMER1 + ID);
        }
    }
}

unsigned int GetTimeToNextTimer()
{
    //figure out what timer will fire next and return the time left in milliseconds
    unsigned long CurMS;
    int ID;
    int TimeLeft = 100;    //default 100ms incase just waiting on network

    CurMS = GetMS();
    for(ID = 0; ID < CLOCK_COUNT; ID++)
    {
        //printf("id: %d, ms: %d, NextEvent: %d, cur: %d\n", ID, Timers[ID].Milliseconds, Timers[ID].NextEvent, CurMS);

        //if the timer is active and time has passed or matches then fire it
        if(Timers[ID].Milliseconds && ((long)(Timers[ID].NextEvent - CurMS) < TimeLeft))
        {
            TimeLeft = Timers[ID].NextEvent - CurMS;
        }
    }

    //return time left
    return TimeLeft;
}
