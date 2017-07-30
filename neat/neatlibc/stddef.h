#ifndef _STDDEF_H
#define _STDDEF_H

#define NULL			((void *) 0)
#define offsetof(type, field)	((int) (&((type *) 0)->field))

typedef unsigned long size_t;

#endif
