#include "ratfs.h"

#define	SRVFILE		"/srv/ratify"
#define MOUNTPOINT	"/mail/ratify"
#define	CTLFILE		"/mail/lib/blocked"
#define	CONFFILE	"/mail/lib/smtpd.conf.ext"

typedef struct Filetree	Filetree;

	/* prototype file tree */
struct	Filetree
{
	int	level;
	char	*name;
	ushort	type;
	int	mode;
	ulong	qid;
};

	/* names of first-level directories - must be in order of level*/
Filetree	filetree[] =
{
	0,	"/",		Directory,	0555|DMDIR,	Qroot,
	1,	"allow",	Addrdir,	0555|DMDIR,	Qallow,
	1,	"delay",	Addrdir,	0555|DMDIR,	Qdelay,
	1,	"block",	Addrdir,	0555|DMDIR,	Qblock,
	1,	"dial",		Addrdir,	0555|DMDIR,	Qdial,
	1,	"deny",		Addrdir,	0555|DMDIR,	Qdeny,
	1,	"trusted",	Trusted,	0777|DMDIR,	Qtrusted,	/* creation allowed */
	1,	"ctl",		Ctlfile,	0222,		Qctl,
	2,	"ip",		IPaddr,		0555|DMDIR,	Qaddr,
	2,	"account",	Acctaddr,	0555|DMDIR,	Qaddr,
	0,	0,		0,		0,		0,
	
};

int	debugfd = -1;
int	trustedqid = Qtrustedfile;
char	*ctlfile =	CTLFILE;
char	*conffile =	CONFFILE;

#pragma	varargck	type	"I"	Cidraddr*

static	int	ipconv(Fmt*);
static	void	post(int, char*);
static	void	setroot(void);

void
usage(void)
{
	fprint(2, "ratfs [-d] [-c conffile] [-f ctlfile] [-m mountpoint]\n");
	exits("usage");
}

void
main(int argc, char *argv[])
{
	char *mountpoint = MOUNTPOINT;
	int p[2];

	ARGBEGIN {
	case 'c':
		conffile = ARGF();
		break;
	case 'd':
		debugfd = 2;		/* stderr*/
		break;
	case 'f':
		ctlfile = ARGF();
		break;
	case 'm':
		mountpoint = ARGF();
		break;
	} ARGEND
	if(argc != 0)
		usage();

	fmtinstall('I', ipconv);
	setroot();
	getconf();
	reload();

	/* get a pipe and mount it in /srv */
	if(pipe(p) < 0)
		fatal("pipe failed: %r");
	srvfd = p[0];
	post(p[1], mountpoint);

	/* start the 9fs protocol */
	switch(rfork(RFPROC|RFNAMEG|RFENVG|RFFDG|RFNOTEG|RFREND)){
	case -1:
		fatal("fork: %r");
	case 0:
		/* seal off standard input/output */
		close(0);
		open("/dev/null", OREAD);
		close(1);
		open("/dev/null", OWRITE);

		close(p[1]);
		fmtinstall('F', fcallfmt); /* debugging */
		io();
		fprint(2, "ratfs dying\n");
		break;
	default:
		close(p[0]);
		if(mount(p[1], -1, mountpoint, MREPL|MCREATE, "") < 0)
			fatal("mount failed: %r");
	}
	exits(0);
}

static void
setroot(void)
{
	Filetree *fp;
	Node *np;
	int qid;

	root = 0;
	qid = Qaddr;
	for(fp = filetree; fp->name; fp++) {
		switch(fp->level) {
		case 0:		/* root */
		case 1:		/* second level directory */
			newnode(root, fp->name, fp->type, fp->mode, fp->qid);
			break;
		case 2:		/* lay down the Ipaddr and Acctaddr subdirectories */
			for (np = root->children; np; np = np->sibs){
				if(np->d.type == Addrdir)
					newnode(np, fp->name, fp->type, fp->mode, qid++);
			}
			break;
		default:
			fatal("bad filetree");
		}
	}
	dummy.d.type = Dummynode;
	dummy.d.mode = 0444;
	dummy.d.uid = "upas";
	dummy.d.gid = "upas";
	dummy.d.atime = dummy.d.mtime = time(0);
	dummy.d.qid.path = Qdummy;				/* for now */
}

static void
post(int fd, char *mountpoint)
{

	int f;
	char buf[128];

	if(access(SRVFILE,0) >= 0){
		/*
		 * If we can open and mount the /srv node,
		 * another server is already running, so just exit.
		 */
		f = open(SRVFILE, ORDWR);
		if(f >= 0 && mount(f, -1, mountpoint, MREPL|MCREATE, "") >= 0){
				unmount(0, mountpoint);
				close(f);
				exits(0);
		}
		remove(SRVFILE);
	}

	/*
	 * create the server node and post our pipe to it
	 */
	f = create(SRVFILE, OWRITE, 0666);
	if(f < 0)
		fatal("can't create %s", SRVFILE);

	sprint(buf, "%d", fd);
	if(write(f, buf, strlen(buf)) != strlen(buf))
		fatal("can't write %s", SRVFILE);

	close(f);
}

