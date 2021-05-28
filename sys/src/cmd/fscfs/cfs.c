#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include <thread.h>

/*
 * to do:
 *	- concurrency?
 *		- only worthwhile with several connections, perhaps to several file servers
 *	- cache directories
 *	- message size (dynamic buffers)
 *	- more useful stats
 *	- notes
 *	- lru for sfids in paths (also correct ref counts for paths)
 */

#include "dat.h"
#include "fns.h"
#include "stats.h"

#define DPRINT if(debug)fprint

/* maximum length of a file (used for unknown lengths) */
enum { MAXLEN = ((uvlong)1<<63)-1 };

int	debug, statson, noauth, openserver;

#define	MAXFDATA	8192	/* i/o size for read/write */

struct P9fs
{
	int	fd[2];
	Fcall	thdr;
	Fcall	rhdr;
	uchar	sndbuf[MAXFDATA+IOHDRSZ];	/* TO DO: dynamic */
	uchar	rcvbuf[2][MAXFDATA + IOHDRSZ];	/* extra buffer to protect client data over server calls */
	int	cb;	/* current buffer */
	long	len;
	char	*name;
};

P9fs	c;	/* client conversation */
P9fs	s;	/* server conversation */

Host	host[1];	/* clients, just the one for now */

Cfsstat  cfsstat, cfsprev;
char	statbuf[2048];
int	statlen;

int		messagesize = MAXFDATA+IOHDRSZ;	/* must be the same for all connections */

Qid	rootqid;
Qid	ctlqid = {0x5555555555555555LL, 0, 0};

uint	cachesize = 16 * 1024 * 1024;

/*
 * clients directly access directories, auth files, append-only files, exclusive-use files, and mount points
 */
#define	isdirect(qid)	(((qid).type & ~QTTMP) != QTFILE)

static char Efidinuse[] = "fid in use";
static char Ebadfid[] = "invalid fid";
static char Enotdir[] = "not a directory";
static char Enonexist[] = "file does not exist";
static char Eexist[] = "file already exists";
static char Eopened[] = "opened for I/O";
static char Emode[] = "bad open mode";
static char Eremoved[] = "file has been removed";

static SFid*	freefids;
static u32int fidgen;

static Data	datalist;

static Auth*	authlist;

void
usage(void)
{
	fprint(2, "usage: %s [-a netaddr | -F srv] [-dknS] [mntpt] [spec]\n", argv0);
	threadexitsall("usage");
}

void
threadmain(int argc, char *argv[])
{
	int std;
	char *server, *mtpt, *aname;

	std = 0;
	server = "net!$server";
	mtpt = "/n/remote";
	aname = "";

	ARGBEGIN{
	case 'a':
		server = EARGF(usage());
		break;
	case 'd':
		debug = 1;
		break;
	case 'F':
		server = EARGF(usage());
		openserver = 1;
		break;
	case 'm':
		cachesize = atoi(EARGF(usage())) * 1024 * 1024;
		if(cachesize < 8 * 1024 * 1024 ||
		    cachesize > 3750UL * 1024 * 1024)
			sysfatal("implausible cache size %ud", cachesize);
		break;
	case 'n':
		noauth = 1;
		break;
	case 'S':
		statson = 1;
		break;
	case 's':
		std = 1;
		break;
	default:
		usage();
	}ARGEND
	if(argc && *argv)
		mtpt = *argv;
	if(argc > 1 && argv[1])
		aname = argv[1];

	quotefmtinstall();
	if(debug){
		fmtinstall('F', fcallfmt);
		fmtinstall('D', dirfmt);
		fmtinstall('M', dirmodefmt);
	}

	c.name = "client";
	s.name = "server";
	if(std){
		c.fd[0] = c.fd[1] = 1;
		s.fd[0] = s.fd[1] = 0;
	}else
		mountinit(server, mtpt, aname);

	switch(fork()){
	case 0:
		io();
		threadexits("");
	case -1:
		error("fork");
	default:
		_exits("");
	}
}

/*
 * TO DO: need to use libthread variants
 */
void
mountinit(char *server, char *mountpoint, char *aname)
{
	int err;
	int p[2];

	/*
	 *  grab a channel and call up the file server
	 */
	if(openserver){
		s.fd[0] = open(server, ORDWR);
		if(s.fd[0] < 0)
			error("opening srv file %s: %r", server);
	} else {
		s.fd[0] = dial(netmkaddr(server, 0, "9fs"), 0, 0, 0);
		if(s.fd[0] < 0)
			error("dialing %s: %r", server);
	}
	s.fd[1] = s.fd[0];

	/*
 	 *  mount onto name space
	 */
	if(pipe(p) < 0)
		error("pipe failed");
	switch(fork()){
	case 0:
		break;
	default:
		rfork(RFFDG);
		close(p[0]);
		if(noauth)
			err = mount(p[1], -1, mountpoint, MREPL|MCREATE|MCACHE, aname);
		else
			err = amount(p[1], mountpoint, MREPL|MCREATE|MCACHE, aname);
		if(err < 0)
			error("mount failed: %r");
		_exits(0);
	case -1:
		error("fork failed\n");
/*BUG: no wait!*/
	}
	close(p[1]);
	c.fd[0] = c.fd[1] = p[0];
}

void
io(void)
{
	Fcall *t;

	datainit();
	t = &c.thdr;

    loop:
	rcvmsg(&c, t);

	if(statson){
		cfsstat.cm[t->type].n++;
		cfsstat.cm[t->type].s = nsec();
	}
	switch(t->type){
	default:
		sendreply("invalid 9p operation");
		break;
	case Tversion:
		rversion(t);
		break;
	case Tauth:
		rauth(t);
		break;
	case Tflush:
		rflush(t);
		break;
	case Tattach:
		rattach(t);
		break;
	case Twalk:
		rwalk(t);
		break;
	case Topen:
		ropen(t);
		break;
	case Tcreate:
		rcreate(t);
		break;
	case Tread:
		rread(t);
		break;
	case Twrite:
		rwrite(t);
		break;
	case Tclunk:
		rclunk(t);
		break;
	case Tremove:
		rremove(t);
		break;
	case Tstat:
		rstat(t);
		break;
	case Twstat:
		rwstat(t);
		break;
	}
	if(statson){
		cfsstat.cm[t->type].t += nsec() -cfsstat.cm[t->type].s;
	}
	goto loop;
}

void
rversion(Fcall *t)
{
	if(messagesize > t->msize)
		messagesize = t->msize;
	t->msize = messagesize;	/* set downstream size */
	delegate(t, nil, nil);
}

