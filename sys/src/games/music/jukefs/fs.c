#include <u.h>
#include <libc.h>
#include <thread.h>
#include <bio.h>
#include <fcall.h>
#include "object.h"

extern int debug;

extern int mfd[];

enum {
	DbgFs = 0x1000
};

typedef struct Fid Fid;

enum {
	Busy =	0x01,
	Open =	0x02,
	Endf =	0x04,
};

struct Fid
{
	QLock;
	Qid	qid;
	int	fid;
	ushort	flags;
	vlong	offset;		// offset of data[0]
	Fid	*next;
};

Fcall	thdr;
Fcall	rhdr;

enum {
	/* Files making up an object */
	Qchildren,		/* Each of these must be in dirtab */
	Qdigest,
	Qfiles,
	Qfulltext,
	Qkey,
	Qminiparentage,
	Qparent,
	Qparentage,
	Qtext,
	Qtype,

	/* Other files */
	Qtop,	/* Must follow Qtype */
	Qclassical,
	Qdir,
	Qroot,
	Qctl,
};

#define PATH(id, f)	(((id)<<8) | (f))
#define FILE(p)		((p) & 0xff)
#define NUM(p)		((p) >> 8)

char *dirtab[] =
{
[Qchildren]	"children",
[Qdigest]	"digest",
[Qdir]		".",
[Qfiles]	"files",
[Qfulltext]	"fulltext",
[Qkey]		"key",
[Qminiparentage]"miniparentage",
[Qparent]	"parent",
[Qparentage]	"parentage",
[Qtext]		"text",
[Qtype]		"type",
[Qtop]		nil,
};

char	*rflush(Fid*), *rauth(Fid*),
	*rattach(Fid*), *rwalk(Fid*),
	*ropen(Fid*), *rcreate(Fid*),
	*rread(Fid*), *rwrite(Fid*), *rclunk(Fid*),
	*rremove(Fid*), *rstat(Fid*), *rwstat(Fid*),
	*rversion(Fid*);

char 	*(*fcalls[])(Fid*) = {
	[Tflush]	rflush,
	[Tversion]	rversion,
	[Tauth]		rauth,
	[Tattach]	rattach,
	[Twalk]		rwalk,
	[Topen]		ropen,
	[Tcreate]	rcreate,
	[Tread]		rread,
	[Twrite]	rwrite,
	[Tclunk]	rclunk,
	[Tremove]	rremove,
	[Tstat]		rstat,
	[Twstat]	rwstat,
};

int	messagesize = 8*1024+IOHDRSZ;
uchar	mdata[8*1024+IOHDRSZ];
uchar	mbuf[8*1024+IOHDRSZ];
char	bigbuf[1<<23];	/* 8 megabytes */
Fid	*fids;

char	Eperm[] =	"permission denied";
char	Enotdir[] =	"not a directory";
char	Enoauth[] =	"no authentication required";
char	Enotexist[] =	"file does not exist";
char	Einuse[] =	"file in use";
char	Eexist[] =	"file exists";
char	Enotowner[] =	"not owner";
char	Eisopen[] = 	"file already open for I/O";
char	Excl[] = 	"exclusive use file already open";
char	Ename[] = 	"illegal name";
char	Ebadctl[] =	"unknown control message";

Fid *newfid(int fid);

static int
lookup(char *cmd, char *list[])
{
	int i;

	for (i = 0; list[i] != nil; i++)
		if (strcmp(cmd, list[i]) == 0)
			return i;
	return -1;
}

char*
rversion(Fid *)
{
	Fid *f;

	if(thdr.msize < 256)
		return "max messagesize too small";
	if(thdr.msize < messagesize)
		messagesize = thdr.msize;
	rhdr.msize = messagesize;
	if(strncmp(thdr.version, "9P2000", 6) != 0)
		return "unknown 9P version";
	else
		rhdr.version = "9P2000";
	for(f = fids; f; f = f->next)
		if(f->flags & Busy)
			rclunk(f);
	return nil;
}

char*
rauth(Fid*)
{
	return Enoauth;
}

char*
rflush(Fid *)
{
	return 0;
}

char*
rattach(Fid *f)
{
	f->flags |= Busy;
	f->qid.type = QTDIR;
	f->qid.vers = 0;
	f->qid.path = PATH(0, Qtop);
	rhdr.qid = f->qid;
	return 0;
}

static Fid*
doclone(Fid *f, int nfid)
{
	Fid *nf;

	nf = newfid(nfid);
	nf->qid = f->qid;
	if(nf->flags & Busy)
		return nil;
	nf->flags |= Busy;
	nf->flags &= ~Open;
	return nf;
}

