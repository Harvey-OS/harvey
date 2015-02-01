/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include "cec.h"

static	int	fd	= -1;
extern	char	*svc;

void
rawon(void)
{
	if(svc)
		return;
	if((fd = open("/dev/consctl", OWRITE)) == -1 ||
	    write(fd, "rawon", 5) != 5)
		fprint(2, "Can't make console raw\n");
}

void
rawoff(void)
{
	if(svc)
		return;
	close(fd);
}

enum {
	Perline	= 16,
	Perch	= 3,
};

char	line[Perch*Perline+1];

static void
format(uchar *buf, int n, int t)
{
	int i, r;

	for(i = 0; i < n; i++){
		r = (i + t) % Perline;
		if(r == 0 && i + t > 0)
			fprint(2, "%s\n", line);
		sprint(line + r*Perch, "%.2x ", buf[i]);
	}
}

void
dump(uchar *p, int n)
{
	format(p, n, 0);
	if(n % 16 > 0)
		print("%s\n", line);
}