void
rauth(Fcall *t)
{
	Fid *mf;

	mf = findfid(host, t->afid, 1);
	if(mf == nil)
		return;
	mf->path = newpath(nil, "#auth", (Qid){0, 0, 0});	/* path name is never used */
	mf->opened = allocsfid();	/* newly allocated */
	if(delegate(t, mf, nil) == 0){
		mf->qid = s.rhdr.aqid;
		mf->mode = ORDWR;
	}else{
		freesfid(mf->opened);	/* free, don't clunk: failed to establish */
		mf->opened = nil;
		putfid(mf);
	}
}

void
rflush(Fcall*)		/* synchronous so easy */
{
	/*TO DO: need a tag hash to find requests, once auth is asynchronous */
	sendreply(0);
}

void
rattach(Fcall *t)
{
	Fid *mf, *afid;
	SFid *sfid;

	mf = findfid(host, t->fid, 1);
	if(mf == nil)
		return;
	sfid = nil;
	if(t->afid != NOFID){
		afid = findfid(host, t->afid, 0);
		if(afid == nil)
			return;
		sfid = afid->opened;
		if((afid->qid.type & QTAUTH) == 0 || sfid == nil){
			sendreply("bad auth file");
			return;
		}
	}
	mf->path = newpath(nil, "", (Qid){0, 0, 0});
	mf->path->sfid = allocsfid();	/* newly allocated */
	if(delegate(t, mf, sfid) == 0){
		mf->qid = s.rhdr.qid;
		mf->path->qid = mf->qid;
		if(statson == 1){
			statson++;
			rootqid = mf->qid;
		}
	}else{
		freesfid(mf->path->sfid);	/* free, don't clunk: failed to establish */
		mf->path->sfid = nil;
		putfid(mf);
	}
}

void
rwalk(Fcall *t)
{
	Fid *mf, *nmf;
	SFid *sfid;
	Path *p;
	char *n;
	int i;

	mf = findfid(host, t->fid, 0);
	if(mf == nil)
		return;
	if(mf->path){
		DPRINT(2, "walk from  %p %q + %d\n", mf, pathstr(mf->path), t->nwname);
	}
	if(mf->path->inval){
		sendreply(mf->path->inval);
		return;
	}
	nmf = nil;
	if(t->newfid != t->fid){
		nmf = findfid(host, t->newfid, 1);
		if(nmf == nil)
			return;
		nmf->qid = mf->qid;
		if(nmf->path != nil)
			freepath(nmf->path);
		mf->path->ref++;
		nmf->path = mf->path;
		mf = nmf; /* Walk mf */
	}

	if(statson &&
	   mf->qid.type == rootqid.type && mf->qid.path == rootqid.path &&
	   t->nwname == 1 && strcmp(t->wname[0], "cfsctl") == 0){
		/* This is the ctl file */
		mf->qid = ctlqid;
		c.rhdr.nwqid = 1;
		c.rhdr.wqid[0] = ctlqid;
		sendreply(0);
		return;
	}

	/*
	 * need this as an invariant, and it should always be true, because
	 * Twalk.fid should exist and thus have sfid. could compensate by
	 * rewalking from nearest parent with sfid (in the limit, root from attach)
	 */
	assert(mf->path->sfid != nil);

	if(t->nwname == 0){
		/* new local fid (if any) refers to existing path, shares its sfid via that path */
		c.rhdr.nwqid = 0;
		sendreply(0);
		return;
	}

	/* t->nwname != 0 */

	if(localwalk(mf, &c.thdr, &c.rhdr, &p)){
		/* it all worked or is known to fail (in all or in part) */
		if(c.rhdr.type == Rerror){
			DPRINT(2, "walk error: %q\n", c.rhdr.ename);
			sendreply(c.rhdr.ename);
			if(nmf != nil)
				putfid(nmf);
			return;
		}
		if(c.rhdr.nwqid != t->nwname){
			DPRINT(2, "partial walk, hit error\n");
			sendreply(0);
			if(nmf != nil)
				putfid(nmf);
			return;
		}
		DPRINT(2, "seen it: %q %q\n", pathstr(p), p->inval);
		if(p->sfid != nil){
			p->ref++;
			freepath(mf->path);
			mf->path = p;
			if(c.rhdr.nwqid > 0)
				mf->qid = c.rhdr.wqid[c.rhdr.nwqid-1];
			sendreply(0);
			return;
		}
		/* know the path, but need a fid on the server */
	}

	/* walk to a new server fid (ie, server always sees fid != newfid) */
	sfid = allocsfid();	/* must release it, if walk doesn't work */

	if(delegate(t, mf, sfid) < 0){	/* complete failure */
		if(t->nwname > 0){	/* cache first item's failure */
			DPRINT(2, "bad path: %q + %q -> %q\n", pathstr(mf->path), t->wname[0], s.rhdr.ename);
			badpath(mf->path, t->wname[0], s.rhdr.ename);
		}
		if(nmf != nil)
			putfid(nmf);
		freesfid(sfid);	/* no need to clunk it, since it hasn't been assigned */
		return;
	}

	if(s.rhdr.nwqid == t->nwname){	/* complete success */
		for(i = 0; i < t->nwname; i++){
			n = t->wname[i];
			if(strcmp(n, "..") == 0){
				p = mf->path->parent;
				if(p != nil){	/* otherwise at root, no change */
					p->ref++;
					freepath(mf->path);
					mf->path = p;
				}
			}else
				mf->path = newpath(mf->path, n, s.rhdr.wqid[i]);
		}
		mf->qid = s.rhdr.wqid[s.rhdr.nwqid-1];	/* nwname != 0 (above) */
		if(mf->path->sfid != nil){
			/* shouldn't happen (otherwise we wouldn't walk, because localwalk would find it) */
			sysfatal("rwalk: unexpected sfid: %q -> %ud\n", pathstr(mf->path), mf->path->sfid->fid);
		}
		mf->path->sfid = sfid;
		return;
	}

	/* partial success; note success and failure, and release fid */
	p = mf->path;
	for(i = 0; i < s.rhdr.nwqid; i++){
		n = t->wname[i];
		if(strcmp(n, "..") == 0){
			if(p->parent != nil)
				p = p->parent;
		}else
			p = newpath(p, n, s.rhdr.wqid[i]);
	}
	n = t->wname[i];
	if(i == 0 || s.rhdr.wqid[i-1].type & QTDIR)
		badpath(p, n, Enonexist);
	if(nmf != nil)
		putfid(nmf);
	freesfid(sfid);
}

