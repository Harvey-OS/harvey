/* cdfs - CD, DVD and BD reader and writer file system */
#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include <disk.h>
#include "dat.h"
#include "fns.h"

typedef struct Aux Aux;
struct Aux {
	int	doff;
	Otrack	*o;
};

ulong	getnwa(Drive *);

static void checktoc(Drive*);

int vflag;

static Drive *drive;
static int nchange;

enum {
	Qdir = 0,
	Qctl = 1,
	Qwa = 2,
	Qwd = 3,
	Qtrack = 4,
};

char*
geterrstr(void)
{
	static char errbuf[ERRMAX];

	rerrstr(errbuf, sizeof errbuf);
	return errbuf;
}

void*
emalloc(ulong sz)
{
	void *v;

	v = mallocz(sz, 1);
	if(v == nil)
		sysfatal("malloc %lud fails", sz);
	return v;
}

static void
fsattach(Req *r)
{
	char *spec;

	spec = r->ifcall.aname;
	if(spec && spec[0]) {
		respond(r, "invalid attach specifier");
		return;
	}

	checktoc(drive);
	r->fid->qid = (Qid){Qdir, drive->nchange, QTDIR};
	r->ofcall.qid = r->fid->qid;
	r->fid->aux = emalloc(sizeof(Aux));
	respond(r, nil);
}

static char*
fsclone(Fid *old, Fid *new)
{
	Aux *na;

	na = emalloc(sizeof(Aux));
	*na = *((Aux*)old->aux);
	if(na->o)
		na->o->nref++;
	new->aux = na;
	return nil;
}

static char*
fswalk1(Fid *fid, char *name, Qid *qid)
{
	int i;

	checktoc(drive);
	switch((ulong)fid->qid.path) {
	case Qdir:
		if(strcmp(name, "..") == 0) {
			*qid = (Qid){Qdir, drive->nchange, QTDIR};
			return nil;
		}
		if(strcmp(name, "ctl") == 0) {
			*qid = (Qid){Qctl, 0, 0};
			return nil;
		}
		if(strcmp(name, "wa") == 0 && drive->writeok &&
		    (drive->mmctype == Mmcnone ||
		     drive->mmctype == Mmccd)) {
			*qid = (Qid){Qwa, drive->nchange, QTDIR};
			return nil;
		}
		if(strcmp(name, "wd") == 0 && drive->writeok) {
			*qid = (Qid){Qwd, drive->nchange, QTDIR};
			return nil;
		}
		for(i=0; i<drive->ntrack; i++)
			if(strcmp(drive->track[i].name, name) == 0)
				break;
		if(i == drive->ntrack)
			return "file not found";
		*qid = (Qid){Qtrack+i, 0, 0};
		return nil;

	case Qwa:
	case Qwd:
		if(strcmp(name, "..") == 0) {
			*qid = (Qid){Qdir, drive->nchange, QTDIR};
			return nil;
		}
		return "file not found";
	default:	/* bug: lib9p could handle this */
		return "walk in non-directory";
	}
}

static void
fscreate(Req *r)
{
	int omode, type;
	Otrack *o;
	Fid *fid;

	fid = r->fid;
	omode = r->ifcall.mode;
	
	if(omode != OWRITE) {
		respond(r, "bad mode (use OWRITE)");
		return;
	}

	switch((ulong)fid->qid.path) {
	case Qdir:
	default:
		respond(r, "permission denied");
		return;

	case Qwa:
		if (drive->mmctype != Mmcnone &&
		    drive->mmctype != Mmccd) {
			respond(r, "audio supported only on cd");
			return;
		}
		type = TypeAudio;
		break;

	case Qwd:
		type = TypeData;
		break;
	}

	if((drive->cap & Cwrite) == 0) {
		respond(r, "drive does not write");
		return;
	}

	o = drive->create(drive, type);
	if(o == nil) {
		respond(r, geterrstr());
		return;
	}
	drive->nchange = -1;
	checktoc(drive);	/* update directory info */
	o->nref = 1;
	((Aux*)fid->aux)->o = o;

	fid->qid = (Qid){Qtrack+(o->track - drive->track), drive->nchange, 0};
	r->ofcall.qid = fid->qid;
	respond(r, nil);
}

