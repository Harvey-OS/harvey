/*
 *  a pity the code isn't also tiny...
 */
#include "u.h"
#include "../port/lib.h"
#include "../port/error.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

enum{
	Qdir,
	Qmedium,

	Maxfs=		10,	/* max file systems */

	Blen=		48,	/* block length */
	Nlen=		28,	/* name length */
	Dlen=		Blen - 4,

	Tagdir=		'd',
	Tagdata=	'D',
	Tagend=		'e',
	Tagfree=	'f',

	Notapin=		0xffff,
	Notabno=		0xffff,

	Fcreating=	1,
	Frmonclose=	2
};

/* representation of a Tdir on medium */
typedef struct Mdir Mdir;
struct Mdir {
	uchar	type;
	uchar	bno[2];
	uchar	pin[2];
	char	name[Nlen];
	char	pad[Blen - Nlen - 6];
	uchar	sum;
};

/* representation of a Tdata/Tend on medium */
typedef struct Mdata Mdata;
struct Mdata {
	uchar	type;
	uchar	bno[2];
	uchar	data[Dlen];
	uchar	sum;
};

typedef struct Tfile Tfile;
struct Tfile {
	int	r;
	char	name[NAMELEN];
	ushort	bno;
	ushort	dbno;
	ushort	pin;
	uchar	flag;
	ulong	length;

	/* hint to avoid egregious reading */
	ushort	fbno;
	ulong	finger;
};

typedef struct Tfs Tfs;
struct Tfs {
	QLock	ql;
	int	r;
	Chan	*c;
	uchar	*map;
	int	nblocks;
	Tfile	*f;
	int	nf;
	int	fsize;
};

struct {
	Tfs	fs[Maxfs];
} tinyfs;

#define GETS(x) ((x)[0]|((x)[1]<<8))
#define PUTS(x, v) {(x)[0] = (v);(x)[1] = ((v)>>8);}

#define GETL(x) (GETS(x)|(GETS(x+2)<<16))
#define PUTL(x, v) {PUTS(x, v);PUTS(x+2, (v)>>16)};

static uchar
checksum(uchar *p)
{
	uchar *e;
	uchar s;

	s = 0;
	for(e = p + Blen; p < e; p++)
		s += *p;
	return s;
}

static void
mapclr(Tfs *fs, ulong bno)
{
	fs->map[bno>>3] &= ~(1<<(bno&7));
}

static void
mapset(Tfs *fs, ulong bno)
{
	fs->map[bno>>3] |= 1<<(bno&7);
}

static int
isalloced(Tfs *fs, ulong bno)
{
	return fs->map[bno>>3] & (1<<(bno&7));
}

static int
mapalloc(Tfs *fs)
{
	int i, j, lim;
	uchar x;

	lim = (fs->nblocks + 8 - 1)/8;
	for(i = 0; i < lim; i++){
		x = fs->map[i];
		if(x == 0xff)
			continue;
		for(j = 0; j < 8; j++)
			if((x & (1<<j)) == 0){
				fs->map[i] = x|(1<<j);
				return i*8 + j;
			}
	}

	return Notabno;
}

static Mdir*
validdir(Tfs *fs, uchar *p)
{
	Mdir *md;
	ulong x;

	if(checksum(p) != 0)
		return 0;
	if(p[0] != Tagdir)
		return 0;
	md = (Mdir*)p;
	x = GETS(md->bno);
	if(x >= fs->nblocks)
		return 0;
	return md;
}

static Mdata*
validdata(Tfs *fs, uchar *p, int *lenp)
{
	Mdata *md;
	ulong x;

	if(checksum(p) != 0)
		return 0;
	md = (Mdata*)p;
	switch(md->type){
	case Tagdata:
		x = GETS(md->bno);
		if(x >= fs->nblocks)
			return 0;
		if(lenp)
			*lenp = Dlen;
		break;
	case Tagend:
		x = GETS(md->bno);
		if(x > Dlen)
			return 0;
		if(lenp)
			*lenp = x;
		break;
	default:
		return 0;
	}
	return md;
}

static Mdata*
readdata(Tfs *fs, ulong bno, uchar *buf, int *lenp)
{
	if(bno >= fs->nblocks)
		return 0;
	if(devtab[fs->c->type]->read(fs->c, buf, Blen, Blen*bno) != Blen)
		error(Eio);
	return validdata(fs, buf, lenp);
}