int
localwalk(Fid *mf, Fcall *t, Fcall *r, Path **pp)
{
	Path *p, *q;
	char *n, *err;
	int i;

	*pp = nil;
	if(t->nwname == 0)
		return 0;	/* clone */
	if(mf->path == nil)
		return 0;
	r->type = Rwalk;
	r->nwqid = 0;
	p = mf->path;
	for(i = 0; i < t->nwname; i++){
		n = t->wname[i];
		if((err = p->inval) != nil)
			goto Err;
		if((p->qid.type & QTDIR) == 0){
			err = Enotdir;
			goto Err;
		}
		if(strcmp(n, "..") == 0){
			if((q = p->parent) == nil)
				q = p;
		}else{
			for(q = p->child; q != nil; q = q->next){
				if(strcmp(n, q->name) == 0){
					if((err = q->inval) != nil)
						goto Err;
					break;
				}
			}
			if(q == nil)
				return 0;	/* not found in cache, must try server */
		}
		r->wqid[r->nwqid++] = q->qid;
		p = q;
	}
	/* found them all: if p's not locally NOFID, link incoming fid to it as local value */
	*pp = p;
	return 1;

Err:
	if(r->nwqid == 0){
		r->type = Rerror;
		r->ename = err;
	}
	return 1;
}

void
ropen(Fcall *t)
{
	Fid *mf;
	SFid *sfid;
	int omode;
	File *file;

	mf = findfid(host, t->fid, 0);
	if(mf == nil)
		return;
	if(mf->opened != nil){
		sendreply(Eopened);
		return;
	}
	omode = openmode(t->mode);
	if(omode < 0){
		sendreply(Emode);
		return;
	}
	if(statson && ctltest(mf)){
		/* Opening ctl file */
		if(t->mode != OREAD){
			sendreply("permission denied");
			return;
		}
		mf->mode = OREAD;
		c.rhdr.qid = ctlqid;
		c.rhdr.iounit = 0;
		sendreply(0);
		genstats();
		return;
	}
	if(t->mode & (ORCLOSE|OTRUNC) || isdirect(mf->qid)){
		/* must have private sfid */
		DPRINT(2, "open dir/auth: %q\n", pathstr(mf->path));
		sfid = sfclone(mf->path->sfid);
		if(delegate(t, mf, sfid) < 0){
			sfclunk(sfid);
			return;
		}
		mf->qid = s.rhdr.qid;
		/* don't currently need iounit */
	}else if((sfid = alreadyopen(mf, omode)) == nil){
		sfid = sfclone(mf->path->sfid);
		DPRINT(2, "new open %q -> %ud\n", pathstr(mf->path), sfid->fid);
		if(delegate(t, mf, sfid) < 0){
			sfclunk(sfid);
			return;
		}
		if(mf->qid.type != s.rhdr.qid.type || mf->qid.path != s.rhdr.qid.path){
			/* file changed type or naming is volatile */
			print("file changed underfoot: %q %#ux %llud -> %#ux %llud\n",
				pathstr(mf->path), mf->qid.type, mf->qid.path, s.rhdr.qid.type, s.rhdr.qid.path);
			mf->path->qid = mf->qid;
			fileinval(mf->path);
		}
		mf->qid = s.rhdr.qid;	/* picks up vers */
		openfile(mf, omode, s.rhdr.iounit, sfid);
	}else{
		DPRINT(2, "cached open %q -> %ud\n", pathstr(mf->path), sfid->fid);
		c.rhdr.qid = mf->path->file->qid;
		c.rhdr.iounit = mf->path->file->iounit;
		sendreply(0);
	}
	mf->opened = sfid;
	mf->mode = omode;
	if(t->mode & OTRUNC && !(mf->qid.type & QTAPPEND)){
		file = mf->path->file;
		if(file != nil){
			cacheinval(file);	/* discard cached data */
			file->length = 0;
		}
	}
}

void
rcreate(Fcall *t)
{
	Fid *mf;
	SFid *sfid;
	int omode;

	mf = findfid(host, t->fid, 0);
	if(mf == nil)
		return;
	if(statson && ctltest(mf)){
		sendreply(Eexist);
		return;
	}
	omode = openmode(t->mode);
	if(omode < 0){
		sendreply(Emode);
		return;
	}
	/* always write-through, and by definition can't exist and be open */
	sfid = sfclone(mf->path->sfid);
	if(delegate(t, mf, sfid) == 0){
		mf->opened = sfid;
		mf->mode = omode;
		mf->qid = s.rhdr.qid;
		mf->path = newpath(mf->path, t->name, mf->qid);
		if(!(t->mode & ORCLOSE || isdirect(mf->qid))){
			/* can cache (if it's volatile, find out on subsequent open) */
			openfile(mf, omode, s.rhdr.iounit, sfid);
		}
		//mf->iounit = s.rhdr.iounit;
	}else
		sfclunk(sfid);
}

void
rclunk(Fcall *t)
{
	Fid *mf;

	mf = findfid(host, t->fid, 0);
	if(mf == nil)
		return;
	if(mf->opened != nil)
		closefile(mf, 1, 0);
	/* local clunk, not delegated */
	sendreply(0);
	putfid(mf);
}

void
rremove(Fcall *t)
{
	Fid *mf;
	Path *p;
	SFid *sfid;
	File *file;

	mf = findfid(host, t->fid, 0);
	if(mf == nil)
		return;
	/* invalidate path entry; discard file; discard any local fids in use */
	if(statson && ctltest(mf)){
		sendreply("not removed");
		return;
	}
	file = nil;
	p = mf->path;
	if(delegate(t, mf, nil) == 0){
		setinval(p, Eremoved);
		file = p->file;
		p->file = nil;
	}
	/* in any case, the fid was clunked: free the sfid */
	if(mf->opened == nil){
		sfid = p->sfid;
		p->sfid = nil;
		freesfid(sfid);
	}else
		closefile(mf, 0, 1);
	putfid(mf);
	if(file != nil)
		putfile(file);
}

