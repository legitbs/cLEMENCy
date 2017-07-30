#include <stddef.h>

void *memcpy(void *dst, void *src, long n);
void *memmove(void *dst, void *src, long n);
void *memset(void *s, int v, long n);
void *memchr(void *s, int c, long n);
void *memrchr(void *s, int c, long n);
int memcmp(char *s1, char *s2, long n);

char *strcpy(char *dst, char *src);
char *strchr(char *s, int c);
char *strrchr(char *s, int c);
long strlen(char *s);
int strcmp(char *s1, char *s2);

char *strncpy(char *d, char *s, long n);
char *strcat(char *d, char *s);
int strncmp(char *d, char *s, long n);
char *strstr(char *s, char *r);
