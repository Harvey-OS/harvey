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
#include <mp.h>
#include <libsec.h>
#include <thread.h>

void
usage(void)
{
	fprint(2, "usage: root [-h host] score\n");
	threadexitsall("usage");
}

void
threadmain(int argc, char *argv[])
{
	int i, n;
	unsigned char score[VtScoreSize];
	unsigned char *buf;
	VtConn *z;
	char *host;
	VtRoot root;

	fmtinstall('F', vtfcallfmt);
	fmtinstall('V', vtscorefmt);
	quotefmtinstall();

	host = nil;
	ARGBEGIN{
	case 'h':
		host = EARGF(usage());
		break;
	default:
		usage();
		break;
	}ARGEND

	if(argc == 0)
		usage();

	buf = vtmallocz(VtMaxLumpSize);

	z = vtdial(host);
	if(z == nil)
		sysfatal("could not connect to server: %r");

	if(vtconnect(z) < 0)
		sysfatal("vtconnect: %r");

	for(i=0; i<argc; i++){
		if(vtparsescore(argv[i], nil, score) < 0){
			fprint(2, "cannot parse score '%s': %r\n", argv[i]);
			continue;
		}
		n = vtread(z, score, VtRootType, buf, VtMaxLumpSize);
		if(n < 0){
			fprint(2, "could not read block %V: %r\n", score);
			continue;
		}
		if(n != VtRootSize){
			fprint(2, "block %V is wrong size %d != 300\n", score, n);
			continue;
		}
		if(vtrootunpack(&root, buf) < 0){
			fprint(2, "unpacking block %V: %r\n", score);
			continue;
		}
		print("%V: %q %q %V %d %V\n", score, root.name, root.type, root.score, root.blocksize, root.prev);
	}
	vthangup(z);
	threadexitsall(0);
}