void
rread(Fcall *t)
{
	Fid *mf;
	int cnt, done;
	long n;
	vlong off, first;
	char *cp;
	File *file;
	char data[MAXFDATA];

	mf = findfid(host, t->fid, 0);
	if(mf == nil)
		return;

	off = t->offset;
	cnt = t->count;

	if(statson && ctltest(mf)){
		if(cnt > statlen-off)
			c.rhdr.count = statlen-off;
		else
			c.rhdr.count = cnt;
		if((int)c.rhdr.count < 0){
			c.rhdr.count = 0;
			sendreply(0);
			return;
		}
		c.rhdr.data = statbuf + off;
		sendreply(0);
		return;
	}

	if(mf->opened == nil || mf->mode == OWRITE){
		sendreply("not open for reading");
		return;
	}

	file = mf->path->file;
	if(isdirect(mf->qid) || file == nil){
		DPRINT(2, "delegating read\n");
		delegate(t, mf, nil);
		if(statson){
			if(mf->qid.type & QTDIR)
				cfsstat.ndirread++;
			else
				cfsstat.ndelegateread++;
			if(s.rhdr.count > 0){
				cfsstat.bytesread += s.rhdr.count;
				if(mf->qid.type & QTDIR)
					cfsstat.bytesfromdirs += s.rhdr.count;
				else
					cfsstat.bytesfromserver += s.rhdr.count;
			}
		}
		return;
	}

	first = off;
	cp = data;
	done = 0;
	while(cnt>0 && !done){
		if(off >= file->clength){
			DPRINT(2, "offset %lld >= length %lld\n", off, file->clength);
			break;
		}
		n = cacheread(file, cp, off, cnt);
		if(n <= 0){
			n = -n;
			if(n==0 || n>cnt)
				n = cnt;
			DPRINT(2, "fetch %ld bytes of data from server at offset %lld\n", n, off);
			s.thdr.tag = t->tag;
			s.thdr.type = t->type;
			s.thdr.fid = t->fid;
			s.thdr.offset = off;
			s.thdr.count = n;
			if(statson)
				cfsstat.ndelegateread++;
			if(askserver(t, mf) < 0){
				sendreply(s.rhdr.ename);
				return;
			}
			if(s.rhdr.count != n)
				done = 1;
			n = s.rhdr.count;
			if(n == 0){
				/* end of file */
				if(file->clength > off){
					DPRINT(2, "file %llud.%ld, length %lld\n",
						file->qid.path,
						file->qid.vers, off);
					file->clength = off;
				}
				break;
			}
			memmove(cp, s.rhdr.data, n);
			cachewrite(file, cp, off, n);
			if(statson){
				cfsstat.bytestocache += n;
				cfsstat.bytesfromserver += n;
			}
		}else{
			DPRINT(2, "fetched %ld bytes from cache\n", n);
			if(statson)
				cfsstat.bytesfromcache += n;
		}
		cnt -= n;
		off += n;
		cp += n;
	}
	c.rhdr.data = data;
	c.rhdr.count = off - first;
	if(statson)
		cfsstat.bytesread += c.rhdr.count;
	sendreply(0);
}

void
rwrite(Fcall *t)
{
	Fid *mf;
	File *file;
	u32int v1, v2, count;

	mf = findfid(host, t->fid, 0);
	if(mf == nil)
		return;
	if(statson && ctltest(mf)){
		sendreply("read only");
		return;
	}
	if(mf->opened == nil || mf->mode == OREAD){
		sendreply("not open for writing");
		return;
	}
	file = mf->path->file;
	if(isdirect(mf->qid) || file == nil){
		delegate(t, mf, nil);
		if(statson && s.rhdr.count > 0)
			cfsstat.byteswritten += s.rhdr.count;
		return;
	}

	/* add to local cache as write through */
	if(delegate(t, mf, nil) < 0)
		return;

	count = s.rhdr.count;
	cfsstat.byteswritten += count;

	v1 = mf->qid.vers + 1;
	v2 = mf->path->qid.vers + 1;
	if(v1 > v2)
		v1 = v2;
	mf->qid.vers = v1;
	mf->path->qid.vers = v1;
	file->qid.vers = v1;
	if(file->clength < t->offset + count)
		file->clength = t->offset + count;
	cachewrite(file, t->data, t->offset, count);
}

void
rstat(Fcall *t)
{
	Fid *mf;
	Dir d;
	uchar statbuf[256];

	mf = findfid(host, t->fid, 0);
	if(mf == nil)
		return;
	if(statson && ctltest(mf)){
		genstats();
		d.qid = ctlqid;
		d.mode = 0444;
		d.length = statlen;
		d.name = "cfsctl";
		d.uid = "none";
		d.gid = "none";
		d.muid = "none";
		d.atime = time(nil);
		d.mtime = d.atime;
		c.rhdr.stat = statbuf;
		c.rhdr.nstat = convD2M(&d, statbuf, sizeof(statbuf));
		sendreply(0);
		return;
	}
	if(delegate(t, mf, nil) == 0){
		convM2D(s.rhdr.stat, s.rhdr.nstat , &d, nil);
		//mf->qid = d.qid;
		if(mf->path->file != nil)
			copystat(mf->path->file, &d, 0);
	}
}

void
rwstat(Fcall *t)
{
	Fid *mf;
	Path *p;
	Dir *d;
	int qt;

	mf = findfid(host, t->fid, 0);
	if(mf == nil)
		return;
	if(statson && ctltest(mf)){
		sendreply("read only");
		return;
	}
	if(delegate(t, mf, nil) == 0){
		d = malloc(sizeof(*d)+t->nstat);
		if(convM2D(t->stat, t->nstat, d, (char*)(d+1)) != 0){
			if(*d->name){	/* file renamed */
				p = mf->path;
				free(p->name);
				p->name = strdup(d->name);
				freeinval(p);
			}
			if(d->mode != ~0){
				qt = d->mode>>24;
				mf->qid.type = qt;
				mf->path->qid.type = qt;
			}
			if(mf->path->file != nil)
				copystat(mf->path->file, d, 1);
		}
		free(d);
	}
}

int
openmode(uint o)
{
	uint m;

	m = o & ~(OTRUNC|OCEXEC|ORCLOSE);
	if(m > OEXEC)
		return -1;
	if(m == OEXEC)
		m = OREAD;
	if(o&OTRUNC && m == OREAD)
		return -1;
	return m;
}

void
error(char *fmt, ...)
{
	va_list arg;
	static char buf[2048];

	va_start(arg, fmt);
	vseprint(buf, buf+sizeof(buf), fmt, arg);
	va_end(arg);
	fprint(2, "%s: %s\n", argv0, buf);
	threadexitsall("error");
}

void
warning(char *s)
{
	fprint(2, "%s: %s: %r\n", argv0, s);
}

/*
 *  send a reply to the client
 */
void
sendreply(char *err)
{
	if(err){
		c.rhdr.type = Rerror;
		c.rhdr.ename = err;
	}else{
		c.rhdr.type = c.thdr.type+1;
		c.rhdr.fid = c.thdr.fid;
	}
	c.rhdr.tag = c.thdr.tag;
	sendmsg(&c, &c.rhdr);
}

/*
 * local fids
 */
Fid*
findfid(Host *host, u32int fid, int mk)
{
	Fid *f, **l;

	if(fid == NOFID){
		sendreply(Ebadfid);
		return nil;
	}
	l = &host->fids[fid & (FidHash-1)];
	for(f = *l; f != nil; f = f->next){
		if(f->fid == fid){
			if(mk){
				sendreply(Efidinuse);
				return nil;
			}
			return f;
		}
	}
	if(!mk){
		sendreply(Ebadfid);
		return nil;
	}
	f = mallocz(sizeof(*f), 1);
	f->fid = fid;
	f->next = *l;
	*l = f;
	return f;
}

