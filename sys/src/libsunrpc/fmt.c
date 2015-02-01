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
#include <sunrpc.h>

/*
 * print formatters
 */
int
sunRpcFmt(Fmt *f)
{
	SunRpc *rpc;

	rpc = va_arg(f->args, SunRpc*);
	sunRpcPrint(f, rpc);
	return 0;
}

static SunProg **fmtProg;
static int nfmtProg;
static RWLock fmtLock;

void
sunFmtInstall(SunProg *p)
{
	int i;

	wlock(&fmtLock);
	for(i=0; i<nfmtProg; i++){
		if(fmtProg[i] == p){
			wunlock(&fmtLock);
			return;
		}
	}
	if(nfmtProg%16 == 0)
		fmtProg = erealloc(fmtProg, sizeof(fmtProg[0])*(nfmtProg+16));
	fmtProg[nfmtProg++] = p;
	wunlock(&fmtLock);
}

int
sunCallFmt(Fmt *f)
{
	int i;
	void (*fmt)(Fmt*, SunCall*);
	SunCall *c;
	SunProg *p;

	c = va_arg(f->args, SunCall*);
	rlock(&fmtLock);
	for(i=0; i<nfmtProg; i++){
		p = fmtProg[i];
		if(p->prog == c->rpc.prog && p->vers == c->rpc.vers){
			runlock(&fmtLock);
			if(c->type < 0 || c->type >= p->nproc || (fmt=p->proc[c->type].fmt) == nil)
				return fmtprint(f, "unknown proc %c%d", "TR"[c->type&1], c->type>>1);
			(*fmt)(f, c);
			return 0;
		}
	}
	runlock(&fmtLock);
	fmtprint(f, "<sunrpc %d %d %c%d>", c->rpc.prog, c->rpc.vers, "TR"[c->type&1], c->type>>1);
	return 0;
}
