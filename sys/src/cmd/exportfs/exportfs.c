/*
 * exportfs - Export a plan 9 name space across a network
 */
#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include <libsec.h>
#define Extern
#include "exportfs.h"

#define QIDPATH	((1LL<<48)-1)
vlong newqid = 0;

enum {
	Encnone,
	Encssl,
	Enctls,
};

void (*fcalls[])(Fsrpc*) =
{
	[Tversion]	Xversion,
	[Tauth]	Xauth,
	[Tflush]	Xflush,
	[Tattach]	Xattach,
	[Twalk]		Xwalk,
	[Topen]		slave,
	[Tcreate]	Xcreate,
	[Tclunk]	Xclunk,
	[Tread]		slave,
	[Twrite]	slave,
	[Tremove]	Xremove,
	[Tstat]		Xstat,
	[Twstat]	Xwstat,
};

/* accounting and debugging counters */
int	filecnt;
int	freecnt;
int	qidcnt;
int	qfreecnt;
int	ncollision;

int	netfd;				/* initially stdin */
int	srvfd = -1;
int	nonone = 1;
char	*filterp;
char	*ealgs = "rc4_256 sha1";
char	*aanfilter = "/bin/aan";
int	encproto = Encnone;
int	readonly;

static void	mksecret(char *, uchar *);
static int localread9pmsg(int, void *, uint, ulong *);
static char *anstring  = "tcp!*!0";

char *netdir = "", *local = "", *remote = "";

int	filter(int, char *);

void
usage(void)
{
	fprint(2, "usage: %s [-adnsR] [-f dbgfile] [-m msize] [-r root] "
		"[-S srvfile] [-e 'crypt hash'] [-P exclusion-file] "
		"[-A announce-string] [-B address]\n", argv0);
	fatal("usage");
}

static void
noteconn(int fd)
{
	NetConnInfo *nci;

	nci = getnetconninfo(nil, fd);
	if (nci == nil)
		return;
	netdir = strdup(nci->dir);
	local = strdup(nci->lsys);
	remote = strdup(nci->rsys);
	freenetconninfo(nci);
}

