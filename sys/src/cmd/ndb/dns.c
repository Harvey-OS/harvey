#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include <bio.h>
#include <ctype.h>
#include "dns.h"

enum
{
	Maxrequest=		4*NAMELEN,
	Ncache=			8,

	Qdns=			1,
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

	int		tag;		/* tag of current request */
	RR		*rp;		/* start of reply */
	int		type;		/* reply type */
};

Mfile	*mfile;
int	nmfile = 0;
int	mfd[2];
char	user[NAMELEN];
Fcall	*rhp;
Fcall	*thp;
int	debug;

void	rsession(void);
void	rsimple(void);
void	rflush(int tag);
void	rattach(Mfile*);
void	rclone(Mfile*);
char*	rwalk(Mfile*);
void	rclwalk(Mfile*);
void	ropen(Mfile*);
void	rcreate(Mfile*);
void	rread(Mfile*);
void	rwrite(Mfile*, Request*);
void	rclunk(Mfile*);
void	rremove(Mfile*);
void	rstat(Mfile*);
void	rauth(void);
void	rwstat(Mfile*);
void	sendmsg(char*);
void	mountinit(char*);
void	io(void);
int	lookup(Mfile*, char*, char*, char*);
int	fillreply(Mfile*, int);
int	mygetfields(char*, char**, int, char);

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

char *logfile = "dns";
char *dbfile;

void
main(int argc, char *argv[])
{
	Fcall	rhdr;
	Fcall	thdr;
	int	serve;


	serve = 0;
	rhp = &rhdr;
	thp = &thdr;
	ARGBEGIN{
	case 'd':
		debug = 1;
		break;
	case 'f':
		dbfile = ARGF();
		break;
	case 's':
		serve = 1;	/* serve network */
		break;
	}ARGEND
	USED(argc);
	USED(argv);

	remove("#s/dns");
	unmount("/net/dns", "/net");
	mountinit("#s/dns");

	fmtinstall('F', fcallconv);
	dninit();

	syslog(1, logfile, "started%s%s", serve?" serving":"",
		debug?" debuging":"");
	if(serve)
		dnserver();
	io();
	exits(0);
}

