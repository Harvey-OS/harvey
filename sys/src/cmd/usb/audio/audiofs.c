#include <u.h>
#include <libc.h>
#include <thread.h>
#include <auth.h>
#include <fcall.h>
#include <libsec.h>
#include "usb.h"
#include "usbaudio.h"
#include "usbaudioctl.h"

int attachok;

#define STACKSIZE 16*1024

enum
{
	OPERM	= 0x3,		// mask of all permission types in open mode
};

typedef struct Fid Fid;
typedef struct Audioctldata Audioctldata;
typedef struct Worker Worker;

struct Audioctldata
{
	long		offoff;			// offset of the offset for audioctl
	long		values[2][Ncontrol][8];	// last values transmitted
	char	*	s;
	int		ns;
};

enum {
	Busy =	0x01,
	Open =	0x02,
	Eof =		0x04,
};

struct Fid
{
	QLock;
	int		fid;
	Dir	*	dir;
	ushort	flags;
	short	readers;
	void	*	fiddata;		// file specific per-fid data (used for audioctl)
	Fid	*	next;
};

struct Worker
{
	Fid		*fid;
	ushort	tag;
	Fcall		*rhdr;
	Dir		*dir;
	Channel	*eventc;
	Worker	*next;
};

enum {
	// Event channel messages for worker
	Work =	0x01,
	Check =	0x02,
	Flush =	0x03,
};

enum {
	Qdir,
	Qvolume,
	Qaudioctl,
	Qaudiostat,
	Qaudio,
	Qaudioin,
	Nqid,
};

Dir dirs[] = {
	/* Note: Qaudio{in} used as mount point for /dev/usb/%d/ep%ddata only */
[Qdir] =		{0,0,{Qdir, 0,QTDIR},0555|DMDIR,0,0,0,	".",		nil,nil,nil},
[Qvolume] =	{0,0,{Qvolume, 0,QTFILE},0666,0,0,0,	"volume",	nil,nil,nil},
[Qaudioctl] =	{0,0,{Qaudioctl, 0,QTFILE},0666,0,0,0,	"audioctl",nil,nil,nil},
[Qaudiostat] =	{0,0,{Qaudiostat, 0,QTFILE},0666,0,0,0,	"audiostat",nil,nil,nil},
[Qaudio] =	{0,0,{Qaudio, 0,QTFILE},0222,0,0,0,		"audio",	nil,nil,nil},
[Qaudioin] =	{0,0,{Qaudioin, 0,QTFILE},0444,0,0,0,	"audioin",	nil,nil,nil},
};

int	messagesize = 4*1024+IOHDRSZ;
uchar	mdata[8*1024+IOHDRSZ];
uchar	mbuf[8*1024+IOHDRSZ];

Fcall	thdr;
Fcall	rhdr;
Worker *workers;

char srvfile[64], mntdir[64], epdata[64], audiofile[64];
int mfd[2], p[2];
char user[32];
char *srvpost;

Channel *procchan;
Channel *replchan;

Fid *fids;

Fid*		newfid(int);
void		io(void *);
void		usage(void);

extern char *mntpt;

char	*rflush(Fid*), *rauth(Fid*),
	*rattach(Fid*), *rwalk(Fid*),
	*ropen(Fid*), *rcreate(Fid*),
	*rread(Fid*), *rwrite(Fid*), *rclunk(Fid*),
	*rremove(Fid*), *rstat(Fid*), *rwstat(Fid*),
	*rversion(Fid*);

char 	*(*fcalls[])(Fid*) = {
	[Tflush]		rflush,
	[Tversion]		rversion,
	[Tauth]		rauth,
	[Tattach]		rattach,
	[Twalk]		rwalk,
	[Topen]		ropen,
	[Tcreate]		rcreate,
	[Tread]		rread,
	[Twrite]		rwrite,
	[Tclunk]		rclunk,
	[Tremove]	rremove,
	[Tstat]		rstat,
	[Twstat]		rwstat,
};

