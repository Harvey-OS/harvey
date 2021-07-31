
/*
 * exportfs - Export a plan 9 name space across a network
 */
#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#define Extern
#include "exportfs.h"

#define	QIDMODE	(CHDIR|CHAPPEND|CHEXCL|CHMOUNT)
ulong newqid = ~QIDMODE;

void (*fcalls[])(Fsrpc*) =
{
	[Tnop]		Xnop,
	[Tsession]	Xsession,
	[Tflush]	Xflush,
	[Tattach]	Xattach,
	[Tclone]	Xclone,
	[Twalk]		Xwalk,
	[Topen]		slave,
	[Tcreate]	Xcreate,
	[Tclunk]	Xclunk,
	[Tread]		slave,
	[Twrite]	slave,
	[Tremove]	Xremove,
	[Tstat]		Xstat,
	[Twstat]	Xwstat,
	[Tclwalk]	Xclwalk,
};

/* accounting and debugging counters */
int	filecnt;
int	freecnt;
int	qidcnt;
int	qfreecnt;
int	ncollision;

int	fflag;
int	netfd;
int	filter(int);

void
usage(void)
{
	fprint(2, "usage: %s [-as] [-c ctlfile]\n", argv0);
	fatal("usage");
}

void
main(int argc, char **argv)
{
	char buf[128];
	Fsrpc *r;
	int n;
	char *dbfile, *srv;
	char user[NAMELEN];

	dbfile = "/tmp/exportdb";
	srv = nil;

	ARGBEGIN{
	case 'a':
	//	fprint(2, "srvauth\n");
		if(srvauth(0, user) < 0)
			fatal("srvauth");
	//	fprint(2, "newns\n");
		if(newns(user, 0) < 0)
			fatal("newns");
		putenv("service", "exportfs");
		break;

	case 'd':
		dbg++;
		break;

	case 'f':
		dbfile = EARGF(usage());
		break;

	case 'F':
		fflag++;
		break;

	case 'r':
		srv = EARGF(usage());
		break;

	case 's':
		srv = "/";
		break;

	default:
		usage();
	}ARGEND
	USED(argc, argv);

	if(dbg) {
		n = create(dbfile, OWRITE|OTRUNC, 0666);
		dup(n, DFD);
		close(n);
	}

	DEBUG(DFD, "exportfs: started\n");

	rfork(RFNOTEG);

	Workq = mallocz(sizeof(Fsrpc)*Nr_workbufs, 1);
	fhash = mallocz(sizeof(Fid*)*FHASHSIZE, 1);

	if(Workq == 0 || fhash == 0)
		fatal("no initial memory");


	fmtinstall('F', fcallconv);

	/*
	 * Get tree to serve from network connection,
	 * check we can get there and ack the connection
 	 */
	if(srv) {
		chdir(srv);
		DEBUG(DFD, "invoked as server for %s", srv);
	}
	else {
		buf[0] = 0;
		n = read(0, buf, sizeof(buf));
		if(n < 0) {
			errstr(buf);
			fprint(0, "read(0): %s", buf);
			DEBUG(DFD, "read(0): %s", buf);
			exits(buf);
		}
		buf[n] = 0;
		if(chdir(buf) < 0) {
			char ebuf[128];
			errstr(ebuf);
			fprint(0, "chdir(%d:\"%s\"): %s", n, buf, ebuf);
			DEBUG(DFD, "chdir(%d:\"%s\"): %s", n, buf, ebuf);
			exits(ebuf);
		}
	}

	DEBUG(DFD, "initing root\n");
	initroot();

	DEBUG(DFD, "exportfs: %s\n", buf);

	if(srv == nil && write(0, "OK", 2) != 2)
		fatal("open ack write");

	/*
	 * push the fcall line discipline
	 */
	netfd = 0;
	if(fflag)
		netfd = filter(netfd);

	/*
	 * Start serving file requests from the network
	 */
	for(;;) {
		r = getsbuf();
		if(r == 0)
			fatal("Out of service buffers");

		do
			n = read9p(netfd, r->buf, sizeof(r->buf));
		while(n == 0);

		if(n < 0)
			fatal("server read");

		if(convM2S(r->buf, &r->work, n) == 0)
			fatal("format error");

		DEBUG(DFD, "%F\n", &r->work);
		(fcalls[r->work.type])(r);
	}
}

