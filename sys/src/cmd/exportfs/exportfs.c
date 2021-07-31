/*
 * exportfs - Export a plan 9 name space across a network
 */
#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#define Extern
#include "exportfs.h"

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

int nonone;

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
	int n, srv;
	char *dbfile;
	char *ctlfile;
	char user[NAMELEN];

	dbfile = "/tmp/exportdb";
	ctlfile = 0;
	srv = 0;

	ARGBEGIN{
	case 'a':
		if(srvauth(0, user) < 0)
			fatal("srvauth");
		if(newns(user, 0) < 0)
			fatal("newns");
		putenv("service", "exportfs");
		break;

	case 'c':
		ctlfile = ARGF();
		break;

	case 'd':
		dbg++;
		break;

	case 'f':
		dbfile = ARGF();
		break;

	case 's':
		srv++;
		break;

	default:
		usage();
	}ARGEND
	USED(argc, argv);

	if(dbg) {
		n = create(dbfile, OWRITE|OTRUNC, 0666);
		dup(n, 2);
		close(n);
	}

	DEBUG(2, "exportfs: started\n");

	rfork(RFNOTEG);

	Workq = malloc(sizeof(Fsrpc)*Nr_workbufs);
	fhash = malloc(sizeof(Fid*)*FHASHSIZE);

	if(Workq == 0 || fhash == 0)
		fatal("no initial memory");

	memset(Workq, 0, sizeof(Fsrpc)*Nr_workbufs);

	fmtinstall('F', fcallconv);

	/*
	 * Get tree to serve from network connection,
	 * check we can get there and ack the connection
 	 */
	if(srv) {
		chdir("/");
		DEBUG(2, "invoked as server for /");
	}
	else {
		buf[0] = 0;
		n = read(0, buf, sizeof(buf));
		if(n < 0) {
			errstr(buf);
			fprint(0, "read(0): %s", buf);
			DEBUG(2, "read(0): %s", buf);
			exits(buf);
		}
		buf[n] = 0;
		if(chdir(buf) < 0) {
			char ebuf[128];
			errstr(ebuf);
			fprint(0, "chdir(%d:\"%s\"): %s", n, buf, ebuf);
			DEBUG(2, "chdir(%d:\"%s\"): %s", n, buf, ebuf);
			exits(ebuf);
		}
	}

	/*
	 * take ownership of the network connection and
	 * push the fcall line discipline
	 */
	if(ctlfile)
		pushfcall(ctlfile);

	DEBUG(2, "initing root\n");
	initroot();

	DEBUG(2, "exportfs: %s\n", buf);

	if(srv == 0 && write(0, "OK", 2) != 2)
		fatal("open ack write");

	/*
	 * Start serving file requests from the network
	 */
	for(;;) {
		r = getsbuf();
		if(r == 0)
			fatal("Out of service buffers");

		do
			n = read9p(0, r->buf, sizeof(r->buf));
		while(n == 0);

		if(n < 0)
			fatal("server read");


		if(convM2S(r->buf, &r->work, n) == 0)
			fatal("format error");

		DEBUG(2, "%F\n", &r->work, &r->work);
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

	DEBUG(2, "\t%F\n", t);

	n = convS2M(t, data);
	if(write9p(0, data, n)!=n)
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
		fidfree = malloc(sizeof(Fid) * Fidchunk);
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
	int look;
	Fsrpc *wb;

	for(look = 0; look < Nr_workbufs; look++) {
		if(++ap == Nr_workbufs)
			ap = 0;
		if(Workq[ap].busy == 0)
			break;
	}

	if(look == Nr_workbufs)
		fatal("No more work buffers");

	wb = &Workq[ap];
	wb->pid = 0;
	wb->canint = 0;
	wb->flushtag = NOTAG;
	wb->busy = 1;

	return wb;
}

char *
strcatalloc(char *p, char *n)
{
	char *v;

	v = realloc(p, strlen(p)+strlen(n)+1);
	if(v == 0)
		fatal("no memory");
	strcat(v, n);
	return v;
}

File *
file(File *parent, char *name)
{
	Dir dir;
	char buf[128];
	File *f, *new;

	DEBUG(2, "\tfile: 0x%x %s name %s\n", parent, parent->name, name);

	for(f = parent->child; f; f = f->childlist)
		if(strcmp(name, f->name) == 0)
			return f;

	makepath(buf, parent, name);
	if(dirstat(buf, &dir) < 0)
		return 0;
	
	new = malloc(sizeof(File));
	if(new == 0)
		fatal("no memory");

	memset(new, 0, sizeof(File));
	strcpy(new->name, name);
	new->qid.vers = dir.qid.vers;
	new->qid.path = (dir.qid.path&CHDIR)|++qid;

	new->parent = parent;
	new->childlist = parent->child;
	parent->child = new;

	return new;
}

void
initroot(void)
{
	Dir dir;

	root = malloc(sizeof(File));
	if(root == 0)
		fatal("no memory");

	memset(root, 0, sizeof(File));
	strcpy(root->name, ".");
	if(dirstat(root->name, &dir) < 0)
		fatal("root stat");

	root->qid.vers = dir.qid.vers;
	root->qid.path = (dir.qid.path&CHDIR)|++qid;

	psmpt = malloc(sizeof(File));
	if(psmpt == 0)
		fatal("no memory");

	memset(psmpt, 0, sizeof(File));
	strcpy(psmpt->name, "/");
	if(dirstat(psmpt->name, &dir) < 0)
		return;

	psmpt->qid.vers = dir.qid.vers;
	psmpt->qid.path = (dir.qid.path&CHDIR)|++qid;

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

void
fatal(char *s)
{
	char buf[128];
	Proc *m;

	sprint(buf, "exportfs: %r: %s", s);

	/* Clear away the slave children */
	for(m = Proclist; m; m = m->next)
		postnote(PNPROC, m->pid, "kill");

	DEBUG(2, "%s\n", buf);
	exits(buf);
}

char pushmsg[] = "push fcall";

void
pushfcall(char *ctl)
{
	int cfd;
	Dir dir;

	if(dirfstat(0, &dir) < 0){
		fprint(2, "dirfstat(0) failed: %r\n");
		return;
	}
	memmove(dir.uid, getuser(), NAMELEN);
	if(dirfwstat(1, &dir) < 0){
		fprint(2, "dirfwstat(1) failed: %r\n");
		return;
	}
	cfd = open(ctl, ORDWR);
	if(cfd < 0){
		fprint(2, "open(%s0) failed: %r\n", ctl);
		return;
	}
	if(write(cfd, pushmsg, strlen(pushmsg)) < 0){
		fprint(2, "%s failed: %r\n", pushmsg);
		return;
	}
	close(cfd);
}
