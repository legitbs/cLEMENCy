//
//  main.c
//  cLEMENCy - LEgitbs Middle ENdian Computer
//
//  Created by Lightning on 2015/11/21.
//  Copyright (c) 2017 Lightning. All rights reserved.
//

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <arpa/inet.h>
#include <pwd.h>
#include <grp.h>
#include <pthread.h>
#include <wait.h>
#include <seccomp.h>
#include <sys/prctl.h>
#include "network.h"
#include "io.h"
#include "interpretter.h"
#include "memory.h"
#include "clock.h"
#include "cpu.h"
#include "exceptions.h"
#include "interrupts.h"
#include "debug.h"
#include "recordstate.h"
#include "util.h"

typedef void * scmp_filter_ctx;

int ClientList[17];
int IsNum(char *);
double TotalSleepMS;

pthread_mutex_t lock;

void Init()
{
    //initialize everything
#ifndef NODEBUG
    InitDebug();
#ifndef TEAMBUILD
    InitRecordState();
#endif
#endif
    InitMemory();
    InitNetwork();
    InitClock();
    InitIO();
    InitExceptions();
    InitInterrupts();
    InitInterpretter();
}

void RunCPU(unsigned long MaxInstructions, int IdleAlarmTime)
{
    int MSToNextTimer;
    int InstructionSize;
    double TickCount;
    struct timespec ts;

    //figure out the Subnet from the ip then see if we already have a session from them
    struct sockaddr_in inaddr;
    socklen_t IP = sizeof(struct sockaddr_in);
    if(getpeername(NetworkInFD, (struct sockaddr *)&inaddr, &IP) < 0)
        IP = 0;
    else
        IP = inaddr.sin_addr.s_addr;

#ifndef NOTIME
    struct timespec startTime, endTime;
    double TotalMS;
    char InstChar;

    clock_gettime(CLOCK_REALTIME, &startTime);
    TotalSleepMS = 0;
#endif

    //start running the processor
    CPU.Running = 1;

    //setup location to be the exception handler
    if(sigsetjmp(ExceptionJumpBuf, 1))
    {
        //exception happened, just continue
        //continue;
    }

    while(CPU.Running && (!MaxInstructions || (CPU.TickCount < MaxInstructions)))
    {
#ifndef NODEBUG
    	//if the processor is being debugged then handle the debugger
    	if(CPU.Debugging)
        {
    		HandleDebugger();
            if(!CPU.Running)
                break;
        }
#endif

        //if the cpu is not waiting then process
        if(!CPU.Wait)
        {
            //run an instruction
            CPU.TickCount++;
#ifndef NODEBUG
#ifndef TEAMBUILD
            RecordBeforeState();
#endif
#endif
            InstructionSize = Interpretter_RunInstruction();

            //if no branch  then increment to the next instruction
            //if the debugger tripped then don't advance further
            //with an exception this will result in it returning to the failing code
            //unless the exception handler changes it
#ifdef NODEBUG
            if(!CPU.BranchHit)
#else
            if(!CPU.BranchHit && (!CPU.DebugTripped || (CPU.DebugTripped && DebugState == DEBUG_STATE_EXCEPTION)))
#endif
                CPU_PC = CPU_PC + InstructionSize;
            else
                CPU.BranchHit = 0;

#ifndef NODEBUG
#ifndef TEAMBUILD
            RecordAfterState();
#endif
#endif
            //check the timers
            Timers_Check(0);

            //now check the network
            Network_Check(IdleAlarmTime, &TotalSleepMS, 0);
        }
        else
        {
            //find out when the next timer will fire and delay up to 100ms to next timer
            //whichever is shorter
            MSToNextTimer = GetTimeToNextTimer();

            //if more than 100ms then delay 100ms
            if(MSToNextTimer > 100)
                MSToNextTimer = 100;

            //delay if need be
            if(MSToNextTimer > 0)
            {
                TotalSleepMS += (double)MSToNextTimer;
                ts.tv_sec = 0;
                ts.tv_nsec = MSToNextTimer * 1000000;

                //i don't account for this being interrupted but it shouldn't happen often enough to be an issue
                nanosleep(&ts, 0);
            }

            //check the timers
            Timers_Check(1);

            //now force check the network
            Network_Check(IdleAlarmTime, &TotalSleepMS, 1);
        }
    }

    Network_Flush();
    printf("Connected IP: %d.%d.%d.%d\n", (IP >> 0) & 0xff, (IP >> 8) & 0xff, (IP >> 16)  & 0xff, (IP >> 24) & 0xff);

#ifndef NODEBUG
    printf("CPU no longer running\n");
#endif

#ifndef NOTIME
    //get the end time
    clock_gettime(CLOCK_REALTIME, &endTime);

    //subtract start from end and adjust nano to millisecond
    endTime.tv_sec -= startTime.tv_sec;
    endTime.tv_nsec -= startTime.tv_nsec;
    endTime.tv_nsec /= 1000;

    //calculate number of nanoseconds then adjust to seconds
    TotalMS = (double)endTime.tv_sec;
    TotalMS *= 1000000.0;
    TotalMS += (double)endTime.tv_nsec;
    TotalMS /= 1000000.0;

    TotalSleepMS /= 1000.0;
    TotalMS -= TotalSleepMS;

    InstChar = 0;
//    if(CPU.TickCount >= 1000000)
//    {
        InstChar = 'm';
        TickCount = (double)CPU.TickCount / 1000000.0;
/*
    }
    else if(CPU.TickCount >= 1000)
    {
        InstChar = 'k';
        CPU.TickCount /= 1000;
    }
    */
    printf("Total instructions: %ld, %0.4lfm instructions/sec\n", CPU.TickCount, TickCount / TotalMS);
    printf("Running time: %lf seconds, sleep time: %lf seconds\n", TotalMS, TotalSleepMS);

    //printf("%0.4lf%c instructions/sec\n", TickCount / TotalMS, InstChar);
#endif
}

