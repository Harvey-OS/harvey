#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include <bio.h>
#include <ctype.h>
#include <ndb.h>
#include <ip.h>
#include <lock.h>

enum
{
	Nreply=			8,
	Maxreply=		256,
	Maxrequest=		4*NAMELEN,
	Ncache=			8,

	Qcs=			1,
};

typedef struct Mfile	Mfile;
typedef struct Mlist	Mlist;
typedef struct Network	Network;

int vers;		/* incremented each clone/attach */

struct Mfile
{
	int		busy;

	char		user[NAMELEN];
	Qid		qid;
	int		fid;

	int		nreply;
	char		*reply[Nreply];
	int		replylen[Nreply];
};

struct Mlist
{
	Mlist	*next;
	Mfile	mf;
};

Fcall	*rhp;
Fcall	*thp;
Mlist	*mlist;
int	mfd[2];
char	user[NAMELEN];
int	debug;
jmp_buf	masterjmp;	/* return through here after a slave process has been created */
int	*isslave;	/* *isslave non-zero means this is a slave process */
char	*dbfile;

void	rsession(void);
void	rflush(void);
void	rattach(Mfile *);
void	rclone(Mfile *);
char*	rwalk(Mfile *);
void	rclwalk(Mfile *);
void	ropen(Mfile *);
void	rcreate(Mfile *);
void	rread(Mfile *);
void	rwrite(Mfile *);
void	rclunk(Mfile *);
void	rremove(Mfile *);
void	rstat(Mfile *);
void	rauth(void);
void	rwstat(Mfile *);
void	sendmsg(char*);
void	error(char*);
void	mountinit(char*, ulong);
void	io(void);
void	netinit(void);
void	netadd(char*);
int	lookup(Mfile*, char*, char*, char*);
char	*genquery(Mfile*, char*);
int	mygetfields(char*, char**, int, char);
int	needproto(Network*, Ndbtuple*);
Ndbtuple*	lookval(Ndbtuple*, Ndbtuple*, char*, char*);
Ndbtuple*	reorder(Ndbtuple*, Ndbtuple*);

extern void	paralloc(void);

char *mname[]={
	[Tnop]		"Tnop",
	[Tsession]	"Tsession",
	[Tflush]	"Tflush",
	[Tattach]	"Tattach",
	[Tclone]	"Tclone",
	[Twalk]		"Twalk",
	[Topen]		"Topen",
	[Tcreate]	"Tcreate",
	[Tclunk]	"Tclunk",
	[Tread]		"Tread",
	[Twrite]	"Twrite",
	[Tremove]	"Tremove",
	[Tstat]		"Tstat",
	[Twstat]	"Twstat",
			0,
};

Lock	dblock;		/* mutex on database operations */

char *logfile = "cs";


void
main(int argc, char *argv[])
{
	Fcall	rhdr;
	Fcall	thdr;
	char	*service = "#s/cs";
	int	justsetname;

	justsetname = 0;
	rhp = &rhdr;
	thp = &thdr;
	ARGBEGIN{
	case 'd':
		debug = 1;
		service = "#s/dcs";
		break;
	case 'f':
		dbfile = ARGF();
		break;
	case 'n':
		justsetname = 1;
		break;
	}ARGEND
	USED(argc);
	USED(argv);

	if(justsetname){
		netinit();
		exits(0);
	}
	unmount(service, "/net");
	remove(service);
	mountinit(service, MAFTER);

	lockinit();
	fmtinstall('F', fcallconv);
	netinit();

	io();
	exits(0);
}

void
mountinit(char *service, ulong flags)
{
	int f;
	int p[2];
	char buf[32];

	if(pipe(p) < 0)
		error("pipe failed");
	switch(rfork(RFFDG|RFPROC|RFNAMEG)){
	case 0:
		close(p[1]);
		break;
	case -1:
		error("fork failed\n");
	default:
		/*
		 *  make a /srv/cs
		 */
		f = create(service, 1, 0666);
		if(f < 0)
			error(service);
		snprint(buf, sizeof(buf), "%d", p[1]);
		if(write(f, buf, strlen(buf)) != strlen(buf))
			error("write /srv/cs");
		close(f);

		/*
		 *  put ourselves into the file system
		 */
		close(p[0]);
		if(mount(p[1], "/net", flags, "") < 0)
			error("mount failed\n");
		_exits(0);
	}
	mfd[0] = mfd[1] = p[0];
}

Mfile*
newfid(int fid)
{
	Mlist *f, *ff;
	Mfile *mf;

	ff = 0;
	for(f = mlist; f; f = f->next)
		if(f->mf.busy && f->mf.fid == fid)
			return &f->mf;
		else if(!ff && !f->mf.busy)
			ff = f;
	if(ff == 0){
		ff = malloc(sizeof *f);
		if(ff == 0){
			fprint(2, "cs: malloc(%d)\n", sizeof *ff);
			error("malloc failure");
		}
		ff->next = mlist;
		mlist = ff;
	}
	mf = &ff->mf;
	memset(mf, 0, sizeof *mf);
	mf->fid = fid;
	return mf;
}

