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

static void
ding(void *x, char *msg)
{
	USED(x);
	if(strcmp(msg, "alarm") == 0)
		noted(NCONT);
	else
		noted(NDFLT);
}

void
lock(Lock *l)
{
	char token[1];
	char error[ERRLEN];

	notify(ding);
	alarm(1000*5*60);
	if(read(l->parfd[0], token, 1) < 0){
		errstr(error);
		if(strstr(error, "interrupt"))
			unlock(l);
	}
	alarm(0);
}

void
unlock(Lock *l)
{
	char	token[1];

	write(l->parfd[1], token, 1);
}
