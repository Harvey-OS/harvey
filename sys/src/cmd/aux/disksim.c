#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>

typedef struct Part Part;
typedef struct Trip Trip;
typedef struct Dbl Dbl;
typedef struct Ind Ind;

/*
 * with 8192-byte blocks and 4-byte pointers,
 * double-indirect gets us 34 GB. 
 * triple-indirect gets us 70,368 GB.
 */

enum
{
	LOGBLKSZ = 13,
	BLKSZ = 1<<LOGBLKSZ,	/* 8192 */
	LOGNPTR = LOGBLKSZ-2,	/* assume sizeof(void*) == 4 */
	NPTR = 1<<LOGNPTR,
};
static uchar zero[BLKSZ];

struct Trip
{
	Dbl *dbl[NPTR];	
};

struct Dbl
{
	Ind *ind[NPTR];
};

struct Ind
{
	uchar *blk[NPTR];
};

Trip trip;

struct Part
{
	int inuse;
	int vers;
	ulong mode;
	char *name;
	vlong offset;	/* in sectors */
	vlong length;	/* in sectors */
};

enum
{
	Qroot = 0,
	Qdir,
	Qctl,
	Qpart,
};

Part tab[64];
int fd = -1;
char *sdname = "sdXX";
ulong ctlmode = 0666;
char *inquiry = "aux/disksim hard drive";
vlong nsect, sectsize, c, h, s;
ulong time0;
int rdonly;

char*
ctlstring(void)
{
	int i;
	Fmt fmt;

	fmtstrinit(&fmt);
	fmtprint(&fmt, "inquiry %s\n", inquiry);
	fmtprint(&fmt, "geometry %lld %lld %lld %lld %lld\n", nsect, sectsize, c, h, s);
	for(i=0; i<nelem(tab); i++)
		if(tab[i].inuse)
			fmtprint(&fmt, "part %s %lld %lld\n", tab[i].name, tab[i].offset, tab[i].length);
	return fmtstrflush(&fmt);
}

int
addpart(char *name, vlong start, vlong end)
{
	int i;

	if(start < 0 || start > end || end > nsect){
		werrstr("bad partition boundaries");
		return -1;
	}

	for(i=0; i<nelem(tab); i++)
		if(tab[i].inuse == 0)
			break;
	if(i == nelem(tab)){
		werrstr("no free partition slots");
		return -1;	
	}

	free(tab[i].name);
	tab[i].inuse = 1;
	tab[i].name = estrdup9p(name);
	tab[i].offset = start;
	tab[i].length = end - start;
	tab[i].mode = ctlmode;
	tab[i].vers++;

	return 0;
}

int
delpart(char *s)
{
	int i;

	for(i=0; i<nelem(tab); i++)
		if(tab[i].inuse && strcmp(tab[i].name, s) == 0)
			break;
	if(i==nelem(tab)){
		werrstr("partition not found");
		return -1;
	}

	tab[i].inuse = 0;
	free(tab[i].name);
	tab[i].name = 0;
	return 0;
}

