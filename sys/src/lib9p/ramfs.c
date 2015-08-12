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

static char Ebad[] = "something bad happened";
static char Enomem[] = "no memory";

typedef struct Ramfile Ramfile;
struct Ramfile {
	char* data;
	int ndata;
};

void
fsread(Req* r)
{
	Ramfile* rf;
	int64_t offset;
	int32_t count;

	rf = r->fid->file->aux;
	offset = r->ifcall.offset;
	count = r->ifcall.count;

	// print("read %ld %lld\n", *count, offset);
	if(offset >= rf->ndata) {
		r->ofcall.count = 0;
		respond(r, nil);
		return;
	}

	if(offset + count >= rf->ndata)
		count = rf->ndata - offset;

	memmove(r->ofcall.data, rf->data + offset, count);
	r->ofcall.count = count;
	respond(r, nil);
}

void
fswrite(Req* r)
{
	void* v;
	Ramfile* rf;
	int64_t offset;
	int32_t count;

	rf = r->fid->file->aux;
	offset = r->ifcall.offset;
	count = r->ifcall.count;

	if(offset + count >= rf->ndata) {
		v = realloc(rf->data, offset + count);
		if(v == nil) {
			respond(r, Enomem);
			return;
		}
		rf->data = v;
		rf->ndata = offset + count;
		r->fid->file->length = rf->ndata;
	}
	memmove(rf->data + offset, r->ifcall.data, count);
	r->ofcall.count = count;
	respond(r, nil);
}

void
fscreate(Req* r)
{
	Ramfile* rf;
	File* f;

	if(f = createfile(r->fid->file, r->ifcall.name, r->fid->uid,
	                  r->ifcall.perm, nil)) {
		rf = emalloc9p(sizeof *rf);
		f->aux = rf;
		r->fid->file = f;
		r->ofcall.qid = f->qid;
		respond(r, nil);
		return;
	}
	respond(r, Ebad);
}

void
fsopen(Req* r)
{
	Ramfile* rf;

	rf = r->fid->file->aux;

	if(rf && (r->ifcall.mode & OTRUNC)) {
		rf->ndata = 0;
		r->fid->file->length = 0;
	}

	respond(r, nil);
}

void
fsdestroyfile(File* f)
{
	Ramfile* rf;

	// fprint(2, "clunk\n");
	rf = f->aux;
	if(rf) {
		free(rf->data);
		free(rf);
	}
}

Srv fs = {
    .open = fsopen, .read = fsread, .write = fswrite, .create = fscreate,
};

void
usage(void)
{
	fprint(2, "usage: ramfs [-D] [-s srvname] [-m mtpt]\n");
	exits("usage");
}

void
main(int argc, char** argv)
{
	char* addr = nil;
	char* srvname = nil;
	char* mtpt = nil;
	Qid q;

	fs.tree = alloctree(nil, nil, DMDIR | 0777, fsdestroyfile);
	q = fs.tree->root->qid;

	ARGBEGIN
	{
	case 'D':
		chatty9p++;
		break;
	case 'a':
		addr = EARGF(usage());
		break;
	case 's':
		srvname = EARGF(usage());
		break;
	case 'm':
		mtpt = EARGF(usage());
		break;
	default:
		usage();
	}
	ARGEND;

	if(argc)
		usage();

	if(chatty9p)
		fprint(2, "ramsrv.nopipe %d srvname %s mtpt %s\n", fs.nopipe,
		       srvname, mtpt);
	if(addr == nil && srvname == nil && mtpt == nil)
		sysfatal("must specify -a, -s, or -m option");
	if(addr)
		listensrv(&fs, addr);

	if(srvname || mtpt)
		postmountsrv(&fs, srvname, mtpt, MREPL | MCREATE);
	exits(0);
}