void
io(void)
{
	long n;
	Mfile *mf;
	int slaveflag;
	char mdata[MAXFDATA + MAXMSG];

	/*
	 *  if we ask dns to fulfill requests,
	 *  a slave process is created to wait for replies.  The
	 *  master process returns immediately via a longjmp's
	 *  through 'masterjmp'.
	 *
	 *  *isslave is a pointer into the call stack to a variable
	 *  that tells whether or not the current process is a slave.
	 */
	slaveflag = 0;		/* init slave variable */
	isslave = &slaveflag;
	setjmp(masterjmp);

	for(;;){
		n = read9p(mfd[0], mdata, sizeof mdata);
		if(n<=0)
			error("mount read");
		if(convM2S(mdata, rhp, n) == 0){
			syslog(1, logfile, "format error %ux %ux %ux %ux %ux", mdata[0], mdata[1], mdata[2], mdata[3], mdata[4]);
			continue;
		}
		if(rhp->fid<0)
			error("fid out of range");
		lock(&dblock);
		mf = newfid(rhp->fid);
		if(debug)
			syslog(0, logfile, "%F", rhp);
	
	
		switch(rhp->type){
		default:
			syslog(1, logfile, "unknown request type %d", rhp->type);
			break;
		case Tsession:
			rsession();
			break;
		case Tnop:
			rflush();
			break;
		case Tflush:
			rflush();
			break;
		case Tattach:
			rattach(mf);
			break;
		case Tclone:
			rclone(mf);
			break;
		case Twalk:
			rwalk(mf);
			break;
		case Tclwalk:
			rclwalk(mf);
			break;
		case Topen:
			ropen(mf);
			break;
		case Tcreate:
			rcreate(mf);
			break;
		case Tread:
			rread(mf);
			break;
		case Twrite:
			rwrite(mf);
			break;
		case Tclunk:
			rclunk(mf);
			break;
		case Tremove:
			rremove(mf);
			break;
		case Tstat:
			rstat(mf);
			break;
		case Twstat:
			rwstat(mf);
			break;
		}
		unlock(&dblock);
		/*
		 *  slave processes die after replying
		 */
		if(*isslave){
			if(debug)
				syslog(0, logfile, "slave death %d", getpid());
			_exits(0);
		}
	}
}

void
rsession(void)
{
	memset(thp->authid, 0, sizeof(thp->authid));
	memset(thp->authdom, 0, sizeof(thp->authdom));
	memset(thp->chal, 0, sizeof(thp->chal));
	sendmsg(0);
}

void
rflush(void)		/* synchronous so easy */
{
	sendmsg(0);
}

void
rattach(Mfile *mf)
{
	if(mf->busy == 0){
		mf->busy = 1;
		strcpy(mf->user, rhp->uname);
	}
	mf->qid.vers = vers++;
	mf->qid.path = CHDIR;
	thp->qid = mf->qid;
	sendmsg(0);
}

void
rclone(Mfile *mf)
{
	Mfile *nmf;
	char *err=0;

	if(rhp->newfid<0){
		err = "clone nfid out of range";
		goto send;
	}
	nmf = newfid(rhp->newfid);
	if(nmf->busy){
		err = "clone to used channel";
		goto send;
	}
	*nmf = *mf;
	nmf->fid = rhp->newfid;
	nmf->qid.vers = vers++;
    send:
	sendmsg(err);
}

void
rclwalk(Mfile *mf)
{
	Mfile *nmf;

	if(rhp->newfid<0){
		sendmsg("clone nfid out of range");
		return;
	}
	nmf = newfid(rhp->newfid);
	if(nmf->busy){
		sendmsg("clone to used channel");
		return;
	}
	*nmf = *mf;
	nmf->fid = rhp->newfid;
	rhp->fid = rhp->newfid;
	nmf->qid.vers = vers++;
	if(rwalk(nmf))
		nmf->busy = 0;
}

char*
rwalk(Mfile *mf)
{
	char *err;
	char *name;

	err = 0;
	name = rhp->name;
	if((mf->qid.path & CHDIR) == 0){
		err = "not a directory";
		goto send;
	}
	if(strcmp(name, ".") == 0){
		mf->qid.path = CHDIR;
		goto send;
	}
	if(strcmp(name, "cs") == 0){
		mf->qid.path = Qcs;
		goto send;
	}
	err = "nonexistent file";
    send:
	thp->qid = mf->qid;
	sendmsg(err);
	return err;
}

void
ropen(Mfile *mf)
{
	int mode;
	char *err;

	err = 0;
	mode = rhp->mode;
	if(mf->qid.path & CHDIR){
		if(mode)
			err = "permission denied";
	}
    send:
	thp->qid = mf->qid;
	sendmsg(err);
}

void
rcreate(Mfile *mf)
{
	USED(mf);
	sendmsg("creation permission denied");
}

