/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* Copyright (c) 2004 Russ Cox */
#include <u.h>
#include <libc.h>
#include <venti.h>
#include <thread.h>
#include <libsec.h>

#ifndef _UNISTD_H_
#endif

VtConn *z;
int verbose;

enum
{
	STACK = 8192
};

void
usage(void)
{
	fprint(2, "usage: venti/ro [-v] [-a address] [-h address]\n");
	threadexitsall("usage");
}

void
readthread(void *v)
{
	char err[ERRMAX];
	VtReq *r;
	uint8_t *buf;
	int n;
	
	r = v;
	buf = vtmalloc(r->tx.count);
	if((n=vtread(z, r->tx.score, r->tx.blocktype, buf, r->tx.count)) < 0){
		r->rx.msgtype = VtRerror;
		rerrstr(err, sizeof err);
		r->rx.error = vtstrdup(err);
		free(buf);
	}else{
		r->rx.data = packetforeign(buf, n, free, buf);
	}
	if(verbose)
		fprint(2, "-> %F\n", &r->rx);
	vtrespond(r);
}

void
threadmain(int argc, char **argv)
{
	VtReq *r;
	VtSrv *srv;
	char *address, *ventiaddress;

	fmtinstall('F', vtfcallfmt);
	fmtinstall('V', vtscorefmt);
	
	address = "tcp!*!venti";
	ventiaddress = nil;
	
	ARGBEGIN{
	case 'v':
		verbose++;
		break;
	case 'a':
		address = EARGF(usage());
		break;
	case 'h':
		ventiaddress = EARGF(usage());
		break;
	default:
		usage();
	}ARGEND

	if((z = vtdial(ventiaddress)) == nil)
		sysfatal("vtdial %s: %r", ventiaddress);
	if(vtconnect(z) < 0)
		sysfatal("vtconnect: %r");

	srv = vtlisten(address);
	if(srv == nil)
		sysfatal("vtlisten %s: %r", address);

	while((r = vtgetreq(srv)) != nil){
		r->rx.msgtype = r->tx.msgtype+1;
		if(verbose)
			fprint(2, "<- %F\n", &r->tx);
		switch(r->tx.msgtype){
		case VtTping:
			break;
		case VtTgoodbye:
			break;
		case VtTread:
			threadcreate(readthread, r, 16384);
			continue;
		case VtTwrite:
			r->rx.error = vtstrdup("read-only server");
			r->rx.msgtype = VtRerror;
			break;
		case VtTsync:
			break;
		}
		if(verbose)
			fprint(2, "-> %F\n", &r->rx);
		vtrespond(r);
	}
	threadexitsall(nil);
}

