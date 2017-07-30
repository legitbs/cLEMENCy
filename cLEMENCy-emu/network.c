//
//  network.c
//  cLEMENCy network code
//
//  Created by Lightning on 2015/12/22.
//  Copyright (c) 2017 Lightning. All rights reserved.
//

#include <string.h>
#include <sys/select.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include "network.h"
#include "interrupts.h"
#include "cpu.h"
#include "memory.h"

int NetworkInFD;
int NetworkOutFD;
unsigned short Network_In_Buffer[0x2000];
unsigned int Network_In_Buffer_Size;
unsigned short Network_Out_Buffer[0x2000];
unsigned int Network_Out_Buffer_Size;
unsigned int Network_Out_Buffer_CurPos;

//avoid extra debug logic in here
#undef NODEBUG
#define NODEBUG

void InitNetwork()
{
    memset(Network_In_Buffer, 0, sizeof(Network_In_Buffer));
    memset(Network_Out_Buffer, 0, sizeof(Network_Out_Buffer));
    Network_In_Buffer_Size = 0;
    Network_Out_Buffer_Size = 0;
    Network_Out_Buffer_CurPos = 0;
    NetworkInFD = -1;
    NetworkOutFD = -1;
}

void Network_HandleReceiveBuffer(int IdleAlarmTime,  double *TotalSleepMS)
{
    fd_set FDs;
    struct timeval timeout;
    int Result;
    unsigned char TempBuffer[0x2000];

    memset(TempBuffer, 0, sizeof(TempBuffer));

    //check select for data
    FD_ZERO(&FDs);
    FD_SET(NetworkInFD, &FDs);

    //0 timeout is immediate return
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    //check select
    Result = select(NetworkInFD + 1, &FDs, 0, 0, &timeout);

    if(Result < 0)
    {
        //select failed, just bail
#ifndef NODEBUG
	printf("Result < 0\n");
#endif
        CPU.Running = 0;
        return;
    }

    //if we have data to process then read it
    if(Result)
    {
#ifndef NODEBUG
        printf("trying to receive data\n");
#endif
        Result = read(NetworkInFD, TempBuffer, sizeof(TempBuffer));
#ifndef NODEBUG
	printf("Received %d bytes\n", Result);
#endif
        if(Result <= 0)
        {
            //read failed, just bail
            CPU.Running = 0;
            return;
        }

#ifndef NODEBUG
    	if(Result)
    	{
    		for(int i = 0; i < Result; i++)
    			printf("%02x ", (unsigned char)(TempBuffer[i]) & 0xff);
    		printf("\n");
    	}
#endif

    	//convert the data
    	Result = Convert_8_to_9(Network_In_Buffer, TempBuffer, Result);

        //indicate how much data we have and fire the interrupt indicating DMA completed
        Network_In_Buffer_Size = Result;
        FireInterrupt(INTERRUPT_DATA_RECIEVED);

        //reset the idle timer if set
        if(IdleAlarmTime)
            alarm(IdleAlarmTime);
    }
}

void Network_HandleSendBuffer(int FinalFlush)
{
	struct timespec ts;

	int Result;
	int TempSize;
	unsigned char TempBuffer[0x2000];

    #ifndef NODEBUG
        	if(Network_Out_Buffer_Size)
        	{
        		for(int i = 0; i < Network_Out_Buffer_Size; i++)
        			printf("%03x ", Network_Out_Buffer[i] & 0x1ff);
        		printf("\n");
        	}
    #endif

	//convert the data to send
	TempSize = Convert_9_to_8(TempBuffer, &Network_Out_Buffer[Network_Out_Buffer_CurPos], sizeof(TempBuffer), &Network_Out_Buffer_Size);

#ifndef NODEBUG
	printf("Sending %d bytes\n", TempSize);
#endif

#ifndef NODEBUG
        if(TempSize)
        {
            for(int i = 0; i < TempSize; i++)
                printf("%02x ", TempBuffer[i] & 0xff);
            printf("\n");
        }
#endif

	//attempting to send data, if it fails turn off the cpu
	Result = write(NetworkOutFD, TempBuffer, TempSize);
	if(Result < 0)
	{
#ifndef NODEBUG
		printf("Send failed\n");
#endif
		CPU.Running = 0;
        NetworkOutFD = -1;
		return;
	}

	//sleep for a very short time to make sure the packet goes if network based
	ts.tv_sec = 0;
	ts.tv_nsec =  10;	//0.01ms
	nanosleep(&ts, 0);

	//if 0 bytes are left then indicate data was sent
	if(!FinalFlush && (Network_Out_Buffer_Size == 0))
	{
		Network_Out_Buffer_CurPos = 0;
		FireInterrupt(INTERRUPT_DATA_SENT);
	}
}

void Network_Check(int IdleAlarmTime, double *TotalSleepMS, int Forced)
{
    static int Counter = 0;

    Counter++;

    //if we don't have a network fd then exit
    if((NetworkInFD == -1) || (NetworkOutFD == -1))
        return;

    //if the in buffer is empty then check for new data after some time has passed
    if(Forced || (Counter >= 1000))
    {
        Counter = 0;
        if(Network_In_Buffer_Size == 0)
	       Network_HandleReceiveBuffer(IdleAlarmTime, TotalSleepMS);
    }

    //if we have data to send then send it
    if(Network_Out_Buffer_Size)
        Network_HandleSendBuffer(0);
}

void Network_Flush()
{
    struct timespec ts;

    //if we have data to send then send it
    //if the write has not failed previously
    if((NetworkOutFD != -1) && Network_Out_Buffer_Size)
        Network_HandleSendBuffer(1);

    if(NetworkOutFD != -1)
    {
        ts.tv_sec = 0;
        ts.tv_nsec = 1000000;

        //i don't account for this being interrupted but it shouldn't happen often enough to be an issue
        nanosleep(&ts, 0);
    }
}