void
rread(Mfile *mf)
{
	int i, n, cnt;
	long off, toff, clock;
	Dir dir;
	char buf[MAXFDATA];
	char *err;

	n = 0;
	err = 0;
	off = rhp->offset;
	cnt = rhp->count;
	if(mf->qid.path & CHDIR){
		if(off%DIRLEN || cnt%DIRLEN){
			err = "bad offset";
			goto send;
		}
		clock = time(0);
		if(off == 0){
			memmove(dir.name, "cs", NAMELEN);
			dir.qid.vers = vers;
			dir.qid.path = Qcs;
			dir.mode = 0666;
			dir.length = 0;
			dir.hlength = 0;
			strcpy(dir.uid, mf->user);
			strcpy(dir.gid, mf->user);
			dir.atime = clock;	/* wrong */
			dir.mtime = clock;	/* wrong */
			convD2M(&dir, buf+n);
			n += DIRLEN;
		}
		thp->data = buf;
	} else {
		toff = 0;
		for(i = 0; mf->reply[i] && i < mf->nreply; i++){
			n = mf->replylen[i];
			if(off < toff + n)
				break;
			toff += n;
		}
		if(i >= mf->nreply){
			n = 0;
			goto send;
		}
		thp->data = mf->reply[i] + (off - toff);
		if(cnt > toff - off + n)
			n = toff - off + n;
		else
			n = cnt;
	}
send:
	thp->count = n;
	sendmsg(err);
}

void
rwrite(Mfile *mf)
{
	int cnt, n;
	char *err;
	char *field[3];
	int rv;

	err = 0;
	cnt = rhp->count;
	if(mf->qid.path & CHDIR){
		err = "can't write directory";
		goto send;
	}
	if(cnt >= Maxrequest){
		err = "request too long";
		goto send;
	}
	rhp->data[cnt] = 0;

	/*
	 *  toggle debugging
	 */
	if(strncmp(rhp->data, "debug", 5)==0){
		debug ^= 1;
		syslog(1, logfile, "debug %d", debug);
		goto send;
	}

	/*
	 *  add networks to the default list
	 */
	if(strncmp(rhp->data, "add ", 4)==0){
		if(rhp->data[cnt-1] == '\n')
			rhp->data[cnt-1] = 0;
		netadd(rhp->data+4);
		goto send;
	}

	/*
	 *  look for a general query
	 */
	if(*rhp->data == '!'){
		err = genquery(mf, rhp->data+1);
		goto send;
	}

	/*
	 *  break up name
	 */
	if(debug)
		syslog(0, logfile, "write %s", rhp->data);

	n = mygetfields(rhp->data, field, 3, '!');
	rv = -1;
	switch(n){
	case 1:
		rv = lookup(mf, "net", field[0], 0);
		break;
	case 2:
		rv = lookup(mf, field[0], field[1], 0);
		break;
	case 3:
		rv = lookup(mf, field[0], field[1], field[2]);
		break;
	}

	if(rv < 0)
		err = "can't translate address";

    send:
	thp->count = cnt;
	sendmsg(err);
}

void
rclunk(Mfile *mf)
{
	int i;

	for(i = 0; i < mf->nreply; i++)
		free(mf->reply[i]);
	mf->busy = 0;
	mf->fid = 0;
	sendmsg(0);
}

void
rremove(Mfile *mf)
{
	USED(mf);
	sendmsg("remove permission denied");
}

void
rstat(Mfile *mf)
{
	Dir dir;

	if(mf->qid.path & CHDIR){
		strcpy(dir.name, ".");
		dir.mode = CHDIR|0555;
	} else {
		strcpy(dir.name, "cs");
		dir.mode = 0666;
	}
	dir.qid = mf->qid;
	dir.length = 0;
	dir.hlength = 0;
	strcpy(dir.uid, mf->user);
	strcpy(dir.gid, mf->user);
	dir.atime = dir.mtime = time(0);
	convD2M(&dir, (char*)thp->stat);
	sendmsg(0);
}

void
rwstat(Mfile *mf)
{
	USED(mf);
	sendmsg("wstat permission denied");
}

void
sendmsg(char *err)
{
	int n;
	char mdata[MAXFDATA + MAXMSG];

	if(err){
		thp->type = Rerror;
		snprint(thp->ename, sizeof(thp->ename), "cs: %s", err);
	}else{
		thp->type = rhp->type+1;
		thp->fid = rhp->fid;
	}
	thp->tag = rhp->tag;
	n = convS2M(thp, mdata);
	if(n == 0){
		syslog(1, logfile, "sendmsg convS2M of %F returns 0", thp);
		abort();
	}
	if(write9p(mfd[1], mdata, n)!=n)
		error("mount write");
	if(debug)
		syslog(0, logfile, "%F %d", thp, n);
}

void
error(char *s)
{
	syslog(1, "cs", "%s: %r", s);
	_exits(0);
}

/*
 *  Network specific translators
 */
Ndbtuple*	iplookup(Network*, char*, char*, int);
char*		iptrans(Ndbtuple*, Network*, char*);
Ndbtuple*	dklookup(Network*, char*, char*, int);
char*		dktrans(Ndbtuple*, Network*, char*);
Ndbtuple*	telcolookup(Network*, char*, char*, int);
char*		telcotrans(Ndbtuple*, Network*, char*);
Ndbtuple*	dnsiplookup(char*, Ndbs*, char*);

