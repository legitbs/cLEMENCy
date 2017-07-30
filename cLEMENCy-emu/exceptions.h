//
//  exceptions.h
//  cLEMENCy exception handler
//
//  Created by Lightning on 2015/12/20.
//  Copyright (c) 2017 Lightning. All rights reserved.
//

#ifndef __CLEMENCY__exceptions__
#define __CLEMENCY__exceptions__

#include <setjmp.h>

typedef enum ExceptionEnum
{
    EXCEPTION_MEMORY_READ,
    EXCEPTION_MEMORY_WRITE,
    EXCEPTION_MEMORY_EXECUTE,
    EXCEPTION_INVALID_INSTRUCTION,
    EXCEPTION_FLOATING_POINT,
    EXCEPTION_DIVIDE_BY_0,
    EXCEPTION_DBRK,			//not an official exception
    EXCEPTION_COUNT
} ExceptionEnum;

void InitExceptions();
void RaiseException(enum ExceptionEnum ExceptionType, unsigned int Value);

extern jmp_buf ExceptionJumpBuf;

#endif