void
putfid(Fid *f)
{
	Fid **l;
	static Qid zeroqid;

	for(l = &host->fids[f->fid & (FidHash-1)]; *l != nil; l = &(*l)->next){
		if(*l == f){
			*l = f->next;
			break;
		}
	}
	if(f->opened != nil){
		sfclunk(f->opened);
		f->opened = nil;
	}
	freepath(f->path);
	/* poison values just in case */
	f->path = nil;
	f->fid = NOFID;
	f->qid = zeroqid;
	free(f);
}

/*
 * return server fid for local fid
 */
u32int
mapfid(Fid *f)
{
	if(f->opened != nil){
		DPRINT(2, "mapfid: use open fid %ud -> %ud\n", f->fid, f->opened->fid);
		return f->opened->fid;
	}
	if(f->path->sfid == nil)
		sysfatal("mapfid: missing sfid for path %q\n", pathstr(f->path));
	return f->path->sfid->fid;
}

/*
 *  send a request to the server, get the reply,
 * and send that to the client
 */
int
delegate(Fcall *t, Fid *f1, SFid *f2)
{
	int type;

	type = t->type;
	if(statson){
		cfsstat.sm[type].n++;
		cfsstat.sm[type].s = nsec();
	}

	switch(type){
	case Tversion:
	case Tflush:	/* no fid */
		break;
	case Tauth:	/* afid */
		t->afid = mapfid(f1);
		break;
	case Tattach:	/* fid, afid */
		t->fid = mapfid(f1);
		if(f2 != nil)
			t->afid = f2->fid;
		else
			t->afid = NOFID;
		break;
	case Twalk:	/* fid, newfid */
		t->fid = mapfid(f1);
		t->newfid = f2->fid;
		break;
	default:	/* fid */
		if(f2 != nil)
			t->fid = f2->fid;
		else
			t->fid = mapfid(f1);
		break;
	}

	sendmsg(&s, t);
	rcvmsg(&s, &s.rhdr);

	if(statson)
		cfsstat.sm[type].t += nsec() - cfsstat.sm[type].s;

	sendmsg(&c, &s.rhdr);
	return t->type+1 == s.rhdr.type? 0: -1;
}

/*
 *  send an i/o request to the server and get a reply
 */
int
askserver(Fcall *t, Fid *f)
{
	int type;

	s.thdr.tag = t->tag;

	type = s.thdr.type;
	if(statson){
		cfsstat.sm[type].n++;
		cfsstat.sm[type].s = nsec();
	}

	s.thdr.fid = mapfid(f);
	sendmsg(&s, &s.thdr);
	rcvmsg(&s, &s.rhdr);

	if(statson)
		cfsstat.sm[type].t += nsec() - cfsstat.sm[type].s;

	return s.thdr.type+1 == s.rhdr.type ? 0 : -1;
}

/*
 *  send/receive messages with logging
 */

void
sendmsg(P9fs *p, Fcall *f)
{
	DPRINT(2, "->%s: %F\n", p->name, f);

	p->len = convS2M(f, p->sndbuf, messagesize);
	if(p->len <= 0)
		error("convS2M");
	if(write(p->fd[1], p->sndbuf, p->len)!=p->len)
		error("sendmsg");
}

void
rcvmsg(P9fs *p, Fcall *f)
{
	int rlen;
	char buf[128];

	p->len = read9pmsg(p->fd[0], p->rcvbuf[p->cb], sizeof(p->rcvbuf[0]));
	if(p->len <= 0){
		snprint(buf, sizeof buf, "read9pmsg(%d)->%ld: %r",
			p->fd[0], p->len);
		error(buf);
	}

	if((rlen = convM2S(p->rcvbuf[p->cb], p->len, f)) != p->len)
		error("rcvmsg format error, expected length %d, got %d",
			rlen, p->len);
	DPRINT(2, "<-%s: %F\n", p->name, f);
	p->cb = (p->cb+1)%nelem(p->rcvbuf);
}

void
dump(void *a, int len)
{
	uchar *p;

	p = a;
	fprint(2, "%d bytes", len);
	while(len-- > 0)
		fprint(2, " %.2ux", *p++);
	fprint(2, "\n");
}

/*
 * requests (later, unused now)
 */

void
readtmsgproc(void *a)
{
	//uint messagesize;
	int n;
	Req *r;
	Channel *reqs;

	reqs = a;
	for(;;){
		r = malloc(sizeof(*r)+messagesize);
		r->msize = messagesize;
		r->buf = (uchar*)(r+1);
		n = read9pmsg(c.fd[0], r->buf, r->msize);
		if(n <= 0){
			free(r);
			sendp(reqs, nil);
			threadexits(nil);
		}
		if(convM2S(r->buf, n, &r->t) != n){
			free(r);
			error("convM2S error in readtmsgproc");
		}
		sendp(reqs, r);
	}
}

void
readrmsgproc(void *a)
{
	//uint messagesize;
	int n;
	Fcall *r;
	uchar *buf;
	Channel *reps;

	reps = a;
	for(;;){
		r = malloc(sizeof(*r)+messagesize);
		buf = (uchar*)(r+1);
		n = read9pmsg(c.fd[0], buf, messagesize);
		if(n <= 0){
			free(r);
			sendp(reps, nil);
			threadexits(nil);
		}
		if(convM2S(buf, n, r) != n){
			free(r);
			error("convM2S error in readtmsgproc");
		}
		sendp(reps, r);
	}
}

void
freereq(Req *r)
{
	free(r);
}

/*
 * server fids
*/

SFid*
allocsfid(void)
{
	SFid *sf;

	sf = freefids;
	if(sf != nil){
		freefids = sf->next;
		sf->ref = 1;
		return sf;
	}
	sf = mallocz(sizeof(*sf), 1);
	sf->ref = 1;
	sf->fid = ++fidgen;
	return sf;
}

void
freesfid(SFid *sf)
{
	if(--sf->ref != 0)
		return;
	/* leave sf->fid alone */
	sf->next = freefids;
	freefids = sf;
}

SFid*
sfclone(SFid *sf)
{
	Fcall t, r;
	SFid *cf;

	cf = allocsfid();
	t.tag = c.thdr.tag;
	t.type = Twalk;
	t.fid = sf->fid;
	t.newfid = cf->fid;
	t.nwname = 0;
	sendmsg(&s, &t);
	rcvmsg(&s, &r);
	if(r.type == Rerror){
		werrstr("%s", r.ename);
		return nil;
	}
	assert(r.type == Rwalk);	/* TO DO: out of order */
	return cf;
}

