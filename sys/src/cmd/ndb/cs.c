#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include <bio.h>
#include <ctype.h>
#include <ndb.h>
#include <ip.h>
#include <String.h>

enum
{
	Nreply=			20,
	Maxreply=		256,
	Maxrequest=		128,
	Maxpath=		128,
	Maxfdata=		8192,
	Maxhost=		64,		/* maximum host name size */
	Maxservice=		64,		/* maximum service name size */

	Qdir=			0,
	Qcs=			1,
};

typedef struct Mfile	Mfile;
typedef struct Mlist	Mlist;
typedef struct Network	Network;
typedef struct Flushreq	Flushreq;
typedef struct Job	Job;

int vers;		/* incremented each clone/attach */

struct Mfile
{
	int		busy;

	char		*user;
	Qid		qid;
	int		fid;

	/*
	 *  current request
	 */
	char		*net;
	char		*host;
	char		*serv;
	char		*rem;

	/*
	 *  result of the last lookup
	 */
	Network		*nextnet;
	int		nreply;
	char		*reply[Nreply];
	int		replylen[Nreply];
};

struct Mlist
{
	Mlist	*next;
	Mfile	mf;
};


//
//  active requests
//
struct Job
{
	Job	*next;
	int	flushed;
	Fcall	request;
	Fcall	reply;
};
Lock	joblock;
Job	*joblist;

Mlist	*mlist;
int	mfd[2];
int	debug;
int	paranoia;
jmp_buf	masterjmp;	/* return through here after a slave process has been created */
int	*isslave;	/* *isslave non-zero means this is a slave process */
char	*dbfile;
Ndb	*db, *netdb;

void	rversion(Job*);
void	rflush(Job*);
void	rattach(Job*, Mfile*);
char*	rwalk(Job*, Mfile*);
void	ropen(Job*, Mfile*);
void	rcreate(Job*, Mfile*);
void	rread(Job*, Mfile*);
void	rwrite(Job*, Mfile*);
void	rclunk(Job*, Mfile*);
void	rremove(Job*, Mfile*);
void	rstat(Job*, Mfile*);
void	rwstat(Job*, Mfile*);
void	rauth(Job*);
void	sendmsg(Job*, char*);
void	error(char*);
void	mountinit(char*, char*);
void	io(void);
void	ndbinit(void);
void	netinit(int);
void	netadd(char*);
char	*genquery(Mfile*, char*);
char*	ipinfoquery(Mfile*, char**, int);
int	needproto(Network*, Ndbtuple*);
int	lookup(Mfile*);
Ndbtuple*	reorder(Ndbtuple*, Ndbtuple*);
void	ipid(void);
void	readipinterfaces(void);
void*	emalloc(int);
char*	estrdup(char*);
Job*	newjob(void);
void	freejob(Job*);
void	setext(char*, int, char*);
void	cleanmf(Mfile*);

extern void	paralloc(void);

Lock	dblock;		/* mutex on database operations */
Lock	netlock;	/* mutex for netinit() */

char	*logfile = "cs";
char	*paranoiafile = "cs.paranoia";

char	mntpt[Maxpath];
char	netndb[Maxpath];

/*
 *  Network specific translators
 */
Ndbtuple*	iplookup(Network*, char*, char*, int);
char*		iptrans(Ndbtuple*, Network*, char*, char*, int);
Ndbtuple*	telcolookup(Network*, char*, char*, int);
char*		telcotrans(Ndbtuple*, Network*, char*, char*, int);
Ndbtuple*	dnsiplookup(char*, Ndbs*);

struct Network
{
	char		*net;
	Ndbtuple	*(*lookup)(Network*, char*, char*, int);
	char		*(*trans)(Ndbtuple*, Network*, char*, char*, int);
	int		considered;
	int		fasttimeouthack;
	Network		*next;
};

enum
{
	Nilfast,
	Ntcp,
	Nil,
	Nudp,
	Nicmp,
	Nicmpv6,
	Nrudp,
	Ntelco,
};

/*
>  *  net doesn't apply to (r)udp, icmp(v6), or telco (for speed)
 */
Network network[] = {
[Nilfast]	{ "il",		iplookup,	iptrans,	0, 1 },
[Ntcp]		{ "tcp",	iplookup,	iptrans,	0, 0 },
[Nil]		{ "il",		iplookup,	iptrans,	0, 0 },
[Nudp]		{ "udp",	iplookup,	iptrans,	1, 0 },
[Nicmp]		{ "icmp",	iplookup,	iptrans,	1, 0 },
[Nicmpv6]	{ "icmpv6",	iplookup,	iptrans,	1, 0 },
[Nrudp]		{ "rudp",	iplookup,	iptrans,	1, 0 },
[Ntelco]	{ "telco",	telcolookup,	telcotrans,	1, 0 },
		{ 0 },
};

Lock ipifclock;
Ipifc *ipifcs;

char	eaddr[16];		/* ascii ethernet address */
char	ipaddr[64];		/* ascii internet address */
uchar	ipa[IPaddrlen];		/* binary internet address */
char	*mysysname;

Network *netlist;		/* networks ordered by preference */
Network *last;

