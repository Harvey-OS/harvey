#include <alef.h>
#include <fcall.h>
#include <graphics.h>
#include <frame.h>
#include "y.h"

void Ynop(Ioreq*);
void Yflush(Ioreq*);
void Yattach(Ioreq*);
void Yclone(Ioreq*);
void Ywalk(Ioreq*);
void Yopen(Ioreq*);
void Yclunk(Ioreq*);
void Yread(Ioreq*);
void Ywrite(Ioreq*);
void Ystat(Ioreq*);
void Yperm(Ioreq*);
void post(void);

void (*fcalls[])(Ioreq*) =
{
	[Tnop]		Ynop,
	[Tsession]	Ynop,
	[Tflush]	Yflush,
	[Tattach]	Yattach,
	[Tclone]	Yclone,
	[Twalk]		Ywalk,
	[Topen]		Yopen,
	[Tcreate]	Yperm,
	[Tclunk]	Yclunk,
	[Tread]		Yread,
	[Twrite]	Ywrite,
	[Tremove]	Yperm,
	[Tstat]		Ystat,
	[Twstat]	Yperm,
	[Tclwalk]	Yperm,
	[Tauth]		Yperm,
};

Dirtab dirtab[]=
{
	{ "bitblt",	Qbitblt,	0600, },
	{ "cons",	Qcons,		0600, },
	{ "consctl",	Qctl,		0200, },
	{ "mouse",	Qmouse,		0600, },
	{ "nbmouse",	Qnbmouse,	0600, },
	{ "snarf",	Qsnarf,		0600, },
	{ "window",	Qwindow,	0600, },
	{ "label",	Qlabel,		0600, },
	{ nil }
};

intern char *eperm	= "permission denied";
intern char *efid	= "bad fid";
intern char *edfid	= "duplicate fid";
intern char *efmt	= "bad attach";
intern char *enotdir	= "not a directory";
intern char *enoexist	= "file does not exist";
intern char *ebusy	= "file busy";
intern char *edel	= "window was deleted";
intern char *eio	= "i/o error";

intern Fid 	*flist;
intern char	*user;

void
files()
{
	int n;
	Ioreq *i;

	post();
	fmtinstall('F', fcallconv);

	for(;;) {
		/* UTFmax allows rune conversion in place */
		i = malloc(sizeof(Ioreq));
		if(i == nil)
			error("out of memory");

		n = read(mtp[1], i->buf, MAXRPC);
		if(n < 0)
			error("server read");

		if(convM2S(i->buf, &i->fcall, n) == 0)
			error("format error");

/*		print("T> %F\n", &i->fcall);	/**/
		(fcalls[i->fcall.type])(i);
	}
}

void
post(void)
{
	int s;
	char num[10];

	if(pipe(mtp) < 0)
		error("pipe: %r");
	
	user = getuser();
	sprint(srv, "/srv/Y.%s.%d", user, getpid());
	s = create(srv, OWRITE, 0666);
	if(s < 0)
		error("open: %s %r");

	sprint(num, "%d", mtp[0]);
	if(write(s, num, strlen(num)) < 0)
		error("post: %r");
	close(s);
	close(mtp[0]);
}

void
Yattach(Ioreq *f)
{
	Fid *n;
	int id;
	char *e;
	Window *w;
	Fcall *r, thdr;

	rescue {
		reply(r, &thdr, e);
		free(f);
		return;
	}
	r = &f->fcall;
	e = efid;
	n = getfid(r->fid);
	if(n != nil)
		raise;

	e = efmt;
	if(r->aname[0] != 'Y')
		raise;

	id = atoi(r->aname+1);
	for(w = wlist; w; w = w->list)
		if(w->id == id)
			break;
	if(w == nil)
		raise;

	n = malloc(sizeof(Fid));
	n->qid = Qdir|CHDIR;
	n->w = w;
	n->fid = r->fid;
	n->next = flist;
	flist = n;
	w->ref++;
	thdr.fid = r->fid;
	thdr.qid = (Qid){n->qid, 0};

	reply(r, &thdr, nil);
	free(f);
}

