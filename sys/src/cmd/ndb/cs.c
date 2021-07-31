#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <bio.h>
#include <ctype.h>
#include <ndb.h>
#include <ip.h>
#include "lock.h"

enum
{
	Maxreply=		256,
	Maxrequest=		4*NAMELEN,
	Ncache=			8,

	Qcs=			1,
};

typedef struct Mfile	Mfile;
typedef struct Network	Network;

int vers;		/* incremented each clone/attach */

struct Mfile
{
	int		busy;
	char		user[NAMELEN];
	Qid		qid;
	int		fid;

	Ndbtuple	*t;		/* last entry found */
	Ndbtuple	*nt;		/* current attr/value pair in entry */
	int		off;		/* file offset of start of reply */
	Network		*np;		/* network requested */
	char		serv[Ndbvlen];	/* service requested */
	char		reply[Maxreply];
};

Fcall	*rhp;
Fcall	*thp;
Mfile	*mfile;
int	nmfile = 0;
int	mfd[2];
char	user[NAMELEN];
int	debug;
jmp_buf	masterjmp;	/* return through here after a slave process has been created */
int	*isslave;	/* *isslave non-zero means this is a slave process */
char	*dbfile;

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
void	mountinit(char*);
void	io(void);
void	netinit(void);
void	netadd(char*);
int	lookup(Mfile*, char*, char*, char*);
int	fillreply(Mfile*, int);
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
	[Tauth]		"Tauth",
			0,
};

Lock	dblock;		/* mutex on database operations */

void
main(int argc, char *argv[])
{
	Fcall	rhdr;
	Fcall	thdr;

	paralloc();
	rhp = &rhdr;
	thp = &thdr;
	ARGBEGIN{
	case 'd':
		debug = 1;
		break;
	case 'f':
		dbfile = ARGF();
		break;
	}ARGEND
	USED(argc);
	USED(argv);

	/* Clean up */
	remove("/srv/cs");
	unmount("/net/cs", "/net");
	netinit();

	if(debug)
		mountinit("#s/dcs");
	else
		mountinit("#s/cs");

	switch(fork()){
	case 0:
		io();
		exits("");
	case -1:
		error("fork");
	default:
		exits("");
	}
}

void
mountinit(char *service)
{
	int f;
	int p[2];
	char buf[32];

	if(pipe(p) < 0)
		error("pipe failed");
	switch(fork()){
	case 0:
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
			error("write #s/cs");
		close(f);

		/*
		 *  put ourselves into the file system
		 */
		if(mount(p[1], "/net", MBEFORE, "", "") < 0)
			error("mount failed\n");
		exits(0);
/*BUG: no wait!*/
	}
	mfd[0] = mfd[1] = p[0];
}