#ifdef NODEBUG
int ClientAllowed(int fd)
{
    struct sockaddr_in inaddr;
    unsigned int Subnet;
    int status;
    int ret;

    //figure out the Subnet from the ip then see if we already have a session from them
    Subnet = sizeof(struct sockaddr_in);
    if(getpeername(fd, (struct sockaddr *)&inaddr, &Subnet) < 0)
        Subnet = 0;
    else
        Subnet = (inaddr.sin_addr.s_addr >> 16) & 0xff;

    //if the IP is 10.3.1.x then allow it
    if((inaddr.sin_addr.s_addr & 0x00ffffff) ==  0x0001030a)
        return 0;

    //if we have a value in the client list then don't allow them
    //remember 16 is our oracle
    pthread_mutex_lock(&lock);
    if(Subnet > 16)
        Subnet = -1;
    else if(ClientList[Subnet])
    {
        //check and see if the pid is still valid, if invalid then remove the list entry
        ret = waitpid(ClientList[Subnet], &status, WNOHANG);
        if((ret == -1) || ((ret > 0) && (WIFEXITED(status) || WIFSIGNALED(status))))
            ClientList[Subnet] = 0;
        Subnet = -1;
    }
    pthread_mutex_unlock(&lock);

    //allow them in
    return Subnet;
}

void ChildHandler(int signal, siginfo_t *siginfo, void *vcontext)
{
    unsigned long i;

    //see if we can find the pid, if so then remove it from the list
    pthread_mutex_lock(&lock);
    for(i = 0; i < (sizeof(ClientList) / sizeof(int)); i++)
    {
        if(ClientList[i] == siginfo->si_pid)
        {
            ClientList[i] = 0;
            break;
        }
    }
    pthread_mutex_unlock(&lock);
}

void SetupChildHandler()
{
    struct sigaction sa;

    //capture child closing signal so we can remove them from the list
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_NOCLDSTOP | SA_NOCLDWAIT | SA_SIGINFO;
    sa.sa_sigaction = ChildHandler;
    sigaction(SIGCHLD, &sa, 0);
}

