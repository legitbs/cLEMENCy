//
//  debug-cmds.h
//  cLEMENCy debugger commands
//
//  Created by Lightning on 2015/12/20.
//  Copyright (c) 2017 Lightning. All rights reserved.
//

#ifndef __CLEMENCY_DEBUG_CMDS__
#define __CLEMENCY_DEBUG_CMDS__


typedef int (*DebugFunc)(char *);

typedef struct DebugCmdsStruct
{
	char *Cmd;
	DebugFunc Func;
	char *Help;
} DebugCmdsStruct;

extern DebugCmdsStruct DebugCmds[];

#endif