char	Eperm[] =		"permission denied";
char	Enotdir[] =	"not a directory";
char	Enoauth[] =	"no authentication in ramfs";
char	Enotexist[] =	"file does not exist";
char	Einuse[] =		"file in use";
char	Eexist[] =		"file exists";
char	Enotowner[] =	"not owner";
char	Eisopen[] = 	"file already open for I/O";
char	Excl[] = 		"exclusive use file already open";
char	Ename[] = 	"illegal name";
char	Ebadctl[] =	"unknown control message";

int
notifyf(void *, char *s)
{
	if(strncmp(s, "interrupt", 9) == 0)
		return 1;
	return 0;
}

void
post(char *name, char *envname, int srvfd)
{
	int fd;
	char buf[32];

	fd = create(name, OWRITE, attachok?0666:0600);
	if(fd < 0)
		return;
	sprint(buf, "%d",srvfd);
	if(write(fd, buf, strlen(buf)) != strlen(buf))
		sysfatal("srv write");
	close(fd);
	putenv(envname, name);
}

void
serve(void *)
{
	int i;
	ulong t;

	if(pipe(p) < 0)
		sysfatal("pipe failed");
	mfd[0] = p[0];
	mfd[1] = p[0];

	atnotify(notifyf, 1);
	strcpy(user, getuser());
	t = time(nil);
	for (i = 0; i < Nqid; i++){
		dirs[i].uid = user;
		dirs[i].gid = user;
		dirs[i].muid = user;
		dirs[i].atime = t;
		dirs[i].mtime = t;
	}
	if(mntpt == nil){
		snprint(mntdir, sizeof(mntdir), "/dev");
		mntpt = mntdir;
	}

	if(debug)
		fmtinstall('F', fcallfmt);

	procrfork(io, nil, STACKSIZE, RFFDG|RFNAMEG);

	close(p[0]);	/* don't deadlock if child fails */
	if(srvpost){
		sprint(srvfile, "/srv/%s", srvpost);
		remove(srvfile);
		post(srvfile, "usbaudio", p[1]);
	}
	if(mount(p[1], -1, mntpt, MBEFORE, "") < 0)
		sysfatal("mount failed");
	if (endpt[Play] >= 0){
		sprint(epdata, "#U/usb%d/%d/ep%ddata", ad->ctlrno, ad->id, endpt[Play]);
		sprint(audiofile, "%s/audio", mntpt);
		if(bind(epdata, audiofile, MREPL) < 0)
			sysfatal("bind failed");
	}
	if (endpt[Record] >= 0){
		sprint(epdata, "#U/usb%d/%d/ep%ddata", ad->ctlrno, ad->id, endpt[Record]);
		sprint(audiofile, "%s/audioin", mntpt);
		if(bind(epdata, audiofile, MREPL) < 0)
			sysfatal("bind failed");
	}
	threadexits(nil);
}

char*
rversion(Fid*)
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
	return "usbaudio: no authentication required";
}

char*
rflush(Fid *)
{
	Worker *w;
	int waitflush;

	do {
		waitflush = 0;
		for (w = workers; w; w = w->next)
			if (w->tag == thdr.oldtag){
				waitflush++;
				nbsendul(w->eventc, thdr.oldtag << 16 | Flush);
			}
		if (waitflush)
			sleep(50);
	} while(waitflush);
	if (debug & Dbgproc)
		fprint(2, "flush done on tag %d\n", thdr.oldtag);
	return 0;
}

char*
rattach(Fid *f)
{
	f->flags |= Busy;
	f->dir = &dirs[Qdir];
	rhdr.qid = f->dir->qid;
	if(attachok == 0 && strcmp(thdr.uname, user) != 0)
		return Eperm;
	return 0;
}

static Fid*
doclone(Fid *f, int nfid)
{
	Fid *nf;

	nf = newfid(nfid);
	if(nf->flags & Busy)
		return nil;
	nf->flags |= Busy;
	nf->flags &= ~Open;
	nf->dir = f->dir;
	return nf;
}