int DropPrivileges(const char *username)
{
    struct passwd *userinfo;

    userinfo = getpwnam(username);
    if(userinfo == 0)
    {
        printf("Failed to locate user %s\n", username);
        return -1;
    }

    if(setgid(userinfo->pw_gid) == -1)
    {
        printf("Failed to set gid\n");
        return -1;
    }

    if(setgroups(0, NULL) == -1)
    {
        printf("Failed to remove groups\n");
        return -1;
    }

	if(setuid(userinfo->pw_uid) == -1)
    {
        printf("Failed to set uid\n");
        return -1;
    }

    return 0;
}
#endif

int ListenOnPort(int ListenPort)
{
	struct sockaddr_in addr;
    int i;

    memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(ListenPort);
	addr.sin_addr.s_addr = INADDR_ANY;

	// Create socket, setup the reuse, bind, then listen for a connection
	ListenPort = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ListenPort == -1)
	{
		DebugOut("Failed to create tcp socket");
    		return -1;
	}

	i = 1;
	if (setsockopt(ListenPort, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(int)) == -1)
	{
		DebugOut("Failed to set socket reuse");
		return -1;
	}

	if (bind(ListenPort, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1)
	{
		DebugOut("Failed to bind to ipv4 socket");
		return -1;
	}

	if (listen(ListenPort, 20) == -1)
	{
		DebugOut("Failed to setup listen for socket");
    		return -1;
	}

    return ListenPort;
}

void SendErrorMessage(int fd, char *Msg)
{
        unsigned short InMsg[200];
        unsigned char OutMsg[200];
        unsigned int i;
        unsigned int DataLen;
        unsigned int SendLen;

        //convert the message into shorts to send, make sure the data isn't too long
        DataLen = strlen(Msg);
        if(DataLen >= (sizeof(InMsg) / sizeof(short)))
            DataLen = (sizeof(InMsg) / sizeof(short)) - 1;

        //convert the data to shorts
        for(i = 0; i < DataLen; i++)
            InMsg[i] = (unsigned short)Msg[i];

        SendLen = Convert_9_to_8(OutMsg, InMsg, sizeof(OutMsg), &DataLen);
        send(fd, OutMsg, SendLen, 0);
}

int HandleConnectionsAndFork(int ListenPort, int DebugPort, int DoNotFork)
{
    int Client = 0;
    int ChildID = 0;

    //get a connection
    Client = accept(ListenPort, NULL, NULL);
    if (Client == -1)
        return -1;

    //see if they are allowed to connect
#ifdef NODEBUG
    int ClientID = ClientAllowed(Client);
    if(ClientID == -1)
    {
        SendErrorMessage(Client, "Already connected\n");
        close(Client);
        return -1;
    }
#endif

#ifndef NODEBUG
    //if we have a debug port then check it for a onnection
    int DebugClient = 0;
    if(DebugPort)
    {
        DebugClient = accept(DebugPort, NULL, NULL);
        if (DebugClient == -1)
        {
            close(Client);
            return -1;
        }
    }
#endif

    //fork
    if(!DoNotFork)
        ChildID = fork();
    else
        ChildID = 0;    //lie saying we are the child

    if (ChildID == -1)
    {
        close(Client);
#ifndef NODEBUG
        if(DebugPort)
            close(DebugClient);
#endif
        return -1;
    };

    if (ChildID == 0)
    {
        //child, handle properly, close the listening port
        close(ListenPort);

        //setup network fd's
        NetworkInFD = Client;
		NetworkOutFD = Client;

#ifndef NODEBUG
		//if debugging then dup accordingly for stdin/stdout
		if(DebugPort)
		{
            close(DebugPort);

			dup2(DebugClient, 0);
			dup2(DebugClient, 1);

			//indicate we are going over the network
			printf("Debug over network set\n");
			DebugOverNetwork = 1;
		}
#endif
        //indicate child
        return 1;
    }
    else
    {
#ifdef NODEBUG
        pthread_mutex_lock(&lock);
        ClientList[ClientID] = ChildID;     //store the pid
        pthread_mutex_unlock(&lock);
#endif
    }

    //parent, close the client port fd and indicate parent
    close(Client);
#ifndef NODEBUG
    if(DebugPort)
        close(DebugClient);
#endif
    return 0;
}

typedef void (*sighandler_t)(int);
static void alarmHandle()
{
    CPU.Running = 0;
}

