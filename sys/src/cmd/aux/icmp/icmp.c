#include <u.h>
#include <libc.h>
#include <ip.h>
#include <auth.h>
#include <fcall.h>
#include "icmp.h"
#include "lock.h"
#define min(a, b)	((a) < (b) ? (a) : (b))

typedef struct Fid Fid;
typedef struct File File;
typedef struct Readq Readq;

struct File
{
	short	busy;
	short	open;
	long	parent;		/* index in File array */
	Qid	qid;
	long	perm;
	char	name[NAMELEN];
	ulong	atime;
	ulong	mtime;
	char	*user;
	char	*group;
};
typedef enum {
	Rrequest,
	Rdata,
} Rtype;

/*
 * queued read request or data waiting for a read
 */
struct Readq {
	union	{
		Fcall	thdr;
		struct {
			char	*pktdata;
			int	pktsize;
		};
	};
	Rtype	t;
	Readq	*next;
};

struct Fid
{
	short	busy;
	short	open;
	int	fid;
	Fid	*next;
	char	*user;
	File	*file;
	Lock	flock;
	Readq	*readq;
};

enum
{
	Pexec =		1,
	Pwrite = 	2,
	Pread = 	4,
	Pother = 	1,
	Pgroup = 	8,
	Powner =	64,
};

enum
{
	OPERM	= 0x3,		/* mask of all permission types in open mode */
	Nfile	= 10,
	Nconn	= 512,
	Bufsize	= 8*1024,
};

ulong	path;		/* incremented for each new file */
Fid	*fids;
File	files[Nfile];
int	nfile;
int	mfd[2];
char	user[NAMELEN];
Fcall	rhdr;
Fcall	thdr;
int	icmpfd;
uchar	myip[4];
Lock	lmalloc;	/* lock for malloc */

Fid *	newfid(int);
void	error(char*);
void	io(void);
void	usage(void);
void	filestat(File*, char*);
int	perm(Fid*, File*, int);
void	loop(void);
ushort	ip_cksum(uchar*, int);
ushort	nhgets(uchar*);
void	hnputs(uchar*, ushort);
void	send9p(Fcall*);
void	answerrequest(Fid *, char*, int);
void	queuedata(Fid*, char*, int);
void	freereadq(Readq*);
void *	emalloc(ulong);

char	*rflush(Fid*), *rnop(Fid*), *rsession(Fid*),
	*rattach(Fid*), *rclone(Fid*), *rwalk(Fid*),
	*rclwalk(Fid*), *ropen(Fid*), *rcreate(Fid*),
	*rread(Fid*), *rwrite(Fid*), *rclunk(Fid*),
	*rremove(Fid*), *rstat(Fid*), *rwstat(Fid*);

char 	*(*fcalls[])(Fid*) = {
	[Tflush]	rflush,
	[Tsession]	rsession,
	[Tnop]		rnop,
	[Tattach]	rattach,
	[Tclone]	rclone,
	[Twalk]		rwalk,
	[Tclwalk]	rclwalk,
	[Topen]		ropen,
	[Tcreate]	rcreate,
	[Tread]		rread,
	[Twrite]	rwrite,
	[Tclunk]	rclunk,
	[Tremove]	rremove,
	[Tstat]		rstat,
	[Twstat]	rwstat,
};

char	Eperm[] =	"permission denied";
char	Enotexist[] =	"file does not exist";
char	Enotdir[] = 	"not a directory";
char	Eexist[] =	"file exists";
char	Eisopen[] = 	"file already open for I/O";
char	Excl[] = 	"exclusive use file already open";
char	EDontreply[] = 	"deferred";	/* not passed to user program */

/*
 * the code assumes that people do not write through shared fid's and
 * expect to get replies disambiguated between processes. To do that,
 * we'd have to keep more state (e.g. have subdirectories for connections)
 */
