/*
 * exportfs - Export a plan 9 name space across a network
 */
#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <libsec.h>
#include "drawterm.h"
#define Extern
#include "exportfs.h"

/* #define QIDPATH	((1LL<<48)-1) */
#define QIDPATH	((((vlong)1)<<48)-1)
vlong newqid = 0;

void (*fcalls[256])(Fsrpc*);

/* accounting and debugging counters */
int	filecnt;
int	freecnt;
int	qidcnt;
int	qfreecnt;
int	ncollision;
int	netfd;

int
exportfs(int fd, int msgsz)
{
	char buf[ERRMAX], ebuf[ERRMAX];
	Fsrpc *r;
	int i, n;

	fcalls[Tversion] = Xversion;
	fcalls[Tauth] = Xauth;
	fcalls[Tflush] = Xflush;
	fcalls[Tattach] = Xattach;
	fcalls[Twalk] = Xwalk;
	fcalls[Topen] = slave;
	fcalls[Tcreate] = Xcreate;
	fcalls[Tclunk] = Xclunk;
	fcalls[Tread] = slave;
	fcalls[Twrite] = slave;
	fcalls[Tremove] = Xremove;
	fcalls[Tstat] = Xstat;
	fcalls[Twstat] = Xwstat;

	srvfd = -1;
	netfd = fd;
	//dbg = 1;

	strcpy(buf, "this is buf");
	strcpy(ebuf, "this is ebuf");
	DEBUG(DFD, "exportfs: started\n");

//	rfork(RFNOTEG);

	messagesize = msgsz;
	if(messagesize == 0){
		messagesize = iounit(netfd);
		if(messagesize == 0)
			messagesize = 8192+IOHDRSZ;
	}

	Workq = emallocz(sizeof(Fsrpc)*Nr_workbufs);
//	for(i=0; i<Nr_workbufs; i++)
//		Workq[i].buf = emallocz(messagesize);
	fhash = emallocz(sizeof(Fid*)*FHASHSIZE);

	fmtinstall('F', fcallfmt);

	initroot();

	DEBUG(DFD, "exportfs: %s\n", buf);

	/*
	 * Start serving file requests from the network
	 */
	for(;;) {
		r = getsbuf();
		if(r == 0)
			fatal("Out of service buffers");
			
		DEBUG(DFD, "read9p...");
		n = read9pmsg(netfd, r->buf, messagesize);
		if(n <= 0)
			fatal("eof: n=%d %r", n);

		if(convM2S(r->buf, n, &r->work) == 0){
			iprint("convM2S %d byte message\n", n);
			for(i=0; i<n; i++){
				iprint(" %.2ux", r->buf[i]);
				if(i%16 == 15)
					iprint("\n");
			}
			if(i%16)
				iprint("\n");
			fatal("convM2S format error");
		}

if(0) iprint("<- %F\n", &r->work);
		DEBUG(DFD, "%F\n", &r->work);
		(fcalls[r->work.type])(r);
	}
}

void
reply(Fcall *r, Fcall *t, char *err)
{
	uchar *data;
	int m, n;

	t->tag = r->tag;
	t->fid = r->fid;
	if(err) {
		t->type = Rerror;
		t->ename = err;
	}
	else 
		t->type = r->type + 1;

if(0) iprint("-> %F\n", t);
	DEBUG(DFD, "\t%F\n", t);

	data = malloc(messagesize);	/* not mallocz; no need to clear */
	if(data == nil)
		fatal(Enomem);
	n = convS2M(t, data, messagesize);
	if((m=write(netfd, data, n))!=n){
		iprint("wrote %d got %d (%r)\n", n, m);
		fatal("write");
	}
	free(data);
}

Fid *
getfid(int nr)
{
	Fid *f;

	for(f = fidhash(nr); f; f = f->next)
		if(f->nr == nr)
			return f;

	return 0;
}

