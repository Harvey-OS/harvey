#include <u.h>
#include <libc.h>
#include	<cda/fizz.h>

Signal *maxsig;

void
netlen(Signal *s)
{
	if((s->type == NORMSIG) || (s->type == SUBVSIG))
		if((maxsig == (Signal *) 0) || (s->n > maxsig->n))
			maxsig = s;
}