void
main(int argc, char *argv[])
{
	char *defmnt;
	int p[2];
	char buf[12];
	int fd;
	int stdio = 0;
	File	*r;

	defmnt = "/net";
	ARGBEGIN{
	case 'i':
		defmnt = 0;
		stdio = 1;
		mfd[0] = 0;
		mfd[1] = 1;
		break;
	case 's':
		defmnt = 0;
		break;
	case 'm':
		defmnt = ARGF();
		break;
	default:
		usage();
	}ARGEND

	if(myipaddr(myip, "/net/tcp") < 0)
		error("I must know my ip address!");
	icmpfd = open("/net/icmp", ORDWR);
	if(icmpfd < 0)
		error("open of /net/icmp failed");
	if(pipe(p) < 0)
		error("pipe failed");
	if(!stdio){
		mfd[0] = p[0];
		mfd[1] = p[0];
		if(defmnt == 0){
			fd = create("#s/icmpfs", OWRITE, 0666);
			if(fd < 0)
				error("create of /srv/icmpfs failed");
			sprint(buf, "%d", p[1]);
			if(write(fd, buf, strlen(buf)) < 0)
				error("writing /srv/icmpfs");
		}
	}

	/*
	 * this is overkill, but is expandable
	 */
	nfile = 0;
	r = &files[nfile++];
	r->busy = 1;
	r->perm = CHDIR | 0555;
	r->qid.path = CHDIR;
	r->qid.vers = 0;
	r->parent = 0;
	r->user = user;
	r->group = user;
	r->atime = time(0);
	r->mtime = r->atime;
	strcpy(r->name, ".");

	r = &files[nfile++];
	r->busy = 1;
	r->perm = 0666;
	r->qid.path = ++path;
	r->qid.vers = 0;
	r->parent = 0;
	r->user = user;
	r->group = user;
	r->atime = time(0);
	r->mtime = r->atime;
	strcpy(r->name, "icmp");

	strcpy(user, getuser());

/*	fmtinstall('F', fcallconv);/**/
	lockinit(&lmalloc);
	switch(rfork(RFFDG|RFPROC|RFMEM)){
	case -1:
		error("fork");
	case 0:
		close(p[1]);
		loop();
		break;
	default:
		break;
	}
	switch(rfork(RFFDG|RFPROC|RFNAMEG|RFNOTEG)){
	case -1:
		error("fork");
	case 0:
		close(p[1]);
		io();
		break;
	default:
		close(p[0]);	/* don't deadlock if child fails */
		if(defmnt && mount(p[1], defmnt, MBEFORE, "") < 0)
			error("mount failed");
	}
	exits(0);
}

char*
rnop(Fid *unused)
{
	USED(unused);
	return 0;
}

char*
rsession(Fid *unused)
{
	Fid *f;

	USED(unused);
	for(f = fids; f; f = f->next)
		if(f->busy)
			rclunk(f);
	memset(thdr.authid, 0, sizeof(thdr.authid));
	memset(thdr.authdom, 0, sizeof(thdr.authdom));
	memset(thdr.chal, 0, sizeof(thdr.chal));
	return 0;
}

char*
rflush(Fid *unused)
{
	Readq	*rr, *next, **prev;
	Fid	*f;

	/*
	 * find and delete the request from the list of
	 * pending requests
	 */
	USED(unused);
	for(f = fids; f; f = f->next) {
		lock(&f->flock);
		if(f->busy && f->readq && f->readq->t == Rrequest) {
			prev = &f->readq;
			for(rr = f->readq; rr; rr = next) {
				next = rr->next;
				if(rr->thdr.tag == rhdr.oldtag) {
					*prev = rr->next;
					freereadq(rr);
				} else
					prev = &rr->next;
			}
		}
		unlock(&f->flock);
	}
	return 0;
}

char*
rattach(Fid *f)
{
	f->busy = 1;
	f->file = &files[0];
	thdr.qid = f->file->qid;
	if(rhdr.uname[0])
		f->user = strdup(rhdr.uname);
	else
		f->user = "none";
	return 0;
}

char*
rclone(Fid *f)
{
	Fid *nf;

	if(f->open)
		return Eisopen;
	if(f->file->busy == 0)
		return Enotexist;
	nf = newfid(rhdr.newfid);
	nf->busy = 1;
	nf->open = 0;
	nf->file = f->file;
	nf->user = f->user;	/* no ref count; the leakage is minor */
	return 0;
}