static void
writedata(Tfs *fs, ulong bno, ulong next, uchar *buf, int len, int last)
{
	Mdata md;

	if(bno >= fs->nblocks)
		error(Eio);
	if(len > Dlen)
		len = Dlen;
	if(len < 0)
		error(Eio);
	memset(&md, 0, sizeof(md));
	if(last){
		md.type = Tagend;
		PUTS(md.bno, len);
	} else {
		md.type = Tagdata;
		PUTS(md.bno, next);
	}
	memmove(md.data, buf, len);
	md.sum = 0 - checksum((uchar*)&md);
	
	if(devtab[fs->c->type]->write(fs->c, &md, Blen, Blen*bno) != Blen)
		error(Eio);
}

static void
writedir(Tfs *fs, Tfile *f)
{
	Mdir *md;
	uchar buf[Blen];

	if(f->bno == Notabno)
		return;

	md = (Mdir*)buf;
	memset(buf, 0, Blen);
	md->type = Tagdir;
	strncpy(md->name, f->name, sizeof(md->name)-1);
	PUTS(md->bno, f->dbno);
	PUTS(md->pin, f->pin);
	md->sum = 0 - checksum(buf);
	
	if(devtab[fs->c->type]->write(fs->c, buf, Blen, Blen*f->bno) != Blen)
		error(Eio);
}

static void
freeblocks(Tfs *fs, ulong bno, ulong bend)
{
	uchar buf[Blen];
	Mdata *md;

	if(waserror())
		return;

	while(bno != bend && bno != Notabno){
		mapclr(fs, bno);
		if(devtab[fs->c->type]->read(fs->c, buf, Blen, Blen*bno) != Blen)
			break;
		md = validdata(fs, buf, 0);
		if(md == 0)
			break;
		if(md->type == Tagend)
			break;
		bno = GETS(md->bno);
	}

	poperror();
}

static void
freefile(Tfs *fs, Tfile *f, ulong bend)
{
	uchar buf[Blen];

	/* remove blocks from map */
	freeblocks(fs, f->dbno, bend);

	/* change file type to free on medium */
	if(f->bno != Notabno){
		memset(buf, 0x55, Blen);
		devtab[fs->c->type]->write(fs->c, buf, Blen, Blen*f->bno);
		mapclr(fs, f->bno);
	}

	/* forget we ever knew about it */
	memset(f, 0, sizeof(*f));
}

static void
expand(Tfs *fs)
{
	Tfile *f;

	fs->fsize += 8;
	f = malloc(fs->fsize*sizeof(*f));

	if(fs->f){
		memmove(f, fs->f, fs->nf*sizeof(*f));
		free(fs->f);
	}
	fs->f = f;
}

static Tfile*
newfile(Tfs *fs, char *name)
{
	int i;
	volatile struct {
		Tfile *f;
		Tfs *fs;
	} rock;

	/* find free entry in file table */
	rock.f = 0;
	rock.fs = fs;
	for(;;) {
		for(i = 0; i < rock.fs->fsize; i++){
			rock.f = &rock.fs->f[i];
			if(rock.f->name[0] == 0){
				strncpy(rock.f->name, name, sizeof(rock.f->name)-1);
				break;
			}
		}

		if(i < rock.fs->fsize){
			if(i >= rock.fs->nf)
				rock.fs->nf = i+1;
			break;
		}

		expand(rock.fs);
	}

	rock.f->flag = Fcreating;
	rock.f->dbno = Notabno;
	rock.f->bno = mapalloc(rock.fs);
	rock.f->fbno = Notabno;
	rock.f->r = 1;
	rock.f->pin = Notapin;  // what is a pin??

	/* write directory block */
	if(waserror()){
		freefile(rock.fs, rock.f, Notabno);
		nexterror();
	}
	if(rock.f->bno == Notabno)
		error("out of space");
	writedir(rock.fs, rock.f);
	poperror();
	
	return rock.f;
}

/*
 *  Read the whole medium and build a file table and used
 *  block bitmap.  Inconsistent files are purged.  The medium
 *  had better be small or this could take a while.
 */