static void
nstrcpy(char *to, char *from, int len)
{
	strncpy(to, from, len);
	to[len-1] = 0;
}


void
usage(void)
{
	fprint(2, "usage: %s [-d] [-f ndb-file] [-x netmtpt] [-n]\n", argv0);
	exits("usage");
}

void
main(int argc, char *argv[])
{
	char servefile[Maxpath];
	int justsetname;
	char *p;
	char ext[Maxpath];

	justsetname = 0;
	setnetmtpt(mntpt, sizeof(mntpt), nil);
	ext[0] = 0;
	ARGBEGIN{
	case 'd':
		debug = 1;
		break;
	case 'f':
		p = ARGF();
		if(p == nil)
			usage();
		dbfile = p;
		break;
	case 'x':
		p = ARGF();
		if(p == nil)
			usage();
		setnetmtpt(mntpt, sizeof(mntpt), p);
		setext(ext, sizeof(ext), mntpt);
		break;
	case 'n':
		justsetname = 1;
		break;
	}ARGEND
	USED(argc);
	USED(argv);

	rfork(RFREND|RFNOTEG);

	snprint(servefile, sizeof(servefile), "#s/cs%s", ext);
	snprint(netndb, sizeof(netndb), "%s/ndb", mntpt);
	unmount(servefile, mntpt);
	remove(servefile);

	fmtinstall('E', eipfmt);
	fmtinstall('I', eipfmt);
	fmtinstall('M', eipfmt);
	fmtinstall('F', fcallfmt);

	ndbinit();
	netinit(0);

	if(!justsetname){
		mountinit(servefile, mntpt);
		io();
	}
	exits(0);
}

/*
 *  if a mount point is specified, set the cs extention to be the mount point
 *  with '_'s replacing '/'s
 */
void
setext(char *ext, int n, char *p)
{
	int i, c;

	n--;
	for(i = 0; i < n; i++){
		c = p[i];
		if(c == 0)
			break;
		if(c == '/')
			c = '_';
		ext[i] = c;
	}
	ext[i] = 0;
}

void
mountinit(char *service, char *mntpt)
{
	int f;
	int p[2];
	char buf[32];

	if(pipe(p) < 0)
		error("pipe failed");

	/*
	 *  make a /srv/cs
	 */
	f = create(service, OWRITE|ORCLOSE, 0666);
	if(f < 0)
		error(service);
	snprint(buf, sizeof(buf), "%d", p[1]);
	if(write(f, buf, strlen(buf)) != strlen(buf))
		error("write /srv/cs");

	switch(rfork(RFFDG|RFPROC|RFNAMEG)){
	case 0:
		close(p[1]);
		break;
	case -1:
		error("fork failed\n");
	default:
		/*
		 *  put ourselves into the file system
		 */
		close(p[0]);
		if(mount(p[1], -1, mntpt, MAFTER, "") < 0)
			error("mount failed\n");
		_exits(0);
	}
	mfd[0] = mfd[1] = p[0];
}

void
ndbinit(void)
{
	db = ndbopen(dbfile);
	if(db == nil)
		error("can't open network database");

	netdb = ndbopen(netndb);
	if(netdb != nil){
		netdb->nohash = 1;
		db = ndbcat(netdb, db);
	}
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
		ff = emalloc(sizeof *f);
		ff->next = mlist;
		mlist = ff;
	}
	mf = &ff->mf;
	memset(mf, 0, sizeof *mf);
	mf->fid = fid;
	return mf;
}

Job*
newjob(void)
{
	Job *job;

	job = mallocz(sizeof(Job), 1);
	lock(&joblock);
	job->next = joblist;
	joblist = job;
	job->request.tag = -1;
	unlock(&joblock);
	return job;
}

void
freejob(Job *job)
{
	Job **l;

	lock(&joblock);
	for(l = &joblist; *l; l = &(*l)->next){
		if((*l) == job){
			*l = job->next;
			free(job);
			break;
		}
	}
	unlock(&joblock);
}

void
flushjob(int tag)
{
	Job *job;

	lock(&joblock);
	for(job = joblist; job; job = job->next){
		if(job->request.tag == tag && job->request.type != Tflush){
			job->flushed = 1;
			break;
		}
	}
	unlock(&joblock);
}

