#include <u.h>
#include <libc.h>
#include <fcall.h>

#include "cformat.h"
#include "lru.h"
#include "bcache.h"
#include "disk.h"
#include "inode.h"
#include "file.h"

enum
{
	Nfid=		1024,
};

ulong	path;		/* incremented for each new file */

typedef struct Mfile Mfile;
typedef struct Ram Ram;
typedef struct P9fs P9fs;

struct Mfile
{
	Qid	qid;
	short	oldfid;
	char	needclone;
	char	busy;
};

Mfile	mfile[Nfid];
char	user[NAMELEN];
Icache	ic;
int	debug;

struct P9fs
{
	int	fd[2];
	Fcall	rhdr;
	Fcall	thdr;
	long	len;
	char	*name;
};

P9fs	c;	/* client conversation */
P9fs	s;	/* server conversation */

char	datasnd[MAXFDATA + MAXMSG];
char	datarcv[MAXFDATA + MAXMSG];

void	rflush(void);
void	rattach(Mfile*);
void	rauth(void);
void	rclone(Mfile*);
void	rwalk(Mfile*);
void	rclwalk(Mfile*);
void	ropen(Mfile*);
void	rcreate(Mfile*);
void	rread(Mfile*);
void	rwrite(Mfile*);
void	rclunk(Mfile*);
void	rremove(Mfile*);
void	rstat(Mfile*);
void	rwstat(Mfile*);
void	error(char*);
void	warning(char*);
void	mountinit(char*, char*);
void	io(void);
void	sendreply(char*);
void	sendmsg(P9fs*, Fcall*);
void	rcvmsg(P9fs*, Fcall*);
int	delegate(void);
int	askserver(void);
void	cachesetup(int, char*);
int	doclone(Mfile*);
void	doclwalk(Mfile*, char*);

#define PREFACE(m) if(doclone(m) < 0) return

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
	[Tclwalk]	"Tclwalk",
	[Tauth]		"Tauth",
	[Rnop]		"Rnop",
	[Rsession]	"Rsession",
	[Rerror]	"Rerror",
	[Rflush]	"Rflush",
	[Rattach]	"Rattach",
	[Rclone]	"Rclone",
	[Rwalk]		"Rwalk",
	[Ropen]		"Ropen",
	[Rcreate]	"Rcreate",
	[Rclunk]	"Rclunk",
	[Rread]		"Rread",
	[Rwrite]	"Rwrite",
	[Rremove]	"Rremove",
	[Rstat]		"Rstat",
	[Rwstat]	"Rwstat",
	[Rclwalk]	"Rclwalk",
	[Rauth]		"Rauth",
			0,
};

void
usage(void)
{
	fprint(2, "usage:\tcfs -s [-rd] [-f partition]");
	fprint(2, "\tcfs [-rd] [-f partition] [-a netaddr] [mt-pt]\n");
}