void PrintHelp(char *MainApp)
{
	printf("%s ", MainApp);
#ifndef NODEBUG
	printf("[-d] [-s] ");
#endif
	printf("[-f x] [-l x] firmware\n");
#ifndef NODEBUG
	printf("-d x\tTurn on debugging using port x, port 0 will use stdin/stdout\n");
	printf("-s\tAuto start execution\n");
#endif
	printf("-f x\tSet in and out file descriptor for networking to x\n");
	printf("-l x\tListen on port x and fork, incompatible with -f\n");
    printf("-L x\tListen on port x, incompatible with -f\n");
	printf("-m x\tMaximum size for firmware. suffixes K (1024) and M (1024*1024) can be used\n");
	printf("-S\tEnable shared memory\n");
	printf("-N\tEnable nvram memory\n");
    printf("-A x\tAlarm after x total seconds of run time, incompatible with -a\n");
    printf("-a x\tAlarm after x seconds of idle time between receives, incompatible with -A\n");
    printf("-I x\tStop execution after x number of cpu instructions\n");
#ifdef NODEBUG
    printf("-u x\tDrop privileges to user x\n");
#endif
    printf("\nEnjoy the NFO section!\n");
}

#ifdef NODEBUG
int SetupSeccomp(int OpenFiles)
{
    scmp_filter_ctx ctx;
    int Ret;

    // turn on seccomp
    Ret = 0;
    ctx = seccomp_init(SCMP_ACT_KILL);

    //make sure x86_64 is added and that if x86 or native exist to remove them
    Ret = seccomp_arch_exist(ctx, SCMP_ARCH_X86);
    if(Ret < 0)
        seccomp_arch_add(ctx, SCMP_ARCH_X86_64);

    Ret = seccomp_arch_exist(ctx, SCMP_ARCH_X86);
    if(Ret == 0)
        seccomp_arch_remove(ctx, SCMP_ARCH_X86);

    Ret = seccomp_arch_exist(ctx, SCMP_ARCH_NATIVE);
    if(Ret == 0)
        seccomp_arch_remove(ctx, SCMP_ARCH_NATIVE);

    if(ctx)
        Ret =   (seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(exit), 0) < 0) ||
                (seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(exit_group), 0) < 0) ||
                (seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(mmap), 0) < 0) ||
                (seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(read), 0) < 0) ||
                (seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(write), 0) < 0) ||
                (seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(shmctl), 0) < 0) ||
                (seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(select), 0) < 0) ||
                (seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(alarm), 0) < 0) ||
                (seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(nanosleep), 0) < 0) ||
                (seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(getpeername), 0) < 0) ||
                (seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(clock_gettime), 0) < 0) ||
                (seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(time), 0) < 0) ||
                (seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(fstat), 0) < 0) ||
                (seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(kill), 0) < 0) ||
                (seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(rt_sigprocmask), 0) < 0) ||
                (seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(rt_sigreturn), 0) < 0);

/*
    if(!Ret && CPU.Debugging)
        Ret =   (seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(ioctl), 0) < 0) ||
                (seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(access), 0) < 0) ||
                (seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(stat), 0) < 0) ||
                (seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(brk), 0) < 0) ||
                (seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(rt_sigaction), 0) < 0) ||
                (seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(rt_sigreturn), 0) < 0) ||
                (seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(munmap), 0) < 0);
*/

    //if we need to open files then allow it
    if(!Ret && OpenFiles) // || CPU.Debugging))
        Ret =   (seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(open), 0) < 0) ||
                (seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(close), 0) < 0);
    if(!Ret && OpenFiles)
        Ret =   (seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(lseek), 0) < 0) ||
                (seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(msync), 0) < 0);

    //load the seccomp
    if(!Ret)
        Ret = (seccomp_load(ctx) < 0);

    if(!ctx || Ret){
            printf("Could not initialize seccomp!\n");
            seccomp_release(ctx);
            exit(-1);
    }

    return 0;
}
#endif

