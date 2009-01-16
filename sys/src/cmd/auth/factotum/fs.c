#include "dat.h"

int		askforkeys = 1;
char		*authaddr;
int		debug;
int		doprivate = 1;
int		gflag;
char		*owner;
int		kflag;
char		*mtpt = "/mnt";
Keyring	*ring;
char		*service;
int		sflag;
int		uflag;

extern Srv		fs;
static void		notifyf(void*, char*);
static void		private(void);

char	Easproto[]		= "auth server protocol botch";
char Ebadarg[]		= "invalid argument";
char Ebadkey[]		= "bad key";
char Enegotiation[]	= "negotiation failed, no common protocols or keys";
char Etoolarge[]	= "rpc too large";

Proto*
prototab[] =
{
	&apop,
	&chap,
	&cram,
	&httpdigest,
	&mschap,
	&p9any,
	&p9cr,
	&p9sk1,
	&p9sk2,
	&pass,
/*	&srs, */
	&rsa,
	&vnc,
	&wep,
	nil,
};

void
usage(void)
{
	fprint(2, "usage: %s [-DSdknpu] [-a authaddr] [-m mtpt] [-s service]\n",
		argv0);
	fprint(2, "or    %s -g 'params'\n", argv0);
	exits("usage");
}

void
main(int argc, char **argv)
{
	int i, trysecstore;
	char err[ERRMAX], *s;
	Dir d;
	Proto *p;
	char *secstorepw;

	trysecstore = 1;
	secstorepw = nil;

	ARGBEGIN{
	case 'D':
		chatty9p++;
		break;
	case 'S':		/* server: read nvram, no prompting for keys */
		askforkeys = 0;
		trysecstore = 0;
		sflag = 1;
		break;
	case 'a':
		authaddr = EARGF(usage());
		break;
	case 'd':
		debug = 1;
		doprivate = 0;
		break;
	case 'g':		/* get: prompt for key for name and domain */
		gflag = 1;
		break;
	case 'k':		/* reinitialize nvram */
		kflag = 1;
		break;
	case 'm':		/* set default mount point */
		mtpt = EARGF(usage());
		break;
	case 'n':
		trysecstore = 0;
		break;
	case 'p':
		doprivate = 0;
		break;
	case 's':		/* set service name */
		service = EARGF(usage());
		break;
	case 'u':		/* user: set hostowner */
		uflag = 1;
		break;
	default:
		usage();
	}ARGEND

	if(argc != 0 && !gflag)
		usage();
	if(doprivate)
		private();

	initcap();

	quotefmtinstall();
	fmtinstall('A', _attrfmt);
	fmtinstall('N', attrnamefmt);
	fmtinstall('H', encodefmt);

	ring = emalloc(sizeof(*ring));
	notify(notifyf);

	if(gflag){
		if(argc != 1)
			usage();
		askuser(argv[0]);
		exits(nil);
	}

	for(i=0; prototab[i]; i++){
		p = prototab[i];
		if(p->name == nil)
			sysfatal("protocol %d has no name", i);
		if(p->init == nil)
			sysfatal("protocol %s has no init", p->name);
		if(p->write == nil)
			sysfatal("protocol %s has no write", p->name);
		if(p->read == nil)
			sysfatal("protocol %s has no read", p->name);
		if(p->close == nil)
			sysfatal("protocol %s has no close", p->name);
		if(p->keyprompt == nil)
			p->keyprompt = "";
	}

	if(sflag){
		s = getnvramkey(kflag ? NVwrite : NVwriteonerr, &secstorepw);
		if(s == nil)
			fprint(2, "factotum warning: cannot read nvram: %r\n");
		else if(ctlwrite(s, 0) < 0)
			fprint(2, "factotum warning: cannot add nvram key: %r\n");
		if(secstorepw != nil)
			trysecstore = 1;
		if (s != nil) {
			memset(s, 0, strlen(s));
			free(s);
		}
	} else if(uflag)
		promptforhostowner();
	owner = getuser();

	if(trysecstore){
		if(havesecstore() == 1){
			while(secstorefetch(secstorepw) < 0){
				rerrstr(err, sizeof err);
				if(strcmp(err, "cancel") == 0)
					break;
				fprint(2, "factotum: secstorefetch: %r\n");
				fprint(2, "Enter an empty password to quit.\n");
				free(secstorepw);
				secstorepw = nil; /* just try nvram pw once */
			}
		}else{
/*
			rerrstr(err, sizeof err);
			if(*err)
				fprint(2, "factotum: havesecstore: %r\n");
*/
		}
	}

	postmountsrv(&fs, service, mtpt, MBEFORE);
	if(service){
		nulldir(&d);
		d.mode = 0666;
		s = emalloc(10+strlen(service));
		strcpy(s, "/srv/");
		strcat(s, service);
		if(dirwstat(s, &d) < 0)
			fprint(2, "factotum warning: cannot chmod 666 %s: %r\n", s);
		free(s);
	}
	exits(nil);
}

char *pmsg = "Warning! %s can't protect itself from debugging: %r\n";
char *smsg = "Warning! %s can't turn off swapping: %r\n";