char*
rwalk(Fid *f)
{
	File *r;
	char *name;
	File *parent;

	if((f->file->qid.path & CHDIR) == 0)
		return Enotdir;
	if(f->file->busy == 0)
		return Enotexist;
	f->file->atime = time(0);
	name = rhdr.name;
	if(strcmp(name, ".") == 0){
		thdr.qid = f->file->qid;
		return 0;
	}
	parent = &files[f->file->parent];
	if(!perm(f, parent, Pexec))
		return Eperm;
	if(strcmp(name, "..") == 0){
		f->file = parent;
		thdr.qid = f->file->qid;
		return 0;
	}
	for(r=files; r < &files[nfile]; r++)
		if(r->busy && r->parent==f->file-files && strcmp(name, r->name)==0){
			thdr.qid = r->qid;
			f->file = r;
			return 0;
		}
	return Enotexist;
}

char *
rclwalk(Fid *f)
{
	Fid *nf;
	char *err;

	nf = newfid(rhdr.newfid);
	nf->busy = 1;
	nf->file = f->file;
	nf->user = f->user;
	if(err = rwalk(nf))
		rclunk(nf);
	return err;
}

char *
ropen(Fid *f)
{
	File *r;
	int mode;

	if(f->open)
		return Eisopen;
	r = f->file;
	if(r->busy == 0)
		return Enotexist;
	mode = rhdr.mode;
	if(r->qid.path & CHDIR){
		if(mode != OREAD)
			return Eperm;
		thdr.qid = r->qid;
		return 0;
	}
	if(mode & ORCLOSE)
		return Eperm;
	mode &= OPERM;
	if(mode==OWRITE || mode==ORDWR)
		if(!perm(f, r, Pwrite))
			return Eperm;
	if(mode==OREAD || mode==ORDWR)
		if(!perm(f, r, Pread))
			return Eperm;
	if(mode==OEXEC)
			return Eperm;
	if((r->perm & CHAPPEND)==0)
		r->qid.vers++;
	thdr.qid = r->qid;
	f->open = 1;
	r->open++;
	return 0;
}

char *
rcreate(Fid *f)
{
	USED(f);
	return Eperm;
}

char*
rread(Fid *f)
{
	File *r;
	Readq	*rr;
	char *buf;
	long off;
	int n, cnt;

	if(f->file->busy == 0)
		return Enotexist;
	if(f->file->qid.path & CHDIR){
		n = 0;
		thdr.count = 0;
		off = rhdr.offset;
		cnt = rhdr.count;
		buf = thdr.data;
		if(off%DIRLEN || cnt%DIRLEN)
			return "i/o error";
		for(r=files+1; off; r++){
			if(r->busy && r->parent==f->file-files)
				off -= DIRLEN;
			if(r == &files[nfile-1])
				return 0;
		}
		for(; r<&files[nfile] && n < cnt; r++){
			if(!r->busy || r->parent!=f->file-files)
				continue;
			filestat(r, buf+n);
			n += DIRLEN;
		}
		thdr.count = n;
		return 0;
	}
	thdr.type = rhdr.type + 1;
	thdr.fid = rhdr.fid;
	thdr.tag = rhdr.tag;
	lock(&f->flock);
	if(f->readq && f->readq->t == Rdata) {
		/*
		 * if there is data queued up, give it to the process
		 */
		rr = f->readq;
		cnt = min(rhdr.count, rr->pktsize);
		thdr.data = rr->pktdata;
		thdr.count = cnt;
		send9p(&thdr);
		f->readq = rr->next;
		freereadq(rr);
	} else {
		/*
		 * prepare the 9p packet, queue up a read request and let
		 * the network reader send it when an icmp packet comes in
		 */
		thdr.count = rhdr.count; /* size of read request */
		rr = (Readq *) emalloc(sizeof(Readq));
		memcpy(&rr->thdr, &thdr, sizeof(thdr));
		rr->t = Rrequest;
		rr->next = f->readq;
		f->readq = rr;
	}
	unlock(&f->flock);
	return EDontreply;
}