int main(int argc, const char * argv[]) {
	int FilenamePos;
	int i;
	int ListenPort = 0;
    int DoNotFork = 0;
	int DebugPort = 0;
	int MaxFirmwareSize = 0;
	int MaxFirmwareSizeMultiplier = 1;
	char *EndChar;
	int SharedMemory = 0;
	int NVRamMemory = 0;
    unsigned long MaxInstructions = 0;
    int TotalAlarmTime = 0;
    int IdleAlarmTime = 0;
	FilenamePos = -1;

#ifdef NODEBUG
    const char *Username = 0;

    prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);

    //setup the mutex lock
    if (pthread_mutex_init(&lock, 0))
    {
        printf("Failed to setup mutex!\n");
        return 1;
    }
#endif

    //init the processor
	Init();

	//debugger is hard set stdin/stdout, -f will let you change it
	NetworkInFD = 0;
	NetworkOutFD = 1;
    memset(ClientList, 0, sizeof(ClientList));

	if((argv[1] == 0) || (strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "--help") == 0))
	{
		PrintHelp((char *)argv[0]);
		_exit(0);
	}

	for(i = 1; argv[i]; i++)
	{
		//if debugging then turn on features
#ifndef NODEBUG
		if(strcmp(argv[i], "-d") == 0)
		{
			//check next argument for port for debugging
			if(argv[i+1] && IsNum((char *)argv[i+1]))
			{
				//set the in FD, if not 0 then set out also
				DebugPort = atoi((char *)argv[i+1]);
				i++;
			}
			else
			{
				DebugOut("No valid file descriptor provided");
				_exit(0);
			}
			CPU.Debugging = 1;
		}
		else if(strcmp(argv[i], "-s") == 0)
			DebugState = DEBUG_STATE_RUNNING;
		else
#endif
		if(strcmp(argv[i], "-f") == 0)
		{
			if(argv[i+1] && IsNum((char *)argv[i+1]))
			{
				NetworkInFD = atoi((char *)argv[i+1]);
				NetworkOutFD = atoi((char *)argv[i+1]);
				i++;
			}
			else
			{
				DebugOut("No valid file descriptor provided");
				_exit(0);
			}
		}
		else if((strcmp(argv[i], "-l") == 0) || (strcmp(argv[i], "-L") == 0))
		{
            //do not fork if capital L
            if(argv[i][1] == 'L')
                DoNotFork = 1;

			if(argv[i+1] && IsNum((char *)argv[i+1]))
			{
				ListenPort = atoi((char *)argv[i+1]);
				i++;
			}
			else
			{
				DebugOut("No valid port provided");
				_exit(0);
			}
		}
		else if(strcmp(argv[i], "-m") == 0)
		{
			if(!argv[i+1])
			{
				DebugOut("No valid maximum size provided");
				_exit(0);
			}

			//figure out the multiplier
			EndChar = &((char *)argv[i+1])[strlen(argv[i+1])-1];
			MaxFirmwareSizeMultiplier = 1;
			if((*EndChar == 'K') || (*EndChar == 'k'))
			{
				MaxFirmwareSizeMultiplier = 1024;
				*EndChar = 0;
			}
			else if((*EndChar == 'M') || (*EndChar == 'm'))
			{
				MaxFirmwareSizeMultiplier = 1024*1024;
				*EndChar = 0;
			}

			if(IsNum((char *)argv[i+1]))
			{
				MaxFirmwareSize = atoi((char *)argv[i+1]) * MaxFirmwareSizeMultiplier;
				i++;
			}
			else
			{
				DebugOut("Invalid maximum size provided");
				_exit(0);
			}
		}
		else if(strcmp(argv[i], "-S") == 0)
			SharedMemory = 1;
		else if(strcmp(argv[i], "-N") == 0)
			NVRamMemory = 1;
        else if(strcmp(argv[i], "-A") == 0)
        {
            if(IsNum((char *)argv[i+1]))
            {
                TotalAlarmTime = atoi((char *)argv[i+1]);
                i++;
            }
            else
            {
                DebugOut("Invalid total alarm time provided");
                _exit(0);
            }
        }
        else if(strcmp(argv[i], "-a") == 0)
        {
            if(IsNum((char *)argv[i+1]))
            {
                IdleAlarmTime = atoi((char *)argv[i+1]);
                i++;
            }
            else
            {
                DebugOut("Invalid idle alarm time provided");
                _exit(0);
            }
        }
        else if(strcmp(argv[i], "-I") == 0)
        {
            if(IsNum((char *)argv[i+1]))
            {
                MaxInstructions = atol((char *)argv[i+1]);
                i++;
            }
            else
            {
                DebugOut("Invalid maximum number of instructions provided");
                _exit(0);
            }
        }
#ifdef NODEBUG
        else if(strcmp(argv[i], "-u") == 0)
        {
            //get the `username` to drop privs to
            Username = argv[i+1];
            i++;
        }
#endif
        else if((strcmp(argv[i], "-h") == 0) || (argv[i][0] == '-') || (FilenamePos != -1))
        {
            //unknown, print the help out
            PrintHelp((char *)argv[0]);
            _exit(0);
        }
		else
			FilenamePos = i;
	}

    //check the alarm times
    if(TotalAlarmTime && IdleAlarmTime)
    {
        DebugOut("-A and -a can not be used at the same time");
        return 0;
    }

	//see if firmware is valid
	if((FilenamePos == -1) || (!argv[FilenamePos]))
	{
		DebugOut("No firmware specified");
		return 0;
	}

	//see if they tried to listen and specify a file descriptor port
	if(ListenPort && (NetworkInFD == NetworkOutFD))
	{
		DebugOut("-f and -l can not be used at the same time");
		return 0;
	}

	//if there is shared memory then init it
	if(SharedMemory)
		InitSharedMemory((char *)argv[FilenamePos]);

	//if we have a listen port then spawn a socket to listen on and start listening
	if(ListenPort)
	{
        //establish both port listens
        ListenPort = ListenOnPort(ListenPort);
#ifndef NODEBUG
        if(DebugPort)
            DebugPort = ListenOnPort(DebugPort);
#endif

#ifdef NODEBUG
        //if we have a username then drop privileges
        if(Username && (DropPrivileges(Username) < 0))
            return 0;

        //setup the child handler
        SetupChildHandler();
#endif

        while(1)
        {
            //handle connections, break on child
            i = HandleConnectionsAndFork(ListenPort, DebugPort, DoNotFork);
            if(i == 1)
                break;
        };
	}