struct Network
{
	char		*net;
	int		nolookup;
	Ndbtuple	*(*lookup)(Network*, char*, char*, int);
	char		*(*trans)(Ndbtuple*, Network*, char*);
	int		needproto;
	Network		*next;
	int		def;
};

Network network[] = {
	{ "il",		0,	iplookup,	iptrans,	1, },
	{ "fil",	0,	iplookup,	iptrans,	1, },
	{ "tcp",	0,	iplookup,	iptrans,	0, },
	{ "udp",	0,	iplookup,	iptrans,	0, },
	{ "dk",		1,	dklookup,	dktrans,	0, },
	{ "telco",	0,	telcolookup,	telcotrans,	0, },
	{ 0,		0, 	0,		0,		0, },
};

char	eaddr[Ndbvlen];		/* ascii ethernet address */
char	ipaddr[Ndbvlen];	/* ascii internet address */
char	dknet[Ndbvlen];		/* ascii datakit network name */
uchar	ipa[4];			/* binary internet address */
char	sysname[Ndbvlen];
int	isdk;

Network *netlist;		/* networks ordered by preference */
Network *last;

static Ndb *db;


/*
 *  get ip address and system name
 */
void
ipid(void)
{
	uchar addr[6];
	Ndbtuple *t;
	char *p, *attr;
	Ndbs s;
	int f;
	static int isether;

	/* grab ether addr from the device */
	if(isether == 0){
		if(myetheraddr(addr, "/net/ether") >= 0){
			snprint(eaddr, sizeof(eaddr), "%E", addr);
			isether = 1;
		}
	}

	/* grab ip addr from the device */
	if(*ipa == 0){
		if(myipaddr(ipa, "/net/tcp") >= 0){
			if(*ipa)
				snprint(ipaddr, sizeof(ipaddr), "%I", ipa);
		}
	}

	/* use ether addr plus db to get ipaddr */
	if(*ipa == 0 && isether){
		t = ndbgetval(db, &s, "ether", eaddr, "ip", ipaddr);
		if(t){
			ndbfree(t);
			parseip(ipa, ipaddr);
		}
	}

	/* use environment, ether addr, or ipaddr to get system name */
	if(*sysname == 0){
		/* environment has priority */
		p = getenv("sysname");
		if(p ){
			attr = ipattr(p);
			if(strcmp(attr, "ip") != 0)
				strcpy(sysname, p);
		}

		/* next use ether and ip addresses to find system name */
		if(*sysname == 0){
			t = 0;
			if(isether)
				t = ndbgetval(db, &s, "ether", eaddr, "sys", sysname);
			if(t == 0 && *ipa)
				t = ndbgetval(db, &s, "ip", ipaddr, "sys", sysname);
			if(t)
				ndbfree(t);
		}

		/* set /dev/sysname if we now know it */
		if(*sysname){
			f = open("/dev/sysname", OWRITE);
			if(f >= 0){
				write(f, sysname, strlen(sysname));
				close(f);
			}
		}
	}
}

/*
 *  set the datakit network name from a datakit network address
 */
int
setdknet(char *x)
{
	char *p;

	strncpy(dknet, x, sizeof(dknet)-2);
	p = strrchr(dknet, '/');
	if(p == 0 || p == strchr(dknet, '/')){
		*dknet = 0;
		return 0;
	}
	*++p = '*';
	*(p+1) = 0;
	return 1;
}

/*
 *  get datakit address
 */
void
dkid(void)
{
	Ndbtuple *t;
	Ndbs s;
	char dkname[Ndbvlen];
	char raddr[Ndbvlen];
	int i, f, n;
	static int isdknet;

	/* try for a datakit network name in the database */
	if(isdknet == 0){
		/* use ether and ip addresses to find dk name */
		t = 0;
		if(t == 0 && *ipa)
			t = ndbgetval(db, &s, "ip", ipaddr, "dk", dkname);
		if(t == 0 && *sysname)
			t = ndbgetval(db, &s, "sys", sysname, "dk", dkname);
		if(t){
			ndbfree(t);
			isdknet = setdknet(dkname);
		}
	}

	/* try for a datakit network name from a system we've connected to */
	if(isdknet == 0){
		for(i = 0; isdknet == 0 && i < 7; i++){
			snprint(raddr, sizeof(raddr), "/net/dk/%d/remote", i);
			f = open(raddr, OREAD);
			if(f < 0){
				isdknet = -1;
				break;
			}
			n = read(f, raddr, sizeof(raddr)-1);
			close(f);
			if(n > 0){
				raddr[n] = 0;
				isdknet = setdknet(raddr);
			}
		}
	}
	/* hack for gnots */
	if(isdknet <= 0)
		isdknet = setdknet("nj/astro/Nfs");
}

/*
 *  Set up a list of default networks by looking for
 *  /net/ * /clone.
 */
