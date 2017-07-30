//
//  clock.h
//  cLEMENCy real time clock header
//
//  Created by Lightning on 2015/12/22.
//  Copyright (c) 2017 Lightning. All rights reserved.
//

#ifndef __CLEMENCY__clock__
#define __CLEMENCY__clock__

#define CLOCK_COUNT 4

unsigned int GetClockTimer(unsigned int ID);
void SetClockTimer(unsigned int ID, unsigned int Milliseconds);
unsigned int GetClockTimer_TimeLeft(unsigned int ID);
void SetClockTimer_TimeLeft(unsigned int ID, unsigned int Milliseconds);

unsigned long GetClock_TimeSinceAug02();
void SetClock_TimeSinceAug02(unsigned long TimeSince);

void InitClock();
void Timers_Check(unsigned int Forced);
unsigned int GetTimeToNextTimer();

#endif
