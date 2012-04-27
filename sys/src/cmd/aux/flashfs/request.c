#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include "flashfs.h"

static	Srv	flashsrv;

typedef struct	State	State;

struct State
{
	Entry	*e;
	Dirr	*r;
};

#define	writeable(e)	((e)->mode & 0222)

static State *
state(Entry *e)
{
	State *s;

	s = emalloc9p(sizeof(State));
	s->e = e;
	s->r = nil;
	return s;
}

static void
destroy(Fid *f)
{
	State *s;

	s = f->aux;
	if(s == nil)		/* Tauth fids have no state */
		return;

	f->aux = nil;
	if(s->e)
		edestroy(s->e);
	if(s->r)
		edirclose(s->r);
	free(s);
}

static void
trace(Req *)
{
	edump();
}

/** T_ **/

static void
flattach(Req *r)
{
	root->ref++;
	r->ofcall.qid = eqid(root);
	r->fid->qid = r->ofcall.qid;
	r->fid->aux = state(root);
	respond(r, nil);
}

static void
flopen(Req *r)
{
	Jrec j;
	int m, p;
	Entry *e;
	State *s;
	char *err;

	s = r->fid->aux;
	e = s->e;
	m = e->mode;
	m = (m | (m >> 3) | (m >> 6)) & 7;
	switch(r->ifcall.mode & 3) {
	case OREAD:
		p = AREAD;
		break;
	case OWRITE:
		p = AWRITE;
		break;
	case ORDWR:
		p = AREAD|AWRITE;
		break;
	case OEXEC:
		p = AEXEC;
		break;
	default:
		p = 0;
		break;
	}

	if((p & m) != p) {
		respond(r, Eperm);
		return;
	}

	if(readonly && (p & AWRITE) != 0) {
		respond(r, Erofs);
		return;
	}

	r->ofcall.qid = eqid(e);
	if(r->ofcall.qid.type & QTDIR) {
		if((p & AWRITE) != 0) {
			respond(r, Eisdir);
			return;
		}
		s->r = ediropen(s->e);
	}
	else if(r->ifcall.mode & OTRUNC) {
		err = need(Ntrunc);
		if(err != nil) {
			respond(r, err);
			return;
		}
		j.type = FT_trunc;
		j.tnum = e->fnum;
		j.mtime = now();
		etrunc(e, 0, j.mtime);
		j.fnum = e->fnum;
		j.parent = e->parent->fnum;
		j.mode = e->mode;
		strcpy(j.name, e->name);
		put(&j, 1);
	}

	respond(r, nil);
}

static void
flcreate(Req *r)
{
	Jrec j;
	State *s;
	char *err;
	Entry *e, *f;

	if(readonly) {
		respond(r, Erofs);
		return;
	}

	s = r->fid->aux;
	e = s->e;
	if((e->mode & DMDIR) == 0) {
		respond(r, Eisdir);
		return;
	}

	if(!writeable(e)) {
		respond(r, Eperm);
		return;
	}

	if(strlen(r->ifcall.name) > MAXNSIZE) {
		respond(r, "filename too long");
		return;
	}

	err = need(Ncreate);
	if(err != nil) {
		respond(r, err);
		return;
	}

	j.type = FT_create;
	j.mtime = now();
	j.parent = e->fnum;
	j.mode = r->ifcall.perm;
	strcpy(j.name, r->ifcall.name);

	f = ecreate(e, r->ifcall.name, 0, r->ifcall.perm, j.mtime, &err);
	if(f == nil) {
		respond(r, err);
		return;
	}

	j.fnum = f->fnum;
	put(&j, 1);
	s->e = f;
	r->ofcall.qid = eqid(f);
	respond(r, nil);
}

static void
flread(Req *r)
{
	Entry *e;
	State *s;

	s = r->fid->aux;
	e = s->e;

	if(e->mode & DMDIR)
		r->ofcall.count = edirread(s->r, r->ofcall.data, r->ifcall.count);
	else
		r->ofcall.count = eread(e, eparity, r->ofcall.data, r->ifcall.count, r->ifcall.offset);

	respond(r, nil);
}

