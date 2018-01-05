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
#include <thread.h>

char *host;

void
usage(void)
{
	fprint(2, "usage: mkroot [-h host] name type score blocksize prev\n");
	threadexitsall("usage");
}

void
threadmain(int argc, char *argv[])
{
	unsigned char score[VtScoreSize];
	unsigned char buf[VtRootSize];
	VtConn *z;
	VtRoot root;

	ARGBEGIN{
	case 'h':
		host = EARGF(usage());
		break;
	default:
		usage();
		break;
	}ARGEND

	if(argc != 5)
		usage();

	fmtinstall('V', vtscorefmt);
	fmtinstall('F', vtfcallfmt);

	strecpy(root.name, root.name+sizeof root.name, argv[0]);
	strecpy(root.type, root.type+sizeof root.type, argv[1]);
	if(vtparsescore(argv[2], nil, root.score) < 0)
		sysfatal("bad score '%s'", argv[2]);
	root.blocksize = atoi(argv[3]);
	if(vtparsescore(argv[4], nil, root.prev) < 0)
		sysfatal("bad score '%s'", argv[4]);
	vtrootpack(&root, buf);

	z = vtdial(host);
	if(z == nil)
		sysfatal("could not connect to server: %r");

	if(vtconnect(z) < 0)
		sysfatal("vtconnect: %r");

	if(vtwrite(z, score, VtRootType, buf, VtRootSize) < 0)
		sysfatal("vtwrite: %r");
	if(vtsync(z) < 0)
		sysfatal("vtsync: %r");
	vthangup(z);
	print("%V\n", score);
	threadexitsall(0);
}
