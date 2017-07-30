#ifndef _STDARG_H
#define _STDARG_H

typedef void *va_list;

void *__va_arg(void **ap, int size);

#define va_start(ap, last)	((ap) = ((void *) &(last)) + sizeof(long))
#define va_arg(ap, type)	(*(type *) __va_arg(&(ap), sizeof(type)))
#define va_end(ap)		((ap) = (void *) 0)

#endif