void
main(int argc, char **argv)
{
	char buf[ERRMAX], ebuf[ERRMAX], *srvfdfile;
	Fsrpc *r;
	int doauth, n, fd;
	char *dbfile, *srv, *na, *nsfile, *keyspec;
	AuthInfo *ai;
	ulong initial;

	dbfile = "/tmp/exportdb";
	srv = nil;
	srvfd = -1;
	srvfdfile = nil;
	na = nil;
	nsfile = nil;
	keyspec = "";
	doauth = 0;

	ai = nil;
	ARGBEGIN{
	case 'a':
		doauth = 1;
		break;

	case 'd':
		dbg++;
		break;

	case 'e':
		ealgs = EARGF(usage());
		if(*ealgs == 0 || strcmp(ealgs, "clear") == 0)
			ealgs = nil;
		break;

	case 'f':
		dbfile = EARGF(usage());
		break;

	case 'k':
		keyspec = EARGF(usage());
		break;

	case 'm':
		messagesize = strtoul(EARGF(usage()), nil, 0);
		break;

	case 'n':
		nonone = 0;
		break;

	case 'r':
		srv = EARGF(usage());
		break;

	case 's':
		srv = "/";
		break;

	case 'A':
		anstring = EARGF(usage());
		break;

	case 'B':
		na = EARGF(usage());
		break;

	case 'F':
		/* accepted but ignored, for backwards compatibility */
		break;

	case 'N':
		nsfile = EARGF(usage());
		break;

	case 'P':
		patternfile = EARGF(usage());
		break;

	case 'R':
		readonly = 1;
		break;

	case 'S':
		if(srvfdfile)
			usage();
		srvfdfile = EARGF(usage());
		break;

	default:
		usage();
	}ARGEND
	USED(argc, argv);

	if(doauth){
		/*
		 * We use p9any so we don't have to visit this code again, with the
		 * cost that this code is incompatible with the old world, which
		 * requires p9sk2. (The two differ in who talks first, so compatibility
		 * is awkward.)
		 */
		ai = auth_proxy(0, auth_getkey, "proto=p9any role=server %s", keyspec);
		if(ai == nil)
			fatal("auth_proxy: %r");
		if(nonone && strcmp(ai->cuid, "none") == 0)
			fatal("exportfs by none disallowed");
		if(auth_chuid(ai, nsfile) < 0)
			fatal("auth_chuid: %r");
		putenv("service", "exportfs");
	}

	if(srvfdfile){
		if((srvfd = open(srvfdfile, ORDWR)) < 0)
			sysfatal("open '%s': %r", srvfdfile);
	}

	if(na){
		if(srv == nil)
			sysfatal("-B requires -s");

		local = "me";
		remote = na;
		if((fd = dial(netmkaddr(na, 0, "importfs"), 0, 0, 0)) < 0)
			sysfatal("can't dial %s: %r", na);
	
		ai = auth_proxy(fd, auth_getkey, "proto=p9any role=client %s", keyspec);
		if(ai == nil)
			sysfatal("%r: %s", na);

		dup(fd, 0);
		dup(fd, 1);
		close(fd);
	}

	exclusions();

	if(dbg) {
		n = create(dbfile, OWRITE|OTRUNC, 0666);
		dup(n, DFD);
		close(n);
	}

	if(srvfd >= 0 && srv){
		fprint(2, "exportfs: -S cannot be used with -r or -s\n");
		usage();
	}

	DEBUG(DFD, "exportfs: started\n");

	rfork(RFNOTEG);

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

	/*
	 * Get tree to serve from network connection,
	 * check we can get there and ack the connection
 	 */
	if(srvfd != -1) {
		/* do nothing */
	}
	else if(srv) {
		if(chdir(srv) < 0) {
			errstr(ebuf, sizeof ebuf);
			fprint(0, "chdir(\"%s\"): %s\n", srv, ebuf);
			DEBUG(DFD, "chdir(\"%s\"): %s\n", srv, ebuf);
			exits(ebuf);
		}
		DEBUG(DFD, "invoked as server for %s", srv);
		strncpy(buf, srv, sizeof buf);
	}
	else {
		noteconn(netfd);
		buf[0] = 0;
		n = read(0, buf, sizeof(buf)-1);
		if(n < 0) {
			errstr(buf, sizeof buf);
			fprint(0, "read(0): %s\n", buf);
			DEBUG(DFD, "read(0): %s\n", buf);
			exits(buf);
		}
		buf[n] = 0;
		if(chdir(buf) < 0) {
			errstr(ebuf, sizeof ebuf);
			fprint(0, "chdir(%d:\"%s\"): %s\n", n, buf, ebuf);
			DEBUG(DFD, "chdir(%d:\"%s\"): %s\n", n, buf, ebuf);
			exits(ebuf);
		}
	}

	DEBUG(DFD, "\niniting root\n");
	initroot();

	DEBUG(DFD, "exportfs: %s\n", buf);

	if(srv == nil && srvfd == -1 && write(0, "OK", 2) != 2)
		fatal("open ack write");

	if (readn(netfd, &initial, sizeof(ulong)) < sizeof(ulong))
		fatal("can't read initial string: %r\n");

	if (strncmp((char *)&initial, "impo", sizeof(ulong)) == 0) {
		char buf[128], *p, *args[3];

		/* New import.  Read import's parameters... */
		initial = 0;

		p = buf;
		while (p - buf < sizeof buf) {
			if ((n = read(netfd, p, 1)) < 0)
				fatal("can't read impo arguments: %r\n");

			if (n == 0)
				fatal("connection closed while reading arguments\n");

			if (*p == '\n') 
				*p = '\0';
			if (*p++ == '\0')
				break;
		}
		
		if (tokenize(buf, args, nelem(args)) != 2)
			fatal("impo arguments invalid: impo%s...\n", buf);

		if (strcmp(args[0], "aan") == 0)
			filterp = aanfilter;
		else if (strcmp(args[0], "nofilter") != 0)
			fatal("import filter argument unsupported: %s\n", args[0]);

		if (strcmp(args[1], "ssl") == 0)
			encproto = Encssl;
		else if (strcmp(args[1], "tls") == 0)
			encproto = Enctls;
		else if (strcmp(args[1], "clear") != 0)
			fatal("import encryption proto unsupported: %s\n", args[1]);

		if (encproto == Enctls)
			sysfatal("%s: tls has not yet been implemented", argv[0]);
	}

	if (encproto != Encnone && ealgs && ai) {
		uchar key[16];
		uchar digest[SHA1dlen];
		char fromclientsecret[21];
		char fromserversecret[21];
		int i;

		memmove(key+4, ai->secret, ai->nsecret);

		/* exchange random numbers */
		srand(truerand());
		for(i = 0; i < 4; i++)
			key[i+12] = rand();

		if (initial) 
			fatal("Protocol botch: old import\n");
		if(readn(netfd, key, 4) != 4)
			fatal("can't read key part; %r\n");

		if(write(netfd, key+12, 4) != 4)
			fatal("can't write key part; %r\n");

		/* scramble into two secrets */
		sha1(key, sizeof(key), digest, nil);
		mksecret(fromclientsecret, digest);
		mksecret(fromserversecret, digest+10);

		if (filterp)
			netfd = filter(netfd, filterp);

		switch (encproto) {
		case Encssl:
			netfd = pushssl(netfd, ealgs, fromserversecret, 
						fromclientsecret, nil);
			break;
		case Enctls:
		default:
			fatal("Unsupported encryption protocol\n");
		}

		if(netfd < 0)
			fatal("can't establish ssl connection: %r");
	}
	else if (filterp) {
		if (initial) 
			fatal("Protocol botch: don't know how to deal with this\n");
		netfd = filter(netfd, filterp);
	}

	/*
	 * Start serving file requests from the network
	 */
	for(;;) {
		r = getsbuf();
		if(r == 0)
			fatal("Out of service buffers");
			
		n = localread9pmsg(netfd, r->buf, messagesize, &initial);
		if(n <= 0)
			fatal(nil);

		if(convM2S(r->buf, n, &r->work) == 0)
			fatal("convM2S format error");

		DEBUG(DFD, "%F\n", &r->work);
		(fcalls[r->work.type])(r);
	}
}

