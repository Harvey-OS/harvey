#include <u.h>
#include <libc.h>
#include "lock.h"

/*
 *  mutex locks.  we use a pipe with a single token since it's
 *  the only thing that works across all our architectures.
 */

void
lockinit(Lock *l)
{
	if(pipe(l->parfd) < 0)
		abort();
	unlock(l);
}

void
lock(Lock *l)
{
	char	token[1];

	read(l->parfd[0], token, 1);
}

void
unlock(Lock *l)
{
	char	token[1];

	write(l->parfd[1], token, 1);
}
