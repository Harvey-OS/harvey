#if !defined(_RESEARCH_SOURCE) && !defined(_PLAN9_SOURCE)
   This header file is an extension of ANSI/POSIX
#endif

#ifndef __LOCK_H
#define __LOCK_H
#pragma lib "/$M/lib/ape/libap.a"

#include <u.h>

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