static void
tfsinit(Tfs *fs)
{
	char dbuf[DIRLEN];
	Dir d;
	uchar buf[Blen];
	ulong x, bno;
	int n, done;
	Tfile *f;
	Mdir *mdir;
	Mdata *mdata;

	devtab[fs->c->type]->stat(fs->c, dbuf);
	convM2D(dbuf, &d);
	fs->nblocks = d.length/Blen;
	if(fs->nblocks < 3)
		error("tinyfs medium too small");

	/* bitmap for block usage */
	x = (fs->nblocks + 8 - 1)/8;
	fs->map = malloc(x);
	memset(fs->map, 0x0, x);
	for(bno = fs->nblocks; bno < x*8; bno++)
		mapset(fs, bno);

	/* find files */
	for(bno = 0; bno < fs->nblocks; bno++){
		n = devtab[fs->c->type]->read(fs->c, buf, Blen, Blen*bno);
		if(n != Blen)
			break;

		mdir = validdir(fs, buf);
		if(mdir == 0)
			continue;

		if(fs->nf >= fs->fsize)
			expand(fs);

		f = &fs->f[fs->nf++];

		x = GETS(mdir->bno);
		mapset(fs, bno);
		strncpy(f->name, mdir->name, sizeof(f->name));
		f->pin = GETS(mdir->pin);
		f->bno = bno;
		f->dbno = x;
		f->fbno = Notabno;
	}

	/* follow files */
	for(f = fs->f; f < &(fs->f[fs->nf]); f++){
		bno = f->dbno;
		for(done = 0; !done;) {
			if(isalloced(fs, bno)){
				freefile(fs, f, bno);
				break;
			}
			n = devtab[fs->c->type]->read(fs->c, buf, Blen, Blen*bno);
			if(n != Blen){
				freefile(fs, f, bno);
				break;
			}
			mdata = validdata(fs, buf, 0);
			if(mdata == 0){
				freefile(fs, f, bno);
				break;
			}
			mapset(fs, bno);
			switch(mdata->type){
			case Tagdata:
				bno = GETS(mdata->bno);
				f->length += Dlen;
				break;
			case Tagend:
				f->length += GETS(mdata->bno);
				done = 1;
				break;
			}
			if(done)
				f->flag &= ~Fcreating;
		}
	}
}

/*
 *  single directory
 */
static int
tinyfsgen(Chan *c, Dirtab *tab, int ntab, int i, Dir *dp)
{
	Tfs *fs;
	Tfile *f;
	Qid qid;

	USED(ntab);
	USED(tab);

	qid.vers = 0;
	fs = &tinyfs.fs[c->dev];
	if(i >= fs->nf)
		return -1;
	if(i == DEVDOTDOT){
		qid.path = CHDIR;
		devdir(c, qid, ".", 0, eve, 0555, dp);
		return 1;
	}
	f = &fs->f[i];
	if(f->name[0] == 0)
		return 0;
	qid.path = i;
	devdir(c, qid, f->name, f->length, eve, 0775, dp);
	return 1;
}

static void
tinyfsinit(void)
{
	if(Nlen > NAMELEN)
		panic("tinyfsinit");
}

/*
 *  specifier is an open file descriptor
 */
static Chan*
tinyfsattach(char *spec)
{
	Tfs *fs;
	Chan *c;
	volatile struct { Chan *cc; } rock;
	int i;
	char buf[NAMELEN*2];

	snprint(buf, sizeof(buf), "/dev/%s", spec);
	rock.cc = namec(buf, Aopen, ORDWR, 0);

	if(waserror()){
		cclose(rock.cc);
		nexterror();
	}

	fs = 0;
	for(i = 0; i < Maxfs; i++){
		fs = &tinyfs.fs[i];
		qlock(&fs->ql);
		if(fs->r && eqchan(rock.cc, fs->c, 1))
			break;
		qunlock(&fs->ql);
	}
	if(i < Maxfs){
		fs->r++;
		qunlock(&fs->ql);
		cclose(rock.cc);
	} else {
		for(fs = tinyfs.fs; fs < &tinyfs.fs[Maxfs]; fs++){
			qlock(&fs->ql);
			if(fs->r == 0)
				break;
			qunlock(&fs->ql);
		}
		if(fs == &tinyfs.fs[Maxfs])
			error("too many tinyfs's");
		fs->c = rock.cc;
		fs->r = 1;
		fs->f = 0;
		fs->nf = 0;
		fs->fsize = 0;
		tfsinit(fs);
		qunlock(&fs->ql);
	}
	poperror();

	c = devattach('F', spec);
	c->dev = fs - tinyfs.fs;
	c->qid.path = CHDIR;
	c->qid.vers = 0;

	return c;
}

static Chan*
tinyfsclone(Chan *c, Chan *nc)
{
	Tfs *fs;

	fs = &tinyfs.fs[c->dev];

	qlock(&fs->ql);
	fs->r++;
	qunlock(&fs->ql);

	return devclone(c, nc);
}

