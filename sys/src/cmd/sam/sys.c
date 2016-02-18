/* 
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

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
	char buf[ERRMAX];

	if(!inerror){
		inerror=TRUE;
		errstr(buf, sizeof buf);
		dprint("%s: ", a);
		error_s(Eio, buf);
	}
}

int
Read(int f, void *a, int n)
{
	char buf[ERRMAX];

	if(read(f, (char *)a, n)!=n) {
		if (lastfile)
			lastfile->rescuing = 1;
		errstr(buf, sizeof buf);
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