Dir*
sfstat(Path *p, u32int tag)
{
	Fcall t, r;
	Dir *d;

	assert(p->sfid != nil);
	t.tag = tag;
	t.type = Tstat;
	t.fid = p->sfid->fid;
	sendmsg(&s, &t);
	rcvmsg(&s, &r);
	if(r.type == Rerror){
		werrstr("%s", r.ename);
		return nil;
	}
	d = malloc(sizeof(*d)+r.nstat);
	if(convM2D(r.stat, r.nstat, d, (char*)(d+1)) == 0){
		free(d);
		werrstr("invalid stat data");
		return nil;
	}
	return d;
}

void
sfclunk(SFid *sf)
{
	Fcall t, r;

	if(sf->ref > 1)
		return;
	t.tag = c.thdr.tag;
	t.type = Tclunk;
	t.fid = sf->fid;
	sendmsg(&s, &t);
	rcvmsg(&s, &r);
	/* don't care about result */
}

void
printpath(Path *p, int level)
{
	Path *q;

	for(int i = 0; i < level; i++)
		print("\t");
	print("%q [%lud] %q\n", p->name, p->ref, p->inval?p->inval:"");
	for(q = p->child; q != nil; q = q->next)
		printpath(q, level+1);
}

int
ctltest(Fid *mf)
{
	return mf->qid.type == ctlqid.type && mf->qid.path == ctlqid.path;
}

char *mname[]={
	[Tversion]		"Tversion",
	[Tauth]	"Tauth",
	[Tflush]	"Tflush",
	[Tattach]	"Tattach",
	[Twalk]		"Twalk",
	[Topen]		"Topen",
	[Tcreate]	"Tcreate",
	[Tclunk]	"Tclunk",
	[Tread]		"Tread",
	[Twrite]	"Twrite",
	[Tremove]	"Tremove",
	[Tstat]		"Tstat",
	[Twstat]	"Twstat",
	[Rversion]	"Rversion",
	[Rauth]	"Rauth",
	[Rerror]	"Rerror",
	[Rflush]	"Rflush",
	[Rattach]	"Rattach",
	[Rwalk]		"Rwalk",
	[Ropen]		"Ropen",
	[Rcreate]	"Rcreate",
	[Rclunk]	"Rclunk",
	[Rread]		"Rread",
	[Rwrite]	"Rwrite",
	[Rremove]	"Rremove",
	[Rstat]		"Rstat",
	[Rwstat]	"Rwstat",
			0,
};

void
genstats(void)
{
	int i;
	char *p;

	p = statbuf;

	p += snprint(p, sizeof statbuf+statbuf-p,
		"        Client                          Server\n");
	p += snprint(p, sizeof statbuf+statbuf-p,
	    "   #calls     Δ  ms/call    Δ      #calls     Δ  ms/call    Δ\n");
	for(i = 0; i < nelem(cfsstat.cm); i++)
		if(cfsstat.cm[i].n || cfsstat.sm[i].n){
			p += snprint(p, sizeof statbuf+statbuf-p,
				"%7lud %7lud ", cfsstat.cm[i].n,
				cfsstat.cm[i].n - cfsprev.cm[i].n);
			if(cfsstat.cm[i].n)
				p += snprint(p, sizeof statbuf+statbuf-p,
					"%7.3f ", 0.000001*cfsstat.cm[i].t/
					cfsstat.cm[i].n);
			else
				p += snprint(p, sizeof statbuf+statbuf-p,
					"        ");
			if(cfsstat.cm[i].n - cfsprev.cm[i].n)
				p += snprint(p, sizeof statbuf+statbuf-p,
					"%7.3f ", 0.000001*
					(cfsstat.cm[i].t - cfsprev.cm[i].t)/
					(cfsstat.cm[i].n - cfsprev.cm[i].n));
			else
				p += snprint(p, sizeof statbuf+statbuf-p,
					"        ");
			p += snprint(p, sizeof statbuf+statbuf-p,
				"%7lud %7lud ", cfsstat.sm[i].n,
				cfsstat.sm[i].n - cfsprev.sm[i].n);
			if(cfsstat.sm[i].n)
				p += snprint(p, sizeof statbuf+statbuf-p,
					"%7.3f ", 0.000001*cfsstat.sm[i].t/
					cfsstat.sm[i].n);
			else
				p += snprint(p, sizeof statbuf+statbuf-p,
					"        ");
			if(cfsstat.sm[i].n - cfsprev.sm[i].n)
				p += snprint(p, sizeof statbuf+statbuf-p,
					"%7.3f ", 0.000001*
					(cfsstat.sm[i].t - cfsprev.sm[i].t)/
					(cfsstat.sm[i].n - cfsprev.sm[i].n));
			else
				p += snprint(p, sizeof statbuf+statbuf-p,
					"        ");
			p += snprint(p, sizeof statbuf+statbuf-p, "%s\n",
				mname[i]);
		}
	p += snprint(p, sizeof statbuf+statbuf-p, "%7lud %7lud ndirread\n",
		cfsstat.ndirread, cfsstat.ndirread - cfsprev.ndirread);
	p += snprint(p, sizeof statbuf+statbuf-p, "%7lud %7lud ndelegateread\n",
		cfsstat.ndelegateread, cfsstat.ndelegateread -
		cfsprev.ndelegateread);
	p += snprint(p, sizeof statbuf+statbuf-p, "%7lud %7lud ninsert\n",
		cfsstat.ninsert, cfsstat.ninsert - cfsprev.ninsert);
	p += snprint(p, sizeof statbuf+statbuf-p, "%7lud %7lud ndelete\n",
		cfsstat.ndelete, cfsstat.ndelete - cfsprev.ndelete);
	p += snprint(p, sizeof statbuf+statbuf-p, "%7lud %7lud nupdate\n",
		cfsstat.nupdate, cfsstat.nupdate - cfsprev.nupdate);

	p += snprint(p, sizeof statbuf+statbuf-p, "%7llud %7llud bytesread\n",
		cfsstat.bytesread, cfsstat.bytesread - cfsprev.bytesread);
	p += snprint(p, sizeof statbuf+statbuf-p, "%7llud %7llud byteswritten\n",
		cfsstat.byteswritten, cfsstat.byteswritten -
		cfsprev.byteswritten);
	p += snprint(p, sizeof statbuf+statbuf-p, "%7llud %7llud bytesfromserver\n",
		cfsstat.bytesfromserver, cfsstat.bytesfromserver -
		cfsprev.bytesfromserver);
	p += snprint(p, sizeof statbuf+statbuf-p, "%7llud %7llud bytesfromdirs\n",
		cfsstat.bytesfromdirs, cfsstat.bytesfromdirs -
		cfsprev.bytesfromdirs);
	p += snprint(p, sizeof statbuf+statbuf-p, "%7llud %7llud bytesfromcache\n",
		cfsstat.bytesfromcache, cfsstat.bytesfromcache -
		cfsprev.bytesfromcache);
	p += snprint(p, sizeof statbuf+statbuf-p, "%7llud %7llud bytestocache\n",
		cfsstat.bytestocache, cfsstat.bytestocache -
		cfsprev.bytestocache);
	statlen = p - statbuf;
	cfsprev = cfsstat;
}