void
netinit(void)
{
	char clone[256];
	Dir d;
	Network *np;

	/* add the mounted networks to the default list */
	for(np = network; np->net; np++){
		snprint(clone, sizeof(clone), "/net/%s/clone", np->net);
		if(dirstat(clone, &d) < 0)
			continue;
		if(netlist)
			last->next = np;
		else
			netlist = np;
		last = np;
		np->next = 0;
		np->def = 1;
	}

	fmtinstall('E', eipconv);
	fmtinstall('I', eipconv);

	db = ndbopen(dbfile);
	ipid();
	dkid();

	if(debug)
		syslog(0, logfile, "sysname %s dknet %s eaddr %s ipaddr %s ipa %I\n",
			sysname, dknet, eaddr, ipaddr, ipa);
}

/*
 *  add networks to the standard list
 */
void
netadd(char *p)
{
	Network *np;
	char *field[12];
	int i, n;

	n = mygetfields(p, field, 12, ' ');
	for(i = 0; i < n; i++){
		for(np = network; np->net; np++){
			if(strcmp(field[i], np->net) != 0)
				continue;
			if(np->def)
				break;
			if(netlist)
				last->next = np;
			else
				netlist = np;
			last = np;
			np->next = 0;
			np->def = 1;
		}
	}
}

/*
 *  make a tuple
 */
Ndbtuple*
mktuple(char *attr, char *val)
{
	Ndbtuple *t;

	t = malloc(sizeof(Ndbtuple));
	strcpy(t->attr, attr);
	strncpy(t->val, val, sizeof(t->val));
	t->val[sizeof(t->val)-1] = 0;
	t->line = t;
	t->entry = 0;
	return t;
}

/*
 *  lookup a request.  the network "net" means we should pick the
 *  best network to get there.
 */
int
lookup(Mfile *mf, char *net, char *host, char *serv)
{
	Network *np, *p;
	char *cp;
	Ndbtuple *nt, *t;
	int i;
	char reply[Maxreply];

	/* start transaction with a clean slate */
	for(i = 0; i < Nreply; i++){
		if(mf->reply[i])
			free(mf->reply[i]);
		mf->reply[i] = 0;
		mf->replylen[i] = 0;
	}
	mf->nreply = 0;

	/* open up the standard db files */
	if(db == 0)
		db = ndbopen(dbfile);
	if(db == 0)
		error("can't open network database\n");

	nt = 0;
	if(strcmp(net, "net") == 0){
		/*
		 *  go through set of default nets
		 */
		for(np = netlist; np; np = np->next){
			nt = (*np->lookup)(np, host, serv, 0);
			if(nt){
				if(needproto(np, nt) == 0)
					break;
				ndbfree(nt);
				nt = 0;
			}
		}

		/*
		 *   try first net that requires no table lookup
		 */
		if(nt == 0)
			for(np = netlist; np; np = np->next){
				if(np->nolookup && *host != '$'){
					nt = (*np->lookup)(np, host, serv, 1);
					if(nt)
						break;
				}
			}

		if(nt == 0)
			return -1;

		/*
		 *  create replies
		 */
		for(p = np; p; p = p->next){
			for(t = nt; mf->nreply < Nreply && t; t = t->entry){
				if(needproto(p, nt) < 0)
					continue;
				cp = (*p->trans)(t, p, serv);
				if(cp){
					mf->replylen[mf->nreply] = strlen(cp);
					mf->reply[mf->nreply++] = cp;
				}
			}
		}
		for(p = netlist; mf->nreply < Nreply && p != np; p = p->next){
			for(t = nt; mf->nreply < Nreply && t; t = t->entry){
				if(needproto(p, nt) < 0)
					continue;
				cp = (*p->trans)(t, p, serv);
				if(cp){
					mf->replylen[mf->nreply] = strlen(cp);
					mf->reply[mf->nreply++] = cp;
				}
			}
		}
		ndbfree(nt);
		return 0;
	} else {
		/*
		 *  look on a specific network
		 */
		for(p = network; p->net; p++){
			if(strcmp(p->net, net) == 0){
				nt = (*p->lookup)(p, host, serv, 1);
				if (nt == 0)
					return -1;

				/* create replies */
				for(t = nt; mf->nreply < Nreply && t; t = t->entry){
					cp = (*p->trans)(t, p, serv);
					if(cp){
						mf->replylen[mf->nreply] = strlen(cp);
						mf->reply[mf->nreply++] = cp;
					}
				}
				ndbfree(nt);
				return 0;
			}
		}
	}

	/*
	 *  not a known network, don't translate host or service
	 */
	if(serv)
		snprint(reply, sizeof(reply), "/net/%s/clone %s!%s",
			net, host, serv);
	else
		snprint(reply, sizeof(reply), "/net/%s/clone %s",
			net, host);
	mf->reply[0] = strdup(reply);
	mf->replylen[0] = strlen(reply);
	mf->nreply = 1;
	return 0;
}

/*
 *  see if we can use this protocol
 */
int
needproto(Network *np, Ndbtuple *t)
{
	if(np->needproto == 0)
		return 0;
	for(; t; t = t->entry)
		if(strcmp(t->attr, "proto")==0 && strcmp(t->val, np->net)==0)
			return 0;
	return -1;
}

/*
 *  translate an ip service name into a port number.  If it's a numeric port
 *  number, look for restricted access.
 *
 *  the service '*' needs no translation.
 */
