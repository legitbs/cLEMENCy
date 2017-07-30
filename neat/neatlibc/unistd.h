#include <stddef.h>

/* access() flags */
#define R_OK		4
#define W_OK		2
#define X_OK		1
#define F_OK		0

int access(char *name, int type);
int unlink(char *path);

extern char **environ;

int close(int fd);
long write(void *buf, long n);
long read(void *buf, long n);

/* lseek() whence */
#define SEEK_SET	0
#define SEEK_CUR	1
#define SEEK_END	2

long lseek(int fd, long offset, int whence);

int pipe(int fds[2]);
int dup(int fd);
int dup2(int fd, int fd2);

int fork(void);
int getpid(void);
int getppid(void);

int execve(char *path, char *argv[], char *envp[]);
int execle(char *path, ...);
int execvp(char *file, char *argv[]);
int execv(char *path, char *argv[]);

void _exit(int status);

int sleep(int n);

/* standard file descriptors */
#define STDIN_FILENO	0
#define STDOUT_FILENO	1
#define STDERR_FILENO	2

void halt();
int setup_interrupt(int InterruptNum, void *Function);

