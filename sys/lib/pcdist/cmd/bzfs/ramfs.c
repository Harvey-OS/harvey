#include <u.h>
#include <libc.h>
#include <bio.h>
#include <auth.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include "bzfs.h"

static char Ebad[] = "something bad happened";
static char Enomem[] = "no memory";
static char Eperm[] = "permission denied";

void*
emalloc(ulong sz)
{
	void *v;
	v = malloc(sz);
	if(v == nil)
		sysfatal("out of memory");
	memset(v, 0, sz);
	return v;
}

void
ramread(Req *r, Fid *fid, void *buf, long *count, vlong offset)
{
	Ramfile *rf;

	rf = fid->file->aux;

	if(offset >= rf->ndata){
		*count = 0;
		respond(r, nil);
		return;
	}

	if(offset+*count >= rf->ndata)
		*count = rf->ndata - offset;

	memmove(buf, rf->data+offset, *count);
	respond(r, nil);
}

void
ramwrite(Req *r, Fid *fid, void *buf, long *count, vlong offset)
{
	void *v;
	Ramfile *rf;

	rf = fid->file->aux;
	if(offset+*count >= rf->ndata){
		v = realloc(rf->data, offset+*count);
		if(v == nil){
			respond(r, Enomem);
			return;
		}
		rf->data = v;
		rf->ndata = offset+*count;
		fid->file->length = rf->ndata;
	}
	memmove(rf->data+offset, buf, *count);
	respond(r, nil);
}

void
ramopen(Req *r, Fid *fid, int mode, Qid*)
{
	Ramfile *rf;

	if((mode&OTRUNC) && (rf = fid->file->aux)){
		fid->file->length = 0;
		rf->ndata = 0;
	}
	respond(r, nil);
}

void
ramcreate(Req *r, Fid *fid, char *name, int, ulong perm, Qid *qid)
{
	Ramfile *rf;
	File *f;

	if(f = fcreate(fid->file, name, fid->uid, fid->uid, perm)){
		if((perm & CHDIR) == 0) {
			rf = emalloc(sizeof *rf);
			f->aux = rf;
		} else
			f->aux = nil;
		fid->file = f;
		*qid = f->qid;
		respond(r, nil);
		return;
	}
	respond(r, Ebad);
}

void
ramwstat(Req *r, Fid *fid, Dir *d)
{
	File *f;
	Dir *od;

	/* BUG check if new name exists */
	f = fid->file;
	od = &f->Dir;
	if(strcmp(od->name, d->name) != 0
	&& !hasperm(f->parent, fid->uid, AWRITE)){
		respond(r, Eperm);
		return;
	}
	strncpy(od->name, d->name, NAMELEN-1);
	od->name[NAMELEN-1] = '\0';

	if((od->mode != d->mode || od->mtime != d->mtime)
	&& strcmp(fid->uid, f->uid) != 0){
		respond(r, Eperm);
		return;
	}

	if((od->mode&CHDIR) != (d->mode&CHDIR)){
		respond(r, Eperm);
		return;
	}
	od->mode = d->mode;
	od->mtime = d->mtime;

	/* owner if member of new group */
	/* group leader if leader of new group */
	if(strcmp(d->gid, od->gid) != 0 && strcmp(fid->uid, f->uid) != 0){
		respond(r, Eperm);
		return;
	}
	strncpy(od->gid, d->gid, NAMELEN-1);
	od->gid[NAMELEN-1] = '\0';

	respond(r, nil);
}

/*
 * don't worry about owner/group stuff, but try to respect modes.
 */
int
hasperm(File *file, char*, int p)
{
	int m;
	m = file->mode;
	m = m | (m>>3) | (m>>6);
	return (m&p) == p;
}

void
ramrmaux(File *f)
{
	Ramfile *rf;

	rf = f->aux;
	if(rf == nil)	/* e.g. is a directory */
		return;
	free(rf->data);
	rf->data = (void*)0xDeadBeef;
	free(rf);
}

Srv ramsrv = {
	.read=	ramread,
	.open=	ramopen,
	.write=	ramwrite,
	.create=	ramcreate,
	.wstat=	ramwstat,
};

void
ramfs(Tree *t, char *mtpt, int stdin)
{
	ramsrv.tree = t;
/*	ramsrv.tree->rmaux = ramrmaux; */

	if(stdin) {
		switch(fork()){
		case -1:
			sysfatal("fork");
		case 0:
			close(0);
			srv(&ramsrv, 1);
			exits(0);
		default:
			_exits(0);
		}
	} else
		postmountsrv(&ramsrv, nil, mtpt, MREPL|MCREATE);
}
