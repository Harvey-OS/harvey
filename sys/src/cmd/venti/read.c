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
	fprint(2, "usage: read [-h host] [-t type] score\n");
	threadexitsall("usage");
}

void
threadmain(int argc, char *argv[])
{
	int type, n;
	uchar score[VtScoreSize];
	uchar *buf;
	VtConn *z;
	char *host;

	fmtinstall('F', vtfcallfmt);
	fmtinstall('V', vtscorefmt);

	host = nil;
	type = -1;
	ARGBEGIN{
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

	if(argc != 1)
		usage();

	if(vtparsescore(argv[0], nil, score) < 0)
		sysfatal("could not parse score '%s': %r", argv[0]);

	buf = vtmallocz(VtMaxLumpSize);

	z = vtdial(host);
	if(z == nil)
		sysfatal("could not connect to server: %r");

	if(vtconnect(z) < 0)
		sysfatal("vtconnect: %r");

	if(type == -1){
		n = -1;
		for(type=0; type<VtMaxType; type++){
			n = vtread(z, score, type, buf, VtMaxLumpSize);
			if(n >= 0){
				fprint(2, "venti/read%s%s %V %d\n", host ? " -h" : "", host ? host : "",
					score, type);
				break;
			}
		}
	}else
		n = vtread(z, score, type, buf, VtMaxLumpSize);

	vthangup(z);
	if(n < 0)
		sysfatal("could not read block: %r");
	if(write(1, buf, n) != n)
		sysfatal("write: %r");
	threadexitsall(0);
}
