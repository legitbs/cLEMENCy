#ifndef	_TIME_H
#define	_TIME_H

#include <sys/types.h>

struct timespec {
	long tv_sec;
#if ARCH != clemency
	long tv_nsec;
#else
	long tv_msec;
#endif
};

int nanosleep(struct timespec *req, struct timespec *rem);

struct tm {
	int tm_sec;
	int tm_min;
	int tm_hour;
	int tm_mday;
	int tm_mon;
	int tm_year;
	int tm_wday;
	int tm_yday;
	int tm_isdst;
	long tm_gmtoff;
	char *tm_zone;
};

time_t time(time_t *timep);
long strftime(char *s, long len, char *fmt, struct tm *tm);
struct tm *localtime(time_t *timep);
struct tm *gmtime(time_t *timep);

extern long timezone;
void tzset(void);

#endif
