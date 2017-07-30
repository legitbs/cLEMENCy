//
//  network.h
//  cLEMENCy network code
//
//  Created by Lightning on 2015/12/22.
//  Copyright (c) 2017 Lightning. All rights reserved.
//

#ifndef __CLEMENCY__network__
#define __CLEMENCY__network__

extern int NetworkInFD;
extern int NetworkOutFD;
extern unsigned short Network_In_Buffer[0x2000];
extern unsigned int Network_In_Buffer_Size;
extern unsigned short Network_Out_Buffer[0x2000];
extern unsigned int Network_Out_Buffer_Size;
extern unsigned int Network_Out_Buffer_CurPos;

void InitNetwork();
void Network_Check(int IdleAlarmTime, double *TotalSleepMS, int Forced);
void Network_Flush();

#endif