static int
tinyfswalk(Chan *c, char *name)
{
	int n;
	Tfs *fs;

	fs = &tinyfs.fs[c->dev];

	qlock(&fs->ql);
	n = devwalk(c, name, 0, 0, tinyfsgen);
	if(n != 0 && c->qid.path != CHDIR){
		fs = &tinyfs.fs[c->dev];
		fs->f[c->qid.path].r++;
	}
	qunlock(&fs->ql);
	return n;
}

static void
tinyfsstat(Chan *c, char *db)
{
	devstat(c, db, 0, 0, tinyfsgen);
}

static Chan*
tinyfsopen(Chan *c, int omode)
{
	Tfile *f;
	volatile struct { Tfs *fs; } rock;

	rock.fs = &tinyfs.fs[c->dev];

	if(c->qid.path & CHDIR){
		if(omode != OREAD)
			error(Eperm);
	} else {
		qlock(&rock.fs->ql);
		if(waserror()){
			qunlock(&rock.fs->ql);
			nexterror();
		}
		switch(omode){
		case OTRUNC|ORDWR:
		case OTRUNC|OWRITE:
			f = newfile(rock.fs, rock.fs->f[c->qid.path].name);
			rock.fs->f[c->qid.path].r--;
			c->qid.path = f - rock.fs->f;
			break;
		case OREAD:
		case OEXEC:
			break;
		default:
			error(Eperm);
		}
		qunlock(&rock.fs->ql);
		poperror();
	}

	return devopen(c, omode, 0, 0, tinyfsgen);
}

static void
tinyfscreate(Chan *c, char *name, int omode, ulong perm)
{
	volatile struct { Tfs *fs; } rock;
	Tfile *f;

	USED(perm);

	rock.fs = &tinyfs.fs[c->dev];

	qlock(&rock.fs->ql);
	if(waserror()){
		qunlock(&rock.fs->ql);
		nexterror();
	}
	f = newfile(rock.fs, name);
	qunlock(&rock.fs->ql);
	poperror();

	c->qid.path = f - rock.fs->f;
	c->qid.vers = 0;
	c->mode = openmode(omode);
}

static void
tinyfsremove(Chan *c)
{
	Tfs *fs;
	Tfile *f;

	if(c->qid.path == CHDIR)
		error(Eperm);
	fs = &tinyfs.fs[c->dev];
	f = &fs->f[c->qid.path];
	qlock(&fs->ql);
	freefile(fs, f, Notabno);
	qunlock(&fs->ql);
}

static void
tinyfsclose(Chan *c)
{
	volatile struct { Tfs *fs; } rock;
	Tfile *f, *nf;
	int i;

	rock.fs = &tinyfs.fs[c->dev];

	qlock(&rock.fs->ql);

	/* dereference file and remove old versions */
	if(!waserror()){
		if(c->qid.path != CHDIR){
			f = &rock.fs->f[c->qid.path];
			f->r--;
			if(f->r == 0){
				if(f->flag & Frmonclose)
					freefile(rock.fs, f, Notabno);
				else if(f->flag & Fcreating){
					/* remove all other files with this name */
					for(i = 0; i < rock.fs->fsize; i++){
						nf = &rock.fs->f[i];
						if(f == nf)
							continue;
						if(strcmp(nf->name, f->name) == 0){
							if(nf->r)
								nf->flag |= Frmonclose;
							else
								freefile(rock.fs, nf, Notabno);
						}
					}
					f->flag &= ~Fcreating;
				}
			}
		}
		poperror();
	}

	/* dereference rock.fs and remove on zero refs */
	rock.fs->r--;
	if(rock.fs->r == 0){
		if(rock.fs->f)
			free(rock.fs->f);
		rock.fs->f = 0;
		rock.fs->nf = 0;
		rock.fs->fsize = 0;
		if(rock.fs->map)
			free(rock.fs->map);
		rock.fs->map = 0;
		cclose(rock.fs->c);
		rock.fs->c = 0;
	}
	qunlock(&rock.fs->ql);
}

