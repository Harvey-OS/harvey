#include "u.h"
#include "lib.h"
#include "dat.h"
#include "fns.h"
#include "error.h"

Label*
pwaserror(void)
{
	if(up->nerrlab == NERR)
		panic("error stack overflow");
	return &up->errlab[up->nerrlab++];
}

void
nexterror(void)
{
	longjmp(up->errlab[--up->nerrlab].buf, 1);
}

void
error(char *e)
{
	kstrcpy(up->errstr, e, ERRMAX);
	setjmp(up->errlab[NERR-1].buf);
	nexterror();
}