static void
fsremove(Req *r)
{
	switch((ulong)r->fid->qid.path){
	case Qwa:
	case Qwd:
		if(drive->fixate(drive) < 0)
			respond(r, geterrstr());
// let us see if it can figure this out:	drive->writeok = 0;	
		else
			respond(r, nil);
		checktoc(drive);
		break;
	default:
		respond(r, "permission denied");
		break;
	}
}

/* result is one word, so it can be used as a uid in Dir structs */
static char *
disctype(Drive *drive)
{
	char *type, *rw;

	switch (drive->mmctype) {
	case Mmccd:
		type = "cd-";
		break;
	case Mmcdvdminus:
	case Mmcdvdplus:
		type = drive->dvdtype;
		break;
	case Mmcbd:
		type = "bd-";
		break;
	case Mmcnone:
		type = "no-disc";
		break;
	default:
		type = "**GOK**";		/* traditional */
		break;
	}
	rw = "";
	if (drive->mmctype != Mmcnone && drive->dvdtype == nil)
		if (drive->erasable)
			rw = drive->mmctype == Mmcbd? "re": "rw";
		else if (drive->recordable)
			rw = "r";
		else
			rw = "rom";
	return smprint("%s%s", type, rw);
}

int
fillstat(ulong qid, Dir *d)
{
	char *ty;
	Track *t;
	static char buf[32];

	nulldir(d);
	d->type = L'M';
	d->dev = 1;
	d->length = 0;
	ty = disctype(drive);
	strncpy(buf, ty, sizeof buf);
	free(ty);
	d->uid = d->gid = buf;
	d->muid = "";
	d->qid = (Qid){qid, drive->nchange, 0};
	d->atime = time(0);
	d->mtime = drive->changetime;

	switch(qid){
	case Qdir:
		d->name = "/";
		d->qid.type = QTDIR;
		d->mode = DMDIR|0777;
		break;

	case Qctl:
		d->name = "ctl";
		d->mode = 0666;
		break;

	case Qwa:
		if(drive->writeok == 0 ||
		    drive->mmctype != Mmcnone &&
		    drive->mmctype != Mmccd)
			return 0;
		d->name = "wa";
		d->qid.type = QTDIR;
		d->mode = DMDIR|0777;
		break;

	case Qwd:
		if(drive->writeok == 0)
			return 0;
		d->name = "wd";
		d->qid.type = QTDIR;
		d->mode = DMDIR|0777;
		break;

	default:
		if(qid-Qtrack >= drive->ntrack)
			return 0;
		t = &drive->track[qid-Qtrack];
		if(strcmp(t->name, "") == 0)
			return 0;
		d->name = t->name;
		d->mode = t->mode;
		d->length = t->size;
		break;
	}
	return 1;
}

static ulong 
cddb_sum(int n)
{
	int ret;
	ret = 0;
	while(n > 0) {
		ret += n%10;
		n /= 10;
	}
	return ret;
}

static ulong
diskid(Drive *d)
{
	int i, n;
	ulong tmp;
	Msf *ms, *me;

	n = 0;
	for(i=0; i < d->ntrack; i++)
		n += cddb_sum(d->track[i].mbeg.m*60+d->track[i].mbeg.s);

	ms = &d->track[0].mbeg;
	me = &d->track[d->ntrack].mbeg;
	tmp = (me->m*60+me->s) - (ms->m*60+ms->s);

	/*
	 * the spec says n%0xFF rather than n&0xFF.  it's unclear which is
	 * correct.  most CDs are in the database under both entries.
	 */
	return ((n % 0xFF) << 24 | (tmp << 8) | d->ntrack);
}

static void
readctl(Req *r)
{
	int i, isaudio;
	char *p, *e, *ty;
	char s[1024];
	Msf *m;

	isaudio = 0;
	for(i=0; i<drive->ntrack; i++)
		if(drive->track[i].type == TypeAudio)
			isaudio = 1;

	p = s;
	e = s + sizeof s;
	*p = '\0';
	if(isaudio){
		p = seprint(p, e, "aux/cddb query %8.8lux %d", diskid(drive),
			drive->ntrack);
		for(i=0; i<drive->ntrack; i++){
			m = &drive->track[i].mbeg;
			p = seprint(p, e, " %d", (m->m*60 + m->s)*75 + m->f);
		}
		m = &drive->track[drive->ntrack].mbeg;
		p = seprint(p, e, " %d\n", m->m*60 + m->s);
	}

	if(drive->readspeed == drive->writespeed)
		p = seprint(p, e, "speed %d\n", drive->readspeed);
	else
		p = seprint(p, e, "speed read %d write %d\n",
			drive->readspeed, drive->writespeed);
	p = seprint(p, e, "maxspeed read %d write %d\n",
		drive->maxreadspeed, drive->maxwritespeed);

	if (drive->Scsi.changetime != 0 && drive->ntrack != 0) { /* have disc? */
		ty = disctype(drive);
		p = seprint(p, e, "%s", ty);
		free(ty);
		if (drive->mmctype != Mmcnone)
			p = seprint(p, e, " next writable sector %lud",
				getnwa(drive));
		seprint(p, e, "\n");
	}
	readstr(r, s);
}