static long
tinyfsread(Chan *c, void *a, long n, vlong offset)
{
	volatile struct { Tfs *fs; } rock;
	Tfile *f;
	int sofar, i, off;
	ulong bno;
	Mdata *md;
	uchar buf[Blen];
	uchar *p;

	if(c->qid.path & CHDIR)
		return devdirread(c, a, n, 0, 0, tinyfsgen);

	p = a;
	rock.fs = &tinyfs.fs[c->dev];
	f = &rock.fs->f[c->qid.path];
	if(offset >= f->length)
		return 0;

	qlock(&rock.fs->ql);
	if(waserror()){
		qunlock(&rock.fs->ql);
		nexterror();
	}
	if(n + offset >= f->length)
		n = f->length - offset;

	/* walk to starting data block */
	if(0 && f->finger <= offset && f->fbno != Notabno){
		sofar = f->finger;
		bno = f->fbno;
	} else {
		sofar = 0;
		bno = f->dbno;
	}
	for(; sofar + Dlen <= offset; sofar += Dlen){
		md = readdata(rock.fs, bno, buf, 0);
		if(md == 0)
			error(Eio);
		bno = GETS(md->bno);
	}

	/* read data */
	off = offset%Dlen;
	offset -= off;
	for(sofar = 0; sofar < n; sofar += i){
		md = readdata(rock.fs, bno, buf, &i);
		if(md == 0)
			error(Eio);

		/* update finger for successful read */
		f->finger = offset;
		f->fbno = bno;
		offset += Dlen;

		i -= off;
		if(i > n - sofar)
			i = n - sofar;
		memmove(p, md->data+off, i);
		p += i;
		bno = GETS(md->bno);
		off = 0;
	}
	qunlock(&rock.fs->ql);
	poperror();

	return sofar;
}

/*
 *  if we get a write error in this routine, blocks will
 *  be lost.  They should be recovered next fsinit.
 */
static long
tinyfswrite(Chan *c, void *a, long n, vlong offset)
{
	Tfile *f;
	int last, next, i, finger, off, used;
	ulong bno, fbno;
	Mdata *md;
	uchar buf[Blen];
	uchar *p;
	volatile struct {
		Tfs *fs;
		ulong dbno;
	} rock;

	if(c->qid.path & CHDIR)
		error(Eperm);

	if(n == 0)
		return 0;

	p = a;
	rock.fs = &tinyfs.fs[c->dev];
	f = &rock.fs->f[c->qid.path];

	qlock(&rock.fs->ql);
	rock.dbno = Notabno;
	if(waserror()){
		freeblocks(rock.fs, rock.dbno, Notabno);
		qunlock(&rock.fs->ql);
		nexterror();
	}

	/* files are append only, anything else is illegal */
	if(offset != f->length)
		error("append only");

	/* write blocks backwards */
	p += n;
	last = offset + n;
	fbno = Notabno;
	finger = 0;
	off = offset; /* so we have something signed to compare against */
	for(next = ((last-1)/Dlen)*Dlen; next >= off; next -= Dlen){
		bno = mapalloc(rock.fs);
		if(bno == Notabno)
			error("out of space");
		i = last - next;
		p -= i;
		if(last == n+offset){
			writedata(rock.fs, bno, rock.dbno, p, i, 1);
			finger = next;	/* remember for later */
			fbno = bno;
		} else {
			writedata(rock.fs, bno, rock.dbno, p, i, 0);
		}
		rock.dbno = bno;
		last = next;
	}

	/* walk to last data block */
	md = (Mdata*)buf;
	if(0 && f->finger < offset && f->fbno != Notabno){
		next = f->finger;
		bno = f->fbno;
	} else {
		next = 0;
		bno = f->dbno;
	}

	used = 0;
	while(bno != Notabno){
		md = readdata(rock.fs, bno, buf, &used);
		if(md == 0)
			error(Eio);
		if(md->type == Tagend){
			if(next + Dlen < offset)
				panic("devtinyfs1");
			break;
		}
		next += Dlen;
		if(next > offset)
			panic("devtinyfs1");
		bno = GETS(md->bno);
	}

	/* point to new blocks */
	if(offset == 0){
		/* first block in a file */
		f->dbno = rock.dbno;
		writedir(rock.fs, f);
	} else {
		/* updating a current block */
		i = last - offset;
		if(i > 0){
			p -= i;
			memmove(md->data + used, p, i);
			used += i;
		}
		writedata(rock.fs, bno, rock.dbno, md->data, used, last == n+offset);
	}
	f->length += n;

	/* update finger */
	if(fbno != Notabno){
		f->finger = finger;
		f->fbno =  fbno;
	}
	poperror();
	qunlock(&rock.fs->ql);

	return n;
}

Dev tinyfsdevtab = {
	'F',
	"tinyfs",

	devreset,
	tinyfsinit,
	tinyfsattach,
	tinyfsclone,
	tinyfswalk,
	tinyfsstat,
	tinyfsopen,
	tinyfscreate,
	tinyfsclose,
	tinyfsread,
	devbread,
	tinyfswrite,
	devbwrite,
	tinyfsremove,
	devwstat,
};