#ifndef NODEBUG
	//attempt to load up the map file for the firmware if debugging
	if(CPU.Debugging)
		LoadMapFile((char *)argv[FilenamePos]);
#endif

	//make sure we weren't given a bad file descriptor if not in listen mode
	if(!ListenPort && (NetworkInFD != 0) && (fcntl(NetworkInFD, F_GETFD) == -1 || errno == EBADF))
	{
		DebugOut("Input file descriptor is invalid");
		_exit(0);
	}

	//load up the file passed in
	if(LoadFile((char *)argv[FilenamePos], MaxFirmwareSize))
	{
		printf("Error loading %s\n", (char *)argv[FilenamePos]);
		return 0;
	}

	//if the nvram is backed then init it
	if(NVRamMemory)
		InitNVRamMemory((char *)argv[FilenamePos], NetworkInFD);

#ifndef NODEBUG
    //if any alarms are set and we are debuggin then tell them we are not alarming
    if(CPU.Debugging && (TotalAlarmTime || IdleAlarmTime))
    {
        DebugOut("Disabling alarms due to debugging");
        TotalAlarmTime = 0;
        IdleAlarmTime = 0;
    }
#endif

    //if either alarm is active setup the signal handler to call exit
    if(TotalAlarmTime || IdleAlarmTime)
        signal(SIGALRM, (sighandler_t)alarmHandle);

    //if we have an alarm time then start it
    if(TotalAlarmTime)
        alarm(TotalAlarmTime);
    else if(IdleAlarmTime)
        alarm(IdleAlarmTime);

    //read the flag before we remove open/close along with cpu random
    ReadFlagFile();
    CPU_Random();

#ifdef NODEBUG
    //Setup seccomp
    SetupSeccomp(SharedMemory | NVRamMemory);
#endif

    //and run
	RunCPU(MaxInstructions, IdleAlarmTime);

	return 0;
}
