#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include <thread.h>
#include "9p.h"

struct Fidpool {
	Intmap	*map;
};

static void
incfidref(void *v)
{
	Fid *f;

	f = v;
	if(f)
		incref(&f->ref);
}

Fidpool*
allocfidpool(void)
{
	Fidpool *f;

	if((f = mallocz(sizeof *f, 1)) == nil)
		return nil;
	if((f->map = allocmap(incfidref)) == nil){
		free(f);
		return nil;
	}
	return f;
}

Fid*
allocfid(Fidpool *fpool, ulong fid)
{
	Fid *f;

	if((f = mallocz(sizeof *f, 1)) == nil)
		return nil;

	f->fid = fid;
	f->omode = -1;
	f->pool = fpool;

	incfidref(f);
	if(!caninsertkey(fpool->map, fid, f)){
		closefid(f);
		return nil;
	}

	return f;
}

int
closefid(Fid *f)
{
	int n;
	int fid;

	fid = f->fid;
	if((n = decref(&f->ref)) == 0) {
		if(f->file)
			fclose(f->file);
		free(f);
	}
if(lib9p_chatty)
	fprint(2, "closefid %d %p: %ld refs\n", fid, f, n ? f->ref.ref : n);
	return n;
}

void
freefid(Fid *f)
{
	Fid *nf;

	if(f == nil)
		return;

	nf = deletekey(f->pool->map, f->fid);
	assert(f == nf);

	if(closefid(f) == 0)	/* can't be last one; we have one more */
		abort();
	closefid(f);
}

Fid*
lookupfid(Fidpool *fpool, ulong fid)
{
	return lookupkey(fpool->map, fid);
}