char*
rwrite(Fid *f)
{
	File *r;
	int cnt;
	uchar	outpkt[Bufsize];
	Icmp	*pkt;

	r = f->file;
	if(r->busy == 0)
		return Enotexist;
	if(r->qid.path & CHDIR)
		return "file is a directory";
	cnt = rhdr.count;
	if(cnt > Bufsize)
		return "write too big";
	memcpy(outpkt, rhdr.data, cnt);
	r->qid.vers++;
	r->mtime = time(0);
	if(cnt % 2)
		outpkt[cnt++] = 0;
	thdr.count = cnt;
	pkt = (void *) outpkt;
	memcpy(pkt->src, myip, sizeof(myip));
	hnputs(pkt->icmpid, f->fid);
	memset(pkt->cksum, 0, sizeof(pkt->cksum));
	hnputs(pkt->cksum, ip_cksum(&pkt->type, cnt - ETHERIPHDR));
	if(write(icmpfd, outpkt, cnt) < 0)
		error("write to icmpfd failed");
	return 0;
}

char *
rclunk(Fid *f)
{
	Readq	*rr, *next;

	lock(&f->flock);
	if(f->open)
		f->file->open--;
	if(f->busy && f->readq)
		for(rr = f->readq; rr; rr = next) {
			next = rr->next;
			freereadq(rr);
		}
	f->readq = 0;
	f->busy = 0;
	f->open = 0;
	f->file = 0;
	unlock(&f->flock);
	return 0;
}

char *
rremove(Fid *f)
{
	USED(f);
	return Eperm;
}

char *
rstat(Fid *f)
{
	if(f->file->busy == 0)
		return Enotexist;
	filestat(f->file, thdr.stat);
	return 0;
}

char *
rwstat(Fid *f)
{
	USED(f);
	return Eperm;
}

Fid *
newfid(int fid)
{
	Fid *f, *ff;

	ff = 0;
	for(f = fids; f; f = f->next)
		if(f->fid == fid)
			return f;
		else if(!ff && !f->busy)
			ff = f;
	if(ff){
		ff->fid = fid;
		return ff;
	}
	f = emalloc(sizeof *f);
	f->fid = fid;
	f->next = fids;
	lockinit(&f->flock);
	lock(&f->flock);
		f->readq = 0;
	unlock(&f->flock);
	fids = f;
	return f;
}

void
filestat(File *r, char *buf)
{
	Dir dir;

	memmove(dir.name, r->name, NAMELEN);
	dir.qid = r->qid;
	dir.mode = r->perm;
	dir.length = 0;
	dir.hlength = 0;
	strcpy(dir.uid, r->user);
	strcpy(dir.gid, r->group);
	dir.atime = r->atime;
	dir.mtime = r->mtime;
	convD2M(&dir, buf);
}

int
perm(Fid *f, File *r, int p)
{
	if((p*Pother) & r->perm)
		return 1;
	if(strcmp(f->user, r->group)==0 && ((p*Pgroup) & r->perm))
		return 1;
	if(strcmp(f->user, r->user)==0 && ((p*Powner) & r->perm))
		return 1;
	return 0;
}

void
send9p(Fcall *fc)
{
	char	mdata[MAXMSG+MAXFDATA];
	int	nsend;

	nsend = convS2M(fc, mdata);
	if(write(mfd[1], mdata, nsend) != nsend)
		error("mount write");
}

/*
 * we come here locked
 */
void
answerrequest(Fid *f, char *data, int size)
{
	Readq	*rr;
	int	cnt;

	rr = f->readq;
	if(rr) {
		rr->thdr.data = data;
		cnt = min(rr->thdr.count, size);
		rr->thdr.count = cnt;
		send9p(&rr->thdr);
		f->readq = rr->next;
		freereadq(rr);
	}
}

void
freereadq(Readq *rr)
{
	lock(&lmalloc);
	if(rr->t == Rdata)
		free(rr->pktdata);
	free(rr);
	unlock(&lmalloc);
}

/*
 * We come here locked
 */
void
queuedata(Fid *f, char *data, int size)
{
	Readq	*rr, *r, **prev;

	rr = (Readq *) emalloc(sizeof(Readq));
	rr->pktdata = (char *) emalloc(size);
	memcpy(rr->pktdata, data, size);
	rr->pktsize = size;
	rr->t = Rdata;
	/*
	 * preserve the sequence order
	 */
	rr->next = 0;
	prev = &f->readq;
	for(r = f->readq; r; prev = &r->next, r = r->next)
			;
	*prev = rr;
}

