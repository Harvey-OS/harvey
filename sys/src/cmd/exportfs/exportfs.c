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

int	srvfd = -1;
int	nonone = 1;
char	*filterp;
char	*ealgs = "rc4_256 sha1";
char	*aanfilter = "/bin/aan";
int	encproto = Encnone;
int	readonly;

static void mksecret(char *, uchar *);
static int localread9pmsg(int, void *, uint, void *);
static char *anstring  = "tcp!*!0";

char *netdir = "", *local = "", *remote = "";

void	filter(int, char *, char *);

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
	if(nci == nil)
		return;
	netdir = estrdup(nci->dir);
	local = estrdup(nci->lsys);
	remote = estrdup(nci->rsys);
	freenetconninfo(nci);
}

void
main(int argc, char **argv)
{
	char buf[ERRMAX], ebuf[ERRMAX], initial[4], *ini, *srvfdfile;
	char *dbfile, *srv, *na, *nsfile, *keyspec;
	int doauth, n, fd;
	AuthInfo *ai;
	Fsrpc *r;

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
		if(srvfdfile != nil)
			usage();
		srvfdfile = EARGF(usage());
		break;

	default:
		usage();
	}ARGEND
	USED(argc, argv);

	if(na == nil && doauth){
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

	if(srvfdfile != nil){
		if((srvfd = open(srvfdfile, ORDWR)) < 0)
			fatal("open %s: %r", srvfdfile);
	}

	if(na != nil){
		if(srv == nil)
			fatal("-B requires -s");

		local = "me";
		remote = na;
		if((fd = dial(netmkaddr(na, 0, "importfs"), 0, 0, 0)) < 0)
			fatal("can't dial %s: %r", na);
	
		ai = auth_proxy(fd, auth_getkey, "proto=p9any role=client %s", keyspec);
		if(ai == nil)
			fatal("%r: %s", na);

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

	if(srvfd >= 0 && srv != nil){
		fprint(2, "exportfs: -S cannot be used with -r or -s\n");
		usage();
	}

	DEBUG(DFD, "exportfs: started\n");

	rfork(RFNOTEG|RFREND);

	if(messagesize == 0){
		messagesize = iounit(0);
		if(messagesize == 0)
			messagesize = 8192+IOHDRSZ;
	}
	fhash = emallocz(sizeof(Fid*)*FHASHSIZE);

	fmtinstall('F', fcallfmt);

	/*
	 * Get tree to serve from network connection,
	 * check we can get there and ack the connection
 	 */
	if(srvfd != -1) {
		/* do nothing */
	}
	else if(srv != nil) {
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
		noteconn(0);
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

	ini = initial;
	n = readn(0, initial, sizeof(initial));
	if(n == 0)
		fatal(nil);	/* port scan or spurious open/close on exported /srv file (unmount) */
	if(n < sizeof(initial))
		fatal("can't read initial string: %r");

	if(memcmp(ini, "impo", 4) == 0) {
		char buf[128], *p, *args[3];

		ini = nil;
		p = buf;
		for(;;){
			if((n = read(0, p, 1)) < 0)
				fatal("can't read impo arguments: %r");
			if(n == 0)
				fatal("connection closed while reading arguments");
			if(*p == '\n') 
				*p = '\0';
			if(*p++ == '\0')
				break;
			if(p >= buf + sizeof(buf))
				fatal("import parameters too long");
		}
		
		if(tokenize(buf, args, nelem(args)) != 2)
			fatal("impo arguments invalid: impo%s...", buf);

		if(strcmp(args[0], "aan") == 0)
			filterp = aanfilter;
		else if(strcmp(args[0], "nofilter") != 0)
			fatal("import filter argument unsupported: %s", args[0]);

		if(strcmp(args[1], "ssl") == 0)
			encproto = Encssl;
		else if(strcmp(args[1], "tls") == 0)
			encproto = Enctls;
		else if(strcmp(args[1], "clear") != 0)
			fatal("import encryption proto unsupported: %s", args[1]);

		if(encproto == Enctls)
			fatal("%s: tls has not yet been implemented", argv[0]);
	}

	if(encproto != Encnone && ealgs != nil && ai != nil) {
		uchar key[16], digest[SHA1dlen];
		char fromclientsecret[21];
		char fromserversecret[21];
		int i;

		if(ai->nsecret < 8)
			fatal("secret too small for ssl");
		memmove(key+4, ai->secret, 8);

		/* exchange random numbers */
		srand(truerand());
		for(i = 0; i < 4; i++)
			key[i+12] = rand();

		if(ini != nil) 
			fatal("Protocol botch: old import");
		if(readn(0, key, 4) != 4)
			fatal("can't read key part; %r");

		if(write(0, key+12, 4) != 4)
			fatal("can't write key part; %r");

		/* scramble into two secrets */
		sha1(key, sizeof(key), digest, nil);
		mksecret(fromclientsecret, digest);
		mksecret(fromserversecret, digest+10);

		if(filterp != nil)
			filter(0, filterp, na);

		switch(encproto) {
		case Encssl:
			fd = pushssl(0, ealgs, fromserversecret, fromclientsecret, nil);
			if(fd < 0)
				fatal("can't establish ssl connection: %r");
			if(fd != 0){
				dup(fd, 0);
				close(fd);
			}
			break;
		case Enctls:
		default:
			fatal("Unsupported encryption protocol");
		}
	}
	else if(filterp != nil) {
		if(ini != nil)
			fatal("Protocol botch: don't know how to deal with this");
		filter(0, filterp, na);
	}
	dup(0, 1);

	if(ai != nil)
		auth_freeAI(ai);

	/*
	 * Start serving file requests from the network
	 */
	for(;;) {
		r = getsbuf();
		while((n = localread9pmsg(0, r->buf, messagesize, ini)) == 0)
			;
		if(n < 0)
			fatal(nil);
		if(convM2S(r->buf, n, &r->work) == 0)
			fatal("convM2S format error");

		DEBUG(DFD, "%F\n", &r->work);
		(fcalls[r->work.type])(r);
		ini = nil;
	}
}

/*
 * WARNING: Replace this with the original version as soon as all 
 * _old_ imports have been replaced with negotiating imports.  Also
 * cpu relies on this (which needs to be fixed!) -- pb.
 */
static int
localread9pmsg(int fd, void *abuf, uint n, void *ini)
{
	int m, len;
	uchar *buf;

	buf = abuf;

	/* read count */
	if(ini != nil)
		memcpy(buf, ini, BIT32SZ);
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
	if(err != nil) {
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
	if(write(0, data, n) != n){
		/* not fatal, might have got a note due to flush */
		fprint(2, "exportfs: short write in reply: %r\n");
	}
	free(data);
}

Fid *
getfid(int nr)
{
	Fid *f;

	for(f = fidhash(nr); f != nil; f = f->next)
		if(f->nr == nr)
			return f;

	return nil;
}

int
freefid(int nr)
{
	Fid *f, **l;
	char buf[128];

	l = &fidhash(nr);
	for(f = *l; f != nil; f = f->next) {
		if(f->nr == nr) {
			if(f->mid) {
				snprint(buf, sizeof(buf), "/mnt/exportfs/%d", f->mid);
				unmount(0, buf);
				psmap[f->mid] = 0;
			}
			if(f->f != nil) {
				freefile(f->f);
				f->f = nil;
			}
			if(f->dir != nil){
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
	for(new = *l; new != nil; new = new->next)
		if(new->nr == nr)
			return nil;

	if(fidfree == nil) {
		fidfree = emallocz(sizeof(Fid) * Fidchunk);

		for(i = 0; i < Fidchunk-1; i++)
			fidfree[i].next = &fidfree[i+1];

		fidfree[Fidchunk-1].next = nil;
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

static struct {
	Lock;
	Fsrpc	*free;

	/* statistics */
	int	nalloc;
	int	nfree;
}	sbufalloc;

Fsrpc *
getsbuf(void)
{
	Fsrpc *w;

	lock(&sbufalloc);
	w = sbufalloc.free;
	if(w != nil){
		sbufalloc.free = w->next;
		w->next = nil;
		sbufalloc.nfree--;
		unlock(&sbufalloc);
	} else {
		sbufalloc.nalloc++;
		unlock(&sbufalloc);
		w = emallocz(sizeof(*w) + messagesize);
	}
	w->flushtag = NOTAG;
	return w;
}

void
putsbuf(Fsrpc *w)
{
	w->flushtag = NOTAG;
	lock(&sbufalloc);
	w->next = sbufalloc.free;
	sbufalloc.free = w;
	sbufalloc.nfree++;
	unlock(&sbufalloc);
}

void
freefile(File *f)
{
	File *parent, *child;

	while(--f->ref == 0){
		freecnt++;
		DEBUG(DFD, "free %s\n", f->name);
		/* delete from parent */
		parent = f->parent;
		if(parent->child == f)
			parent->child = f->childlist;
		else{
			for(child = parent->child; child->childlist != f; child = child->childlist) {
				if(child->childlist == nil)
					fatal("bad child list");
			}
			child->childlist = f->childlist;
		}
		freeqid(f->qidt);
		free(f->name);
		free(f);
		f = parent;
	}
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

	for(f = parent->child; f != nil; f = f->childlist)
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
	if(psmpt == nil)
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
	path = emallocz(n);
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

	if(--q->ref)
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
	qidcnt++;
	q = emallocz(sizeof(Qidtab));
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

	if(s != nil) {
		va_start(arg, s);
		vsnprint(buf, ERRMAX, s, arg);
		va_end(arg);
	}

	/* Clear away the slave children */
	for(m = Proclist; m != nil; m = m->next)
		postnote(PNPROC, m->pid, "kill");

	if(s != nil) {
		DEBUG(DFD, "%s\n", buf);
		sysfatal("%s", buf);	/* caution: buf could contain '%' */
	} else
		exits(nil);
}

void*
emallocz(uint n)
{
	void *p;

	p = mallocz(n, 1);
	if(p == nil)
		fatal(Enomem);
	setmalloctag(p, getcallerpc(&n));
	return p;
}

char*
estrdup(char *s)
{
	char *t;

	t = strdup(s);
	if(t == nil)
		fatal(Enomem);
	setmalloctag(t, getcallerpc(&s));
	return t;
}

void
filter(int fd, char *cmd, char *host)
{
	char addr[128], buf[256], *s, *file, *argv[16];
	int lfd, p[2], len, argc;

	if(host == nil){
		/* Get a free port and post it to the client. */
		if (announce(anstring, addr) < 0)
			fatal("filter: Cannot announce %s: %r", anstring);

		snprint(buf, sizeof(buf), "%s/local", addr);
		if ((lfd = open(buf, OREAD)) < 0)
			fatal("filter: Cannot open %s: %r", buf);
		if ((len = read(lfd, buf, sizeof buf - 1)) < 0)
			fatal("filter: Cannot read %s: %r", buf);
		close(lfd);
		buf[len] = '\0';
		if ((s = strchr(buf, '\n')) != nil)
			len = s - buf;
		if (write(fd, buf, len) != len) 
			fatal("filter: cannot write port; %r");
	} else {
		/* Read address string from connection */
		if ((len = read(fd, buf, sizeof buf - 1)) < 0)
			sysfatal("filter: cannot write port; %r");
		buf[len] = '\0';

		if ((s = strrchr(buf, '!')) == nil)
			sysfatal("filter: illegally formatted port %s", buf);
		strecpy(addr, addr+sizeof(addr), netmkaddr(host, "tcp", s+1));
		strecpy(strrchr(addr, '!'), addr+sizeof(addr), s);
	}

	DEBUG(DFD, "filter: %s\n", addr);

	snprint(buf, sizeof(buf), "%s", cmd);
	argc = tokenize(buf, argv, nelem(argv)-3);
	if (argc == 0)
		sysfatal("filter: empty command");

	if(host != nil)
		argv[argc++] = "-c";
	argv[argc++] = addr;
	argv[argc] = nil;

	file = argv[0];
	if((s = strrchr(argv[0], '/')) != nil)
		argv[0] = s+1;

	if(pipe(p) < 0)
		sysfatal("pipe: %r");

	switch(rfork(RFNOWAIT|RFPROC|RFMEM|RFFDG|RFREND)) {
	case -1:
		fatal("filter: rfork; %r\n");
	case 0:
		close(fd);
		if (dup(p[0], 1) < 0)
			fatal("filter: Cannot dup to 1; %r");
		if (dup(p[0], 0) < 0)
			fatal("filter: Cannot dup to 0; %r");
		close(p[0]);
		close(p[1]);
		exec(file, argv);
		fatal("filter: exec; %r");
	default:
		dup(p[1], fd);
		close(p[0]);
		close(p[1]);
	}
}

static void
mksecret(char *t, uchar *f)
{
	sprint(t, "%2.2ux%2.2ux%2.2ux%2.2ux%2.2ux%2.2ux%2.2ux%2.2ux%2.2ux%2.2ux",
		f[0], f[1], f[2], f[3], f[4], f[5], f[6], f[7], f[8], f[9]);
}