/*
 * paths
 */

Path*
newpath(Path *parent, char *name, Qid qid)
{
	Path *p;

	if(parent != nil){
		for(p = parent->child; p != nil; p = p->next){
			if(strcmp(p->name, name) == 0){
				if(p->inval != nil){
					free(p->inval);
					p->inval = nil;
				}
				p->qid = qid;
				p->ref++;
				return p;
			}
		}
	}
	p = mallocz(sizeof(*p), 1);
	p->ref = 2;
	if(name != nil)
		p->name = strdup(name);
	p->qid = qid;
	p->parent = parent;
	if(parent != nil){
		parent->ref++;
		p->next = parent->child;
		parent->child = p;
	}
	return p;
}

void
freeinval(Path *p)
{
	Path *q, **r;

	for(r = &p->parent->child; (q = *r) != nil; r = &q->next){
		if(q == p)
			continue;
		if(strcmp(q->name, p->name) == 0 && q->inval != nil){
			if(q->ref > 1){
				*r = q->next;
				q->next = p->next;
				p->next = q;
			}
			freepath(q);
			break;
		}
	}
}

void
setinval(Path *p, char *err)
{
	if(p->inval != nil){
		free(p->inval);
		p->inval = nil;
	}
	if(err != nil)
		p->inval = strdup(err);
}

void
badpath(Path *parent, char *name, char *err)
{
	Path *p;

	for(p = parent->child; p != nil; p = p->next){
		if(strcmp(p->name, name) == 0){
			setinval(p, err);
			return;
		}
	}
	p = mallocz(sizeof(*p), 1);
	p->ref = 2;
	if(name != nil)
		p->name = strdup(name);
	p->inval = strdup(err);
	p->parent = parent;
	if(parent != nil){
		parent->ref++;
		p->next = parent->child;
		parent->child = p;
	}
}

void
fileinval(Path *p)
{
	if(p->file != nil){
		putfile(p->file);
		p->file = nil;
	}
}

void
freepath(Path *p)
{
	Path *q, **r;

	while(p != nil && --p->ref == 0){
		if(p->child != nil)
			error("freepath child");
		q = p->parent;
		if(q != nil){
			for(r = &q->child; *r != nil; r = &(*r)->next){
				if(*r == p){
					*r = p->next;
					break;
				}
			}
		}
		if(p->sfid != nil){
			/* TO DO: could queue these for a great clunking proc */
			sfclunk(p->sfid);
			p->sfid = nil;
		}
		if(p->inval != nil)
			free(p->inval);
		if(p->file != nil)
			putfile(p->file);
		free(p->name);
		free(p);
		p = q;
	}
}

static char*
pathstr1(Path *p, char *ep)
{
	ep -= strlen(p->name);
	memmove(ep, p->name, strlen(p->name));
	if(p->parent != nil){
		*--ep = '/';
		ep = pathstr1(p->parent, ep);
	}
	return ep;
}

char*
pathstr(Path *p)
{
	static char buf[1000];
	return pathstr1(p, buf+sizeof(buf)-1);
}

/*
 * files
 */

void
openfile(Fid *mf, int omode, u32int iounit, SFid *sfid)
{
	/* open mf and cache open File at p */
	Path *p;
	File *file;
	SFid *osfid;

	p = mf->path;
	file = p->file;
	if(file == nil){
		file = mallocz(sizeof(*file), 1);
		file->ref = 1;
		file->qid = mf->qid;	/* TO DO: check for clone files */
		file->clength = MAXLEN;
		p->file = file;
	}
	osfid = file->open[omode];
	if(osfid != nil){
		DPRINT(2, "openfile: existing sfid %ud open %d\n", osfid->fid, omode);
		return;
	}
	DPRINT(2, "openfile: cached %ud for %q mode %d\n", sfid->fid, pathstr(p), omode);
	sfid->ref++;
	file->open[omode] = sfid;
	if(file->iounit == 0)
		file->iounit = iounit;	/* BUG: might vary */
}

void
closefile(Fid *mf, int mustclunk, int removed)
{
	Path *p;
	File *file;
	SFid *sfid, *osfid;

	if((sfid = mf->opened) == nil)
		return;
	p = mf->path;
	file = p->file;
	if(file == nil){	/* TO DO: messy */
		mf->opened = nil;
		if(sfid->ref == 1 && mustclunk)
			sfclunk(sfid);
		freesfid(sfid);
		return;
	}
	osfid = file->open[mf->mode];
	if(osfid == sfid){
		if(removed || sfid->ref == 1)
			file->open[mf->mode] = nil;
		if(sfid->ref == 1){
			if(mustclunk)
				sfclunk(sfid);
			freesfid(sfid);
		}else
			sfid->ref--;
	}
	mf->opened = nil;
}

SFid*
alreadyopen(Fid *mf, uint mode)
{
	File *file;
	SFid *sfid;

	file = mf->path->file;
	if(file == nil)
		return nil;
	sfid = file->open[mode&3];
	if(sfid == nil)
		return nil;
	DPRINT(2, "openfile: existing sfid %ud open %d\n", sfid->fid, mode&3);
	sfid->ref++;
	return sfid;
}

void
copystat(File *file, Dir *d, int iswstat)
{
	if(d->length != ~(vlong)0){
		/* length change: discard cached data */
		if(iswstat && file->length < file->clength)
			cacheinval(file);
		file->length = d->length;
	}
	if(d->mode != ~0){
		file->mode = d->mode;
		file->qid.type = d->mode>>24;
	}
	if(d->atime != ~0)
		file->atime = d->atime;
	if(d->mtime != ~0)
		file->mtime = d->mtime;
	if(*d->uid){
		freestr(file->uid);
		file->uid = newstr(d->uid);
	}
	if(*d->gid){
		freestr(file->gid);
		file->gid = newstr(d->gid);
	}
	if(*d->muid){
		freestr(file->muid);
		file->muid = newstr(d->muid);
	}
}

void
putfile(File *f)
{
	int i;
	SFid *sfid;

	if(--f->ref != 0)
		return;
	for(i = 0; i < nelem(f->open); i++){
		sfid = f->open[i];
		if(sfid != nil){
			f->open[i] = nil;
			sfclunk(sfid);
			freesfid(sfid);
		}
	}
	freestr(f->uid);
	freestr(f->gid);
	freestr(f->muid);
	cacheinval(f);
	free(f->cached);
	free(f);
}

/*
 * data
 */

static	int	radix[] = {10, 12, 14};
static	uint	range[] = {8, 32, 0};
static	uint	cacheused;