void
io(void)
{
	char	*err;
	char	mdata[MAXMSG+MAXFDATA];
	int n;

	for(;;){
		/*
		 * reading from a pipe or a network device
		 * will give an error after a few eof reads
		 * however, we cannot tell the difference
		 * between a zero-length read and an interrupt
		 * on the processes writing to us,
		 * so we wait for the error
		 */
		n = read(mfd[0], mdata, sizeof mdata);
		if(n == 0)
			continue;
		if(n < 0)
			error("mount read");
		if(convM2S(mdata, &rhdr, n) == 0)
			continue;

/*		fprint(2, "icmpfs:%F\n", &rhdr);/**/

		thdr.data = mdata + MAXMSG;
		if(!fcalls[rhdr.type])
			err = "bad fcall type";
		else
			err = (*fcalls[rhdr.type])(newfid(rhdr.fid));
		if(err){
			thdr.type = Rerror;
			strncpy(thdr.ename, err, ERRLEN);
		}else{
			thdr.type = rhdr.type + 1;
			thdr.fid = rhdr.fid;
		}
		if(err != EDontreply) {
			thdr.tag = rhdr.tag;
			send9p(&thdr);
		}
	}
}

ushort
ip_cksum(uchar *addr, int len)
{
	ulong sum = 0;

	while(len > 0) {
		sum += addr[0]<<8 | addr[1] ;
		len -= 2;
		addr += 2;
	}
	sum = (sum & 0xffff) + (sum >> 16);
	sum = (sum & 0xffff) + (sum >> 16);
	return (sum^0xffff);
}

void
mkechoreply(Icmp *q, Icmp *p, int n)
{
	memcpy(q, p, n);
	memcpy(q->dst, p->src, sizeof(q->dst));
	memcpy(q->src, p->dst, sizeof(q->src));
	q->type = EchoReply;
	memset(q->cksum, 0, sizeof(q->cksum));
	hnputs(q->cksum, ip_cksum(&q->type, n - ETHERIPHDR));
}

void
goticmpkt(char *q, int n)
{
	Icmp	*p;
	Fid	*f;
	ushort	recid;

	p = (Icmp *) q;
	recid = nhgets(p->icmpid);
	for(f = fids; f; f = f->next) {
		/*
		 * if there are outstanding read requests, answer them
		 */
		lock(&f->flock);
		if(f->busy && n > 0 && f->readq &&
		      f->readq->t == Rrequest && f->fid == recid) {
			answerrequest(f, q, n);
			unlock(&f->flock);
			return;
		}
		unlock(&f->flock);
	}
	/*
	 * otherwise, queue up the data
	 */
	for(f = fids; f; f = f->next) {
		lock(&f->flock);
		if(f->busy && n > 0 && f->fid == recid) {
			queuedata(f, q, n);
			break;
		}
		unlock(&f->flock);
	}
}

void
loop(void)
{
	int	n;
	char	inbuf[Bufsize], outbuf[Bufsize];
	Icmp	*p;

	p = (Icmp*) inbuf;
	while((n = read(icmpfd, inbuf, sizeof(inbuf))) > 0) {
		if(ip_cksum(&p->type, n - ETHERIPHDR))
			continue;
		switch(p->type) {
		case EchoRequest:
			mkechoreply((Icmp*) outbuf, p, n);
			if(write(icmpfd, outbuf, n) < 0)
				error("write to icmpfd:");
			break;
		default:
			goticmpkt(inbuf, n);
		}
	}
}

ushort
nhgets(uchar *ptr)
{
	return (ptr[0] << 8) + ptr[1];
}

void
hnputs(uchar *ptr, ushort val)
{
	ptr[0] = val>>8;
	ptr[1] = val;
}

void
error(char *s)
{
	fprint(2, "%s: %s: %r\n", argv0, s);
	exits(s);
}

void
usage(void)
{
	fprint(2, "usage: %s [-s] [-m mountpoint]\n", argv0);
	exits("usage");
}

void *
emalloc(ulong size)
{
	void *ret;

	lock(&lmalloc);
		ret = malloc(size);
	unlock(&lmalloc);
	if(ret == 0)
		error("cannot malloc");
	return ret;
}