char*
dowalk(Fid *f, char *name)
{
	int t;

	if (strcmp(name, ".") == 0)
		return nil;
	if (strcmp(name, "..") == 0){
		f->dir = &dirs[Qdir];
		return nil;
	}
	if(f->dir != &dirs[Qdir])
		return Enotexist;
	for (t = 1; t < Nqid; t++){
		if (t == Qaudio && endpt[Play] < 0)
			continue;
		if (t == Qaudioin && endpt[Record] < 0)
			continue;
		if(strcmp(name, dirs[t].name) == 0){
			f->dir = &dirs[t];
			return nil;
		}
	}
	return Enotexist;
}

char*
rwalk(Fid *f)
{
	Fid *nf;
	char *rv;
	int i;
	Dir *savedir;

	if(f->flags & Open)
		return Eisopen;

	rhdr.nwqid = 0;
	nf = nil;
	savedir = f->dir;
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
			else
				f->dir = savedir;
			break;
		}
		rhdr.wqid[i] = f->dir->qid;
	}
	rhdr.nwqid = i;

	/* we only error out if no walk  */
	if(i > 0)
		rv = nil;

	return rv;
}

Audioctldata *
allocaudioctldata(void)
{
	int i, j, k;
	Audioctldata *a;

	a = emallocz(sizeof(Audioctldata), 1);
	for (i = 0; i < 2; i++)
		for(j=0; j < Ncontrol; j++)
			for(k=0; k < 8; k++)
			a->values[i][j][k] = Undef;
	return a;
}

char *
ropen(Fid *f)
{
	if(f->flags & Open)
		return Eisopen;

	if(f->dir == &dirs[Qaudio] || f->dir == &dirs[Qaudioin])
		return Eperm;
	if(thdr.mode != OREAD && (f->dir->mode & 0x2) == 0)
		return Eperm;
	qlock(f);
	if(f->dir == &dirs[Qaudioctl] && f->fiddata == nil)
		f->fiddata = allocaudioctldata();
	qunlock(f);
	rhdr.iounit = 0;
	rhdr.qid = f->dir->qid;
	f->flags |= Open;
	return nil;
}

char *
rcreate(Fid*)
{
	return Eperm;
}

