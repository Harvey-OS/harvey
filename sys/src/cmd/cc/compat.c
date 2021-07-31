#include	<u.h>
#include	<libc.h>

int
mycreat(char *n, int p)
{

	return create(n, 1, p);
}

char*
myerrstr(int eno)
{
	char err[ERRLEN];

	USED(eno);
	errstr(err);
	return err;
}

int
mywait(int *s)
{
	int p;
	Waitmsg status;

	p = wait(&status);
	*s = 0;
	if(status.msg[0])
		*s = 1;
	return p;
}

int
unix(void)
{

	return 0;
}