void
ctlwrite(Req *r)
{
	int i;
	Cmdbuf *cb;
	vlong start, end;

	r->ofcall.count = r->ifcall.count;
	cb = parsecmd(r->ifcall.data, r->ifcall.count);
	if(cb->nf < 1){
		respond(r, "empty control message");
		free(cb);
		return;
	}

	if(strcmp(cb->f[0], "part") == 0){
		if(cb->nf != 4){
			respondcmderror(r, cb, "part takes 3 args");
			free(cb);
			return;
		}
		start = strtoll(cb->f[2], 0, 0);
		end = strtoll(cb->f[3], 0, 0);
		if(addpart(cb->f[1], start, end) < 0){
			respondcmderror(r, cb, "%r");
			free(cb);
			return;
		}
	}
	else if(strcmp(cb->f[0], "delpart") == 0){
		if(cb->nf != 2){
			respondcmderror(r, cb, "delpart takes 1 arg");
			free(cb);
			return;
		}
		if(delpart(cb->f[1]) < 0){
			respondcmderror(r, cb, "%r");
			free(cb);
			return;
		}
	}
	else if(strcmp(cb->f[0], "inquiry") == 0){
		if(cb->nf != 2){
			respondcmderror(r, cb, "inquiry takes 1 arg");
			free(cb);
			return;
		}
		free(inquiry);
		inquiry = estrdup9p(cb->f[1]);
	}
	else if(strcmp(cb->f[0], "geometry") == 0){
		if(cb->nf != 6){
			respondcmderror(r, cb, "geometry takes 5 args");
			free(cb);
			return;
		}
		nsect = strtoll(cb->f[1], 0, 0);
		sectsize = strtoll(cb->f[2], 0, 0);
		c = strtoll(cb->f[3], 0, 0);
		h = strtoll(cb->f[4], 0, 0);
		s = strtoll(cb->f[5], 0, 0);
		if(tab[0].inuse && strcmp(tab[0].name, "data") == 0 && tab[0].vers == 0){
			tab[0].offset = 0;
			tab[0].length = nsect;
		}
		for(i=0; i<nelem(tab); i++){
			if(tab[i].inuse && tab[i].offset+tab[i].length > nsect){
				tab[i].inuse = 0;
				free(tab[i].name);
				tab[i].name = 0;
			}
		}
	}
	else{
		respondcmderror(r, cb, "unknown control message");
		free(cb);
		return;
	}

	free(cb);
	respond(r, nil);
}
	
void*
allocblk(vlong addr)
{
	uchar *op;
	static uchar *p;
	static ulong n;

	if(n == 0){
		p = malloc(4*1024*1024);
		if(p == 0)
			sysfatal("out of memory");
		n = 4*1024*1024;
	}
	op = p;
	p += BLKSZ;
	n -= BLKSZ;
	memset(op, 0, BLKSZ);
	if(fd != -1 && addr != -1)
		pread(fd, op, BLKSZ, addr);
	return op;
}

uchar*
getblock(vlong addr, int alloc)
{
 	Dbl *p2;
	Ind *p1;
	uchar *p0;
	uint i0, i1, i2;
	vlong oaddr;

	if(fd >= 0)
		alloc = 1;

	addr >>= LOGBLKSZ;
	oaddr = addr<<LOGBLKSZ;
	i0 = addr & (NPTR-1);
	addr >>= LOGNPTR;
	i1 = addr & (NPTR-1);
	addr >>= LOGNPTR;
	i2 = addr & (NPTR-1);
	addr >>= LOGNPTR;
	assert(addr == 0);

	if((p2 = trip.dbl[i2]) == 0){
		if(!alloc)
			return zero;
		trip.dbl[i2] = p2 = allocblk(-1);
	}

	if((p1 = p2->ind[i1]) == 0){
		if(!alloc)
			return zero;
		p2->ind[i1] = p1 = allocblk(-1);
	}

	if((p0 = p1->blk[i0]) == 0){
		if(!alloc)
			return zero;
		p1->blk[i0] = p0 = allocblk(oaddr);
	}
	return p0;
}

void
dirty(vlong addr, uchar *buf)
{
	vlong oaddr;

	if(fd == -1 || rdonly)
		return;
	oaddr = addr&~((vlong)BLKSZ-1);
	if(pwrite(fd, buf, BLKSZ, oaddr) != BLKSZ)
		sysfatal("write: %r");
}
	
int
rootgen(int off, Dir *d, void*)
{
	memset(d, 0, sizeof *d);
	d->atime = time0;
	d->mtime = time0;
	if(off == 0){
		d->name = estrdup9p(sdname);
		d->mode = DMDIR|0777;
		d->qid.path = Qdir;
		d->qid.type = QTDIR;
		d->uid = estrdup9p("disksim");
		d->gid = estrdup9p("disksim");
		d->muid = estrdup9p("");
		return 0;
	}
	return -1;
}	
		
