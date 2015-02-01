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
#include <venti.h>
#include <libsec.h>
#include <thread.h>

void
usage(void)
{
	fprint(2, "usage: write [-z] [-h host] [-t type] <datablock\n");
	threadexitsall("usage");
}

void
threadmain(int argc, char *argv[])
{
	char *host;
	int dotrunc, n, type;
	uchar *p, score[VtScoreSize];
	VtConn *z;

	fmtinstall('F', vtfcallfmt);
	fmtinstall('V', vtscorefmt);

	host = nil;
	dotrunc = 0;
	type = VtDataType;
	ARGBEGIN{
	case 'z':
		dotrunc = 1;
		break;
	case 'h':
		host = EARGF(usage());
		break;
	case 't':
		type = atoi(EARGF(usage()));
		break;
	default:
		usage();
		break;
	}ARGEND

	if(argc != 0)
		usage();

	p = vtmallocz(VtMaxLumpSize+1);
	n = readn(0, p, VtMaxLumpSize+1);
	if(n > VtMaxLumpSize)
		sysfatal("input too big: max block size is %d", VtMaxLumpSize);
	z = vtdial(host);
	if(z == nil)
		sysfatal("could not connect to server: %r");
	if(vtconnect(z) < 0)
		sysfatal("vtconnect: %r");
	if(dotrunc)
		n = vtzerotruncate(type, p, n);
	if(vtwrite(z, score, type, p, n) < 0)
		sysfatal("vtwrite: %r");
	vthangup(z);
	print("%V\n", score);
	threadexitsall(0);
}
