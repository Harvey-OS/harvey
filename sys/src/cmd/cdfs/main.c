#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include <disk.h>
#include "dat.h"
#include "fns.h"

static void checktoc(Drive*);

int vflag;

Drive *drive;
int nchange;

enum {
	Qdir = 0|CHDIR,
	Qctl = 0,
	Qwa = 1|CHDIR,
	Qwd = 2|CHDIR,
	Qtrack = 3,
};

char*
geterrstr(void)
{
	static char errbuf[ERRLEN];

	errbuf[0] = 0;
	errstr(errbuf);
	return errbuf;
}

void*
emalloc(ulong sz)
{
	void *v;

	v = malloc(sz);
	if(v == nil)
		sysfatal("malloc %lud fails\n", sz);
	memset(v, 0, sz);
	return v;
}

static void
cdattach(Req *r, Fid*, char *spec, Qid *qid)
{
	if(spec && spec[0]) {
		respond(r, "invalid attach specifier");
		return;
	}

	checktoc(drive);
	*qid = (Qid){Qdir, drive->nchange};
	respond(r, nil);
}

static void
cdclone(Req *r, Fid *old, Fid *new)
{
	Otrack *aux;

	if(aux = old->aux)
		aux->nref++;
	new->aux = aux;

	respond(r, nil);
}

static void
cdwalk(Req *r, Fid *fid, char *name, Qid *qid)
{
	int i;

	checktoc(drive);
	switch(fid->qid.path) {
	case Qdir:
		if(strcmp(name, "..") == 0) {
			*qid = (Qid){Qdir, drive->nchange};
			respond(r, nil);
			return;
		}
		if(strcmp(name, "ctl") == 0) {
			*qid = (Qid){Qctl, 0};
			respond(r, nil);
			return;
		}
		if(strcmp(name, "wa") == 0 && drive->writeok) {
			*qid = (Qid){Qwa, drive->nchange};
			respond(r, nil);
			return;
		}
		if(strcmp(name, "wd") == 0 && drive->writeok) {
			*qid = (Qid){Qwd, drive->nchange};
			respond(r, nil);
			return;
		}
		for(i=0; i<drive->ntrack; i++)
			if(strcmp(drive->track[i].name, name) == 0)
				break;
		if(i == drive->ntrack) {
			respond(r, "file not found");
			return;
		}
		*qid = (Qid){Qtrack+i, 0};
		respond(r, nil);
		return;

	case Qwa:
	case Qwd:
		if(strcmp(name, "..") == 0) {
			*qid = (Qid){Qdir, drive->nchange};
			respond(r, nil);
			return;
		}
		respond(r, "file not found");
		return;
	default:	/* bug: lib9p could handle this */
		respond(r, "walk in non-directory");
		return;
	}
}