int
dirgen(int off, Dir *d, void*)
{
	int n, j;

	memset(d, 0, sizeof *d);
	d->atime = time0;
	d->mtime = time0;
	if(off == 0){
		d->name = estrdup9p("ctl");
		d->mode = ctlmode;
		d->qid.path = Qctl;
		goto Have;
	}

	off--;
	n = 0;
	for(j=0; j<nelem(tab); j++){
		if(tab[j].inuse==0)
			continue;
		if(n == off){
			d->name = estrdup9p(tab[j].name);
			d->length = tab[j].length*sectsize;
			d->mode = tab[j].mode;
			d->qid.path = Qpart+j;
			d->qid.vers = tab[j].vers;
			goto Have;
		}
		n++;
	}
	return -1;

Have:
	d->uid = estrdup9p("disksim");
	d->gid = estrdup9p("disksim");
	d->muid = estrdup9p("");
	return 0;
}

void*
evommem(void *a, void *b, ulong n)
{
	return memmove(b, a, n);
}

int
isnonzero(void *v, ulong n)
{
	uchar *a, *ea;
	
	a = v;
	ea = a+n;
	for(; a<ea; a++)
		if(*a)
			return 1;
	return 0;
}

int
rdwrpart(Req *r)
{
	int q, nonzero;
	Part *p;
	vlong offset;
	long count, tot, n, o;
	uchar *blk, *dat;
	void *(*move)(void*, void*, ulong);

	q = r->fid->qid.path-Qpart;
	if(q < 0 || q > nelem(tab) || !tab[q].inuse || tab[q].vers != r->fid->qid.vers){
		respond(r, "unknown partition");
		return -1;
	}

	p = &tab[q];
	offset = r->ifcall.offset;
	count = r->ifcall.count;
	if(offset < 0){
		respond(r, "negative offset");
		return -1;
	}
	if(count < 0){
		respond(r, "negative count");
		return -1;
	}
	if(offset > p->length*sectsize){
		respond(r, "offset past end of partition");
		return -1;
	}
	if(offset+count > p->length*sectsize)
		count = p->length*sectsize - offset;
	offset += p->offset*sectsize;

	if(r->ifcall.type == Tread)
		move = memmove;
	else
		move = evommem;

	tot = 0;
	nonzero = 1;
	if(r->ifcall.type == Tread)
		dat = (uchar*)r->ofcall.data;
	else{
		dat = (uchar*)r->ifcall.data;
		nonzero = isnonzero(dat, r->ifcall.count);
	}
	o = offset & (BLKSZ-1);

	/* left fringe block */
	if(o && count){
		blk = getblock(offset, r->ifcall.type==Twrite && nonzero);
		n = BLKSZ - o;
		if(n > count)
			n = count;
		if(r->ifcall.type != Twrite || blk != zero)
			(*move)(dat, blk+o, n);
		if(r->ifcall.type == Twrite)
			dirty(offset, blk);
		tot += n;
	}
	/* full and right fringe blocks */
	while(tot < count){
		blk = getblock(offset+tot, r->ifcall.type==Twrite && nonzero);
		n = BLKSZ;
		if(n > count-tot)
			n = count-tot;
		if(r->ifcall.type != Twrite || blk != zero)
			(*move)(dat+tot, blk, n);
		if(r->ifcall.type == Twrite)
			dirty(offset+tot, blk);
		tot += n;
	}
	r->ofcall.count = tot;
	respond(r, nil);
	return 0;
}

void
fsread(Req *r)
{
	char *s;

	switch((int)r->fid->qid.path){
	case Qroot:
		dirread9p(r, rootgen, nil);
		respond(r, nil);
		break;

	case Qdir:
		dirread9p(r, dirgen, nil);
		respond(r, nil);
		break;

	case Qctl:
		s = ctlstring();
		readstr(r, s);
		free(s);
		respond(r, nil);
		break;

	default:
		rdwrpart(r);
		break;
	}
}

void
fswrite(Req *r)
{
	switch((int)r->fid->qid.path){
	case Qroot:
	case Qdir:
		respond(r, "write to a directory?");
		break;

	case Qctl:
		ctlwrite(r);
		break;

	default:
		rdwrpart(r);
		break;
	}
}

