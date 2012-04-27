#ifndef __WAIT_H
#define __WAIT_H
#pragma lib "/$M/lib/ape/libap.a"

/* flag bits for third argument of waitpid */
#define WNOHANG		0x1
#define WUNTRACED	0x2

/* macros for examining status returned */
#ifndef WIFEXITED
#define WIFEXITED(s)	(((s) & 0xFF) == 0)
#define WEXITSTATUS(s)	((s>>8)&0xFF)
#define WIFSIGNALED(s)	(((s) & 0xFF) != 0)
#define WTERMSIG(s)	((s) & 0xFF)
#define WIFSTOPPED(s)	(0)
#define WSTOPSIG(s)	(0)
#endif

#ifdef __cplusplus
extern "C" {
#endif

pid_t wait(int *);
pid_t waitpid(pid_t, int *, int);
#ifdef _BSD_EXTENSION
struct rusage;
pid_t wait3(int *, int, struct rusage *);
pid_t wait4(pid_t, int *, int, struct rusage *);
#endif

#ifdef __cplusplus
}
#endif

#endif