/*
 * WARNING: Replace this with the original version as soon as all 
 * _old_ imports have been replaced with negotiating imports.  Also
 * cpu relies on this (which needs to be fixed!) -- pb.
 */
static int
localread9pmsg(int fd, void *abuf, uint n, ulong *initial)
{
	int m, len;
	uchar *buf;

	buf = abuf;

	/* read count */
	assert(BIT32SZ == sizeof(ulong));
	if (*initial) {
		memcpy(buf, initial, BIT32SZ);
		*initial = 0;
	}
	else {
		m = readn(fd, buf, BIT32SZ);
		if(m != BIT32SZ){
			if(m < 0)
				return -1;
			return 0;
		}
	}

	len = GBIT32(buf);
	if(len <= BIT32SZ || len > n){
		werrstr("bad length in 9P2000 message header");
		return -1;
	}
	len -= BIT32SZ;
	m = readn(fd, buf+BIT32SZ, len);
	if(m < len)
		return 0;
	return BIT32SZ+m;
}
void
reply(Fcall *r, Fcall *t, char *err)
{
	uchar *data;
	int n;

	t->tag = r->tag;
	t->fid = r->fid;
	if(err) {
		t->type = Rerror;
		t->ename = err;
	}
	else 
		t->type = r->type + 1;

	DEBUG(DFD, "\t%F\n", t);

	data = malloc(messagesize);	/* not mallocz; no need to clear */
	if(data == nil)
		fatal(Enomem);
	n = convS2M(t, data, messagesize);
	if(write(netfd, data, n)!=n)
{syslog(0, "exportfs", "short write: %r");
		fatal("mount write");
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
			if(f->dir){
				free(f->dir);
				f->dir = nil;
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
	if(patternfile != nil && excludefile(path)){
		free(path);
		return nil;
	}
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
	Proc *m;

	if (s) {
		va_start(arg, s);
		vsnprint(buf, ERRMAX, s, arg);
		va_end(arg);
	}

	/* Clear away the slave children */
	for(m = Proclist; m; m = m->next)
		postnote(PNPROC, m->pid, "kill");

	DEBUG(DFD, "%s\n", buf);
	if (s) 
		sysfatal("%s", buf);	/* caution: buf could contain '%' */
	else
		exits(nil);
}

void*
emallocz(uint n)
{
	void *p;

	p = mallocz(n, 1);
	if(p == nil)
		fatal(Enomem);
	return p;
}

char*
estrdup(char *s)
{
	char *t;

	t = strdup(s);
	if(t == nil)
		fatal(Enomem);
	return t;
}

/* Network on fd1, mount driver on fd0 */
int
filter(int fd, char *cmd)
{
	int p[2], lfd, len, nb, argc;
	char newport[128], buf[128], devdir[40], *s, *file, *argv[16];

	/* Get a free port and post it to the client. */
	if (announce(anstring, devdir) < 0)
		sysfatal("filter: Cannot announce %s: %r", anstring);

	snprint(buf, sizeof(buf), "%s/local", devdir);
	buf[sizeof buf - 1] = '\0';
	if ((lfd = open(buf, OREAD)) < 0)
		sysfatal("filter: Cannot open %s: %r", buf);
	if ((len = read(lfd, newport, sizeof newport - 1)) < 0)
		sysfatal("filter: Cannot read %s: %r", buf);
	close(lfd);
	newport[len] = '\0';

	if ((s = strchr(newport, '\n')) != nil)
		*s = '\0';

	if ((nb = write(fd, newport, len)) < 0) 
		sysfatal("getport; cannot write port; %r");
	assert(nb == len);

	argc = tokenize(cmd, argv, nelem(argv)-2);
	if (argc == 0)
		sysfatal("filter: empty command");
	argv[argc++] = buf;
	argv[argc] = nil;
	file = argv[0];
	if (s = strrchr(argv[0], '/'))
		argv[0] = s+1;

	if(pipe(p) < 0)
		fatal("pipe");

	switch(rfork(RFNOWAIT|RFPROC|RFFDG)) {
	case -1:
		fatal("rfork record module");
	case 0:
		if (dup(p[0], 1) < 0)
			fatal("filter: Cannot dup to 1; %r\n");
		if (dup(p[0], 0) < 0)
			fatal("filter: Cannot dup to 0; %r\n");
		close(p[0]);
		close(p[1]);
		exec(file, argv);
		fatal("exec record module");
	default:
		close(fd);
		close(p[0]);
	}
	return p[1];	
}

static void
mksecret(char *t, uchar *f)
{
	sprint(t, "%2.2ux%2.2ux%2.2ux%2.2ux%2.2ux%2.2ux%2.2ux%2.2ux%2.2ux%2.2ux",
		f[0], f[1], f[2], f[3], f[4], f[5], f[6], f[7], f[8], f[9]);
}