int
freefid(int nr)
{
	Fid *f, **l;
	char buf[128];

	l = &fidhash(nr);
	for(f = *l; f; f = f->next) {
		if(f->nr == nr) {
			if(f->mid) {
				sprint(buf, "/mnt/exportfs/%d", f->mid);
				unmount(0, buf);
				psmap[f->mid] = 0;
			}
			if(f->f) {
				freefile(f->f);
				f->f = nil;
			}
			*l = f->next;
			f->next = fidfree;
			fidfree = f;
			return 1;
		}
		l = &f->next;
	}

	return 0;	
}

Fid *
newfid(int nr)
{
	Fid *new, **l;
	int i;

	l = &fidhash(nr);
	for(new = *l; new; new = new->next)
		if(new->nr == nr)
			return 0;

	if(fidfree == 0) {
		fidfree = emallocz(sizeof(Fid) * Fidchunk);

		for(i = 0; i < Fidchunk-1; i++)
			fidfree[i].next = &fidfree[i+1];

		fidfree[Fidchunk-1].next = 0;
	}

	new = fidfree;
	fidfree = new->next;

	memset(new, 0, sizeof(Fid));
	new->next = *l;
	*l = new;
	new->nr = nr;
	new->fid = -1;
	new->mid = 0;

	return new;	
}

Fsrpc *
getsbuf(void)
{
	static int ap;
	int look, rounds;
	Fsrpc *wb;
	int small_instead_of_fast = 1;

	if(small_instead_of_fast)
		ap = 0;	/* so we always start looking at the beginning and reuse buffers */

	for(rounds = 0; rounds < 10; rounds++) {
		for(look = 0; look < Nr_workbufs; look++) {
			if(++ap == Nr_workbufs)
				ap = 0;
			if(Workq[ap].busy == 0)
				break;
		}

		if(look == Nr_workbufs){
			sleep(10 * rounds);
			continue;
		}

		wb = &Workq[ap];
		wb->pid = 0;
		wb->canint = 0;
		wb->flushtag = NOTAG;
		wb->busy = 1;
		if(wb->buf == nil)	/* allocate buffers dynamically to keep size down */
			wb->buf = emallocz(messagesize);
		return wb;
	}
	fatal("No more work buffers");
	return nil;
}

void
freefile(File *f)
{
	File *parent, *child;

Loop:
	f->ref--;
	if(f->ref > 0)
		return;
	freecnt++;
	if(f->ref < 0) abort();
	DEBUG(DFD, "free %s\n", f->name);
	/* delete from parent */
	parent = f->parent;
	if(parent->child == f)
		parent->child = f->childlist;
	else{
		for(child=parent->child; child->childlist!=f; child=child->childlist)
			if(child->childlist == nil)
				fatal("bad child list");
		child->childlist = f->childlist;
	}
	freeqid(f->qidt);
	free(f->name);
	f->name = nil;
	free(f);
	f = parent;
	if(f != nil)
		goto Loop;
}

File *
file(File *parent, char *name)
{
	Dir *dir;
	char *path;
	File *f;

	DEBUG(DFD, "\tfile: 0x%p %s name %s\n", parent, parent->name, name);

	path = makepath(parent, name);
	dir = dirstat(path);
	free(path);
	if(dir == nil)
		return nil;

	for(f = parent->child; f; f = f->childlist)
		if(strcmp(name, f->name) == 0)
			break;

	if(f == nil){
		f = emallocz(sizeof(File));
		f->name = estrdup(name);

		f->parent = parent;
		f->childlist = parent->child;
		parent->child = f;
		parent->ref++;
		f->ref = 0;
		filecnt++;
	}
	f->ref++;
	f->qid.type = dir->qid.type;
	f->qid.vers = dir->qid.vers;
	f->qidt = uniqueqid(dir);
	f->qid.path = f->qidt->uniqpath;

	f->inval = 0;

	free(dir);

	return f;
}

