#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include <thread.h>
#include "9p.h"
#include "impl.h"

static void
increqref(void *v)
{
	Req *r;

	r = v;
	if(r)
		incref(&r->ref);
}

Req*
allocreq(Reqpool *pool, ulong tag)
{
	Req *r;

	if((r = mallocz(sizeof *r, 1)) == nil)
		return nil;

	r->tag = tag;
	r->pool = pool;

	incref(&r->ref);
	if(caninsertkey(pool->map, tag, r) == 0){
		closereq(r);
		return nil;
	}

	return r;
}

int
closereq(Req *r)
{
	int n;

	if((n = decref(&r->ref)) == 0){
		free(r->buf);
		free(r->rbuf);
		free(r);
	}
//print("closereq %p: %d refs\n", r, n ? r->ref.ref : n);
	return n;
}

void
freereq(Req *r)
{
	Req *nr;

	if(r == nil)
		return;

	nr = deletekey(r->pool->map, r->tag);
if(r != nr)
	fprint(2, "r %p nr %p\n", r, nr);

	assert(r == nr);

	if(closereq(r) == 0)	/* intmap reference */
		abort();
	closereq(r);
}

Req*
lookupreq(Reqpool *pool, ulong req)
{
	return lookupkey(pool->map, req);
}

Reqpool*
allocreqpool(void)
{
	Reqpool *f;

	if((f = mallocz(sizeof *f, 1)) == nil)
		return nil;
	if((f->map = allocmap(increqref)) == nil){
		free(f);
		return nil;
	}
	return f;
}