int
readtopdir(Fid*, uchar *buf, long off, int cnt, int blen)
{
	int i, m, n;
	long pos;

	n = 0;
	pos = 0;
	for (i = 1; i < Nqid; i++){
		if (endpt[Play] < 0 && i == Qaudio)
			continue;
		if (endpt[Record] < 0 && i == Qaudioin)
			continue;
		m = convD2M(&dirs[i], &buf[n], blen-n);
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
makeaudioctldata(Fid *f)
{
	int rec, ctl, i, different;
	char *p, *e;
	Audiocontrol *c;
	Audioctldata *a;

	if ((a = f->fiddata) == nil)
		sysfatal("fiddata");
	if ((p = a->s) == nil){
		p = emalloc(1024);
		a->s = p;
	}
	e = p + 1024;
	for (rec = 0; rec < 2; rec++)
		for (ctl = 0; ctl < Ncontrol; ctl++) {
			c = &controls[rec][ctl];
			different = 0;
			if (c->chans){
				for (i = 1; i < 8; i++)
					if ((c->chans & 1<<i) && c->value[i] != a->values[rec][ctl][i])
						different = 1;
			}else
				if (c->value[0] != a->values[rec][ctl][0])
					different = 1;
			if (different){
				p = seprint(p, e, "%s %s %A", c->name, rec?"in":"out", c);
				memmove(a->values[rec][ctl], c->value, sizeof c->value);
				if (c->min != Undef){
					p = seprint(p, e, " %ld %ld", c->min, c->max);
					if (c->step != Undef)
						p = seprint(p, e, " %ld", c->step);
				}
				p = seprint(p, e, "\n");
			}
		}
	assert(strlen(a->s) < 1024);
	a->ns = p - a->s;
	return a->ns;
}

void
readproc(void *x)
{
	Fcall *rhdr;
	Audioctldata *a;
	Fid *f;
	uchar *mdata;
	int n, cnt;
	long off;
	Worker *w;
	ulong event;

	w = x;
	mdata = emalloc(8*1024+IOHDRSZ);
	while(event = recvul(w->eventc)){
		if (event != Work)
			continue;
		f = w->fid;
		rhdr = w->rhdr;
		a = f->fiddata;
		off = rhdr->offset;
		cnt = rhdr->count;
		assert(a->offoff == off);
		/* f is already locked */
		for(;;){
			qunlock(f);
			event = recvul(w->eventc);
			qlock(f);
			if (debug & Dbgproc)
				fprint(2, "readproc unblocked fid %d %lld\n", f->fid, f->dir->qid.path);
			switch (event & 0xffff) {
			case Work:
				sysfatal("readproc phase error");
			case Check:
				if (f->fiddata && makeaudioctldata(f) == 0)
					continue;
				break;
			case Flush:
				if ((event >> 16) == rhdr->tag) {
					if (debug & Dbgproc)
						fprint(2, "readproc flushing fid %d, tag %d\n", f->fid, rhdr->tag);
					goto flush;
				}
				continue;
			}
			if (f->fiddata){
				rhdr->data = a->s;
				rhdr->count = a->ns;
				break;
			}
			yield();
		}
		if (rhdr->count > cnt)
			rhdr->count = cnt;
		if (rhdr->count)
			f->flags &= ~Eof;
		if(debug & (Dbgproc|Dbgfs))
			fprint(2, "readproc:->%F\n", rhdr);
		n = convS2M(rhdr, mdata, messagesize);
		if(write(mfd[1], mdata, n) != n)
			sysfatal("mount write");
flush:
		w->tag = NOTAG;
		f->readers--;
		assert(f->readers == 0);
		free(rhdr);
		w->rhdr = nil;
		qunlock(f);
		sendp(procchan, w);
	}
	threadexits(nil);
}

char*
rread(Fid *f)
{
	long off;
	int i, n, cnt, rec;
	char *p;
	static char buf[1024];
	Audiocontrol *c;
	Audioctldata *a;
	Worker *w;

	rhdr.count = 0;
	off = thdr.offset;
	cnt = thdr.count;

	if(cnt > messagesize - IOHDRSZ)
		cnt = messagesize - IOHDRSZ;

	rhdr.data = (char*)mbuf;

	if(f->dir == &dirs[Qdir]){
		n = readtopdir(f, mbuf, off, cnt, messagesize - IOHDRSZ);
		rhdr.count = n;
		return nil;
	}

	if(f->dir == &dirs[Qvolume]){
		p = buf;
		n = sizeof buf;
		for (rec = 0; rec < 2; rec++){
			c = &controls[rec][Volume_control];
			if (c->readable){
				i = snprint(p, n, "audio %s %ld\n", rec?"in":"out", (c->min != Undef) ?
					100*(c->value[0]-c->min)/(c->max-c->min) : c->value[0]);
				p+=i; n-=i;
			}
			c = &controls[rec][Treble_control];
			if (c->readable){
				i = snprint(p, n, "treb %s %ld\n", rec?"in":"out", (c->min != Undef) ?
					100*(c->value[0]-c->min)/(c->max-c->min) : c->value[0]);
				p+=i; n-=i;
			}
			c = &controls[rec][Bass_control];
			if (c->readable){
				i = snprint(p, n, "bass %s %ld\n", rec?"in":"out", (c->min != Undef) ?
					100*(c->value[0]-c->min)/(c->max-c->min) : c->value[0]);
				p+=i; n-=i;
			}
			c = &controls[rec][Speed_control];
			if (c->readable){
				i = snprint(p, n, "speed %s %ld\n", rec?"in":"out", c->value[0]);
				p+=i; n-=i;
			}
		}
		n = sizeof buf - n;
		if (off > n)
			rhdr.count = 0;
		else{
			rhdr.data = buf + off;
			rhdr.count = n - off;
			if (rhdr.count > cnt)
				rhdr.count = cnt;
		}
		return nil;
	}

	if(f->dir == &dirs[Qaudioctl]){
		Fcall *hdr;

		qlock(f);
		a = f->fiddata;
		if (off - a->offoff < 0){
			/* there was a seek */
			a->offoff = off;
			a->ns = 0;
		}
		do {
			if (off - a->offoff < a->ns){
				rhdr.data = a->s + (off - a->offoff);
				rhdr.count = a->ns - (off - a->offoff);
				if (rhdr.count > cnt)
					rhdr.count = cnt;
				qunlock(f);
				return nil;
			}
			if (a->offoff != off){
				a->ns = 0;
				a->offoff = off;
				rhdr.count = 0;
				qunlock(f);
				return nil;
			}
		} while (makeaudioctldata(f) != 0);

		assert(a->offoff == off);
		/* Wait for data off line */
		f->readers++;
		w = nbrecvp(procchan);
		if (w == nil){
			w = emallocz(sizeof(Worker), 1);
			w->eventc = chancreate(sizeof(ulong), 1);
			w->next = workers;
			workers = w;
			proccreate(readproc, w, 4096);
		}
		hdr = emalloc(sizeof(Fcall));
		w->fid = f;
		w->tag = thdr.tag;
		assert(w->rhdr == nil);
		w->rhdr = hdr;
		hdr->count = cnt;
		hdr->offset = off;
		hdr->type = thdr.type+1;
		hdr->fid = thdr.fid;
		hdr->tag = thdr.tag;
		sendul(w->eventc, Work);
		return (char*)~0;
	}

	return Eperm;
}

char*
rwrite(Fid *f)
{
	long cnt, value;
	char *lines[2*Ncontrol], *fields[4], *subfields[9], *err, *p;
	int nlines, i, nf, nnf, rec, ctl;
	Audiocontrol *c;
	Worker *w;
	static char buf[256];

	rhdr.count = 0;
	cnt = thdr.count;

	if(cnt > messagesize - IOHDRSZ)
		cnt = messagesize - IOHDRSZ;

	err = nil;
	if(f->dir == &dirs[Qvolume] || f->dir == &dirs[Qaudioctl]){
		thdr.data[cnt] = '\0';
		nlines = getfields(thdr.data, lines, 2*Ncontrol, 1, "\n");
		for(i = 0; i < nlines; i++){
			if (debug) fprint(2, "line: %s\n", lines[i]);
			nf = tokenize(lines[i], fields, 4);
			if (nf == 0)
				continue;
			if (nf == 3){
				if (strcmp(fields[1], "in") == 0 || strcmp(fields[1], "record") == 0)
					rec = 1;
				else if (strcmp(fields[1], "out") == 0 || strcmp(fields[1], "playback") == 0)
					rec = 0;
				else {
					if (debug) fprint(2, "bad1\n");
					return Ebadctl;
				}
			}else if (nf == 2)
				rec = 0;
			else {
				if (debug) fprint(2, "bad2 %d\n", nf);
				return Ebadctl;
			}
			c = nil;
			if (strcmp(fields[0], "audio") == 0)	/* special case */
				fields[0] = "volume";
			for (ctl = 0; ctl < Ncontrol; ctl++){
				c = &controls[rec][ctl];
				if (strcmp(fields[0], c->name) == 0)
					break;
			}
			if (ctl == Ncontrol){
				if (debug) fprint(2, "bad3\n");
				return Ebadctl;
			}
			if (f->dir == &dirs[Qvolume] && c->min != Undef  && c->max != Undef){
				nnf = tokenize(fields[nf-1], subfields, nelem(subfields));
				if (nnf <= 0 || nnf > 8){
					if (debug) fprint(2, "bad4\n");
					return Ebadctl;
				}
				p = buf;
				for (i = 0; i < nnf; i++){
					value = strtol(subfields[i], nil, 0);
					value = ((100 - value) * c->min + value * c->max) / 100;
					if (debug) {
						if (p == buf)
							fprint(2, "rwrite: %s %s '%ld",
								c->name, rec?"record":"playback", value);
						else
							fprint(2, " %ld", value);
					}
					if (p == buf)
						p = seprint(p, buf+sizeof buf, "0x%p %s %s '%ld",
							replchan, c->name, rec?"record":"playback", value);
					else
						p = seprint(p, buf+sizeof buf, " %ld", value);
				}
				if (debug) fprint(2, "'\n");
				seprint(p, buf+sizeof buf, "'");
				chanprint(controlchan, buf);
			} else {
				if (debug) fprint(2, "rwrite: %s %s %q", c->name, rec?"record":"playback", fields[nf-1]);
				chanprint(controlchan, "0x%p %s %s %q", replchan, c->name, rec?"record":"playback", fields[nf-1]);
			}
			p = recvp(replchan);
			if (p){
				if (strcmp(p, "ok") == 0){
					free(p);
					p = nil;
				}
				if (err == nil)
					err = p;
			}
		}
		for (w = workers; w; w = w->next)
			nbsendul(w->eventc, Qaudioctl << 16 | Check);
		rhdr.count = thdr.count;
		return err;
	}
	return Eperm;
}

char *
rclunk(Fid *f)
{
	Audioctldata *a;

	qlock(f);
	f->flags &= ~(Open|Busy);
	assert (f->readers ==0);
	if (f->fiddata){
		a = f->fiddata;
		if (a->s)
			free(a->s);
		free(a);
		f->fiddata = nil;
	}
	qunlock(f);
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
	Audioctldata *a;

	if (f->dir == &dirs[Qaudio] && endpt[Play] < 0)
			return Enotexist;
	if (f->dir == &dirs[Qaudioin] && endpt[Record] < 0)
			return Enotexist;
	if (f->dir == &dirs[Qaudioctl]){
		qlock(f);
		if (f->fiddata == nil)
			f->fiddata = allocaudioctldata();
		a = f->fiddata;
		if (a->ns == 0)
			makeaudioctldata(f);
		f->dir->length = a->offoff + a->ns;
		qunlock(f);
	}
	rhdr.nstat = convD2M(f->dir, mbuf, messagesize - IOHDRSZ);
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
		ff = emallocz(sizeof *ff, 1);
		ff->next = fids;
		fids = ff;
	}
	ff->fid = fid;
	ff->flags &= ~(Busy|Open);
	ff->dir = nil;
	return ff;
}

void
io(void *)
{
	char *err, e[32];
	int n;

	close(p[1]);

	procchan = chancreate(sizeof(Channel*), 8);
	replchan = chancreate(sizeof(char*), 0);
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
				if (debug) fprint(2, "read9pmsg interrupted\n");
				continue;
			}
			return;
		}
		if(convM2S(mdata, n, &thdr) == 0)
			continue;

		if(debug & Dbgfs)
			fprint(2, "io:<-%F\n", &thdr);

		rhdr.data = (char*)mdata + messagesize;
		if(!fcalls[thdr.type])
			err = "bad fcall type";
		else
			err = (*fcalls[thdr.type])(newfid(thdr.fid));
		if (err == (char*)~0)
			continue;	/* handled off line */
		if(err){
			rhdr.type = Rerror;
			rhdr.ename = err;
		}else{
			rhdr.type = thdr.type + 1;
			rhdr.fid = thdr.fid;
		}
		rhdr.tag = thdr.tag;
		if(debug & Dbgfs)
			fprint(2, "io:->%F\n", &rhdr);
		n = convS2M(&rhdr, mdata, messagesize);
		if(write(mfd[1], mdata, n) != n)
			sysfatal("mount write");
	}
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

void
ctlevent(void)
{
	Worker *w;

	for (w = workers; w; w = w->next)
		nbsendul(w->eventc, Qaudioctl << 16 | Check);
}