static void
flwrite(Req *r)
{
	Jrec j;
	uchar *a;
	Entry *e;
	State *s;
	Extent *x;
	char *err;
	ulong c, n, o, mtime;

	c = r->ifcall.count;
	o = r->ifcall.offset;
	a = (uchar *)r->ifcall.data;

	if(c == 0) {
		respond(r, nil);
		return;
	}

	if(o + c >= MAXFSIZE) {
		respond(r, "file too big");
		return;
	}

	if(used + c > limit) {
		respond(r, "filesystem full");
		return;
	}

	r->ofcall.count = c;
	s = r->fid->aux;
	e = s->e;
	mtime = now();

	for(;;) {
		n = c;
		if(n > maxwrite)
			n = maxwrite;

		err = need(Nwrite + n);
		if(err != nil) {
			respond(r, err);
			return;
		}

		x = emalloc9p(sizeof(Extent));
		x->size = n;
		x->off = o;
		ewrite(e, x, eparity, mtime);
		j.type = FT_WRITE;
		j.fnum = e->fnum;
		j.size = n;
		j.offset = o;
		j.mtime = mtime;
		putw(&j, 1, x, a);
		c -= n;

		if(c == 0)
			break;

		o += n;
		a += n;
	}

	respond(r, nil);
}

static void
flremove(Req *r)
{
	Jrec j;
	State *s;
	Entry *e;
	char *d, *err;

	if(readonly) {
		respond(r, Erofs);
		return;
	}

	s = r->fid->aux;
	e = s->e;
	if(writeable(e->parent)) {
		err = need(Nremove);
		if(err != nil) {
			respond(r, err);
			return;
		}

		d = eremove(e);
		if(d == nil) {
			j.type = FT_REMOVE;
			j.fnum = e->fnum;
			put(&j, 0);
		}
		respond(r, d);
	}
	else
		respond(r, Eperm);
}

static void
flstat(Req *r)
{
	State *s;

	s = r->fid->aux;
	estat(s->e, &r->d, 1);
	respond(r, nil);
}

static void
flwstat(Req *r)
{
	int m;
	Jrec j;
	State *s;
	Entry *e;
	char *err;

	s = r->fid->aux;
	e = s->e;

	if(readonly) {
		respond(r, Erofs);
		return;
	}

	if(e->fnum == 0) {
		respond(r, Eperm);
		return;
	}

	m = r->d.mode & 0777;
	if(m != (e->mode & 0777)) {
		err = need(Nchmod);
		if(err != nil) {
			respond(r, err);
			return;
		}

		echmod(e, m, 0);
		j.type = FT_chmod;
		j.mode = m;
		j.fnum = e->fnum;
		j.mnum = e->mnum;
		put(&j, 0);
	}
	respond(r, nil);
}

static void
flwalk(Req *r)
{
	int i;
	State *s;
	char *err;
	Entry *e, *f;

	if(r->ifcall.fid != r->ifcall.newfid)
		r->newfid->aux = state(nil);

	s = r->fid->aux;
	e = s->e;
	f = e;
	e->ref++;
	err = nil;
	for(i = 0; i < r->ifcall.nwname; i++) {
		f = ewalk(e, r->ifcall.wname[i], &err);
		if(f) {
			r->ofcall.wqid[i] = eqid(f);
			e = f;
		}
		else {
			e->ref--;
			break;
		}
	}
	r->ofcall.nwqid = i;
	if (i) err = nil;

	if(f) {
		if(r->ifcall.fid != r->ifcall.newfid) {
			s = r->newfid->aux;
			s->e = f;
			r->newfid->qid = eqid(f);
		}
		else {
			s = r->fid->aux;
			s->e->ref--;
			s->e = f;
			r->fid->qid = eqid(f);
		}
	}
	respond(r, err);
}

void
serve(char *mount)
{
	flashsrv.attach = flattach;
	flashsrv.open = flopen;
	flashsrv.create = flcreate;
	flashsrv.read = flread;
	flashsrv.write = flwrite;
	flashsrv.remove = flremove;
	flashsrv.stat = flstat;
	flashsrv.wstat = flwstat;
	flashsrv.walk = flwalk;

	flashsrv.destroyfid = destroy;
	flashsrv.destroyreq = trace;
	postmountsrv(&flashsrv, "brzr", mount, MREPL|MCREATE);
}
