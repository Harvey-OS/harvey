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
	r->fid->aux = state(root);
	respond(r, nil);
}

static void
flopen(Req *r)
{
	int m, p;
	Entry *e;
	State *s;

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
	if(r->ofcall.qid.type & QTDIR)
		s->r = ediropen(s->e);
	else if(r->ifcall.mode & OTRUNC)
		etrunc(e, 0, time(0));

	respond(r, nil);
}

static void
flcreate(Req *r)
{
	State *s;
	char *err;
	Entry *e, *f;

	if(readonly) {
		respond(r, Erofs);
		return;
	}

	s = r->fid->aux;
	e = s->e;
	if(!writeable(e)) {
		respond(r, Eperm);
		return;
	}

	f = ecreate(e, r->ifcall.name, 0, r->ifcall.perm, time(0), &err);
	if(f == nil) {
		respond(r, err);
		return;
	}

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
		r->ofcall.count = 0;

	respond(r, nil);
}

static void
flwrite(Req *r)
{
	if(r->ifcall.offset + r->ifcall.count >= MAXFSIZE) {
		respond(r, "file too big");
		return;
	}

	r->ofcall.count = r->ifcall.count;
	respond(r, nil);
}

static void
flremove(Req *r)
{
	State *s;
	Entry *e;

	if(readonly) {
		respond(r, Erofs);
		return;
	}

	s = r->fid->aux;
	e = s->e;
	if(writeable(e->parent))
		respond(r, eremove(e));
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
	State *s;
	Entry *e;

	s = r->fid->aux;
	e = s->e;
	m = r->d.mode & 0777;
	if(m != (e->mode & 0777))
		echmod(e, m, 0);
	respond(r, nil);
}

static void
flwalk(Req *r)
{
	int i;
	State *s;
	char *err;
	Entry *e, *f;

	if(readonly) {
		respond(r, Erofs);
		return;
	}

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
	einit();
	postmountsrv(&flashsrv, "brzr", mount, MREPL|MCREATE);
}