void
Yclone(Ioreq *f)
{
	Fcall *r, thdr;
	Fid *n, *n2;

	rescue {
		reply(r, &thdr, efid);
		free(r);
		return;
	}

	r = &f->fcall;
	n = getfid(r->fid);
	if(n == nil)
		raise;

	n2 = getfid(r->newfid);
	if(n2 != nil)
		raise;

	n2 = malloc(sizeof(Fid));
	n2->qid = n->qid;
	n2->fid = r->newfid;
	n2->w = n->w;
	n2->w->ref++;
	n2->next = flist;
	flist = n2;

	reply(r, &thdr, nil);
	free(f);
}

void
Ywalk(Ioreq *f)
{
	Fid *n;
	char *e;
	Dirtab *d;
	Fcall *r, thdr;

	rescue {
		reply(r, &thdr, e);
		free(f);
		return;
	}
	r = &f->fcall;
	e = nil;	
	n = getfid(r->fid);
	if(n == nil) {
		e = efid;
		raise;
	}
	if((n->qid&CHDIR) == 0) {
		e = enotdir;
		raise;
	}
	if(strcmp(r->name, "..") != 0) {
		for(d = dirtab; d->name; d++) {
			if(strcmp(d->name, r->name) == 0) {
				n->qid = d->qid;
				thdr.qid = (Qid){d->qid, 1};
				break;
			}
		}
		if(d->name == nil) {
			e = enoexist;
			raise;
		}
	}
	reply(r, &thdr, nil);
	free(f);
}

void
Yopen(Ioreq *f)
{
	Fid *n;
	char *e;
	Window *w;
	Fcall *r, thdr;

	rescue {
		reply(r, &thdr, e);
		free(f);
		return;
	}
	r = &f->fcall;
	e = nil;	
	n = getfid(r->fid);
	if(n == nil) {
		e = efid;
		raise;
	}
	if((n->qid&CHDIR) && r->mode != OREAD) {
		e = eperm;
		raise;
	}
	w = n->w;
	if(w->closed) {
		e = edel;
		raise;
	}
	switch(n->qid) {
	case Qctl:
		if((r->mode&~OTRUNC) != OWRITE) {
			e = eperm;
			raise;
		}
		w->ctlref++;
		break;
	case Qbitblt:
		if(w->bitopen){
			e = ebusy;
			raise;
		}
		w->bitopen = 1;
		w->bitinit = 0;
		break;

	case Qmouse:
	case Qnbmouse:
		if(w->mouseopen){
			e = ebusy;
			raise;
		}
		w->mouseopen = 1;
		break;
	}
	thdr.qid = (Qid){n->qid, 0};
	reply(r, &thdr, nil);
	free(f);
}

void
Yclunk(Ioreq *f)
{
	Fid *p, **l;
	Fcall *r, thdr;

	r = &f->fcall;
	l = &flist;
	for(p = flist; p; p = p->next) {
		if(p->fid == r->fid) {
			*l = p->next;
			clntclunk(p->w, p->qid);
			p->w->ref--;
			if(p->w->ref == 0)
				p->w->in <-= (Mesg){ MsgClose };
			break;
		}
		l = &p->next;
	}

	reply(r, &thdr, nil);
	free(f);
}

void
statfill(Dirtab *f, void *b)
{
	Dir d;

	if(f == nil) {
		strcpy(d.name, ".");
		d.mode = CHDIR|0555;
		d.qid = (Qid){CHDIR|Qdir, 0};
	}
	else{
		strcpy(d.name, f->name);
		d.mode = f->perm;
		d.qid = (Qid){f->qid, 0};
	}
	strcpy(d.uid, user);
	strcpy(d.gid, user);
	d.atime = time(nil);
	d.mtime = d.atime;
	d.length = 0;
	convD2M(&d, b);
}

