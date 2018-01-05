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
#include <auth.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>

static void
increqref(void *v)
{
	Req *r;

	r = v;
	if(r){
if(chatty9p > 1)
	fprint(2, "increfreq %p %ld\n", r, r->ref.ref);
		incref(&r->ref);
	}
}

Reqpool*
allocreqpool(void (*destroy)(Req*))
{
	Reqpool *f;

	f = emalloc9p(sizeof *f);
	f->map = allocmap(increqref);
	f->destroy = destroy;
	return f;
}

void
freereqpool(Reqpool *p)
{
	freemap(p->map, (void(*)(void*))p->destroy);
	free(p);
}	

Req*
allocreq(Reqpool *pool, uint32_t tag)
{
	Req *r;

	r = emalloc9p(sizeof *r);
	r->tag = tag;
	r->pool = pool;

	increqref(r);
	increqref(r);
	if(caninsertkey(pool->map, tag, r) == 0){
		closereq(r);
		closereq(r);
		return nil;
	}

	return r;
}

Req*
lookupreq(Reqpool *pool, uint32_t tag)
{
	if(chatty9p > 1)
		fprint(2, "lookupreq %lu\n", tag);
	return lookupkey(pool->map, tag);
}

void
closereq(Req *r)
{
	int i;

	if(r == nil)
		return;

	if(chatty9p > 1)
		fprint(2, "closereq %p %ld\n", r, r->ref.ref);

	if(decref(&r->ref) == 0){
		if(r->fid)
			closefid(r->fid);
		if(r->newfid)
			closefid(r->newfid);
		if(r->afid)
			closefid(r->afid);
		if(r->oldreq)
			closereq(r->oldreq);
		for(i=0; i<r->nflush; i++)
			respond(r->flush[i], nil);
		free(r->flush);
		switch(r->ifcall.type){
		case Tstat:
			free(r->ofcall.stat);
			free(r->d.name);
			free(r->d.uid);
			free(r->d.gid);
			free(r->d.muid);
			break;
		}
		if(r->pool->destroy)
			r->pool->destroy(r);
		free(r->buf);
		free(r->rbuf);
		free(r);
	}
}

Req*
removereq(Reqpool *pool, uint32_t tag)
{
	if(chatty9p > 1)
		fprint(2, "removereq %lu\n", tag);
	return deletekey(pool->map, tag);
}