static void
cdcreate(Req *r, Fid *fid, char*, int omode, ulong, Qid *qid)
{
	int type;
	Otrack *o;

	if(omode != OWRITE) {
		respond(r, "bad mode (use OWRITE)");
		return;
	}

	switch(fid->qid.path) {
	case Qdir:
	default:
		respond(r, "permission denied");
		return;

	case Qwa:
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
	fid->aux = o;

	*qid = (Qid){Qtrack+(o->track - drive->track), drive->nchange};
	respond(r, nil);
}

static void
cdremove(Req *r, Fid *fid)
{
	switch(fid->qid.path){
	case Qwa:
	case Qwd:
		if(drive->fixate(drive) < 0)
			respond(r, geterrstr());
// let's see if it can figure this out		drive->writeok = 0;	
		else
			respond(r, nil);
		checktoc(drive);
		break;
	default:
		respond(r, "permission denied");
		break;
	}
}

int
fillstat(int qid, Dir *d)
{
	Track *t;

	memset(d, 0, sizeof(Dir));
	strcpy(d->uid, "cd");
	strcpy(d->gid, "cd");
	d->qid = (Qid){qid, drive->nchange};
	d->atime = time(0);
	d->mtime = drive->changetime;

	switch(qid){
	case Qdir:
		strcpy(d->name, "/");
		d->mode = CHDIR|0777;
		break;

	case Qctl:
		strcpy(d->name, "ctl");
		d->mode = 0666;
		break;

	case Qwa&~CHDIR:
	case Qwa:
		if(drive->writeok == 0)
			return 0;
		strcpy(d->name, "wa");
		d->mode = CHDIR|0777;
		break;

	case Qwd&~CHDIR:
	case Qwd:
		if(drive->writeok == 0)
			return 0;
		strcpy(d->name, "wd");
		d->mode = CHDIR|0777;
		break;

	default:
		if(qid-Qtrack >= drive->ntrack)
			return 0;
		t = &drive->track[qid-Qtrack];
		if(strcmp(t->name, "") == 0)
			return 0;
		strcpy(d->name, t->name);
		d->mode = t->mode;
		d->length = t->size;
		break;
	}
	return 1;
}

static int
readctl(void*, long, long)
{
	return 0;
}

static void
cdread(Req *r, Fid *fid, void *buf, long *count, vlong offset)
{
	int i, j, off, n, m;
	char *p;
	Dir d;
	Otrack *o;

	switch(fid->qid.path) {
	case Qdir:
		checktoc(drive);
		p = buf;
		m = Qtrack+drive->ntrack;
		n = *count/DIRLEN;
		off = offset/DIRLEN;
		for(i=0, j=0; j<m && i<off+n; j++) {
			if(fillstat(j, &d)) {
				if(off<=i && i<off+n) {
					convD2M(&d, p);
					p += DIRLEN;
				}
				i++;
			}
		}
		*count = (i-off)*DIRLEN;
		respond(r, nil);
		return;

	case Qwa:
	case Qwd:
		*count = 0;
		respond(r, nil);
		return;

	case Qctl:
		*count = readctl(buf, *count, offset);
		respond(r, nil);
		return;
	}

	/* a disk track; we can only call read for whole blocks */
	o = fid->aux;

	if((*count = o->drive->read(o, buf, *count, offset)) < 0)
		respond(r, geterrstr());
	else
		respond(r, nil);

	return;
}

static char*
writectl(void *v, long count)
{
	char buf[256];
	char *f[10];
	int nf;

	if(count >= sizeof(buf))
		count = sizeof(buf)-1;
	memmove(buf, v, count);
	buf[count] = '\0';

	nf = tokenize(buf, f, nelem(f));
	return drive->ctl(drive, nf, f);
}

static void
cdwrite(Req *r, Fid *fid, void *buf, long *count, vlong)
{
	Otrack *o;

	if(fid->qid.path == Qctl) {
		respond(r, writectl(buf, *count));
		return;
	}

	if((o = fid->aux) == nil || o->omode != OWRITE) {
		respond(r, "permission denied");
		return;
	}

	if(o->drive->write(o, buf, *count) < 0)
		respond(r, geterrstr());
	else
		respond(r, nil);
}

static void
cdstat(Req *r, Fid *fid, Dir *d)
{
	fillstat(fid->qid.path, d);
	respond(r, nil);
}

static void
cdopen(Req *r, Fid *fid, int omode, Qid *qid)
{
	Otrack *o;

	checktoc(drive);
	*qid = (Qid){fid->qid.path, drive->nchange};

	switch(fid->qid.path){
	case Qdir:
	case Qwa:
	case Qwd:
		if(omode == OREAD)
			respond(r, nil);
		else
			respond(r, "permission denied");
		return;

	case Qctl:
		if(omode&~(OTRUNC|OREAD|OWRITE|ORDWR))
			respond(r, "permission denied");
		else
			respond(r, nil);
		return;

	default:
		if(fid->qid.path >= Qtrack+drive->ntrack) {
			respond(r, "file no longer exists");
			return;
		}

		if(omode != OREAD || (o = drive->openrd(drive, fid->qid.path-Qtrack)) == nil) {
			respond(r, "permission denied");
			return;
		}

		o->nref = 1;
		fid->aux = o;
		respond(r, nil);
	}
}

static uchar zero[BScdda];

static void
cdclunkaux(Fid *fid)
{
	Otrack *o;

	o = fid->aux;
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
			t->mode = 0;
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
			print("unknown type %d\n", t->type);
			break;
		}
	}

	drive->nameok = 1;
}

long
bufread(Otrack *t, void *v, long n, long off)
{
	return bread(t->buf, v, n, off);
}

long
bufwrite(Otrack *t, void *v, long n)
{
	return bwrite(t->buf, v, n);
}

Srv cdsrv = {
.attach=	cdattach,
.clone=	cdclone,
.clunkaux=	cdclunkaux,
.walk=	cdwalk,
.open=	cdopen,
.read=	cdread,
.write=	cdwrite,
.create=	cdcreate,
.remove=	cdremove,
.stat=	cdstat,
};

void
usage(void)
{
	fprint(2, "usage: cdfs [-v] [-d /dev/sdC0] [-m mtpt]\n");
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
	case 'd':
		dev = ARGF();
		break;
	case 'm':
		mtpt = ARGF();
		break;
	case 'v':
		if((fd = create("/tmp/cdfs.log", OWRITE, 0666)) >= 0) {
			dup(fd, 2);
			dup(fd, 1);
			if(fd != 1 && fd != 2)
				close(fd);
			vflag++;
			scsiverbose++;
		}
		break;
	case 'V':
		lib9p_chatty++;
		break;
	default:
		usage();
	}ARGEND

	if(dev == nil || mtpt == nil || argc > 0)
		usage();

	if((s = openscsi(dev)) == nil) {
		fprint(2, "openscsi '%s': %r\n", dev);
		exits("openscsi");
	}

	if((drive = mmcprobe(s)) == nil) {
		fprint(2, "mmcprobe '%s': %r\n", dev);
		exits("mmcprobe");
	}

	checktoc(drive);

	postmountsrv(&cdsrv, nil, mtpt, MREPL|MCREATE);
	exits(nil);
}