#define INC 4
Mfile*
newfid(int fid)
{
	Mfile *mf;
	Mfile *freemf;
	int old;
	Mfile *o;

	freemf = 0;
	for(mf = mfile; mf < &mfile[nmfile]; mf++){
		if(mf->fid==fid)
			return mf;
		if(mf->busy == 0)
			freemf = mf;
	}
	if(freemf == 0){
		old = nmfile;
		nmfile += INC;
		o = mfile;
		mfile = realloc(mfile, nmfile*sizeof(Mfile));
		if(mfile == 0){
			fprint(2, "cs: realloc(0x%lux, %d)\n", o, nmfile*sizeof(Mfile));
			error("realloc failure");
		}
		mf = &mfile[old];
		memset(mf, 0, INC*sizeof(Mfile));
	} else
		mf = freemf;

	memset(mf, 0, sizeof mfile[0]);
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
	lockinit(&dblock);
	slaveflag = 0;		/* init slave variable */
	isslave = &slaveflag;
	setjmp(masterjmp);

	for(;;){
		n = read(mfd[0], mdata, sizeof mdata);
		if(n<=0)
			error("mount read");
		if(convM2S(mdata, rhp, n) == 0){
			fprint(2, "format error\n");
			continue;
		}
		if(rhp->fid<0)
			error("fid out of range");
		lock(&dblock);
		mf = newfid(rhp->fid);
/*		if(debug)
			fprint(2, "msg: %d %s on %d mf = %lux\n", rhp->type,
				mname[rhp->type]? mname[rhp->type] : "mystery",
				rhp->fid, mf);/**/
	
	
		switch(rhp->type){
		default:
			syslog(1, "cs", "unknown request type %d", rhp->type);
			break;
		case Tsession:
			rflush();
			break;
		case Tnop:
			rflush();
			break;
		case Tflush:
			rflush();
			break;
		case Tauth:
			rauth();
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
		if(*isslave)
			_exits(0);
	}
}

void
rflush(void)		/* synchronous so easy */
{
	sendmsg(0);
}

void
rauth(void)
{
	sendmsg("Authentication failed");
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
	int n, cnt, end;
	long off, clock;
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
		if(fillreply(mf, off) == 0){
			end = mf->off + strlen(mf->reply);
			n = cnt;
			if(off+n > end)
				n = end - off;
			thp->data = mf->reply + (off - mf->off);
		}
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
	 *  add networks to the default list
	 */
	if(strncmp(rhp->data, "add ", 4)==0){
		if(rhp->data[cnt-1] == '\n')
			rhp->data[cnt-1] = 0;
		netadd(rhp->data+4);
		goto send;
	}

	/*
	 *  break up name
	 */
	if(debug)
		fprint(2, "%s ->", rhp->data);

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

	if(debug)
		fprint(2, "%s\n", rv==0 ? mf->reply : err);

    send:
	thp->count = cnt;
	sendmsg(err);
}

void
rclunk(Mfile *mf)
{
	if(mf->t){
		ndbfree(mf->t);
		mf->t = 0;
	}
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

	memmove(dir.name, "cs", NAMELEN);
	dir.qid = mf->qid;
	dir.mode = 0666;
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
	if(write(mfd[1], mdata, n)!=n)
		error("mount write");
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
int		iplookup(Mfile*, Network*, char*, char*);
int		iptrans(Mfile*, Network*);
int		dklookup(Mfile*, Network*, char*, char*);
int		dktrans(Mfile*, Network*);
Ndbtuple*	dnsiplookup(char*, Ndbs*, char*);

struct Network
{
	char	*net;
	int	nolookup;
	int	(*lookup)(Mfile*, Network*, char*, char*);
	int	(*trans)(Mfile*, Network*);
	int	needproto;
	Network	*next;
	int	def;
};

Network network[] = {
	{ "il",		0,	iplookup,	iptrans,	1, },
	{ "tcp",	0,	iplookup,	iptrans,	0, },
	{ "udp",	0,	iplookup,	iptrans,	0, },
	{ "dk",		1,	dklookup,	dktrans,	0, },
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

	/* use ether addr or ipaddr to get system name */
	if(*sysname == 0){
		/* use ether and ip addresses to find system name */
		t = 0;
		if(isether)
			t = ndbgetval(db, &s, "ether", eaddr, "sys", sysname);
		if(t == 0 && *ipa)
			t = ndbgetval(db, &s, "ip", ipaddr, "sys", sysname);

		/* set /dev/sysname if we now know it */
		if(t){
			ndbfree(t);
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

	if(debug)/**/
		fprint(2, "cs: sysname %s dknet %s eaddr %s ipaddr %s ipa %I\n",
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
 *  lookup a request.  the network "net" means we should pick the
 *  best network to get there.
 */
int
lookup(Mfile *mf, char *net, char *host, char *serv)
{
	Network *np;

	/* start transaction with a clean slate */
	if(mf->t){
		ndbfree(mf->t);
		mf->t = 0;
	}
	mf->nt = 0;
	mf->off = 0;
	if(serv)
		strncpy(mf->serv, serv, Ndbvlen);
	else
		mf->serv[0] = 0;
	mf->serv[Ndbvlen-1] = 0;

	/* open up the standard db files */
	if(db == 0)
		db = ndbopen(dbfile);
	if(db == 0)
		error("can't open network database\n");

	if(strcmp(net, "net") == 0){
		/*
		 *  go through set of default nets
		 */
		for(np = netlist; np; np = np->next){
			if((*np->lookup)(mf, np, host, serv) == 0){
				if(needproto(np, mf->t) == 0)
					return 0;
				ndbfree(mf->t);
				mf->t = 0;
			}
		}

		/*
		 *   try first net that requires no table lookup
		 */
		for(np = netlist; np; np = np->next){
			if(np->nolookup && *host != '$'){
				net = np->net;
				break;
			}
		}

		/*
		 *  if no match, give up
		 */
		if(np == 0)
			return -1;
	} else {
		/*
		 *  look on a specific network
		 */
		for(np = network; np->net; np++){
			if(strcmp(np->net, net) == 0){
				/* don't translate if we don't have to */
				mf->np = np;
				if(np->nolookup && *host != '$')
					break;
				return (*np->lookup)(mf, np, host, serv);
			}
		}
	}

	/*
	 *  don't translate host or service
	 */
	if(serv)
		snprint(mf->reply, sizeof(mf->reply), "/net/%s/clone %s!%s",
			net, host, serv);
	else
		snprint(mf->reply, sizeof(mf->reply), "/net/%s/clone %s",
			net, host);
	return 0;
}

/*
 *  get the reply that covers off
 */
int
fillreply(Mfile *mf, int off)
{
	Network *np;
	int len;

	len = strlen(mf->reply);
	if(off >= mf->off && off < mf->off + len)
		return 0;

	if(off < mf->off){
		/* back to the beginning */
		mf->off = 0;
		mf->nt = mf->t;
	} else {
		/* skip to next entry */
		mf->off += len;
		if(mf->nt)
			mf->nt = mf->nt->entry;
	}

	/* advance through entries till the offset is right */
	for(; mf->nt; mf->nt = mf->nt->entry){
		if(mf->np){
			/* a specific net was asked for */
			if((*mf->np->trans)(mf, mf->np) < 0)
				continue;
		} else {
			/* a generic search through the default nets */
			for(np = netlist; np; np = np->next)
				if((*np->trans)(mf, np) == 0)
					break;
			if(np == 0)
				continue;
		}
		len = strlen(mf->reply);
		if(off >= mf->off && off < mf->off + len)
			break;
		mf->off += len;
	}

	return mf->nt ? 0 : -1;
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
	if(t == 0)
		return;

	/*
	 *  look for a closely bound attribute
	 */
	lookval(t, s.t, attr, buf);
	ndbfree(t);
	if(*buf)
		return;

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
int
iplookup(Mfile *mf, Network *np, char *host, char *serv)
{
	char *attr;
	Ndbtuple *t;
	Ndbs s;
	char ts[Ndbvlen+1];
	char th[Ndbvlen+1];
	char dollar[Ndbvlen+1];

	/*
	 *  start with the service since it's the most likely to fail
	 *  and costs the least
	 */
	if(serv==0 || ipserv(np, serv, ts) == 0)
		return -1;

	/* for dial strings with no host */
	if(strcmp(host, "*")==0){
		snprint(mf->reply, sizeof(mf->reply), "/net/%s/clone %s", np->net, ts);
		return 0;
	}

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
	 *  figure out what kind of name we have
	 */
	attr = ipattr(host);

	/*
	 *  just accept addresses
	 */
	if(strcmp(attr, "ip") == 0){
		if(serv)
			snprint(mf->reply, sizeof(mf->reply), "/net/%s/clone %s!%s",
				np->net, host, ts);
		else
			snprint(mf->reply, sizeof(mf->reply), "/net/%s/clone %s",
				np->net, host);
		return 0;
	}

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
		return -1;

	/*
	 *  reorder the tuple to have the matched line first and
	 *  save that in the request structure.
	 */
	mf->t = reorder(t, s.t);
	for(mf->nt = mf->t; strcmp(mf->nt->attr, "ip") != 0; mf->nt = mf->nt->entry)
		;

	/*
	 *  manufacture the first reply
	 */
	if(serv)
		snprint(mf->reply, sizeof(mf->reply), "/net/%s/clone %s!%s",
			np->net, th, ts);
	else
		snprint(mf->reply, sizeof(mf->reply), "/net/%s/clone %s",
			np->net, th);
	return 0;
}

/*
 *  translate an ip address
 */
int
iptrans(Mfile *mf, Network *np)
{
	char ts[Ndbvlen+1];

	if(strcmp(mf->nt->attr, "ip") != 0)
		return -1;

	if(mf->serv[0] == 0 || ipserv(np, mf->serv, ts) == 0)
		return -1;

	snprint(mf->reply, sizeof(mf->reply), "/net/%s/clone %s!%s",
		np->net, mf->nt->val, ts);
	return 0;
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
int
dklookup(Mfile *mf, Network *np, char *host, char *serv)
{
	char *p;
	int i, slash = 0;
	Ndbtuple *t, *nt;
	Ndbs s;
	char th[Ndbvlen+1];
	char dollar[Ndbvlen+1];

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
		if(isalnum(*p) || *p == '-')
			;
		else if(*p == '/')
			slash = 1;
		else
			return 0;
	}

	if(slash){
		/* don't translate paths, just believe the user */
		i = snprint(mf->reply, sizeof(mf->reply), "/net/%s/clone %s",
			np->net, host);
	} else {
		t = ndbgetval(db, &s, "sys", host, "dk", th);
		if(t == 0)
			return -1;

		/* don't allow services in calls to consoles */
		for(nt = t; nt; nt = nt->entry)
			if(strcmp("flavor", nt->attr)==0
			&& strcmp("console", nt->val)==0
			&& serv && *serv){
				ndbfree(t);
				mf->reply[0] = 0;
				return 0;
			}

		mf->t = reorder(t, s.t);
		for(mf->nt = mf->t; strcmp(mf->nt->attr, "dk") != 0;
		    mf->nt = mf->nt->entry)
			;
		i = snprint(mf->reply, sizeof(mf->reply), "/net/%s/clone %s",
			np->net, th);
	}

	if(serv)
		snprint(mf->reply + i, sizeof(mf->reply) - i, "!%s", serv);

	return 0;
}

/*
 *  translate a datakit address
 */
int
dktrans(Mfile *mf, Network *np)
{
	if(strcmp(mf->nt->attr, "dk") != 0)
		return -1;

	if(mf->serv[0])
		snprint(mf->reply, sizeof(mf->reply), "/net/%s/clone %s!%s", np->net,
			mf->nt->val, mf->serv);
	else
		snprint(mf->reply, sizeof(mf->reply), "/net/%s/clone %s", np->net,
			mf->nt->val);
	return 0;
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
	Ndbtuple **ll;

	/* find the start of the line and the end of the previous line */
	line = t;
	ll = 0;
	for(nt = t; nt != x; nt = nt->entry){
		if(nt->line != nt->entry){
			line = nt->entry;
			ll = &nt->entry;
		}
	}
	if(ll == 0)
		return t;	/* already first line */

	/* find the end of this line */
	for(; nt->line == nt->entry; nt = nt->line)
		;

	/* unchain line */
	*ll = nt->entry;

	/* rechain at front */
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
		*isslave = 1;
		break;
	default:
		longjmp(masterjmp, 1);
	}
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
	if(fd < 0){
		lock(&dblock);
		return 0;
	}

	t = 0;
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
	if(t)
		strcpy(ht, t->val);
	close(fd);
	s->t = t;
	lock(&dblock);
	return t;
}