/* don't allow other processes to debug us and steal keys */
static void
private(void)
{
	int fd;
	char buf[64];

	snprint(buf, sizeof(buf), "#p/%d/ctl", getpid());
	fd = open(buf, OWRITE);
	if(fd < 0){
		fprint(2, pmsg, argv0);
		return;
	}
	if(fprint(fd, "private") < 0)
		fprint(2, pmsg, argv0);
	if(fprint(fd, "noswap") < 0)
		fprint(2, smsg, argv0);
	close(fd);
}

static void
notifyf(void*, char *s)
{
	if(strncmp(s, "interrupt", 9) == 0)
		noted(NCONT);
	noted(NDFLT);
}

enum
{
	Qroot,
	Qfactotum,
	Qrpc,
	Qkeylist,
	Qprotolist,
	Qconfirm,
	Qlog,
	Qctl,
	Qneedkey,
};

Qid
mkqid(int type, int path)
{
	Qid q;

	q.type = type;
	q.path = path;
	q.vers = 0;
	return q;
}

static void
fsattach(Req *r)
{
	r->fid->qid = mkqid(QTDIR, Qroot);
	r->ofcall.qid = r->fid->qid;
	respond(r, nil);
}

static struct {
	char *name;
	int qidpath;
	ulong perm;
} dirtab[] = {
	"confirm",	Qconfirm,	0600|DMEXCL,		/* we know this is slot #0 below */
	"needkey", Qneedkey,	0600|DMEXCL,		/* we know this is slot #1 below */
	"ctl",		Qctl,			0644,
	"rpc",	Qrpc,		0666,
	"proto",	Qprotolist,	0444,
	"log",	Qlog,		0400|DMEXCL,
};
static int inuse[nelem(dirtab)];
int *confirminuse = &inuse[0];
int *needkeyinuse = &inuse[1];

static void
fillstat(Dir *dir, char *name, int type, int path, ulong perm)
{
	dir->name = estrdup(name);
	dir->uid = estrdup(owner);
	dir->gid = estrdup(owner);
	dir->mode = perm;
	dir->length = 0;
	dir->qid = mkqid(type, path);
	dir->atime = time(0);
	dir->mtime = time(0);
	dir->muid = estrdup("");
}

static int
rootdirgen(int n, Dir *dir, void*)
{
	if(n > 0)
		return -1;
	fillstat(dir, "factotum", QTDIR, Qfactotum, DMDIR|0555);
	return 0;
}

static int
fsdirgen(int n, Dir *dir, void*)
{
	if(n >= nelem(dirtab))
		return -1;
	fillstat(dir, dirtab[n].name, 0, dirtab[n].qidpath, dirtab[n].perm);
	return 0;
}

static char*
fswalk1(Fid *fid, char *name, Qid *qid)
{
	int i;

	switch((ulong)fid->qid.path){
	default:
		return "cannot happen";
	case Qroot:
		if(strcmp(name, "factotum") == 0){
			*qid = mkqid(QTDIR, Qfactotum);
			fid->qid = *qid;
			return nil;
		}
		if(strcmp(name, "..") == 0){
			*qid = fid->qid;
			return nil;
		}
		return "not found";
	case Qfactotum:
		for(i=0; i<nelem(dirtab); i++)
			if(strcmp(name, dirtab[i].name) == 0){
				*qid = mkqid(0, dirtab[i].qidpath);
				fid->qid = *qid;
				return nil;
			}
		if(strcmp(name, "..") == 0){
			*qid = mkqid(QTDIR, Qroot);
			fid->qid = *qid;
			return nil;
		}
		return "not found";
	}
}

static void
fsstat(Req *r)
{
	int i;
	ulong path;

	path = r->fid->qid.path;
	if(path == Qroot){
		fillstat(&r->d, "/", QTDIR, Qroot, 0555|DMDIR);
		respond(r, nil);
		return;
	}
	if(path == Qfactotum){
		fillstat(&r->d, "factotum", QTDIR, Qfactotum, 0555|DMDIR);
		respond(r, nil);
		return;
	}
	for(i=0; i<nelem(dirtab); i++)
		if(dirtab[i].qidpath == path){
			fillstat(&r->d, dirtab[i].name, 0, dirtab[i].qidpath, dirtab[i].perm);
			respond(r, nil);
			return;
		}
	respond(r, "file not found");	
}

static void
fsopen(Req *r)
{
	int i, *p, perm;
	static int need[4] = {4, 2, 6, 1};
	int n;
	Fsstate *fss;

	p = nil;
	for(i=0; i<nelem(dirtab); i++)
		if(dirtab[i].qidpath == r->fid->qid.path)
			break;
	if(i < nelem(dirtab)){
		if(dirtab[i].perm & DMEXCL)
			p = &inuse[i];
		if(strcmp(r->fid->uid, owner) == 0)
			perm = dirtab[i].perm>>6;
		else
			perm = dirtab[i].perm;
	}else
		perm = 5;

	n = need[r->ifcall.mode&3];
	if((r->ifcall.mode&~(3|OTRUNC)) || ((perm&n) != n)){
		respond(r, "permission denied");
		return;
	}
	if(p){
		if(*p){
			respond(r, "file in use");
			return;
		}
		(*p)++;
	}

	r->fid->aux = fss = emalloc(sizeof(Fsstate));
	fss->phase = Notstarted;
	fss->sysuser = r->fid->uid;
	fss->attr = nil;
	strcpy(fss->err, "factotum/fs.c no error");
	respond(r, nil);
}

