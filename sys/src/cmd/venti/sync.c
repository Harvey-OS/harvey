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
#include <thread.h>
#include <venti.h>

char *host;
int donothing;

void
usage(void)
{
	fprint(2, "usage: sync [-h host]\n");
	threadexitsall("usage");
}

void
threadmain(int argc, char *argv[])
{
	VtConn *z;

	fmtinstall('V', vtscorefmt);
	fmtinstall('F', vtfcallfmt);
	
	ARGBEGIN{
	case 'h':
		host = EARGF(usage());
		if(host == nil)
			usage();
		break;
	case 'x':
		donothing = 1;
		break;
	default:
		usage();
		break;
	}ARGEND

	if(argc != 0)
		usage();

	z = vtdial(host);
	if(z == nil)
		sysfatal("could not connect to server: %r");

	if(vtconnect(z) < 0)
		sysfatal("vtconnect: %r");

	if(!donothing)
	if(vtsync(z) < 0)
		sysfatal("vtsync: %r");

	vthangup(z);
	threadexitsall(0);
}
