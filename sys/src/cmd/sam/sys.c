#include "sam.h"

static int inerror=FALSE;

/*
 * A reasonable interface to the system calls
 */

void
resetsys(void)
{
	inerror = FALSE;
}

void
syserror(char *a)
{
	char buf[ERRLEN];

	if(!inerror){
		inerror=TRUE;
		errstr(buf);
		dprint("%s: ", a);
		error_s(Eio, buf);
	}
}

int
Read(int f, void *a, int n)
{
	char buf[ERRLEN];

	if(read(f, (char *)a, n)!=n) {
		if (lastfile)
			lastfile->rescuing = 1;
		errstr(buf);
		if (downloaded)
			fprint(2, "read error: %s\n", buf);
		rescue();
		exits("read");
	}
	return n;
}

int
Write(int f, void *a, int n)
{
	int m;

	if((m=write(f, (char *)a, n))!=n)
		syserror("write");
	return m;
}

void
Seek(int f, long n, int w)
{
	if(seek(f, n, w)==-1)
		syserror("seek");
}