void
datainit(void)
{
	datalist.forw = datalist.back = &datalist;
}

Data*
allocdata(File *file, uint n, uint nbytes)
{
	Data *d;

	while(cacheused+nbytes > cachesize && datalist.forw != datalist.back)
		freedata(datalist.forw);
	cacheused += nbytes;
	d = mallocz(sizeof(*d), 0);
	d->owner = file;
	d->n = n;
	d->base = malloc(nbytes);
	d->size = nbytes;
	d->min = 0;
	d->max = 0;
	d->forw = &datalist;
	d->back = datalist.back;
	d->back->forw = d;
	datalist.back = d;
	return d;
}

void
freedata(Data *d)
{
	d->forw->back = d->back;
	d->back->forw = d->forw;
	cacheused -= d->size;
	if(d->owner != nil)
		d->owner->cached[d->n] = nil;
	free(d->base);
	free(d);
}

/*
 * move recently-used data to end
 */
void
usedata(Data *d)
{
	if(datalist.back == d)
		return;	/* already at end */

	d->forw->back = d->back;
	d->back->forw = d->forw;

	d->forw = &datalist;
	d->back = datalist.back;
	d->back->forw = d;
	datalist.back = d;
}

/*
 * data cache
 */

int
cacheread(File *file, void *buf, vlong offset, int nbytes)
{
	char *p;
	Data *d;
	int n, o;

	DPRINT(2, "file %lld length %lld\n", file->qid.path, file->clength);
	p = buf;
	while(nbytes > 0){
		d = finddata(file, offset, &o);
		if(d == nil)
			break;
		if(o < d->min){
			if(p == (char*)buf)
				return -(d->min-o);	/* fill the gap */
			break;
		}else if(o >= d->max)
			break;
		usedata(d);
		/* o >= d->min && o < d->max */
		n = nbytes;
		if(n > d->max-o)
			n = d->max-o;
		memmove(p, d->base+o, n);
		p += n;
		nbytes -= n;
		offset += n;
	}
	return p-(char*)buf;
}

void
cachewrite(File *file, void *buf, vlong offset, int nbytes)
{
	char *p;
	Data *d;
	int n, o;

	p = buf;
	while(nbytes > 0){
		d = storedata(file, offset, &o);
		if(d == nil)
			break;
		n = nbytes;
		if(n > d->size-o)
			n = d->size-o;
		cachemerge(d, p, o, n);
		p += n;
		offset += n;
		nbytes -= n;
	}
	if(offset > file->clength)
		file->clength = offset;
}

/*
 * merge data in if it overlaps existing contents,
 * or simply replace existing contents otherwise
 */
void
cachemerge(Data *p, char *from, int start, int len)
{
	int end;

	end = start + len;
	memmove(p->base+start, from, len);

	if(start > p->max || p->min > end){
		p->min = start;
		p->max = end;
	}else{
		if(start < p->min)
			p->min = start;
		if(end > p->max)
			p->max = end;
	}
}

void
cacheinval(File *file)
{
	Data *d;
	int i;

	if(file->cached != nil){
		for(i = 0; i < file->ndata; i++){
			d = file->cached[i];
			if(d != nil){
				file->cached[i] = nil;
				d->owner = nil;
				freedata(d);
			}
		}
		/* leave the array */
	}
	file->clength = 0;
}

Data*
finddata(File *file, uvlong offset, int *blkoff)
{
	int r, x;
	uvlong base, o;

	x = 0;
	base = 0;
	for(r = 0; r < nelem(radix); r++){
		o = (offset - base) >> radix[r];
		DPRINT(2, "file %llud offset %llud x %d base %llud o %llud\n", file->qid.path, offset, x, base, o);
		if(range[r] == 0 || o < range[r]){
			o += x;
			if(o >= file->ndata)
				break;
			*blkoff = (offset-base) & ((1<<radix[r])-1);
			return file->cached[(int)o];
		}
		base += range[r]<<radix[r];
		x += range[r];
	}
	return nil;
}

Data*
storedata(File *file, uvlong offset, int *blkoff)
{
	int r, x, v, size;
	uvlong base, o;
	Data **p;

	if(file->cached == nil){
		file->cached = mallocz(16*sizeof(*file->cached), 1);
		file->ndata = 16;
	}
	x = 0;
	base = 0;
	for(r = 0; r < nelem(radix); r++){
		o = (offset - base) >> radix[r];
		DPRINT(2, "store file %llud offset %llud x %d base %llud o %llud\n", file->qid.path, offset, x, base, o);
		if(range[r] == 0 || o < range[r]){
			o += x;
			if(o >= file->ndata){
				if(o >= 512)
					return nil;	/* won't fit */
				v = (o+32+31)&~31;
				file->cached = realloc(file->cached, v*sizeof(*file->cached));
				memset(file->cached+file->ndata, 0, (v-file->ndata)*sizeof(*file->cached));
				file->ndata = v;
			}
			size = 1 << radix[r];
			DPRINT(2, "	-> %d %d\n", (int)o, size);
			*blkoff = (offset-base) & (size-1);
			p = &file->cached[(int)o];
			if(*p == nil)
				*p = allocdata(file, (int)o, size);
			else
				usedata(*p);
			return *p;
		}
		base += range[r]<<radix[r];
		x += range[r];
	}
	return nil;
}

/*
 * Strings
 */

enum{
	Strhashmask=	(1<<8)-1,
};

static struct{
	QLock;
	String*	htab[Strhashmask+1];
} stralloc;

String**
hashslot(char *s)
{
	uint h, g;
	uchar *p;

	h = 0;
	for(p = (uchar*)s; *p != 0; p++){
		h = (h<<4) + *p;
		g = h & (0xF<<28);
		if(g != 0)
			h ^= ((g>>24) & 0xFF) | g;
	}
	return &stralloc.htab[h & Strhashmask];
}

String*
newstr(char *val)
{
	String *s, **l;

	//qlock(&stralloc);
	for(l = hashslot(val); (s = *l) != nil; l = &s->next){
		if(strcmp(s->s, val) == 0){
			s->ref++;
			//qunlock(&stralloc);
			return s;
		}
	}
	s = malloc(sizeof(*s));
	s->s = strdup(val);
	s->len = strlen(s->s);
	s->ref = 1;
	s->next = *l;
	*l = s;
	//qunlock(&stralloc);
	return s;
}

String*
dupstr(String *s)
{
	s->ref++;
	return s;
}

void
freestr(String *s)
{
	String **l;

	if(s != nil && --s->ref == 0){
		//qlock(&stralloc);
		for(l = hashslot(s->s); *l != nil; l = &(*l)->next){
			if(*l == s){
				*l = s->next;
				break;
			}
		}
		//qunlock(&stralloc);
		free(s->s);
		free(s);
	}
}
