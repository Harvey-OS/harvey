#ifndef _PLAN9_SOURCE
  This header file is an extension to ANSI/POSIX
#endif

#ifndef __QLOCK_H_
#define __QLOCK_H_
#pragma lib "/$M/lib/ape/lib9.a"

#include <u.h>
#include <lock.h>

typedef struct QLp QLp;
struct QLp
{
	int	inuse;
	QLp	*next;
	char	state;
};

typedef
struct QLock
{
	Lock	lock;
	int	locked;
	QLp	*head;
	QLp 	*tail;
} QLock;

#ifdef __cplusplus
extern "C" {
#endif

extern	void	qlock(QLock*);
extern	void	qunlock(QLock*);
extern	int	canqlock(QLock*);

#ifdef __cplusplus
}
#endif

#endif