void
fsopen(Req *r)
{
	if(r->ifcall.mode&ORCLOSE)
		respond(r, "cannot open ORCLOSE");

	switch((int)r->fid->qid.path){
	case Qroot:
	case Qdir:
		if(r->ifcall.mode != OREAD){
			respond(r, "bad mode for directory open");
			return;
		}
	}

	respond(r, nil);
}

void
fsstat(Req *r)
{
	int q;
	Dir *d;
	Part *p;

	d = &r->d;
	memset(d, 0, sizeof *d);
	d->qid = r->fid->qid;
	d->atime = d->mtime = time0;
	q = r->fid->qid.path;
	switch(q){
	case Qroot:
		d->name = estrdup9p("/");
		d->mode = DMDIR|0777;
		break;

	case Qdir:
		d->name = estrdup9p(sdname);
		d->mode = DMDIR|0777;
		break;

	default:
		q -= Qpart;
		if(q < 0 || q > nelem(tab) || tab[q].inuse==0 || r->fid->qid.vers != tab[q].vers){
			respond(r, "partition no longer exists");
			return;
		}
		p = &tab[q];
		d->name = estrdup9p(p->name);
		d->length = p->length * sectsize;
		d->mode = p->mode;
		break;
	}
		
	d->uid = estrdup9p("disksim");
	d->gid = estrdup9p("disksim");
	d->muid = estrdup9p("");
	respond(r, nil);
}

void
fsattach(Req *r)
{
	char *spec;

	spec = r->ifcall.aname;
	if(spec && spec[0]){
		respond(r, "invalid attach specifier");
		return;
	}
	r->ofcall.qid = (Qid){Qroot, 0, QTDIR};
	r->fid->qid = r->ofcall.qid;
	respond(r, nil);
}

char*
fswalk1(Fid *fid, char *name, Qid *qid)
{
	int i;
	switch((int)fid->qid.path){
	case Qroot:
		if(strcmp(name, sdname) == 0){
			fid->qid.path = Qdir;
			fid->qid.type = QTDIR;
			*qid = fid->qid;
			return nil;
		}
		break;
	case Qdir:
		if(strcmp(name, "ctl") == 0){
			fid->qid.path = Qctl;
			fid->qid.vers = 0;
			fid->qid.type = 0;
			*qid = fid->qid;
			return nil;
		}
		for(i=0; i<nelem(tab); i++){
			if(tab[i].inuse && strcmp(tab[i].name, name) == 0){
				fid->qid.path = i+Qpart;
				fid->qid.vers = tab[i].vers;
				fid->qid.type = 0;
				*qid = fid->qid;
				return nil;
			}
		}
		break;
	}
	return "file not found";
}

Srv fs = {
	.attach=	fsattach,
	.open=	fsopen,
	.read=	fsread,
	.write=	fswrite,
	.stat=	fsstat,
	.walk1=	fswalk1,
};

char *mtpt = "/dev";
char *srvname;

void
usage(void)
{
	fprint(2, "usage: aux/disksim [-D] [-f file] [-s srvname] [-m mtpt] [sdXX]\n");
	fprint(2, "\tdefault mtpt is /dev\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	char *file;

	file = nil;
	quotefmtinstall();
	time0 = time(0);
	if(NPTR != BLKSZ/sizeof(void*))
		sysfatal("unexpected pointer size");

	ARGBEGIN{
	case 'D':
		chatty9p++;
		break;
	case 'f':
		file = EARGF(usage());
		break;
	case 'r':
		rdonly = 1;
		break;
	case 's':
		srvname = EARGF(usage());
		break;
	case 'm':
		mtpt = EARGF(usage());
		break;
	default:
		usage();
	}ARGEND

	if(argc > 1)
		usage();
	if(argc == 1)
		sdname = argv[0];

	if(file){
		if((fd = open(file, rdonly ? OREAD : ORDWR)) < 0)
			sysfatal("open %s: %r", file);
	}

	inquiry = estrdup9p(inquiry);
	tab[0].name = estrdup9p("data");
	tab[0].inuse = 1;
	tab[0].mode = 0666;

	postmountsrv(&fs, srvname, mtpt, MBEFORE);
	exits(nil);
}