static void
fsread(Req *r)
{
	int j, n, m;
	uchar *p, *ep;
	Dir d;
	Fid *fid;
	Otrack *o;
	vlong offset;
	void *buf;
	long count;
	Aux *a;

	fid = r->fid;
	offset = r->ifcall.offset;
	buf = r->ofcall.data;
	count = r->ifcall.count;

	switch((ulong)fid->qid.path) {
	case Qdir:
		checktoc(drive);
		p = buf;
		ep = p+count;
		m = Qtrack+drive->ntrack;
		a = fid->aux;
		if(offset == 0)
			a->doff = 1;	/* skip root */

		for(j=a->doff; j<m; j++) {
			if(fillstat(j, &d)) {
				if((n = convD2M(&d, p, ep-p)) <= BIT16SZ)
					break;
				p += n;
			}
		}
		a->doff = j;

		r->ofcall.count = p - (uchar*)buf;
		break;
	case Qwa:
	case Qwd:
		r->ofcall.count = 0;
		break;
	case Qctl:
		readctl(r);
		break;
	default:
		/* a disk track; we can only call read for whole blocks */
		o = ((Aux*)fid->aux)->o;
		if((count = o->drive->read(o, buf, count, offset)) < 0) {
			respond(r, geterrstr());
			return;
		}
		r->ofcall.count = count;
		break;
	}
	respond(r, nil);
}

static char Ebadmsg[] = "bad cdfs control message";

static char*
writectl(void *v, long count)
{
	char buf[256];
	char *f[10], *p;
	int i, nf, n, r, w, what;

	if(count >= sizeof(buf))
		count = sizeof(buf)-1;
	memmove(buf, v, count);
	buf[count] = '\0';

	nf = tokenize(buf, f, nelem(f));
	if(nf == 0)
		return Ebadmsg;

	if(strcmp(f[0], "speed") == 0){
		what = 0;
		r = w = -1;
		if(nf == 1)
			return Ebadmsg;
		for(i=1; i<nf; i++){
			if(strcmp(f[i], "read") == 0 || strcmp(f[i], "write") == 0){
				if(what!=0 && what!='?')
					return Ebadmsg;
				what = f[i][0];
			}else{
				if (strcmp(f[i], "best") == 0)
					n = (1<<16) - 1;
				else {
					n = strtol(f[i], &p, 0);
					if(*p != '\0' || n <= 0)
						return Ebadmsg;
				}
				switch(what){
				case 0:
					if(r >= 0 || w >= 0)
						return Ebadmsg;
					r = w = n;
					break;
				case 'r':
					if(r >= 0)
						return Ebadmsg;
					r = n;
					break;
				case 'w':
					if(w >= 0)
						return Ebadmsg;
					w = n;
					break;
				default:
					return Ebadmsg;
				}
				what = '?';
			}
		}
		if(what != '?')
			return Ebadmsg;
		return drive->setspeed(drive, r, w);
	}
	return drive->ctl(drive, nf, f);
}

static void
fswrite(Req *r)
{
	Otrack *o;
	Fid *fid;

	fid = r->fid;
	r->ofcall.count = r->ifcall.count;
	if(fid->qid.path == Qctl) {
		respond(r, writectl(r->ifcall.data, r->ifcall.count));
		return;
	}

	if((o = ((Aux*)fid->aux)->o) == nil || o->omode != OWRITE) {
		respond(r, "permission denied");
		return;
	}

	if(o->drive->write(o, r->ifcall.data, r->ifcall.count) < 0)
		respond(r, geterrstr());
	else
		respond(r, nil);
}