void
initroot(void)
{
	Dir *dir;

	root = emallocz(sizeof(File));
	root->name = estrdup(".");

	dir = dirstat(root->name);
	if(dir == nil)
		fatal("root stat");

	root->ref = 1;
	root->qid.vers = dir->qid.vers;
	root->qidt = uniqueqid(dir);
	root->qid.path = root->qidt->uniqpath;
	root->qid.type = QTDIR;
	free(dir);

	psmpt = emallocz(sizeof(File));
	psmpt->name = estrdup("/");

	dir = dirstat(psmpt->name);
	if(dir == nil)
		return;

	psmpt->ref = 1;
	psmpt->qid.vers = dir->qid.vers;
	psmpt->qidt = uniqueqid(dir);
	psmpt->qid.path = psmpt->qidt->uniqpath;
	free(dir);

	psmpt = file(psmpt, "mnt");
	if(psmpt == 0)
		return;
	psmpt = file(psmpt, "exportfs");
}

char*
makepath(File *p, char *name)
{
	int i, n;
	char *c, *s, *path, *seg[256];

	seg[0] = name;
	n = strlen(name)+2;
	for(i = 1; i < 256 && p; i++, p = p->parent){
		seg[i] = p->name;
		n += strlen(p->name)+1;
	}
	path = malloc(n);
	if(path == nil)
		fatal("out of memory");
	s = path;

	while(i--) {
		for(c = seg[i]; *c; c++)
			*s++ = *c;
		*s++ = '/';
	}
	while(s[-1] == '/')
		s--;
	*s = '\0';

	return path;
}

int
qidhash(vlong path)
{
	int h, n;

	h = 0;
	for(n=0; n<64; n+=Nqidbits){
		h ^= path;
		path >>= Nqidbits;
	}
	return h & (Nqidtab-1);
}

void
freeqid(Qidtab *q)
{
	ulong h;
	Qidtab *l;

	q->ref--;
	if(q->ref > 0)
		return;
	qfreecnt++;
	h = qidhash(q->path);
	if(qidtab[h] == q)
		qidtab[h] = q->next;
	else{
		for(l=qidtab[h]; l->next!=q; l=l->next)
			if(l->next == nil)
				fatal("bad qid list");
		l->next = q->next;
	}
	free(q);
}

Qidtab*
qidlookup(Dir *d)
{
	ulong h;
	Qidtab *q;

	h = qidhash(d->qid.path);
	for(q=qidtab[h]; q!=nil; q=q->next)
		if(q->type==d->type && q->dev==d->dev && q->path==d->qid.path)
			return q;
	return nil;
}

int
qidexists(vlong path)
{
	int h;
	Qidtab *q;

	for(h=0; h<Nqidtab; h++)
		for(q=qidtab[h]; q!=nil; q=q->next)
			if(q->uniqpath == path)
				return 1;
	return 0;
}

Qidtab*
uniqueqid(Dir *d)
{
	ulong h;
	vlong path;
	Qidtab *q;

	q = qidlookup(d);
	if(q != nil){
		q->ref++;
		return q;
	}
	path = d->qid.path;
	while(qidexists(path)){
		DEBUG(DFD, "collision on %s\n", d->name);
		/* collision: find a new one */
		ncollision++;
		path &= QIDPATH;
		++newqid;
		if(newqid >= (1<<16)){
			DEBUG(DFD, "collision wraparound\n");
			newqid = 1;
		}
		path |= newqid<<48;
		DEBUG(DFD, "assign qid %.16llux\n", path);
	}
	q = mallocz(sizeof(Qidtab), 1);
	if(q == nil)
		fatal("no memory for qid table");
	qidcnt++;
	q->ref = 1;
	q->type = d->type;
	q->dev = d->dev;
	q->path = d->qid.path;
	q->uniqpath = path;
	h = qidhash(d->qid.path);
	q->next = qidtab[h];
	qidtab[h] = q;
	return q;
}

void
fatal(char *s, ...)
{
	char buf[ERRMAX];
	va_list arg;

	if (s) {
		va_start(arg, s);
		vsnprint(buf, ERRMAX, s, arg);
		va_end(arg);
	}

	/* Clear away the slave children */
//	for(m = Proclist; m; m = m->next)
//		postnote(PNPROC, m->pid, "kill");

	DEBUG(DFD, "%s\n", buf);
	if (s) 
		sysfatal(buf);
	else
		sysfatal("");
}