void
mountinit(char *service)
{
	int f;
	int p[2];
	char buf[32];

	if(pipe(p) < 0)
		fatal("pipe failed");
	switch(rfork(RFFDG|RFPROC|RFNAMEG)){
	case 0:
		close(p[1]);
		break;
	case -1:
		fatal("fork failed\n");
	default:
		close(p[0]);

		/*
		 *  make a /srv/dns
		 */
		f = create(service, 1, 0666);
		if(f < 0)
			fatal(service);
		snprint(buf, sizeof(buf), "%d", p[1]);
		if(write(f, buf, strlen(buf)) != strlen(buf))
			fatal("write %s", service);
		close(f);

		/*
		 *  put ourselves into the file system
		 */
		if(mount(p[1], "/net", MAFTER, "") < 0)
			fatal("mount failed\n");
		_exits(0);
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
			fprint(2, "dns: realloc(0x%lux, %d)\n", o, nmfile*sizeof(Mfile));
			fatal("realloc failure");
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
	char mdata[MAXFDATA + MAXMSG];
	Request req;

	/*
	 *  a slave process is sometimes forked to wait for replies from other
	 *  servers.  The master process returns immediately via a longjmp
	 *  through 'mret'.
	 */
	setjmp(req.mret);
	req.isslave = 0;
    loop:
	n = read(mfd[0], mdata, sizeof mdata);
	if(n<=0)
		fatal("mount read");
	if(convM2S(mdata, rhp, n) == 0)
		goto loop;
	if(rhp->fid<0)
		fatal("fid out of range");
	mf = newfid(rhp->fid);
	if(debug)
		syslog(0, logfile, "%F", rhp);
	mf->tag = rhp->tag;
	switch(rhp->type){
	default:
		fatal("type");
		break;
	case Tsession:
		rsession();
		break;
	case Tnop:
		rsimple();
		break;
	case Tflush:
		rflush(rhp->tag);
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
		rwrite(mf, &req);
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
	/*
	 *  slave processes die after replying
	 */
	if(req.isslave)
		_exits(0);
	goto loop;
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
rsimple(void)
{
	sendmsg(0);
}

void
rflush(int tag)
{
	Mfile *mf;

	for(mf = mfile; mf < &mfile[nmfile]; mf++){
		if(mf->busy == 0)
			continue;
		if(mf->tag == tag){
			mf->tag = -1;
			break;
		}
	}
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
	if(strcmp(name, "dns") == 0){
		mf->qid.path = Qdns;
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
	int n, cnt, len;
	long toff, off, clock;
	Dir dir;
	char buf[MAXFDATA];
	char *err;
	RR *rp;

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
			memmove(dir.name, "dns", NAMELEN);
			dir.qid.vers = vers;
			dir.qid.path = Qdns;
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
		n = 0;
		thp->data = buf;
		for(rp = mf->rp; rp && tsame(mf->type, rp->type); rp = rp->next){
			len = snprint(buf, sizeof(buf), "%R", rp);
			if(toff + len > off){
				toff = off - toff;
				if(cnt > len - toff)
					cnt = len - toff;
				thp->data = buf + toff;
				n = cnt;
				break;
			}
			toff += len;
		}
	}
send:
	thp->count = n;
	sendmsg(err);
}

void
rwrite(Mfile *mf, Request *req)
{
	int cnt;
	char *err;
	char *atype;

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
		goto send;
	} else if(strncmp(rhp->data, "dump", 4)==0){
		dndump("/lib/ndb/dnsdump");
		goto send;
	}

	/*
	 *  break up request (into a name and a type)
	 */
	atype = strchr(rhp->data, ' ');
	if(atype == 0){
		err = "illegal request";
		goto send;
	} else
		*atype++ = 0;

	mf->type = rrtype(atype);
	if(mf->type < 0){
		err = "unknown type";
		goto send;
	}

	mf->rp = dnresolve(rhp->data, Cin, mf->type, req, 0);
	if(mf->rp == 0){
		err = "not found";
	}

	/* don't reply if the request was flushed */
	if(mf->tag < 0)
		return;

    send:
	thp->count = cnt;
	sendmsg(err);
}

void
rclunk(Mfile *mf)
{
	if(mf->rp){
		/* free resource records from the database */
		if(mf->rp->db)
			rrfreelist(mf->rp);
		mf->rp = 0;
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

	if(mf->qid.path & CHDIR){
		strcpy(dir.name, ".");
		dir.mode = CHDIR|0555;
	} else {
		strcpy(dir.name, "dns");
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
		snprint(thp->ename, sizeof(thp->ename), "dns: %s", err);
	}else{
		thp->type = rhp->type+1;
		thp->fid = rhp->fid;
	}
	thp->tag = rhp->tag;
	if(debug)
		syslog(0, logfile, "%F", thp);
	n = convS2M(thp, mdata);
	if(write(mfd[1], mdata, n)!=n)
		fatal("mount write");
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
			*lp++=0;
		if(*lp == 0)
			break;
		fields[i]=lp;
		while(*lp && *lp!=sep && *lp!=sep2)
			lp++;
	}
	return i;
}

void
warning(char *fmt, ...)
{
	int n;
	char dnserr[128];

	n = doprint(dnserr, dnserr+sizeof(dnserr), fmt, &fmt+1) - dnserr;
	dnserr[n] = 0;
	syslog(1, "dns", dnserr);
}

void
fatal(char *fmt, ...)
{
	int n;
	char dnserr[128];

	n = doprint(dnserr, dnserr+sizeof(dnserr), fmt, &fmt+1) - dnserr;
	dnserr[n] = 0;
	syslog(1, "dns", dnserr);
	abort();
}

/*
 *  create a slave process to handle a request to avoid one request blocking
 *  another
 */
void
slave(Request *req)
{
	if(req->isslave)
		return;		/* we're already a slave process */

	switch(rfork(RFPROC|RFNOTEG|RFMEM|RFNOWAIT)){
	case -1:
		break;
	case 0:
		req->isslave = 1;
		break;
	default:
		longjmp(req->mret, 1);
	}
}
