#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include <bio.h>
#include <ctype.h>
#include <ip.h>
#include "dns.h"

enum
{
	Maxrequest=		4*NAMELEN,
	Ncache=			8,
	Maxpath=		128,
	Maxreply=		512,
	Maxrrr=			16,

	Qdns=			1,
};

typedef struct Mfile	Mfile;
typedef struct Job	Job;
typedef struct Network	Network;

int vers;		/* incremented each clone/attach */

struct Mfile
{
	Mfile		*next;		/* next free mfile */

	char		user[NAMELEN];
	Qid		qid;
	int		fid;

	int		type;		/* reply type */
	char		reply[Maxreply];
	ushort		rr[Maxrrr];	/* offset of rr's */
	ushort		nrr;		/* number of rr's */
};

//
//  active local requests
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

struct {
	Lock;
	Mfile	*inuse;		/* active mfile's */
} mfalloc;

int	mfd[2];
char	user[NAMELEN];
int	debug;
int	cachedb;
ulong	now;
int	testing;
char	*trace;
int	needrefresh;
int	resolver;
uchar	ipaddr[IPaddrlen];	/* my ip address */
int	maxage;

void	rsession(Job*);
void	rsimple(Job*);
void	rflush(Job*);
void	rattach(Job*, Mfile*);
void	rclone(Job*, Mfile*);
char*	rwalk(Job*, Mfile*);
void	rclwalk(Job*, Mfile*);
void	ropen(Job*, Mfile*);
void	rcreate(Job*, Mfile*);
void	rread(Job*, Mfile*);
void	rwrite(Job*, Mfile*, Request*);
void	rclunk(Job*, Mfile*);
void	rremove(Job*, Mfile*);
void	rstat(Job*, Mfile*);
void	rauth(Job*);
void	rwstat(Job*, Mfile*);
void	sendmsg(Job*, char*);
void	mountinit(char*, char*);
void	io(void);
int	fillreply(Mfile*, int);
Job*	newjob(void);
void	freejob(Job*);
void	setext(char*, int, char*);

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

char	*logfile = "dns";
char	*dbfile;
char	mntpt[Maxpath];
char	*LOG;

void
usage(void)
{
	fprint(2, "usage: %s [-rs] [-f ndb-file] [-x netmtpt]\n", argv0);
	exits("usage");
}

void
main(int argc, char *argv[])
{
	int	serve;
	char	servefile[Maxpath];
	char	ext[Maxpath];
	char	*p;
	Ipifc	*ifcs;

	serve = 0;
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
	case 'r':
		resolver = 1;
		break;
	case 's':
		serve = 1;	/* serve network */
		cachedb = 1;
		break;
	case 'a':
		p = ARGF();
		if(p == nil)
			usage();
		maxage = atoi(p);
		break;
	case 't':
		testing = 1;
		break;
	}ARGEND
	USED(argc);
	USED(argv);

	rfork(RFREND|RFNOTEG);

	/* start syslog before we fork */
	fmtinstall('F', fcallconv);
	dninit();
	ifcs = readipifc(mntpt, nil);
	if(ifcs == nil)
		sysfatal("can't read my ip address");
	ipmove(ipaddr, ifcs->ip);

	syslog(0, logfile, "starting dns on %I", ipaddr);

	opendatabase();

	snprint(servefile, sizeof(servefile), "#s/dns%s", ext);
	unmount(servefile, mntpt);
	remove(servefile);
	mountinit(servefile, mntpt);

	now = time(0);
	srand(now*getpid());
	db2cache(1);

	if(serve)
		dnudpserver(mntpt);
	io();
	syslog(0, logfile, "io returned, exiting");
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
		abort(); /* "pipe failed" */;
	switch(rfork(RFFDG|RFPROC|RFNAMEG)){
	case 0:
		close(p[1]);
		break;
	case -1:
		abort(); /* "fork failed\n" */;
	default:
		close(p[0]);

		/*
		 *  make a /srv/dns
		 */
		f = create(service, 1, 0666);
		if(f < 0)
			abort(); /* service */;
		snprint(buf, sizeof(buf), "%d", p[1]);
		if(write(f, buf, strlen(buf)) != strlen(buf))
			abort(); /* "write %s", service */;
		close(f);

		/*
		 *  put ourselves into the file system
		 */
		if(mount(p[1], mntpt, MAFTER, "") < 0)
			abort(); /* "mount failed\n" */;
		_exits(0);
	}
	mfd[0] = mfd[1] = p[0];
}