/*
 *  print message and die
 */
void
fatal(char *fmt, ...)
{
	va_list arg;
	char buf[8*1024];

	va_start(arg, fmt);
	vseprint(buf, buf + (sizeof(buf)-1) / sizeof(*buf), fmt, arg);
	va_end(arg);

	fprint(2, "%s: %s\n", argv0, buf);
	exits(buf);
}

/*
 *  create a new directory node
 */
Node*
newnode(Node *parent, char *name, ushort type, int mode, ulong qid)
{
	Node *np;

	np = mallocz(sizeof(Node), 1);
	if(np == 0)
		fatal("out of memory");
	np->d.name = atom(name);
	np->d.type = type;
	np->d.mode = mode;
	np->d.mtime = np->d.atime = time(0);
	np->d.uid = atom("upas");
	np->d.gid = atom("upas");
	np->d.muid = atom("upas");
	if(np->d.mode&DMDIR)
		np->d.qid.type = QTDIR;
	np->d.qid.path = qid;
	np->d.qid.vers = 0;
	if(parent){
		np->parent = parent;
		np->sibs = parent->children;
		parent->children = np;
		parent->count++;
	} else {
		/* the root node */
		root = np;
		np->parent = np;
		np->children = 0;
		np->sibs = 0;
	}
	return np;
}

void
printnode(Node *np)
{
	fprint(debugfd, "Node at %p: %s (%s %s)", np, np->d.name, np->d.uid, np->d.gid);
	if(np->d.qid.type&QTDIR)
		fprint(debugfd, " QTDIR");
	fprint(debugfd, "\n");
	fprint(debugfd,"\tQID: %llud.%lud Mode: %lo Type: %d\n", np->d.qid.path,
			np->d.qid.vers, np->d.mode, np->d.type);
	fprint(debugfd, "\tMod: %.15s  Acc: %.15s Count: %d\n", ctime(np->d.mtime)+4,
			ctime(np->d.atime)+4, np->count);
	switch(np->d.type)
	{
	case Directory:
		fprint(debugfd, "\tDirectory Child: %p", np->children);
		break;
	case Addrdir:
		fprint(debugfd, "\tAddrdir Child: %p", np->children);
		break;
	case IPaddr:
		fprint(debugfd, "\tIPaddr Base: %p Alloc: %d BaseQid %lud", np->addrs,
			np->allocated, np->baseqid);
		break;
	case Acctaddr:
		fprint(debugfd, "\tAcctaddr Base: %p Alloc: %d BaseQid %lud", np->addrs,
			np->allocated, np->baseqid);
		break;
	case Trusted:
		fprint(debugfd, "\tTrusted Child: %p", np->children);
		break;
	case Trustedperm:
		fprint(debugfd, "\tPerm Trustedfile: %I", &np->ip);
		break;
	case Trustedtemp:
		fprint(debugfd, "\tTemp Trustedfile: %I", &np->ip);
		break;
	case Ctlfile:
		fprint(debugfd, "\tCtlfile");
		break;
	case Dummynode:
		fprint(debugfd, "\tDummynode");
		break;
	default:
		fprint(debugfd, "\tUnknown Node Type\n\n");
		return;
	}
	fprint(debugfd, " Parent %p Sib: %p\n\n", np->parent, np->sibs);
}

void
printfid(Fid *fp)
{
	fprint(debugfd, "FID: %d (%s %s) Busy: %d Open: %d\n", fp->fid, fp->name,
		fp->uid, fp->busy, fp->open);
	printnode(fp->node);
}

void
printtree(Node *np)
{
	printnode(np);
	if(np->d.type == IPaddr
		|| np->d.type == Acctaddr
		|| np->d.type == Trustedperm
		|| np->d.type == Trustedtemp)
			return;
	for (np = np->children; np; np = np->sibs)
		printtree(np);
}

static int
ipconv(Fmt *f)
{
	Cidraddr *ip;
	int i, j;
	char *p;

	ip = va_arg(f->args, Cidraddr*);
	p = (char*)&ip->ipaddr;
	i = 0;
	for (j = ip->mask; j; j <<= 1)
		i++;
	return fmtprint(f, "%d.%d.%d.%d/%d", p[3]&0xff, p[2]&0xff, p[1]&0xff, p[0]&0xff, i);
}