static void
fsstat(Req *r)
{
	fillstat((ulong)r->fid->qid.path, &r->d);
	r->d.name = estrdup9p(r->d.name);
	r->d.uid = estrdup9p(r->d.uid);
	r->d.gid = estrdup9p(r->d.gid);
	r->d.muid = estrdup9p(r->d.muid);
	respond(r, nil);
}

static void
fsopen(Req *r)
{
	int omode;
	Fid *fid;
	Otrack *o;

	fid = r->fid;
	omode = r->ifcall.mode;
	checktoc(drive);
	r->ofcall.qid = (Qid){fid->qid.path, drive->nchange, fid->qid.vers};

	switch((ulong)fid->qid.path){
	case Qdir:
	case Qwa:
	case Qwd:
		if(omode != OREAD) {
			respond(r, "permission denied");
			return;
		}
		break;
	case Qctl:
		if(omode & ~(OTRUNC|OREAD|OWRITE|ORDWR)) {
			respond(r, "permission denied");
			return;
		}
		break;
	default:
		if(fid->qid.path >= Qtrack+drive->ntrack) {
			respond(r, "file no longer exists");
			return;
		}

		/*
		 * allow the open with OWRITE or ORDWR if the
		 * drive and disc are both capable?
		 */
		if(omode != OREAD ||
		    (o = drive->openrd(drive, fid->qid.path-Qtrack)) == nil) {
			respond(r, "permission denied");
			return;
		}

		o->nref = 1;
		((Aux*)fid->aux)->o = o;
		break;
	}
	respond(r, nil);
}

static void
fsdestroyfid(Fid *fid)
{
	Aux *aux;
	Otrack *o;

	aux = fid->aux;
	if(aux == nil)
		return;
	o = aux->o;
	if(o && --o->nref == 0) {
		bterm(o->buf);
		drive->close(o);
		checktoc(drive);
	}
}

static void
checktoc(Drive *drive)
{
	int i;
	Track *t;

	drive->gettoc(drive);
	if(drive->nameok)
		return;

	for(i=0; i<drive->ntrack; i++) {
		t = &drive->track[i];
		if(t->size == 0)	/* being created */
			t->mode = 0;
		else
			t->mode = 0444;
		sprint(t->name, "?%.3d", i);
		switch(t->type){
		case TypeNone:
			t->name[0] = 'u';
//			t->mode = 0;
			break;
		case TypeData:
			t->name[0] = 'd';
			break;
		case TypeAudio:
			t->name[0] = 'a';
			break;
		case TypeBlank:
			t->name[0] = '\0';
			break;
		default:
			print("unknown track type %d\n", t->type);
			break;
		}
	}

	drive->nameok = 1;
}

long
bufread(Otrack *t, void *v, long n, vlong off)
{
	return bread(t->buf, v, n, off);
}

long
bufwrite(Otrack *t, void *v, long n)
{
	return bwrite(t->buf, v, n);
}

Srv fs = {
.attach=	fsattach,
.destroyfid=	fsdestroyfid,
.clone=		fsclone,
.walk1=		fswalk1,
.open=		fsopen,
.read=		fsread,
.write=		fswrite,
.create=	fscreate,
.remove=	fsremove,
.stat=		fsstat,
};

void
usage(void)
{
	fprint(2, "usage: cdfs [-Dv] [-d /dev/sdC0] [-m mtpt]\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	Scsi *s;
	int fd;
	char *dev, *mtpt;

	dev = "/dev/sdD0";
	mtpt = "/mnt/cd";

	ARGBEGIN{
	case 'D':
		chatty9p++;
		break;
	case 'd':
		dev = EARGF(usage());
		break;
	case 'm':
		mtpt = EARGF(usage());
		break;
	case 'v':
		if((fd = create("/tmp/cdfs.log", OWRITE, 0666)) >= 0) {
			dup(fd, 2);
			dup(fd, 1);
			if(fd != 1 && fd != 2)
				close(fd);
			vflag++;
			scsiverbose = 2; /* verbose but no Readtoc errs */
		}
		break;
	default:
		usage();
	}ARGEND

	if(dev == nil || mtpt == nil || argc > 0)
		usage();

	if((s = openscsi(dev)) == nil)
		sysfatal("openscsi '%s': %r", dev);
	if((drive = mmcprobe(s)) == nil)
		sysfatal("mmcprobe '%s': %r", dev);
	checktoc(drive);

	postmountsrv(&fs, nil, mtpt, MREPL|MCREATE);
	exits(nil);
}