void
io(void)
{
	long n;
	Mfile *mf;
	int slaveflag;
	uchar mdata[IOHDRSZ + Maxfdata];
	Job *job;

	/*
	 *  if we ask dns to fulfill requests,
	 *  a slave process is created to wait for replies.  The
	 *  master process returns immediately via a longjmp
	 *  through 'masterjmp'.
	 *
	 *  *isslave is a pointer into the call stack to a variable
	 *  that tells whether or not the current process is a slave.
	 */
	slaveflag = 0;		/* init slave variable */
	isslave = &slaveflag;
	setjmp(masterjmp);

	for(;;){
		n = read9pmsg(mfd[0], mdata, sizeof mdata);
		if(n<=0)
			error("mount read");
		job = newjob();
		if(convM2S(mdata, n, &job->request) != n){
			syslog(1, logfile, "format error %ux %ux %ux %ux %ux",
				mdata[0], mdata[1], mdata[2], mdata[3], mdata[4]);
			freejob(job);
			continue;
		}
		lock(&dblock);
		mf = newfid(job->request.fid);
		if(debug)
			syslog(0, logfile, "%F", &job->request);
	
	
		switch(job->request.type){
		default:
			syslog(1, logfile, "unknown request type %d", job->request.type);
			break;
		case Tversion:
			rversion(job);
			break;
		case Tauth:
			rauth(job);
			break;
		case Tflush:
			rflush(job);
			break;
		case Tattach:
			rattach(job, mf);
			break;
		case Twalk:
			rwalk(job, mf);
			break;
		case Topen:
			ropen(job, mf);
			break;
		case Tcreate:
			rcreate(job, mf);
			break;
		case Tread:
			rread(job, mf);
			break;
		case Twrite:
			rwrite(job, mf);
			break;
		case Tclunk:
			rclunk(job, mf);
			break;
		case Tremove:
			rremove(job, mf);
			break;
		case Tstat:
			rstat(job, mf);
			break;
		case Twstat:
			rwstat(job, mf);
			break;
		}
		unlock(&dblock);

		freejob(job);

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
rversion(Job *job)
{
	if(job->request.msize > IOHDRSZ + Maxfdata)
		job->reply.msize = IOHDRSZ + Maxfdata;
	else
		job->reply.msize = job->request.msize;
	if(strncmp(job->request.version, "9P2000", 6) != 0)
		sendmsg(job, "unknown 9P version");
	else{
		job->reply.version = "9P2000";
		sendmsg(job, 0);
	}
}

void
rauth(Job *job)
{
	sendmsg(job, "cs: authentication not required");
}

/*
 *  don't flush till all the slaves are done
 */
void
rflush(Job *job)
{
	flushjob(job->request.oldtag);
	sendmsg(job, 0);
}

void
rattach(Job *job, Mfile *mf)
{
	if(mf->busy == 0){
		mf->busy = 1;
		mf->user = estrdup(job->request.uname);
	}
	mf->qid.vers = vers++;
	mf->qid.type = QTDIR;
	mf->qid.path = 0LL;
	job->reply.qid = mf->qid;
	sendmsg(job, 0);
}


char*
rwalk(Job *job, Mfile *mf)
{
	char *err;
	char **elems;
	int nelems;
	int i;
	Mfile *nmf;
	Qid qid;

	err = 0;
	nmf = nil;
	elems = job->request.wname;
	nelems = job->request.nwname;
	job->reply.nwqid = 0;

	if(job->request.newfid != job->request.fid){
		/* clone fid */
		nmf = newfid(job->request.newfid);
		if(nmf->busy){
			nmf = nil;
			err = "clone to used channel";
			goto send;
		}
		*nmf = *mf;
		nmf->user = estrdup(mf->user);
		nmf->fid = job->request.newfid;
		nmf->qid.vers = vers++;
		mf = nmf;
	}
	/* else nmf will be nil */

	qid = mf->qid;
	if(nelems > 0){
		/* walk fid */
		for(i=0; i<nelems && i<MAXWELEM; i++){
			if((qid.type & QTDIR) == 0){
				err = "not a directory";
				break;
			}
			if(strcmp(elems[i], "..") == 0 || strcmp(elems[i], ".") == 0){
				qid.type = QTDIR;
				qid.path = Qdir;
    Found:
				job->reply.wqid[i] = qid;
				job->reply.nwqid++;
				continue;
			}
			if(strcmp(elems[i], "cs") == 0){
				qid.type = QTFILE;
				qid.path = Qcs;
				goto Found;
			}
			err = "file does not exist";
			break;
		}
	}

    send:
	if(nmf != nil && (err!=nil || job->reply.nwqid<nelems)){
		cleanmf(nmf);
		free(nmf->user);
		nmf->user = 0;
		nmf->busy = 0;
		nmf->fid = 0;
	}
	if(err == nil)
		mf->qid = qid;
	sendmsg(job, err);
	return err;
}

void
ropen(Job *job, Mfile *mf)
{
	int mode;
	char *err;

	err = 0;
	mode = job->request.mode;
	if(mf->qid.type & QTDIR){
		if(mode)
			err = "permission denied";
	}
	job->reply.qid = mf->qid;
	job->reply.iounit = 0;
	sendmsg(job, err);
}

void
rcreate(Job *job, Mfile *mf)
{
	USED(mf);
	sendmsg(job, "creation permission denied");
}

void
rread(Job *job, Mfile *mf)
{
	int i, n, cnt;
	long off, toff, clock;
	Dir dir;
	uchar buf[Maxfdata];
	char *err;

	n = 0;
	err = 0;
	off = job->request.offset;
	cnt = job->request.count;
	if(mf->qid.type & QTDIR){
		clock = time(0);
		if(off == 0){
			dir.name = "cs";
			dir.qid.type = QTFILE;
			dir.qid.vers = vers;
			dir.qid.path = Qcs;
			dir.mode = 0666;
			dir.length = 0;
			dir.uid = mf->user;
			dir.gid = mf->user;
			dir.muid = mf->user;
			dir.atime = clock;	/* wrong */
			dir.mtime = clock;	/* wrong */
			n = convD2M(&dir, buf, sizeof buf);
		}
		job->reply.data = (char*)buf;
	} else {
		for(;;){
			/* look for an answer at the right offset */
			toff = 0;
			for(i = 0; mf->reply[i] && i < mf->nreply; i++){
				n = mf->replylen[i];
				if(off < toff + n)
					break;
				toff += n;
			}
			if(i < mf->nreply)
				break;		/* got something to return */

			/* try looking up more answers */
			if(lookup(mf) == 0){
				/* no more */
				n = 0;
				goto send;
			}
		}

		/* give back a single reply (or part of one) */
		job->reply.data = mf->reply[i] + (off - toff);
		if(cnt > toff - off + n)
			n = toff - off + n;
		else
			n = cnt;
	}
send:
	job->reply.count = n;
	sendmsg(job, err);
}
void
cleanmf(Mfile *mf)
{
	int i;

	if(mf->net != nil){
		free(mf->net);
		mf->net = nil;
	}
	if(mf->host != nil){
		free(mf->host);
		mf->host = nil;
	}
	if(mf->serv != nil){
		free(mf->serv);
		mf->serv = nil;
	}
	if(mf->rem != nil){
		free(mf->rem);
		mf->rem = nil;
	}
	for(i = 0; i < mf->nreply; i++){
		free(mf->reply[i]);
		mf->reply[i] = nil;
		mf->replylen[i] = 0;
	}
	mf->nreply = 0;
	mf->nextnet = netlist;
}

void
rwrite(Job *job, Mfile *mf)
{
	int cnt, n;
	char *err;
	char *field[4];
	char curerr[64];

	err = 0;
	cnt = job->request.count;
	if(mf->qid.type & QTDIR){
		err = "can't write directory";
		goto send;
	}
	if(cnt >= Maxrequest){
		err = "request too long";
		goto send;
	}
	job->request.data[cnt] = 0;

	/*
	 *  toggle debugging
	 */
	if(strncmp(job->request.data, "debug", 5)==0){
		debug ^= 1;
		syslog(1, logfile, "debug %d", debug);
		goto send;
	}

	/*
	 *  toggle debugging
	 */
	if(strncmp(job->request.data, "paranoia", 8)==0){
		paranoia ^= 1;
		syslog(1, logfile, "paranoia %d", paranoia);
		goto send;
	}

	/*
	 *  add networks to the default list
	 */
	if(strncmp(job->request.data, "add ", 4)==0){
		if(job->request.data[cnt-1] == '\n')
			job->request.data[cnt-1] = 0;
		netadd(job->request.data+4);
		readipinterfaces();
		goto send;
	}

	/*
	 *  refresh all state
	 */
	if(strncmp(job->request.data, "refresh", 7)==0){
		netinit(1);
		goto send;
	}

	/* start transaction with a clean slate */
	cleanmf(mf);

	/*
	 *  look for a general query
	 */
	if(*job->request.data == '!'){
		err = genquery(mf, job->request.data+1);
		goto send;
	}

	if(debug)
		syslog(0, logfile, "write %s", job->request.data);
	if(paranoia)
		syslog(0, paranoiafile, "write %s by %s", job->request.data, mf->user);

	/*
	 *  break up name
	 */
	n = getfields(job->request.data, field, 4, 1, "!");
	switch(n){
	case 1:
		mf->net = strdup("net");
		mf->host = strdup(field[0]);
		break;
	case 4:
		mf->rem = strdup(field[3]);
		/* fall through */
	case 3:
		mf->serv = strdup(field[2]);
		/* fall through */
	case 2:
		mf->host = strdup(field[1]);
		mf->net = strdup(field[0]);
		break;
	}

	/*
	 *  do the first net worth of lookup
	 */
	if(lookup(mf) == 0){
		rerrstr(curerr, sizeof curerr);
		err = curerr;
	}
send:
	job->reply.count = cnt;
	sendmsg(job, err);
}

void
rclunk(Job *job, Mfile *mf)
{
	cleanmf(mf);
	free(mf->user);
	mf->user = 0;
	mf->busy = 0;
	mf->fid = 0;
	sendmsg(job, 0);
}

void
rremove(Job *job, Mfile *mf)
{
	USED(mf);
	sendmsg(job, "remove permission denied");
}

void
rstat(Job *job, Mfile *mf)
{
	Dir dir;
	uchar buf[IOHDRSZ+Maxfdata];

	if(mf->qid.type & QTDIR){
		dir.name = ".";
		dir.mode = DMDIR|0555;
	} else {
		dir.name = "cs";
		dir.mode = 0666;
	}
	dir.qid = mf->qid;
	dir.length = 0;
	dir.uid = mf->user;
	dir.gid = mf->user;
	dir.muid = mf->user;
	dir.atime = dir.mtime = time(0);
	job->reply.nstat = convD2M(&dir, buf, sizeof buf);
	job->reply.stat = buf;
	sendmsg(job, 0);
}

void
rwstat(Job *job, Mfile *mf)
{
	USED(mf);
	sendmsg(job, "wstat permission denied");
}

void
sendmsg(Job *job, char *err)
{
	int n;
	uchar mdata[IOHDRSZ + Maxfdata];
	char ename[ERRMAX];

	if(err){
		job->reply.type = Rerror;
		snprint(ename, sizeof(ename), "cs: %s", err);
		job->reply.ename = ename;
	}else{
		job->reply.type = job->request.type+1;
	}
	job->reply.tag = job->request.tag;
	n = convS2M(&job->reply, mdata, sizeof mdata);
	if(n == 0){
		syslog(1, logfile, "sendmsg convS2M of %F returns 0", &job->reply);
		abort();
	}
	lock(&joblock);
	if(job->flushed == 0)
		if(write(mfd[1], mdata, n)!=n)
			error("mount write");
	unlock(&joblock);
	if(debug)
		syslog(0, logfile, "%F %d", &job->reply, n);
}

void
error(char *s)
{
	syslog(1, "cs", "%s: %r", s);
	_exits(0);
}

static int
isvalidip(uchar *ip)
{
	return ipcmp(ip, IPnoaddr) != 0 && ipcmp(ip, v4prefix) != 0;
}

static uchar loopbacknet[IPaddrlen] = {
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0xff, 0xff,
	127, 0, 0, 0
};
static uchar loopbackmask[IPaddrlen] = {
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0, 0, 0
};

void
readipinterfaces(void)
{
	if(myipaddr(ipa, mntpt) != 0)
		ipmove(ipa, IPnoaddr);
	sprint(ipaddr, "%I", ipa);
	if (debug)
		syslog(0, "dns", "ipaddr is %s\n", ipaddr);
}

/*
 *  get the system name
 */
void
ipid(void)
{
	uchar addr[6];
	Ndbtuple *t, *tt;
	char *p, *attr;
	Ndbs s;
	int f;
	char buf[Maxpath];

	/* use environment, ether addr, or ipaddr to get system name */
	if(mysysname == 0){
		/*
		 *  environment has priority.
		 *
		 *  on the sgi power the default system name
		 *  is the ip address.  ignore that.
		 *
		 */
		p = getenv("sysname");
		if(p && *p){
			attr = ipattr(p);
			if(strcmp(attr, "ip") != 0)
				mysysname = strdup(p);
		}

		/*
		 *  the /net/ndb contains what the network
		 *  figured out from DHCP.  use that name if
		 *  there is one. 
		 */
		if(mysysname == 0 && netdb != nil){
			ndbreopen(netdb);
			for(tt = t = ndbparse(netdb); t != nil; t = t->entry){
				if(strcmp(t->attr, "sys") == 0){
					mysysname = strdup(t->val);
					break;
				}
			}
			ndbfree(tt);
		}

		/* next network database, ip address, and ether address to find a name */
		if(mysysname == 0){
			t = nil;
			if(isvalidip(ipa))
				free(ndbgetvalue(db, &s, "ip", ipaddr, "sys", &t));
			if(t == nil){
				for(f = 0; f < 3; f++){
					snprint(buf, sizeof buf, "%s/ether%d", mntpt, f);
					if(myetheraddr(addr, buf) >= 0){
						snprint(eaddr, sizeof(eaddr), "%E", addr);
						free(ndbgetvalue(db, &s, "ether", eaddr, "sys", &t));
						if(t != nil)
							break;
					}
				}
			}
			for(tt = t; tt != nil; tt = tt->entry){
				if(strcmp(tt->attr, "sys") == 0){
					mysysname = strdup(tt->val);
					break;
				}
			}
			ndbfree(t);
		}

		/* nothing else worked, use the ip address */
		if(mysysname == 0 && isvalidip(ipa))
			mysysname = strdup(ipaddr);
					

		/* set /dev/sysname if we now know it */
		if(mysysname){
			f = open("/dev/sysname", OWRITE);
			if(f >= 0){
				write(f, mysysname, strlen(mysysname));
				close(f);
			}
		}
	}
}

/*
 *  Set up a list of default networks by looking for
 *  /net/ * /clone.
 */
void
netinit(int background)
{
	char clone[Maxpath];
	Network *np;
	static int working;

	if(background){
		switch(rfork(RFPROC|RFNOTEG|RFMEM|RFNOWAIT)){
		case 0:
			break;
		default:
			return;
		}
		lock(&netlock);
	}

	/* add the mounted networks to the default list */
	for(np = network; np->net; np++){
		if(np->considered)
			continue;
		snprint(clone, sizeof(clone), "%s/%s/clone", mntpt, np->net);
		if(access(clone, AEXIST) < 0)
			continue;
		if(netlist)
			last->next = np;
		else
			netlist = np;
		last = np;
		np->next = 0;
		np->considered = 1;
	}

	/* find out what our ip address is */
	readipinterfaces();

	/* set the system name if we need to, these says ip is all we have */
	ipid();

	if(debug)
		syslog(0, logfile, "mysysname %s eaddr %s ipaddr %s ipa %I\n",
			mysysname?mysysname:"???", eaddr, ipaddr, ipa);

	if(background){
		unlock(&netlock);
		_exits(0);
	}
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

	n = getfields(p, field, 12, 1, " ");
	for(i = 0; i < n; i++){
		for(np = network; np->net; np++){
			if(strcmp(field[i], np->net) != 0)
				continue;
			if(np->considered)
				break;
			if(netlist)
				last->next = np;
			else
				netlist = np;
			last = np;
			np->next = 0;
			np->considered = 1;
		}
	}
}

int
lookforproto(Ndbtuple *t, char *proto)
{
	for(; t != nil; t = t->entry)
		if(strcmp(t->attr, "proto") == 0 && strcmp(t->val, proto) == 0)
			return 1;
	return 0;
}

/*
 *  lookup a request.  the network "net" means we should pick the
 *  best network to get there.
 */
int
lookup(Mfile *mf)
{
	Network *np;
	char *cp;
	Ndbtuple *nt, *t;
	char reply[Maxreply];
	int i, rv;
	int hack;

	/* open up the standard db files */
	if(db == 0)
		ndbinit();
	if(db == 0)
		error("can't open mf->network database\n");

	rv = 0;

	if(mf->net == nil)
		return 0;	/* must have been a genquery */

	if(strcmp(mf->net, "net") == 0){ 
		/*
		 *  go through set of default nets
		 */
		for(np = mf->nextnet; np; np = np->next){
			nt = (*np->lookup)(np, mf->host, mf->serv, 1);
			if(nt == nil)
				continue;
			hack = np->fasttimeouthack && !lookforproto(nt, np->net);
			for(t = nt; mf->nreply < Nreply && t; t = t->entry){
				cp = (*np->trans)(t, np, mf->serv, mf->rem, hack);
				if(cp){
					/* avoid duplicates */
					for(i = 0; i < mf->nreply; i++)
						if(strcmp(mf->reply[i], cp) == 0)
							break;
					if(i == mf->nreply){
						/* save the reply */
						mf->replylen[mf->nreply] = strlen(cp);
						mf->reply[mf->nreply++] = cp;
						rv++;
					}
				}
			}
			ndbfree(nt);
			np = np->next;
			break;
		}
		mf->nextnet = np;
		return rv;
	}

	/*
	 *  if not /net, we only get one lookup
	 */
	if(mf->nreply != 0)
		return 0;
	/*
	 *  look for a specific network
	 */
	for(np = netlist; np && np->net != nil; np++){
		if(np->fasttimeouthack)
			continue;
		if(strcmp(np->net, mf->net) == 0)
			break;
	}

	if(np && np->net != nil){
		/*
		 *  known network
		 */
		nt = (*np->lookup)(np, mf->host, mf->serv, 1);
		for(t = nt; mf->nreply < Nreply && t; t = t->entry){
			cp = (*np->trans)(t, np, mf->serv, mf->rem, 0);
			if(cp){
				mf->replylen[mf->nreply] = strlen(cp);
				mf->reply[mf->nreply++] = cp;
				rv++;
			}
		}
		ndbfree(nt);
		return rv;
	} else {
		/*
		 *  not a known network, don't translate host or service
		 */
		if(mf->serv)
			snprint(reply, sizeof(reply), "%s/%s/clone %s!%s",
				mntpt, mf->net, mf->host, mf->serv);
		else
			snprint(reply, sizeof(reply), "%s/%s/clone %s",
				mntpt, mf->net, mf->host);
		mf->reply[0] = strdup(reply);
		mf->replylen[0] = strlen(reply);
		mf->nreply = 1;
		return 1;
	}
}

/*
 *  translate an ip service name into a port number.  If it's a numeric port
 *  number, look for restricted access.
 *
 *  the service '*' needs no translation.
 */
char*
ipserv(Network *np, char *name, char *buf, int blen)
{
	char *p;
	int alpha = 0;
	int restr = 0;
	char port[10];
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
			{}
		else if(isalpha(*p) || *p == '-' || *p == '$')
			alpha = 1;
		else 
			return 0;
	}
	t = nil;
	p = nil;
	if(alpha){
		p = ndbgetvalue(db, &s, np->net, name, "port", &t);
		if(p == nil)
			return 0;
	} else {
		/* look up only for tcp ports < 1024 to get the restricted
		 * attribute
		 */
		if(atoi(name) < 1024 && strcmp(np->net, "tcp") == 0)
			p = ndbgetvalue(db, &s, "port", name, "port", &t);
		if(p == nil)
			p = strdup(name);
	}

	if(t){
		for(nt = t; nt; nt = nt->entry)
			if(strcmp(nt->attr, "restricted") == 0)
				restr = 1;
		ndbfree(t);
	}
	snprint(buf, blen, "%s%s", p, restr ? "!r" : "");
	free(p);

	return buf;
}

/*
 *  lookup an ip attribute
 */
int
ipattrlookup(Ndb *db, char *ipa, char *attr, char *val, int vlen)
{
	
	Ndbtuple *t, *nt;
	char *alist[2];

	alist[0] = attr;
	t = ndbipinfo(db, "ip", ipa, alist, 1);
	if(t == nil)
		return 0;
	for(nt = t; nt != nil; nt = nt->entry){
		if(strcmp(nt->attr, attr) == 0){
			nstrcpy(val, nt->val, vlen);
			ndbfree(t);
			return 1;
		}
	}

	/* we shouldn't get here */
	ndbfree(t);
	return 0;
}

/*
 *  lookup (and translate) an ip destination
 */
Ndbtuple*
iplookup(Network *np, char *host, char *serv, int nolookup)
{
	char *attr, *dnsname;
	Ndbtuple *t, *nt;
	Ndbs s;
	char ts[Maxservice];
	char dollar[Maxhost];
	uchar ip[IPaddrlen];
	uchar net[IPaddrlen];
	uchar tnet[IPaddrlen];
	Ipifc *ifc;
	Iplifc *lifc;

	USED(nolookup);

	/*
	 *  start with the service since it's the most likely to fail
	 *  and costs the least
	 */
	werrstr("can't translate address");
	if(serv==0 || ipserv(np, serv, ts, sizeof ts) == 0){
		werrstr("can't translate service");
		return 0;
	}

	/* for dial strings with no host */
	if(strcmp(host, "*") == 0)
		return ndbnew("ip", "*");

	/*
	 *  hack till we go v6 :: = 0.0.0.0
	 */
	if(strcmp("::", host) == 0)
		return ndbnew("ip", "*");

	/*
	 *  '$' means the rest of the name is an attribute that we
	 *  need to search for
	 */
	if(*host == '$'){
		if(ipattrlookup(db, ipaddr, host+1, dollar, sizeof dollar))
			host = dollar;
	}

	/*
	 *  turn '[ip address]' into just 'ip address'
	 */
	if(*host == '[' && host[strlen(host)-1] == ']'){
		host++;
		host[strlen(host)-1] = 0;
	}

	/*
	 *  just accept addresses
	 */
	attr = ipattr(host);
	if(strcmp(attr, "ip") == 0)
		return ndbnew("ip", host);

	/*
	 *  give the domain name server the first opportunity to
	 *  resolve domain names.  if that fails try the database.
	 */
	t = 0;
	werrstr("can't translate address");
	if(strcmp(attr, "dom") == 0)
		t = dnsiplookup(host, &s);
	if(t == 0)
		free(ndbgetvalue(db, &s, attr, host, "ip", &t));
	if(t == 0){
		dnsname = ndbgetvalue(db, &s, attr, host, "dom", nil);
		if(dnsname){
			t = dnsiplookup(dnsname, &s);
			free(dnsname);
		}
	}
	if(t == 0)
		t = dnsiplookup(host, &s);
	if(t == 0)
		return 0;

	/*
	 *  reorder the tuple to have the matched line first and
	 *  save that in the request structure.
	 */
	t = reorder(t, s.t);

	/*
	 * reorder according to our interfaces
	 */
	lock(&ipifclock);
	for(ifc = ipifcs; ifc != nil; ifc = ifc->next){
		for(lifc = ifc->lifc; lifc != nil; lifc = lifc->next){ 
			maskip(lifc->ip, lifc->mask, net);
			for(nt = t; nt; nt = nt->entry){
				if(strcmp(nt->attr, "ip") != 0)
					continue;
				parseip(ip, nt->val);
				maskip(ip, lifc->mask, tnet);
				if(memcmp(net, tnet, IPaddrlen) == 0){
					t = reorder(t, nt);
					unlock(&ipifclock);
					return t;
				}
			}
		}
	}
	unlock(&ipifclock);

	return t;
}

/*
 *  translate an ip address
 */
char*
iptrans(Ndbtuple *t, Network *np, char *serv, char *rem, int hack)
{
	char ts[Maxservice];
	char reply[Maxreply];
	char x[Maxservice];

	if(strcmp(t->attr, "ip") != 0)
		return 0;

	if(serv == 0 || ipserv(np, serv, ts, sizeof ts) == 0){
		werrstr("can't translate service");
		return 0;
	}
	if(rem != nil)
		snprint(x, sizeof(x), "!%s", rem);
	else
		*x = 0;

	if(*t->val == '*')
		snprint(reply, sizeof(reply), "%s/%s/clone %s%s",
			mntpt, np->net, ts, x);
	else
		snprint(reply, sizeof(reply), "%s/%s/clone %s!%s%s%s",
			mntpt, np->net, t->val, ts, x, hack?"!fasttimeout":"");

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

	USED(np, nolookup, serv);

	werrstr("can't translate address");
	free(ndbgetvalue(db, &s, "sys", host, "telco", &t));
	if(t == 0)
		return ndbnew("telco", host);

	return reorder(t, s.t);
}

/*
 *  translate a telephone address
 */
char*
telcotrans(Ndbtuple *t, Network *np, char *serv, char *rem, int)
{
	char reply[Maxreply];
	char x[Maxservice];

	if(strcmp(t->attr, "telco") != 0)
		return 0;

	if(rem != nil)
		snprint(x, sizeof(x), "!%s", rem);
	else
		*x = 0;
	if(serv)
		snprint(reply, sizeof(reply), "%s/%s/clone %s!%s%s", mntpt, np->net,
			t->val, serv, x);
	else
		snprint(reply, sizeof(reply), "%s/%s/clone %s%s", mntpt, np->net,
			t->val, x);
	return strdup(reply);
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
 *  another.  parent returns to job loop.
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

/*
 *  call the dns process and have it try to translate a name
 */
Ndbtuple*
dnsiplookup(char *host, Ndbs *s)
{
	char buf[Maxreply];
	Ndbtuple *t;

	unlock(&dblock);

	/* save the name before starting a slave */
	snprint(buf, sizeof(buf), "%s", host);

	slave();

	if(strcmp(ipattr(buf), "ip") == 0)
		t = dnsquery(mntpt, buf, "ptr");
	else
		t = dnsquery(mntpt, buf, "ip");
	s->t = t;

	if(t == nil){
		rerrstr(buf, sizeof buf);
		if(strstr(buf, "exist"))
			werrstr("can't translate address: %s", buf);
		else if(strstr(buf, "dns failure"))
			werrstr("temporary problem: %s", buf);
	}

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
	Ndbtuple *nt;
	String *s;

	s = s_new();
	for(nt = t; mf->nreply < Nreply && nt; nt = nt->entry){
		s_append(s, nt->attr);
		s_append(s, "=");
		s_append(s, nt->val);

		if(nt->line != nt->entry){
			mf->replylen[mf->nreply] = s_len(s);
			mf->reply[mf->nreply++] = strdup(s_to_c(s));
			s_restart(s);
		} else
			s_append(s, " ");
	}
	s_free(s);
}

enum
{
	Maxattr=	32,
};

/*
 *  generic query lookup.  The query is of one of the following
 *  forms:
 *
 *  attr1=val1 attr2=val2 attr3=val3 ...
 *
 *  returns the matching tuple
 *
 *  ipinfo attr=val attr1 attr2 attr3 ...
 *
 *  is like ipinfo and returns the attr{1-n}
 *  associated with the ip address.
 */
char*
genquery(Mfile *mf, char *query)
{
	int i, n;
	char *p;
	char *attr[Maxattr];
	char *val[Maxattr];
	Ndbtuple *t;
	Ndbs s;

	n = getfields(query, attr, 32, 1, " ");
	if(n == 0)
		return "bad query";

	if(strcmp(attr[0], "ipinfo") == 0)
		return ipinfoquery(mf, attr, n);

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
		t = dnsiplookup(val[0], &s);
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

/*
 *  resolve an ip address
 */
static Ndbtuple*
ipresolve(char *attr, char *host)
{
	Ndbtuple *t, *nt, **l;

	t = iplookup(&network[Ntcp], host, "*", 0);
	for(l = &t; *l != nil; ){
		nt = *l;
		if(strcmp(nt->attr, "ip") != 0){
			*l = nt->entry;
			nt->entry = nil;
			ndbfree(nt);
			continue;
		}
		strcpy(nt->attr, attr);
		l = &nt->entry;
	}
	return t;
}

char*
ipinfoquery(Mfile *mf, char **list, int n)
{
	int i, nresolve;
	int resolve[Maxattr];
	Ndbtuple *t, *nt, **l;
	char *attr, *val;

	/* skip 'ipinfo' */
	list++; n--;

	if(n < 1)
		return "bad query";

	/* get search attribute=value, or assume ip=myipaddr */
	attr = *list;
	if((val = strchr(attr, '=')) != nil){
		*val++ = 0;
		list++;
		n--;
	}else{
		attr = "ip";
		val = ipaddr;
	}

	if(n < 1)
		return "bad query";

	/*
	 *  don't let ndbipinfo resolve the addresses, we're
	 *  better at it.
	 */
	nresolve = 0;
	for(i = 0; i < n; i++)
		if(*list[i] == '@'){
			list[i]++;
			resolve[i] = 1;
			nresolve++;
		} else
			resolve[i] = 0;

	t = ndbipinfo(db, attr, val, list, n);
	if(t == nil)
		return "no match";

	if(nresolve != 0){
		for(l = &t; *l != nil;){
			nt = *l;

			/* already an address? */
			if(strcmp(ipattr(nt->val), "ip") == 0){
				l = &(*l)->entry;
				continue;
			}

			/* user wants it resolved? */
			for(i = 0; i < n; i++)
				if(strcmp(list[i], nt->attr) == 0)
					break;
			if(i >= n || resolve[i] == 0){
				l = &(*l)->entry;
				continue;
			}

			/* resolve address and replace entry */
			*l = ipresolve(nt->attr, nt->val);
			while(*l != nil)
				l = &(*l)->entry;
			*l = nt->entry;

			nt->entry = nil;
			ndbfree(nt);
		}
	}

	/* make it all one line */
	for(nt = t; nt != nil; nt = nt->entry){
		if(nt->entry == nil)
			nt->line = t;
		else
			nt->line = nt->entry;
	}

	qreply(mf, t);

	return nil;
}

void*
emalloc(int size)
{
	void *x;

	x = malloc(size);
	if(x == nil)
		abort();
	memset(x, 0, size);
	return x;
}

char*
estrdup(char *s)
{
	int size;
	char *p;

	size = strlen(s)+1;
	p = malloc(size);
	if(p == nil)
		abort();
	memmove(p, s, size);
	return p;
}