static void
fsdestroyfid(Fid *fid)
{
	int i;
	Fsstate *fss;

	if(fid->omode != -1){
		for(i=0; i<nelem(dirtab); i++)
			if(dirtab[i].qidpath == fid->qid.path)
				if(dirtab[i].perm&DMEXCL)
					inuse[i] = 0;
	}

	fss = fid->aux;
	if(fss == nil)
		return;
	if(fss->ps)
		(*fss->proto->close)(fss);
	_freeattr(fss->attr);
	free(fss);
}

static int
readlist(int off, int (*gen)(int, char*, uint, Fsstate*), Req *r, Fsstate *fss)
{
	char *a, *ea;
	int n;

	a = r->ofcall.data;
	ea = a+r->ifcall.count;
	for(;;){
		n = (*gen)(off, a, ea-a, fss);
		if(n == 0){
			r->ofcall.count = a - (char*)r->ofcall.data;
			return off;
		}
		a += n;
		off++;
	}
}

enum { Nearend = 2, };			/* at least room for \n and NUL */

/* result in `a', of `n' bytes maximum */
static int
keylist(int i, char *a, uint n, Fsstate *fss)
{
	int wb;
	Keyinfo ki;
	Key *k;
	static char zero[Nearend];

	k = nil;
	mkkeyinfo(&ki, fss, nil);
	ki.attr = nil;
	ki.skip = i;
	ki.usedisabled = 1;
	if(findkey(&k, &ki, "") != RpcOk)
		return 0;

	memset(a + n - Nearend, 0, Nearend);
	wb = snprint(a, n, "key %A %N\n", k->attr, k->privattr);
	closekey(k);
	if (wb >= n - 1 && a[n - 2] != '\n' && a[n - 2] != '\0') {
		/* line won't fit in `a', so just truncate */
		strcpy(a + n - 2, "\n");
		return 0;
	}
	return wb;
}

static int
protolist(int i, char *a, uint n, Fsstate *fss)
{
	USED(fss);

	if(i >= nelem(prototab)-1)
		return 0;
	if(strlen(prototab[i]->name)+1 > n)
		return 0;
	n = strlen(prototab[i]->name)+1;
	memmove(a, prototab[i]->name, n-1);
	a[n-1] = '\n';
	return n;
}

static void
fsread(Req *r)
{
	Fsstate *s;

	s = r->fid->aux;
	switch((ulong)r->fid->qid.path){
	default:
		respond(r, "bug in fsread");
		break;
	case Qroot:
		dirread9p(r, rootdirgen, nil);
		respond(r, nil);
		break;
	case Qfactotum:
		dirread9p(r, fsdirgen, nil);
		respond(r, nil);
		break;
	case Qrpc:
		rpcread(r);
		break;
	case Qneedkey:
		needkeyread(r);
		break;
	case Qconfirm:
		confirmread(r);
		break;
	case Qlog:
		logread(r);
		break;
	case Qctl:
		s->listoff = readlist(s->listoff, keylist, r, s);
		respond(r, nil);
		break;
	case Qprotolist:
		s->listoff = readlist(s->listoff, protolist, r, s);
		respond(r, nil);
		break;
	}
}

static void
fswrite(Req *r)
{
	int ret;
	char err[ERRMAX], *s;

	switch((ulong)r->fid->qid.path){
	default:
		respond(r, "bug in fswrite");
		break;
	case Qrpc:
		rpcwrite(r);
		break;
	case Qneedkey:
	case Qconfirm:
	case Qctl:
		s = emalloc(r->ifcall.count+1);
		memmove(s, r->ifcall.data, r->ifcall.count);
		s[r->ifcall.count] = '\0';
		switch((ulong)r->fid->qid.path){
		default:
			abort();
		case Qneedkey:
			ret = needkeywrite(s);
			break;
		case Qconfirm:
			ret = confirmwrite(s);
			break;
		case Qctl:
			ret = ctlwrite(s, 0);
			break;
		}
		free(s);
		if(ret < 0){
			rerrstr(err, sizeof err);
			respond(r, err);
		}else{
			r->ofcall.count = r->ifcall.count;
			respond(r, nil);
		}
		break;
	}
}

static void
fsflush(Req *r)
{
	confirmflush(r->oldreq);
	needkeyflush(r->oldreq);
	logflush(r->oldreq);
	respond(r, nil);
}

Srv fs = {
.attach=	fsattach,
.walk1=	fswalk1,
.open=	fsopen,
.read=	fsread,
.write=	fswrite,
.stat=	fsstat,
.flush=	fsflush,
.destroyfid=	fsdestroyfid,
};