char*
dowalk(Fid *f, char *name)
{
	int t, n, m;
	char *rv, *p;

	t = FILE(f->qid.path);	/* Type */

	rv = Enotexist;

	if(strcmp(name, ".") == 0 && f->qid.type == QTDIR)
		return nil;
	if(strcmp(name, "..") == 0){
		switch(t){
		case Qtop:
		case Qclassical:
			f->qid.path = PATH(0, Qtop);
			f->qid.type = QTDIR;
			f->qid.vers = 0;
			rv = nil;
			break;
		case Qdir:
		case Qroot:
			f->qid.path = PATH(0, Qclassical);
			f->qid.type = QTDIR;
			f->qid.vers = 0;
			rv = nil;
			break;
		}
		return rv;
	}
	switch(t){
	case Qtop:
		/* Contains classical */
		if(strcmp(name, "juke") == 0){
			f->qid.path = PATH(root->tabno, Qclassical);
			f->qid.type = QTDIR;
			f->qid.vers = 0;
			rv = nil;
			break;
		}
		break;
	case Qclassical:
		/* main dir, contains `root' and object dirs */
		if(strcmp(name, "root") == 0){
			f->qid.path = PATH(root->tabno, Qroot);
			f->qid.type = QTDIR;
			f->qid.vers = 0;
			rv = nil;
			break;
		}
		if(strcmp(name, "ctl") == 0){
			f->qid.path = PATH(root->tabno, Qctl);
			f->qid.type = QTFILE;
			f->qid.vers = 0;
			rv = nil;
			break;
		}
		n = strtol(name, &p, 0);
		if(*p)
			break;	/* Not a number */
		if(n < 0 || n >= notab)
			break;	/* Outside range */
		if(otab[n] == nil)
			break;	/* Not in object table */
		f->qid.path = PATH(n, Qdir);
		f->qid.type = QTDIR;
		f->qid.vers = 0;
		rv = nil;
		break;
	case Qroot:	/* Root of the object hierarchy */
	case Qdir:	/* Object directory */
		if((m = lookup(name, dirtab)) < 0)
			break;
		n = NUM(f->qid.path);
		f->qid.path = PATH(n, m);
		f->qid.type = QTFILE;
		f->qid.vers = 0;
		rv = nil;
		break;
	}
	return rv;
}

char*
rwalk(Fid *f)
{
	Fid *nf;
	char *rv;
	int i;

	if(f->flags & Open)
		return Eisopen;

	rhdr.nwqid = 0;
	nf = nil;

	/* clone if requested */
	if(thdr.newfid != thdr.fid){
		nf = doclone(f, thdr.newfid);
		if(nf == nil)
			return "new fid in use";
		f = nf;
	}

	/* if it's just a clone, return */
	if(thdr.nwname == 0 && nf != nil)
		return nil;

	/* walk each element */
	rv = nil;
	for(i = 0; i < thdr.nwname; i++){
		rv = dowalk(f, thdr.wname[i]);
		if(rv != nil){
			if(nf != nil)	
				rclunk(nf);
			break;
		}
		rhdr.wqid[i] = f->qid;
	}
	rhdr.nwqid = i;

	/* we only error out if no walk  */
	if(i > 0)
		rv = nil;

	return rv;
}

char *
ropen(Fid *f)
{
	if(f->flags & Open)
		return Eisopen;

	if(thdr.mode != OREAD && FILE(f->qid.path) != Qctl)
		return Eperm;
	rhdr.iounit = 0;
	rhdr.qid = f->qid;
	f->flags |= Open;
	f->flags &= ~Endf;
	return nil;
}

char *
rcreate(Fid*)
{
	return Eperm;
}

static long
fileinfo(char *buf, int bufsize, int onum, int t)
{
	long n;

	n = 0;
	switch(t){
	case Qchildren:
		n = printchildren(buf, bufsize, otab[onum]);
		break;
	case Qdigest:
		n = printdigest(buf, bufsize, otab[onum]);
		break;
	case Qfulltext:
		n = printfulltext(buf, bufsize, otab[onum]);
		break;
	case Qkey:
		n = printkey(buf, bufsize, otab[onum]);
		break;
	case Qparent:
		n = printparent(buf, bufsize, otab[onum]);
		break;
	case Qtext:
		n = printtext(buf, bufsize, otab[onum]);
		break;
	case Qtype:
		n = printtype(buf, bufsize, otab[onum]);
		break;
	case Qfiles:
		n = printfiles(buf, bufsize, otab[onum]);
		break;
	case Qparentage:
		n = printparentage(buf, bufsize, otab[onum]);
		break;
	case Qminiparentage:
		n = printminiparentage(buf, bufsize, otab[onum]);
		break;
	default:
		sysfatal("rread: %d", t);
	}
	return n;
}