char*
ipserv(Network *np, char *name, char *buf)
{
	char *p;
	int alpha = 0;
	int restr = 0;
	char port[Ndbvlen];
	Ndbtuple *t, *nt;
	Ndbs s;

	/* '*' means any service */
	if(strcmp(name, "*")==0){
		strcpy(buf, name);
		return buf;
	}

	/*  see if it's numeric or symbolic */
	port[0] = 0;
	for(p = name; *p; p++){
		if(isdigit(*p))
			;
		else if(isalpha(*p) || *p == '-' || *p == '$')
			alpha = 1;
		else
			return 0;
	}
	if(alpha){
		t = ndbgetval(db, &s, np->net, name, "port", port);
		if(t == 0)
			return 0;
	} else {
		t = ndbgetval(db, &s, "port", name, "port", port);
		if(t == 0){
			strncpy(port, name, sizeof(port));
			port[sizeof(port)-1] = 0;
		}
	}

	if(t){
		for(nt = t; nt; nt = nt->entry)
			if(strcmp(nt->attr, "restricted") == 0)
				restr = 1;
		ndbfree(t);
	}
	sprint(buf, "%s%s", port, restr ? "!r" : ""); 
	return buf;
}

/*
 *  look for the value associated with this attribute for our system.
 *  the precedence is highest to lowest:
 *	- an attr/value pair in this system's entry
 *	- an attr/value pair in this system's subnet entry
 *	- an attr/value pair in this system's net entry
 */
void
ipattrlookup(char *attr, char *buf)
{
	Ndbtuple *t, *st;
	Ndbs s, ss;
	char ip[Ndbvlen+1];
	uchar net[4];
	uchar mask[4];

	*buf = 0;

	/*
	 *  look for an entry for this system
	 */
	ipid();
	if(*ipa == 0)
		return;
	t = ndbsearch(db, &s, "ip", ipaddr);
	if(t){
		/*
		 *  look for a closely bound attribute
		 */
		lookval(t, s.t, attr, buf);
		ndbfree(t);
		if(*buf)
			return;
	}

	/*
	 *  Look up the client's network and find a subnet mask for it.
	 *  Fill in from the subnet (or net) entry anything we can't figure
	 *  out from the client record.
	 */
	maskip(ipa, classmask[CLASS(ipa)], net);
	snprint(ip, sizeof(ip), "%I", net);
	t = ndbsearch(db, &s, "ip", ip);
	if(t){
		/* got a net, look for a subnet */
		if(lookval(t, s.t, "ipmask", ip)){
			parseip(mask, ip);
			maskip(ipa, mask, net);
			snprint(ip, sizeof(ip), "%I", net);
			st = ndbsearch(db, &ss, "ip", ip);
			if(st){
				lookval(st, ss.t, attr, buf);
				ndbfree(st);
			}
		}
			

		/* fill in what the client and subnet entries didn't have */
		if(*buf == 0)
			lookval(t, s.t, attr, buf);
		ndbfree(t);
	}
}

/*
 *  lookup (and translate) an ip destination
 */
Ndbtuple*
iplookup(Network *np, char *host, char *serv, int nolookup)
{
	char *attr;
	Ndbtuple *t;
	Ndbs s;
	char ts[Ndbvlen+1];
	char th[Ndbvlen+1];
	char dollar[Ndbvlen+1];

	USED(nolookup);

	/*
	 *  start with the service since it's the most likely to fail
	 *  and costs the least
	 */
	if(serv==0 || ipserv(np, serv, ts) == 0)
		return 0;

	/* for dial strings with no host */
	if(strcmp(host, "*") == 0)
		return mktuple("ip", "*");

	/*
	 *  '$' means the rest of the name is an attribute that we
	 *  need to search for
	 */
	if(*host == '$'){
		ipattrlookup(host+1, dollar);
		if(*dollar)
			host = dollar;
	}

	/*
	 *  just accept addresses
	 */
	attr = ipattr(host);
	if(strcmp(attr, "ip") == 0)
		return mktuple("ip", host);

	/*
	 *  give the domain name server the first opportunity to
	 *  resolve domain names.  if that fails try the database.
	 */
	t = 0;
	if(strcmp(attr, "dom") == 0)
		t = dnsiplookup(host, &s, th);
	if(t == 0)
		t = ndbgetval(db, &s, attr, host, "ip", th);
	if(t == 0)
		return 0;

	/*
	 *  reorder the tuple to have the matched line first and
	 *  save that in the request structure.
	 */
	return reorder(t, s.t);
}

/*
 *  translate an ip address
 */
char*
iptrans(Ndbtuple *t, Network *np, char *serv)
{
	char ts[Ndbvlen+1];
	char reply[Maxreply];

	if(strcmp(t->attr, "ip") != 0)
		return 0;

	if(serv == 0 || ipserv(np, serv, ts) == 0)
		return 0;

	if(*t->val == '*')
		snprint(reply, sizeof(reply), "/net/%s/clone %s",
			np->net, ts);
	else
		snprint(reply, sizeof(reply), "/net/%s/clone %s!%s",
			np->net, t->val, ts);

	return strdup(reply);
}

