/* Copyright Coraid, Inc.  2006.  All Rights Reserved */
#include <u.h>
#include <libc.h>
#include "cec.h"

static int fd = -1;

void
rawon(void)
{
	if((fd = open("/dev/consctl", OWRITE)) == -1 ||
	    write(fd, "rawon", 5) != 5)
		fprint(2, "Can't make console raw\n");
}

void
rawoff(void)
{
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