static void
mkstat(Dir *d, int n, int t)
{
	static char buf[16];

	d->uid = user;
	d->gid = user;
	d->muid = user;
	d->qid.vers = 0;
	d->qid.type = QTFILE;
	d->type = 0;
	d->dev = 0;
	d->atime = time(0);
	d->mtime = d->atime;
	switch(t){
	case Qtop:
		d->name = ".";
		d->mode = DMDIR|0555;
		d->atime = d->mtime = time(0);
		d->length = 0;
		d->qid.path = PATH(0, Qtop);
		d->qid.type = QTDIR;
		break;
	case Qclassical:
		d->name = "juke";
		d->mode = DMDIR|0555;
		d->atime = d->mtime = time(0);
		d->length = 0;
		d->qid.path = PATH(0, Qclassical);
		d->qid.type = QTDIR;
		break;
	case Qdir:
		snprint(buf, sizeof buf, "%d", n);
		d->name = buf;
		d->mode = DMDIR|0555;
		d->length = 0;
		d->qid.path = PATH(n, Qdir);
		d->qid.type = QTDIR;
		break;
	case Qroot:
		d->name = "root";
		d->mode = DMDIR|0555;
		d->length = 0;
		d->qid.path = PATH(0, Qroot);
		d->qid.type = QTDIR;
		break;
	case Qctl:
		d->name = "ctl";
		d->mode = 0666;
		d->length = 0;
		d->qid.path = PATH(0, Qctl);
		break;
		d->name = "ctl";
		d->mode = 0666;
		d->length = 0;
		d->qid.path = PATH(0, Qctl);
		break;
	case Qchildren:
	case Qdigest:
	case Qfiles:
	case Qfulltext:
	case Qkey:
	case Qminiparentage:
	case Qparent:
	case Qparentage:
	case Qtext:
	case Qtype:
		d->name = dirtab[t];
		d->mode = 0444;
		d->length = fileinfo(bigbuf, sizeof bigbuf, n, t);
		d->qid.path = PATH(n, t);
		break;
	default:
		sysfatal("mkstat: %d", t);
	}
}

int
readtopdir(Fid*, uchar *buf, long off, int cnt, int blen)
{
	int m, n;
	Dir d;

	n = 0;
	mkstat(&d, 0, Qclassical);
	m = convD2M(&d, &buf[n], blen);
	if(off <= 0){
		if(m <= BIT16SZ || m > cnt)
			return n;
		n += m;
	}
	return n;
}

int
readclasdir(Fid*, uchar *buf, long off, int cnt, int blen)
{
	int m, n;
	long pos;
	Dir d;
	Fid *fid;

	n = 0;
	pos = 0;
	mkstat(&d, 0, Qctl);
	m = convD2M(&d, &buf[n], blen);
	if(off <= pos){
		if(m <= BIT16SZ || m > cnt)
			return 0;
		n += m;
		cnt -= m;
	}
	pos += m;
	mkstat(&d, 0, Qroot);
	m = convD2M(&d, &buf[n], blen);
	if(off <= pos){
		if(m <= BIT16SZ || m > cnt)
			return n;
		n += m;
		cnt -= m;
	}
	pos += m;
	for (fid = fids; fid; fid = fid->next){
		if(FILE(fid->qid.path) != Qdir)
			continue;
		mkstat(&d, NUM(fid->qid.path), Qdir);
		m = convD2M(&d, &buf[n], blen-n);
		if(off <= pos){
			if(m <= BIT16SZ || m > cnt)
				break;
			n += m;
			cnt -= m;
		}
		pos += m;
	}
	return n;
}

int
readdir(Fid *f, uchar *buf, long off, int cnt, int blen)
{
	int i, m, n;
	long pos;
	Dir d;

	n = 0;
	pos = 0;
	for (i = 0; i < Qtop; i++){
		mkstat(&d, NUM(f->qid.path), i);
		m = convD2M(&d, &buf[n], blen-n);
		if(off <= pos){
			if(m <= BIT16SZ || m > cnt)
				break;
			n += m;
			cnt -= m;
		}
		pos += m;
	}
	return n;
}

void
readbuf(char *s, long n)
{
	rhdr.count = thdr.count;
	if(thdr.offset >= n){
		rhdr.count = 0;
		return;
	}
	if(thdr.offset+rhdr.count > n)
		rhdr.count = n - thdr.offset;
	rhdr.data = s + thdr.offset;
}