/*
 *  look for the value associated with this attribute for our system.
 *  the precedence is highest to lowest:
 *	- an attr/value pair in this system's entry
 *	- an attr/value pair in this system's net entry
 */
void
dkattrlookup(char *attr, char *buf)
{
	Ndbtuple *t;
	Ndbs s;

	*buf = 0;

	dkid();
	if(*dknet == 0)
		return;
	t = ndbsearch(db, &s, "dk", dknet);
	if(t){
		lookval(t, s.t, attr, buf);
		ndbfree(t);
	}
}

/*
 *  lookup (and translate) a datakit destination
 */
Ndbtuple*
dklookup(Network *np, char *host, char *serv, int nolookup)
{
	char *p;
	int slash = 0;
	Ndbtuple *t, *nt;
	Ndbs s;
	char th[Ndbvlen+1];
	char dollar[Ndbvlen+1];
	char *attr;

	USED(np);

	/*
	 *  '$' means the rest of the name is an attribute that we
	 *  need to search for
	 */
	if(*host == '$'){
		dkattrlookup(host+1, dollar);
		if(*dollar)
			host = dollar;
	}

	for(p = host; *p; p++){
		if(isalnum(*p) || *p == '-' || *p == '.')
			;
		else if(*p == '/')
			slash = 1;
		else
			return 0;
	}

	/* hack for announcements */
	if(nolookup && serv == 0)
		return mktuple("dk", host);

	/* let dk addresses be domain names */
	attr = ipattr(host);

	/* don't translate paths, just believe the user */
	if(slash)
		return mktuple("dk", host);

	t = ndbgetval(db, &s, attr, host, "dk", th);
	if(t == 0){
		if(nolookup)
			return mktuple("dk", host);
		return 0;
	}

	/* don't allow services in calls to consoles */
	for(nt = t; nt; nt = nt->entry)
		if(strcmp("flavor", nt->attr)==0
		&& strcmp("console", nt->val)==0
		&& serv && *serv){
			ndbfree(t);
			return 0;
		}

	return reorder(t, s.t);
}

/*
 *  translate a datakit address
 */
char*
dktrans(Ndbtuple *t, Network *np, char *serv)
{
	char reply[Maxreply];

	if(strcmp(t->attr, "dk") != 0)
		return 0;

	if(serv)
		snprint(reply, sizeof(reply), "/net/%s/clone %s!%s", np->net,
			t->val, serv);
	else
		snprint(reply, sizeof(reply), "/net/%s/clone %s", np->net,
			t->val);
	return strdup(reply);
}

/*
 *  lookup a telephone number
 */
Ndbtuple*
telcolookup(Network *np, char *host, char *serv, int nolookup)
{
	Ndbtuple *t;
	Ndbs s;
	char th[Ndbvlen+1];

	USED(np, nolookup, serv);

	t = ndbgetval(db, &s, "sys", host, "telco", th);
	if(t == 0)
		return mktuple("telco", host);

	return reorder(t, s.t);
}

/*
 *  translate a telephone address
 */
char*
telcotrans(Ndbtuple *t, Network *np, char *serv)
{
	char reply[Maxreply];

	if(strcmp(t->attr, "telco") != 0)
		return 0;

	if(serv)
		snprint(reply, sizeof(reply), "/net/%s/clone %s!%s", np->net,
			t->val, serv);
	else
		snprint(reply, sizeof(reply), "/net/%s/clone %s", np->net,
			t->val);
	return strdup(reply);
}
int
mygetfields(char *lp, char **fields, int n, char sep)
{
	int i;
	char sep2=0;

	if(sep == ' ')
		sep2 = '\t';
	for(i=0; lp && *lp && i<n; i++){
		if(*lp==sep || *lp==sep2)
			*lp++ = 0;
		if(*lp == 0)
			break;
		fields[i] = lp;
		while(*lp && *lp!=sep && *lp!=sep2)
			lp++;
	}
	return i;
}

/*
 *  Look for a pair with the given attribute.  look first on the same line,
 *  then in the whole entry.
 */
Ndbtuple*
lookval(Ndbtuple *entry, Ndbtuple *line, char *attr, char *to)
{
	Ndbtuple *nt;

	/* first look on same line (closer binding) */
	for(nt = line;;){
		if(strcmp(attr, nt->attr) == 0){
			strncpy(to, nt->val, Ndbvlen);
			return nt;
		}
		nt = nt->line;
		if(nt == line)
			break;
	}
	/* search whole tuple */
	for(nt = entry; nt; nt = nt->entry)
		if(strcmp(attr, nt->attr) == 0){
			strncpy(to, nt->val, Ndbvlen);
			return nt;
		}
	return 0;
}

/*
 *  reorder the tuple to put x's line first in the entry
 */
Ndbtuple*
reorder(Ndbtuple *t, Ndbtuple *x)
{
	Ndbtuple *nt;
	Ndbtuple *line;

	/* find start of this entry's line */
	for(line = x; line->entry == line->line; line = line->line)
		;
	line = line->line;
	if(line == t)
		return t;	/* already the first line */

	/* remove this line and everything after it from the entry */
	for(nt = t; nt->entry != line; nt = nt->entry)
		;
	nt->entry = 0;

	/* make that the start of the entry */
	for(nt = line; nt->entry; nt = nt->entry)
		;
	nt->entry = t;
	return line;
}