#define INC 4
Mfile*
newfid(int fid, int needunused)
{
	Mfile *mf;

	lock(&mfalloc);
	for(mf = mfalloc.inuse; mf != nil; mf = mf->next){
		if(mf->fid == fid){
			unlock(&mfalloc);
			if(needunused)
				return nil;
			return mf;
		}
	}
	mf = mallocz(sizeof(*mf), 1);
	if(mf == nil)
		sysfatal("out of memory");
	mf->fid = fid;
	mf->next = mfalloc.inuse;
	mfalloc.inuse = mf;
	unlock(&mfalloc);
	return mf;
}

void
freefid(Mfile *mf)
{
	Mfile **l;

	lock(&mfalloc);
	for(l = &mfalloc.inuse; *l != nil; l = &(*l)->next){
		if(*l == mf){
			*l = mf->next;
			free(mf);
			unlock(&mfalloc);
			return;
		}
	}
	sysfatal("freeing unused fid");
}

Mfile*
copyfid(Mfile *mf, int fid)
{
	Mfile *nmf;

	nmf = newfid(fid, 1);
	if(nmf == nil)
		return nil;
	nmf->fid = fid;
	strncpy(nmf->user, mf->user, sizeof(mf->user));
	nmf->qid.path = mf->qid.path;
	nmf->qid.vers = vers++;
	return nmf;
}