void
main(int argc, char *argv[])
{
	int std;
	int format;
	char *part;
	char *server;
	char *mtpt;

	std = 0;
	format = 0;
	part = "/dev/hd0cache";
	server = "dk!bootes";
	mtpt = "/tmp";

	ARGBEGIN{
	case 'a':
		server = ARGF();
		if(server == 0)
			usage();
		break;
	case 's':
		std = 1;
		break;
	case 'r':
		format = 1;
		break;
	case 'f':
		part = ARGF();
		if(part == 0)
			usage();
		break;
	case 'd':
		debug = 1;
		break;
	default:
		usage();
	}ARGEND
	if(argc && *argv)
		mtpt = *argv;

	cachesetup(format, part);

	c.name = "client";
	s.name = "server";
	if(std){
		c.fd[0] = c.fd[1] = 1;
		s.fd[0] = s.fd[1] = 0;
	}else
		mountinit(server, mtpt);

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
cachesetup(int format, char *partition)
{
	int f;
	int secsize;
	int inodes;
	int blocksize;

	secsize = 512;
	inodes = 1024;
	blocksize = 4*1024;

	f = open(partition, ORDWR);
	if(f < 0)
		error("opening partition");

	if(format || iinit(&ic, f, secsize)<0){
		if(iformat(&ic, f, inodes, "bootes", blocksize, secsize) < 0)
			error("formatting failed");
	}
}

void
mountinit(char *server, char *mountpoint)
{
	int p[2];

	/*
	 *  grab a channel and call up the file server
	 */
	s.fd[0] = s.fd[1] = dial(netmkaddr(server, 0, 0), 0, 0, 0);
	if(s.fd[0] < 0)
		error("opening data");

	/*
 	 *  mount onto name space
	 */
	if(pipe(p) < 0)
		error("pipe failed");
	switch(fork()){
	case 0:
		if(mount(p[1], mountpoint, MREPL|MCREATE, "", "") < 0)
			error("mount failed");
		exits(0);
	case -1:
		error("fork failed\n");
/*BUG: no wait!*/
	}
	c.fd[0] = c.fd[1] = p[0];
}

void
io(void)
{
	Mfile *mf;
    loop:
	rcvmsg(&c, &c.thdr);
	mf = &mfile[c.thdr.fid];
	switch(c.thdr.type){
	default:
		error("type");
		break;
	case Tauth:
		rauth();
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
	case Tattach:
		rattach(mf);
		break;
	case Tclone:
		rclone(mf);
		break;
	case Twalk:
		rwalk(mf);
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
	goto loop;
}

void
rflush(void)		/* synchronous so easy */
{
	sendreply(0);
}

void
rattach(Mfile *mf)
{
	if(delegate() == 0){
		mf->qid = s.rhdr.qid;
		mf->busy = 1;
		mf->needclone = 0;
	}
}

/*
 *  check consistency and perform a delayed clone
 */
int
doclone(Mfile *mf)
{
	Mfile *omf;

	if(!mf->busy)
		error("bad fid");
	if(!mf->needclone)
		return 0;
	omf = &mfile[mf->oldfid];
	if(!omf->busy)
		error("bad old fid");

	s.thdr.type = Tclone;
	s.thdr.fid = mf->oldfid;
	s.thdr.newfid = mf - mfile;
	mf->needclone = 0;
	if(askserver() == 0)
		return 0;
	sendreply(s.rhdr.ename);
	return -1;
}

/*
 *  don't send clone to server, just reply to client
 */
void
rclone(Mfile *mf)
{
	Mfile *nmf;

	PREFACE(mf);
	if(c.thdr.newfid<0 || Nfid<=c.thdr.newfid)
		error("clone nfid out of range");
	nmf = &mfile[c.thdr.newfid];
	if(nmf->busy)
		error("clone to used channel");
	nmf = &mfile[c.thdr.newfid];
	nmf->qid = mf->qid;
	nmf->needclone = 1;
	nmf->oldfid = mf - mfile;
	nmf->busy = 1;
	sendreply(0);
}

/*
 *  do a combined clone and walk. An error implies a clunk.  A reply with
 *  the wrong fid implies a "directory entry not found".
 */
void
doclwalk(Mfile *mf, char *name)
{
	s.thdr.type = Tclwalk;
	s.thdr.fid = mf->oldfid;
	s.thdr.newfid = mf - mfile;
	memmove(s.thdr.name, name, sizeof s.thdr.name);
	if(askserver() == 0){
		if(s.rhdr.fid == s.thdr.fid){
			/*
			 *  this is really a short form of
			 *	Terror "directory entry not found"
			 */
			mf->busy = 0;
			sendreply("directory entry not found");
			return;
		}
		mf->qid = s.rhdr.qid;
		c.rhdr.qid = s.rhdr.qid;
		mf->needclone = 0;
		sendreply(0);
	} else {
		mf->busy = 0;
		sendreply(s.rhdr.ename);
	}
}

void
rwalk(Mfile *mf)
{
	if(mf->needclone){
		doclwalk(mf, c.thdr.name);
		return;
	}
	if(delegate() == 0)
		mf->qid = s.rhdr.qid;
}

void
rclwalk(Mfile *mf)
{
	if(delegate() == 0)
		mf->qid = s.rhdr.qid;
}

void
ropen(Mfile *mf)
{
	PREFACE(mf);
	if(delegate() == 0){
		mf->qid = s.rhdr.qid;
		if(c.thdr.mode & OTRUNC){
			mf->qid.vers++;
			iget(&ic, mf->qid);
		}
	}
}

void
rcreate(Mfile *mf)
{
	PREFACE(mf);
	if(delegate() == 0){
		mf->qid = s.rhdr.qid;
		mf->qid.vers++;
	}
}

void
rauth(void)
{
	delegate();
}

void
rclunk(Mfile *mf)
{
	if(!mf->busy){
		sendreply(0);
		return;
	}
	mf->busy = 0;
	mf->needclone = 0;
	delegate();
}

void
rremove(Mfile *mf)
{
	PREFACE(mf);
	mf->busy = 0;
	delegate();
}

void
rread(Mfile *mf)
{
	int cnt;
	long off, first;
	char *cp;
	Ibuf *b;
	long n;
	char data[MAXFDATA];
	int done;

	PREFACE(mf);
	first = off = c.thdr.offset;
	cnt = c.thdr.count;

	if(mf->qid.path & CHDIR){
		delegate();
		return;
	}

	b = iget(&ic, mf->qid);
	if(b == 0){
		delegate();
		return;
	}

	cp = data;
	done = 0;
	while(cnt>0 && !done){
		n = fread(&ic, b, cp, off, cnt);
		if(n <= 0){
			n = -n;
			if(n==0 || n>cnt)
				n = cnt;
			s.thdr.type = c.thdr.type;
			s.thdr.fid = c.thdr.fid;
			s.thdr.tag = c.thdr.tag;
			s.thdr.offset = off;
			s.thdr.count = n;
			if(askserver() < 0)
				sendreply(s.rhdr.ename);
			if(s.rhdr.count != n)
				done = 1;
			n = s.rhdr.count;
			memmove(cp, s.rhdr.data, n);
			fwrite(&ic, b, cp, off, n);
		}
		cnt -= n;
		off += n;
		cp += n;
	}
	c.rhdr.data = data;
	c.rhdr.count = off - first;
	sendreply(0);
}

void
rwrite(Mfile *mf)
{
	Ibuf *b;
	char buf[MAXFDATA];

	PREFACE(mf);
	if(mf->qid.path & CHDIR){
		delegate();
		return;
	}

	memmove(buf, c.thdr.data, c.thdr.count);
	if(delegate() < 0)
		return;

	b = iget(&ic, mf->qid);
	if(b == 0)
		return;
	mf->qid.vers++;
	if(fwrite(&ic, b, buf, c.thdr.offset, c.thdr.count) == c.thdr.count)
		iinc(&ic, b);
}

void
rstat(Mfile *mf)
{
	PREFACE(mf);
	delegate();
}

void
rwstat(Mfile *mf)
{
	PREFACE(mf);
	delegate();
}

void
error(char *s)
{
	fprint(2, "cfs: %s: ", s);
	perror("");
	exits(s);
}

void
warning(char *s)
{
	char buf[ERRLEN];

	errstr(buf);
	fprint(2, "cfs: %s: %s\n", s, buf);
	if(strstr(buf, "illegal network address"))
		exits("death");
}

/*
 *  send a reply to the client
 */
void
sendreply(char *err)
{

	if(err){
		c.rhdr.type = Rerror;
		strncpy(c.rhdr.ename, err, ERRLEN);
	}else{
		c.rhdr.type = c.thdr.type+1;
		c.rhdr.fid = c.thdr.fid;
	}
	c.rhdr.tag = c.thdr.tag;
	sendmsg(&c, &c.rhdr);
}

/*
 *  send a request to the server, get the reply, and send that to
 *  the client
 */
int
delegate(void)
{
	sendmsg(&s, &c.thdr);
	rcvmsg(&s, &s.rhdr);
	sendmsg(&c, &s.rhdr);
	return c.thdr.type+1 == s.rhdr.type ? 0 : -1;
}

/*
 *  send a request to the server and get a reply
 */
int
askserver(void)
{
	s.thdr.tag = c.thdr.tag;
	sendmsg(&s, &s.thdr);
	rcvmsg(&s, &s.rhdr);
	return s.thdr.type+1 == s.rhdr.type ? 0 : -1;
}

/*
 *  send/receive messages with logging
 */
void
sendmsg(P9fs *p, Fcall *f)
{
	DPRINT(2, "->%s: %d %s on %d\n", p->name, f->type,
		mname[f->type]? mname[f->type] : "mystery",
		f->fid);

	p->len = convS2M(f, datasnd);
	if(write(p->fd[1], datasnd, p->len)!=p->len)
		error("smdmsg");
}

void
dump(uchar *p, int len)
{
	fprint(2, "%d bytes", len);
	while(len > 0){
		fprint(2, " %.2ux", *p++);
		len--;
	}
	fprint(2, "\n");
}

void
rcvmsg(P9fs *p, Fcall *f)
{
	int olen;

	olen = p->len;
retry:
	p->len = read(p->fd[0], datarcv, sizeof(datarcv));
	if(p->len <= 0)
		error("rcvmsg");
	if(p->len==2 && datarcv[0]=='O' && datarcv[1]=='K')
		goto retry;
	if(convM2S(datarcv, f, p->len) == 0)
		error("rcvmsg format error");
	if(f->type != Rauth && (f->fid<0 || Nfid<=f->fid)){
		fprint(2, "<-%s: %d %s on %d\n", p->name, f->type,
			mname[f->type]? mname[f->type] : "mystery",
			f->fid);
		dump((uchar*)datasnd, olen);
		dump((uchar*)datarcv, p->len);
		error("rcvmsg fid out of range");
	}
	DPRINT(2, "<-%s: %d %s on %d\n", p->name, f->type,
		mname[f->type]? mname[f->type] : "mystery",
		f->fid);
}
