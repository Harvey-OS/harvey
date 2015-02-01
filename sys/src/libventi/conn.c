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
#include "queue.h"

int chattyventi;

VtConn*
vtconn(int infd, int outfd)
{
	VtConn *z;
	NetConnInfo *nci;

	z = vtmallocz(sizeof(VtConn));
	z->tagrend.l = &z->lk;
	z->rpcfork.l = &z->lk;
	z->infd = infd;
	z->outfd = outfd;
	z->part = packetalloc();
	nci = getnetconninfo(nil, infd);
	if(nci == nil)
		snprint(z->addr, sizeof z->addr, "/dev/fd/%d", infd);
	else{
		strecpy(z->addr, z->addr+sizeof z->addr, nci->raddr);
		freenetconninfo(nci);
	}
	return z;
}

void
vtfreeconn(VtConn *z)
{
	vthangup(z);
	qlock(&z->lk);
	/*
	 * Wait for send and recv procs to notice
	 * the hangup and clear out the queues.
	 */
	while(z->readq || z->writeq){
		if(z->readq)
			_vtqhangup(z->readq);
		if(z->writeq)
			_vtqhangup(z->writeq);
		rsleep(&z->rpcfork);
	}
	packetfree(z->part);
	vtfree(z->version);
	vtfree(z->sid);
	qunlock(&z->lk);
	vtfree(z);
}