Job*
newjob(void)
{
	Job *job;

	job = mallocz(sizeof(*job), 1);
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
	char mdata[MAXFDATA + MAXMSG];
	Request req;
	Job *job;

	/*
	 *  a slave process is sometimes forked to wait for replies from other
	 *  servers.  The master process returns immediately via a longjmp
	 *  through 'mret'.
	 */
	if(setjmp(req.mret))
		putactivity();
	req.isslave = 0;
	for(;;){
		n = read(mfd[0], mdata, sizeof mdata);
		if(n<=0){
			syslog(0, logfile, "error reading mntpt: %r");
			exits(0);
		}
		job = newjob();
		if(convM2S(mdata, &job->request, n) == 0){
			freejob(job);
			continue;
		}
		mf = newfid(job->request.fid, 0);
		if(debug)
			syslog(0, logfile, "%F", &job->request);

		getactivity(&req);
		req.aborttime = now + 60;	/* don't spend more than 60 seconds */

		switch(job->request.type){
		default:
			abort(); /* "type" */;
			break;
		case Tsession:
			rsession(job);
			break;
		case Tnop:
			rsimple(job);
			break;
		case Tflush:
			rflush(job);
			break;
		case Tattach:
			rattach(job, mf);
			break;
		case Tclone:
			rclone(job, mf);
			break;
		case Twalk:
			rwalk(job, mf);
			break;
		case Tclwalk:
			rclwalk(job, mf);
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
			rwrite(job, mf, &req);
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

		freejob(job);
	
		/*
		 *  slave processes die after replying
		 */
		if(req.isslave){
			putactivity();
			_exits(0);
		}
	
		putactivity();
	}
}

void
rsession(Job *job)
{
	memset(job->reply.authid, 0, sizeof(job->reply.authid));
	memset(job->reply.authdom, 0, sizeof(job->reply.authdom));
	memset(job->reply.chal, 0, sizeof(job->reply.chal));
	sendmsg(job, 0);
}

void
rsimple(Job *job)
{
	sendmsg(job, 0);
}

/* ignore flushes since the operation will time out */
void
rflush(Job *job)
{
	flushjob(job->request.oldtag);
	sendmsg(job, 0);
}

void
rauth(Job *job)
{
	sendmsg(job, "Authentication failed");
}

void
rattach(Job *job, Mfile *mf)
{
	strcpy(mf->user, job->request.uname);
	mf->qid.vers = vers++;
	mf->qid.path = CHDIR;
	job->reply.qid = mf->qid;
	sendmsg(job, 0);
}

char *Eused;

void
rclone(Job *job, Mfile *mf)
{
	Mfile *nmf;
	char *err=0;

	nmf = copyfid(mf, job->request.newfid);
	if(nmf == nil)
		err = Eused;
	sendmsg(job, err);
}

void
rclwalk(Job *job, Mfile *mf)
{
	Mfile *nmf;

	nmf = copyfid(mf, job->request.newfid);
	if(nmf == nil){
		sendmsg(job, Eused);
		return;
	}
	job->request.fid = job->request.newfid;
	if(rwalk(job, nmf) != nil)
		freefid(nmf);
}

char*
rwalk(Job *job, Mfile *mf)
{
	char *err;
	char *name;

	err = 0;
	name = job->request.name;
	if((mf->qid.path & CHDIR) == 0){
		err = "not a directory";
		goto send;
	}
	if(strcmp(name, ".") == 0){
		mf->qid.path = CHDIR;
		goto send;
	}
	if(strcmp(name, "dns") == 0){
		mf->qid.path = Qdns;
		goto send;
	}
	err = "nonexistent file";
    send:
	job->reply.qid = mf->qid;
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
	if(mf->qid.path & CHDIR){
		if(mode)
			err = "permission denied";
	}
	job->reply.qid = mf->qid;
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
	long off;
	Dir dir;
	char buf[MAXFDATA];
	char *err;

	n = 0;
	err = 0;
	off = job->request.offset;
	cnt = job->request.count;
	if(mf->qid.path & CHDIR){
		if(off%DIRLEN || cnt%DIRLEN){
			err = "bad offset";
			goto send;
		}
		if(off == 0){
			memmove(dir.name, "dns", NAMELEN);
			dir.qid.vers = vers;
			dir.qid.path = Qdns;
			dir.mode = 0666;
			dir.length = 0;
			strcpy(dir.uid, mf->user);
			strcpy(dir.gid, mf->user);
			dir.atime = now;
			dir.mtime = now;
			convD2M(&dir, buf+n);
			n += DIRLEN;
		}
		job->reply.data = buf;
	} else {
		for(i = 1; i <= mf->nrr; i++)
			if(mf->rr[i] > off)
				break;
		if(i > mf->nrr)
			goto send;
		if(off + cnt > mf->rr[i])
			n = mf->rr[i] - off;
		else
			n = cnt;
		job->reply.data = mf->reply + off;
	}
send:
	job->reply.count = n;
	sendmsg(job, err);
}

void
rwrite(Job *job, Mfile *mf, Request *req)
{
	int cnt, rooted, status;
	long n;
	char *err, *p, *atype;
	RR *rp, *tp, *neg;
	int wantsav;

	err = 0;
	cnt = job->request.count;
	if(mf->qid.path & CHDIR){
		err = "can't write directory";
		goto send;
	}
	if(cnt >= Maxrequest){
		err = "request too long";
		goto send;
	}
	job->request.data[cnt] = 0;
	if(cnt > 0 && job->request.data[cnt-1] == '\n')
		job->request.data[cnt-1] = 0;

	/*
	 *  special commands
	 */
	if(strncmp(job->request.data, "debug", 5)==0 && job->request.data[5] == 0){
		debug ^= 1;
		goto send;
	} else if(strncmp(job->request.data, "dump", 4)==0 && job->request.data[4] == 0){
		dndump("/lib/ndb/dnsdump");
		goto send;
	} else if(strncmp(job->request.data, "refresh", 7)==0 && job->request.data[7] == 0){
		needrefresh = 1;
		goto send;
	}

	/*
	 *  kill previous reply
	 */
	mf->nrr = 0;
	mf->rr[0] = 0;

	/*
	 *  break up request (into a name and a type)
	 */
	atype = strchr(job->request.data, ' ');
	if(atype == 0){
		err = "illegal request";
		goto send;
	} else
		*atype++ = 0;

	/*
	 *  tracing request
	 */
	if(strcmp(atype, "trace") == 0){
		if(trace)
			free(trace);
		if(*job->request.data)
			trace = strdup(job->request.data);
		else
			trace = 0;
		goto send;
	}

	mf->type = rrtype(atype);
	if(mf->type < 0){
		err = "unknown type";
		goto send;
	}

	p = atype - 2;
	if(p >= job->request.data && *p == '.'){
		rooted = 1;
		*p = 0;
	} else
		rooted = 0;

	p = job->request.data;
	if(*p == '!'){
		wantsav = 1;
		p++;
	} else
		wantsav = 0;
	rp = dnresolve(p, Cin, mf->type, req, 0, 0, Recurse, rooted, &status);
	neg = rrremneg(&rp);
	if(neg){
		status = neg->negrcode;
		rrfreelist(neg);
	}
	if(rp == 0){
		if(status == Rname)
			err = "name does not exist";
		else
			err = "no translation found";
	} else {
		/* format data to be read later */
		n = 0;
		mf->nrr = 0;
		for(tp = rp; mf->nrr < Maxrrr-1 && n < Maxreply && tp &&
				tsame(mf->type, tp->type); tp = tp->next){
			mf->rr[mf->nrr++] = n;
			if(wantsav)
				n += snprint(mf->reply+n, Maxreply-n, "%Q", tp);
			else
				n += snprint(mf->reply+n, Maxreply-n, "%R", tp);
		}
		mf->rr[mf->nrr] = n;
		rrfreelist(rp);
	}

    send:
	job->reply.count = cnt;
	sendmsg(job, err);
}

void
rclunk(Job *job, Mfile *mf)
{
	freefid(mf);
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

	if(mf->qid.path & CHDIR){
		strcpy(dir.name, ".");
		dir.mode = CHDIR|0555;
	} else {
		strcpy(dir.name, "dns");
		dir.mode = 0666;
	}
	dir.qid = mf->qid;
	dir.length = 0;
	strcpy(dir.uid, mf->user);
	strcpy(dir.gid, mf->user);
	dir.atime = dir.mtime = time(0);
	convD2M(&dir, (char*)job->reply.stat);
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
	char mdata[MAXFDATA + MAXMSG];

	if(err){
		job->reply.type = Rerror;
		snprint(job->reply.ename, sizeof(job->reply.ename), "dns: %s", err);
	}else{
		job->reply.type = job->request.type+1;
		job->reply.fid = job->request.fid;
	}
	job->reply.tag = job->request.tag;
	if(debug)
		syslog(0, logfile, "%F", &job->reply);
	n = convS2M(&job->reply, mdata);
	lock(&joblock);
	if(job->flushed == 0){
		if(write(mfd[1], mdata, n)!=n)
			abort(); /* "mount write" */;
	}
	unlock(&joblock);
}

/*
 *  the following varies between dnsdebug and dns
 */
void
logreply(int id, uchar *addr, DNSmsg *mp)
{
	RR *rp;

	syslog(0, LOG, "%d: rcvd %I flags:%s%s%s%s%s", id, addr,
		mp->flags & Fauth ? " auth" : "",
		mp->flags & Ftrunc ? " trunc" : "",
		mp->flags & Frecurse ? " rd" : "",
		mp->flags & Fcanrec ? " ra" : "",
		mp->flags & (Fauth|Rname) == (Fauth|Rname) ?
		" nx" : "");
	for(rp = mp->qd; rp != nil; rp = rp->next)
		syslog(0, LOG, "%d: rcvd %I qd %s", id, addr, rp->owner->name);
	for(rp = mp->an; rp != nil; rp = rp->next)
		syslog(0, LOG, "%d: rcvd %I an %R", id, addr, rp);
	for(rp = mp->ns; rp != nil; rp = rp->next)
		syslog(0, LOG, "%d: rcvd %I ns %R", id, addr, rp);
	for(rp = mp->ar; rp != nil; rp = rp->next)
		syslog(0, LOG, "%d: rcvd %I ar %R", id, addr, rp);
}

void
logsend(int id, int subid, uchar *addr, char *sname, char *rname, int type)
{
	char buf[12];

	syslog(0, LOG, "%d.%d: sending to %I/%s %s %s",
		id, subid, addr, sname, rname, rrname(type, buf));
}

RR*
getdnsservers(int class)
{
	return dnsservers(class);
}
