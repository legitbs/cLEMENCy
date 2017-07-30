#ifndef _FCNTL_H
#define _FCNTL_H

#define O_RDONLY	00000
#define O_WRONLY	00001
#define O_RDWR		00002
#define O_ACCMODE	00003
#define O_CREAT		00100
#define O_EXCL		00200
#define O_NOCTTY	00400
#define O_TRUNC		01000
#define O_APPEND	02000
#define O_NONBLOCK	04000
#define O_SYNC		0010000
#define FASYNC		0020000
#define O_DIRECT	0040000
#define O_LARGEFILE	0100000
#define O_DIRECTORY	0200000
#define O_NOFOLLOW	0400000
#define O_NOATIME	001000000

#define F_DUPFD		0
#define F_GETFD		1
#define F_SETFD		2
#define F_GETFL		3
#define F_SETFL		4
#define F_GETLK		5
#define F_SETLK		6
#define F_SETLKW	7
#define F_SETOWN	8
#define F_GETOWN	9
#define F_SETSIG	10
#define F_GETSIG	11

#define FD_CLOEXEC	1

#define F_RDLCK		0
#define F_WRLCK		1
#define F_UNLCK		2

int open(char *path, int flags, ...);
int creat(char *path, int mode);
int fcntl(int fd, int cmd, ...);

#endif