void
reply(Fcall *r, Fcall *t, char *err)
{
	char data[MAXFDATA+MAXMSG];
	int n;

	t->tag = r->tag;
	t->fid = r->fid;
	if(err) {
		t->type = Rerror;
		strncpy(t->ename, err, ERRLEN);
	}
	else 
		t->type = r->type + 1;

	DEBUG(DFD, "\t%F\n", t);

	n = convS2M(t, data);
	if(write9p(netfd, data, n)!=n)
		fatal("mount write");
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
			if(f->mpend)
				f->mpend->busy = 0;
			if(f->mid) {
				sprint(buf, "/mnt/exportfs/%d", f->mid);
				unmount(0, buf);
				psmap[f->mid] = 0;
			}
			freefile(f->f);
			f->f = nil;
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
		fidfree = mallocz(sizeof(Fid) * Fidchunk, 1);
		if(fidfree == 0)
			fatal("out of memory");

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
	new->mpend = 0;
	new->mid = 0;

	return new;	
}

Fsrpc *
getsbuf(void)
{
	static int ap;
	int look, rounds;
	Fsrpc *wb;

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
	free(f);
	f = parent;
	if(f != nil)
		goto Loop;
}

File *
file(File *parent, char *name)
{
	Dir dir;
	char buf[128];
	File *f;

	DEBUG(DFD, "\tfile: 0x%p %s name %s\n", parent, parent->name, name);

	makepath(buf, parent, name);
	if(dirstat(buf, &dir) < 0)
		return nil;

	for(f = parent->child; f; f = f->childlist)
		if(strcmp(name, f->name) == 0)
			break;

	if(f == nil){
		f = mallocz(sizeof(File), 1);
		if(f == 0)
			fatal("no memory");
		strcpy(f->name, name);

		f->parent = parent;
		f->childlist = parent->child;
		parent->child = f;
		parent->ref++;
		f->ref = 0;
		filecnt++;
	}
	f->ref++;
	f->qid.vers = dir.qid.vers;
	f->qidt = uniqueqid(&dir);
	f->qid.path = f->qidt->uniqpath;

	f->inval = 0;
	

	return f;
}

void
initroot(void)
{
	Dir dir;

	root = mallocz(sizeof(File), 1);
	if(root == 0)
		fatal("no memory");

	strcpy(root->name, ".");
	if(dirstat(root->name, &dir) < 0)
		fatal("root stat");

	root->ref = 1;
	root->qid.vers = dir.qid.vers;
	root->qidt = uniqueqid(&dir);
	root->qid.path = root->qidt->uniqpath;

	psmpt = mallocz(sizeof(File), 1);
	if(psmpt == 0)
		fatal("no memory");

	strcpy(psmpt->name, "/");
	if(dirstat(psmpt->name, &dir) < 0)
		return;

	psmpt->ref = 1;
	psmpt->qid.vers = dir.qid.vers;
	psmpt->qidt = uniqueqid(&dir);
	psmpt->qid.path = psmpt->qidt->uniqpath;

	psmpt = file(psmpt, "mnt");
	if(psmpt == 0)
		return;
	psmpt = file(psmpt, "exportfs");
}

void
makepath(char *s, File *p, char *name)
{
	int i;
	char *c, *seg[256];

	seg[0] = name;
	for(i = 1; i < 100 && p; i++, p = p->parent)
		seg[i] = p->name;

	while(i--) {
		for(c = seg[i]; *c; c++)
			*s++ = *c;
		*s++ = '/';
	}
	while(s[-1] == '/')
		s--;
	*s = '\0';
}

int
qidhash(ulong path)
{
	int h, n;

	h = 0;
	for(n=0; n<32; n+=Nqidbits){
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
qidexists(ulong path)
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
	ulong h, path;
	Qidtab *q;

	q = qidlookup(d);
	if(q != nil){
		q->ref++;
		return q;
	}
	path = d->qid.path;
	while(qidexists(path)){
		/* collision: find a new one */
		ncollision++;
		DEBUG(DFD, "collision on %s\n", d->name);
		path = newqid--;
		if(newqid == 0)
			newqid = ~QIDMODE;
		path |= d->qid.path & (CHDIR|CHAPPEND|CHEXCL|CHMOUNT);
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
fatal(char *s)
{
	char buf[128];
	Proc *m;

	sprint(buf, "exportfs: %r: %s", s);

	/* Clear away the slave children */
	for(m = Proclist; m; m = m->next)
		postnote(PNPROC, m->pid, "kill");

	DEBUG(DFD, "%s\n", buf);
	exits(buf);
}

/* Network on fd1, mount driver on fd0 */
int
filter(int fd)
{
	int p[2];

	if(pipe(p) < 0)
		fatal("pipe");

	switch(rfork(RFNOWAIT|RFPROC|RFFDG)) {
	case -1:
		fatal("rfork record module");
	case 0:
		dup(fd, 1);
		close(fd);
		dup(p[0], 0);
		close(p[0]);
		close(p[1]);
		execl("/bin/aux/fcall", "fcall", 0);
		fatal("exec record module");
	default:
		close(fd);
		close(p[0]);
	}
	return p[1];	
}