void
Yread(Ioreq *f)
{
	Fid *n;
	Mesg m;
	char *e, *tbuf;
	Window *w;
	Dirtab *dp;
	Fcall *r, thdr;

	rescue {
		reply(r, &thdr, e);
		free(f);
		return;
	}
	r = &f->fcall;
	n = getfid(r->fid);
	if(n == nil) {
		e = eperm;
		raise;
	}
	w = n->w;
	if(w->closed) {
		e = edel;
		raise;
	}
	if(n->qid&CHDIR) {
		if(r->offset%DIRLEN || r->count%DIRLEN) {
			e = eio;
			raise;
		}
		tbuf = (char*)r+sizeof(Fcall);
		thdr.count = 0;
		for(dp = &dirtab[r->offset/DIRLEN]; dp->name != nil; dp++) {
			if(r->count <= 0)
				break;
			statfill(dp, tbuf+thdr.count);
			thdr.count += DIRLEN;
			r->count -= DIRLEN;
		}
		thdr.data = tbuf;
		reply(r, &thdr, nil);
		free(f);
		return;
	}
	f->file = n->qid;
	m.type = MsgIO;
	m.io = f;
	w->in <-= m;
}

void
Ywrite(Ioreq *f)
{
	Fid *n;
	Mesg m;
	char *e;
	Window *w;
	Fcall *r, thdr;

	rescue {
		reply(r, &thdr, e);
		free(f);
		return;
	}
	r = &f->fcall;
	n = getfid(r->fid);
	if(n == nil) {
		e = eperm;
		raise;
	}
	w = n->w;
	if(w->closed) {
		e = edel;
		raise;
	}
	f->file = n->qid;
	m.type = MsgIO;
	m.io = f;
	w->in <-= m;
}

void
Ystat(Ioreq *f)
{
	Fid *n;
	Dirtab *dp;
	Fcall *r, thdr;

	r = &f->fcall;
	n = getfid(r->fid);
	if(n == nil) {
		reply(r, &thdr, efid);
		free(r);
		return;
	}
	if(n->qid&CHDIR)
		statfill(nil, thdr.stat);
	else {
		for(dp = dirtab; dp->name; dp++)
			if(dp->qid == n->qid) {
				statfill(dp, thdr.stat);
				break;
			}
	}
	reply(r, &thdr, nil);
	free(f);
}

void
Yperm(Ioreq *f)
{
	Fcall thdr;

	reply(&f->fcall, &thdr, eperm);
	free(f);
}

void
Ynop(Ioreq *f)
{
	Fcall thdr;

	reply(&f->fcall, &thdr, nil);
	free(f);
}

void
Yflush(Ioreq *f)
{
	Mesg m;
	Fcall thdr;

	m.type = MsgFlush;
	m.io = f;
	if(whead != nil) {
		whead->in <-= m;
		return;
	}
	reply(&f->fcall, &thdr, eperm);
	free(f);
}

void
reply(Fcall *r, Fcall *t, char *err)
{
	int n;
	char data[MAXRPC];

	t->tag = r->tag;
	t->fid = r->fid;
	t->type = r->type + 1;
	if(err) {
		t->type = Rerror;
		strncpy(t->ename, err, ERRLEN);
	}
/*	print("R> %F\n", t);		/**/
	n = convS2M(t, data);
	if(write(mtp[1], data, n) != n)
		error("mount write: %r");
}

Fid*
getfid(int n)
{
	Fid *f;

	for(f = flist; f; f = f->next)
		if(f->fid == n)
			return f;
	return nil;
}

void
cleanio(Ioreq **i)
{
	Fcall thdr;
	Ioreq *next, *f;

	for(f = *i; f; f = next) {
		next = f->link;
		reply(&f->fcall, &thdr, edel);
		free(f);
	}
	*i = nil;
}