/*
 *  create a slave process to handle a request to avoid one request blocking
 *  another
 */
void
slave(void)
{
	if(*isslave)
		return;		/* we're already a slave process */

	switch(rfork(RFPROC|RFNOTEG|RFMEM|RFNOWAIT)){
	case -1:
		break;
	case 0:
		if(debug)
			syslog(0, logfile, "slave %d", getpid());
		*isslave = 1;
		break;
	default:
		longjmp(masterjmp, 1);
	}
}

int
dnsmount(void)
{
	int fd;

	fd = open("#s/dns", ORDWR);
	if(fd < 0)
		return -1;
	if(mount(fd, "/net", MAFTER, "") < 0){
		close(fd);
		return -1;
	}
	close(fd);
	return 0;
}

/*
 *  call the dns process and have it try to translate a name
 */
Ndbtuple*
dnsiplookup(char *host, Ndbs *s, char *ht)
{
	int fd, n;
	char buf[Ndbvlen + 4];
	Ndbtuple *t, *nt, **le, **ll;
	char *fields[4];

	unlock(&dblock);

	/* save the name before starting a slave */
	snprint(buf, sizeof(buf), "%s ip", host);

	slave();

	fd = open("/net/dns", ORDWR);
	if(fd < 0 && dnsmount() == 0)
		fd = open("/net/dns", ORDWR);
	if(fd < 0){
		lock(&dblock);
		return 0;
	}

	t = 0;
	ll = le = 0;
	if(write(fd, buf, strlen(buf)) >= 0){
		seek(fd, 0, 0);
		ll = &t;
		le = &t;
		while((n = read(fd, buf, sizeof(buf)-1)) > 0){
			buf[n] = 0;
			n = mygetfields(buf, fields, 4, ' ');
			if(n < 3)
				continue;
			nt = malloc(sizeof(Ndbtuple));
			strcpy(nt->attr, "ip");
			strncpy(nt->val, fields[2], Ndbvlen-1);
			*ll = nt;
			*le = nt;
			ll = &nt->line;
			le = &nt->entry;
			nt->line = t;
		}
	}
	if(t){
		strcpy(ht, t->val);

		/* add in domain name */
		nt = malloc(sizeof(Ndbtuple));
		strcpy(nt->attr, "dom");
		strcpy(nt->val, host);
		*ll = nt;
		*le = nt;
		nt->line = t;
	}
	close(fd);
	s->t = t;
	lock(&dblock);
	return t;
}

int
qmatch(Ndbtuple *t, char **attr, char **val, int n)
{
	int i, found;
	Ndbtuple *nt;

	for(i = 1; i < n; i++){
		found = 0;
		for(nt = t; nt; nt = nt->entry)
			if(strcmp(attr[i], nt->attr) == 0)
				if(strcmp(val[i], "*") == 0
				|| strcmp(val[i], nt->val) == 0){
					found = 1;
					break;
				}
		if(found == 0)
			break;
	}
	return i == n;
}

void
qreply(Mfile *mf, Ndbtuple *t)
{
	int i;
	Ndbtuple *nt;
	char buf[512];

	buf[0] = 0;
	for(nt = t; mf->nreply < Nreply && nt; nt = nt->entry){
		strcat(buf, nt->attr);
		strcat(buf, "=");
		strcat(buf, nt->val);
		i = strlen(buf);
		if(nt->line != nt->entry || sizeof(buf) - i < 2*Ndbvlen+2){
			mf->replylen[mf->nreply] = strlen(buf);
			mf->reply[mf->nreply++] = strdup(buf);
			buf[0] = 0;
		} else
			strcat(buf, " ");
	}
}

/*
 *  generic query lookup.
 */
char*
genquery(Mfile *mf, char *query)
{
	int i, n;
	char *p;
	char *attr[32];
	char *val[32];
	char ip[Ndbvlen];
	Ndbtuple *t;
	Ndbs s;

	n = mygetfields(query, attr, 32, ' ');
	if(n == 0)
		return "bad query";

	/* parse pairs */
	for(i = 0; i < n; i++){
		p = strchr(attr[i], '=');
		if(p == 0)
			return "bad query";
		*p++ = 0;
		val[i] = p;
	}

	/* give dns a chance */
	if((strcmp(attr[0], "dom") == 0 || strcmp(attr[0], "ip") == 0) && val[0]){
		t = dnsiplookup(val[0], &s, ip);
		if(t){
			if(qmatch(t, attr, val, n)){
				qreply(mf, t);
				ndbfree(t);
				return 0;
			}
			ndbfree(t);
		}
	}

	/* first pair is always the key.  It can't be a '*' */
	t = ndbsearch(db, &s, attr[0], val[0]);

	/* search is the and of all the pairs */
	while(t){
		if(qmatch(t, attr, val, n)){
			qreply(mf, t);
			ndbfree(t);
			return 0;
		}

		ndbfree(t);
		t = ndbsnext(&s, attr[0], val[0]);
	}

	return "no match";
}
