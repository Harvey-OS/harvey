#ifndef __LOCK_H
#define __LOCK_H
#ifndef _LOCK_EXTENSION
   This header file is not defined in ANSI/POSIX
#endif
#pragma lib "/$M/lib/ape/libap.a"

typedef struct
{
	int	val;
} Lock;

#ifdef __cplusplus
extern "C" {
#endif

extern	void	lock(Lock*);
extern	void	unlock(Lock*);
extern	int	canlock(Lock*);
extern	int	tas(int*);

#ifdef __cplusplus
}
#endif

#endif