char*
rread(Fid *f)
{
	long off;
	int n, cnt, t;

	rhdr.count = 0;
	off = thdr.offset;
	cnt = thdr.count;

	if(cnt > messagesize - IOHDRSZ)
		cnt = messagesize - IOHDRSZ;

	rhdr.data = (char*)mbuf;

	n = 0;
	t = FILE(f->qid.path);
	switch(t){
	case Qtop:
		n = readtopdir(f, mbuf, off, cnt, messagesize - IOHDRSZ);
		rhdr.count = n;
		return nil;
	case Qclassical:
		n = readclasdir(f, mbuf, off, cnt, messagesize - IOHDRSZ);
		rhdr.count = n;
		return nil;
	case Qdir:
	case Qroot:
		n = readdir(f, mbuf, off, cnt, messagesize - IOHDRSZ);
		rhdr.count = n;
		return nil;
	case Qctl:
		snprint(bigbuf, sizeof bigbuf, "%d objects in tree (%d holes)\n", notab, hotab);
		break;
	case Qchildren:
	case Qdigest:
	case Qfiles:
	case Qfulltext:
	case Qkey:
	case Qminiparentage:
	case Qparent:
	case Qparentage:
	case Qtext:
	case Qtype:
		n = fileinfo(bigbuf, sizeof bigbuf, NUM(f->qid.path), t);
		break;
	default:
		sysfatal("rread: %d", t);
	}
	readbuf(bigbuf, n);
	return nil;
}

char*
rwrite(Fid *f)
{
	long cnt;
	char *p;

	if(FILE(f->qid.path) != Qctl)
		return Eperm;
	rhdr.count = 0;
	cnt = thdr.count;
	if(p = strchr(thdr.data, '\n'))
		*p = '\0';
	if(strncmp(thdr.data, "quit", cnt) == 0)
		threadexitsall(nil);
	else if(strncmp(thdr.data, "reread", cnt) == 0)
		reread();
	else
		return "illegal command";
	rhdr.count = thdr.count;
	return nil;
}

char *
rclunk(Fid *f)
{
	f->flags &= ~(Open|Busy);
	return 0;
}

char *
rremove(Fid *)
{
	return Eperm;
}

char *
rstat(Fid *f)
{
	Dir d;

	mkstat(&d, NUM(f->qid.path), FILE(f->qid.path));
	rhdr.nstat = convD2M(&d, mbuf, messagesize - IOHDRSZ);
	rhdr.stat = mbuf;
	return 0;
}

char *
rwstat(Fid*)
{
	return Eperm;
}

Fid *
newfid(int fid)
{
	Fid *f, *ff;

	ff = nil;
	for(f = fids; f; f = f->next)
		if(f->fid == fid){
			return f;
		}else if(ff == nil && (f->flags & Busy) == 0)
			ff = f;
	if(ff == nil){
		ff = malloc(sizeof *ff);
		if (ff == nil)
			sysfatal("malloc: %r");
		memset(ff, 0, sizeof *ff);
		ff->next = fids;
		fids = ff;
	}
	ff->fid = fid;
	return ff;
}

void
io(void *)
{
	char *err, e[32];
	int n;
	extern int p[];
	Fid *f;

	threadsetname("file server");
	close(p[1]);
	for(;;){
		/*
		 * reading from a pipe or a network device
		 * will give an error after a few eof reads
		 * however, we cannot tell the difference
		 * between a zero-length read and an interrupt
		 * on the processes writing to us,
		 * so we wait for the error
		 */
		n = read9pmsg(mfd[0], mdata, messagesize);
		if(n == 0)
			continue;
		if(n < 0){
			rerrstr(e, sizeof e);
			if (strcmp(e, "interrupted") == 0){
				if (debug & DbgFs) fprint(2, "read9pmsg interrupted\n");
				continue;
			}
			return;
		}
		if(convM2S(mdata, n, &thdr) == 0)
			continue;

		if(debug & DbgFs)
			fprint(2, "io:<-%F\n", &thdr);

		rhdr.data = (char*)mbuf;

		if(!fcalls[thdr.type])
			err = "bad fcall type";
		else {
			f = newfid(thdr.fid);
			err = (*fcalls[thdr.type])(f);
		}
		if(err){
			rhdr.type = Rerror;
			rhdr.ename = err;
		}else{
			rhdr.type = thdr.type + 1;
			rhdr.fid = thdr.fid;
		}
		rhdr.tag = thdr.tag;
		if(debug & DbgFs)
			fprint(2, "io:->%F\n", &rhdr);/**/
		n = convS2M(&rhdr, mdata, messagesize);
		if(write(mfd[1], mdata, n) != n)
			sysfatal("mount write");
	}
	threadexitsall("die yankee pig dog");
}

int
newid(void)
{
	int rv;
	static int id;
	static Lock idlock;

	lock(&idlock);
	rv = ++id;
	unlock(&idlock);

	return rv;
}
