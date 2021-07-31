#ifndef __WAIT_H
#define __WAIT_H
#pragma lib "/$M/lib/ape/libap.a"

/* flag bits for third argument of waitpid */
#define WNOHANG		0x1
#define WUNTRACED	0x2

/* macros for examining status returned */
#define WIFEXITED(s)	(((s) & 0xFF) == 0)
#define WEXITSTATUS(s)	((s>>8)&0xFF)
#define WIFSIGNALED(s)	(((s) & 0xFF) != 0)
#define WTERMSIG(s)	((s) & 0xFF)
#define WIFSTOPPED(s)	(0)
#define WSTOPSIG(s)	(0)

#ifdef __cplusplus
extern "C" {
#endif

pid_t wait(int *);
pid_t waitpid(pid_t, int *, int);

#ifdef __cplusplus
}
#endif

#endif
