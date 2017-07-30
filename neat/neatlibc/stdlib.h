#include <stddef.h>

#define RAND_MAX		0x7ffffff

void init_memory(long addr, long size);
void *malloc(long n);
void free(void *m);
int mprotect(void *ptr, int length, int flags);

#define MPROTECT_NONE           0
#define MPROTECT_READ           1
#define MPROTECT_READWRITE      2
#define MPROTECT_READEXECUTE    3

int atoi(char *s);
long atol(char *s);
int abs(int n);
long labs(long n);

void exit(int status);
void abort(void);
int atexit(void (*func)(void));

char *getenv(char *name);
void qsort(void *a, int n, int sz, int (*cmp)(void *, void *));
int mkstemp(char *t);
int system(char *cmd);

void srand(unsigned int seed);
int rand(void);

/* for examining heap memory allocation */
#ifdef MEMTST
void *memtst_malloc(long n);
void memtst_free(void *v);
#define malloc	memtst_malloc
#define free	memtst_free
#endif
