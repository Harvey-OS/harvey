/*
 * flash memory
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

#include "../port/flashif.h"

typedef struct Flashtype Flashtype;
struct Flashtype {
	char*	name;
	int	(*reset)(Flash*);
	Flashtype* next;
};

enum {
	Nbanks = 2,
};

static struct
{
	Flash*	card[Nbanks];	/* actual card type, reset for access */
	Flashtype* types;	/* possible card types */
}flash;

enum{
	Qtopdir,
	Qflashdir,
	Qdata,
	Qctl,
};

#define	TYPE(q)	((ulong)(q) & 0xFF)
#define	PART(q)	((ulong)(q)>>8)
#define	QID(p,t)	(((p)<<8) | (t))

static	Flashregion*	flashregion(Flash*, ulong);
static	char*	flashnewpart(Flash*, char*, ulong, ulong);
static	ulong	flashaddr(Flash*, Flashpart*, char*);
static	void	protect(Flash*, ulong);
static	void	eraseflash(Flash*, Flashregion*, ulong);
static	long	readflash(Flash*, void*, long, int);
static	long	writeflash(Flash*, long, void*,int);

static char Eprotect[] = "flash region protected";

static int
flash2gen(Chan *c, ulong p, Dir *dp)
{
	Flashpart *fp;
	Flash *f;
	Qid q;
	int mode;

	f = flash.card[c->dev];
	fp = &f->part[PART(p)];
	if(fp->name == nil)
		return 0;
	mkqid(&q, p, 0, QTFILE);
	switch(TYPE(p)){
	case Qdata:
		mode = 0660;
		if(f->write == nil)
			mode = 0440;
		devdir(c, q, fp->name, fp->end-fp->start, eve, mode, dp);
		return 1;
	case Qctl:
		snprint(up->genbuf, sizeof(up->genbuf), "%sctl", fp->name);
		/* no harm in letting everybody read the ctl files */
		devdir(c, q, up->genbuf, 0, eve, 0664, dp);
		return 1;
	default:
		return -1;
	}
}

static int
flashgen(Chan *c, char*, Dirtab*, int, int s, Dir *dp)
{
	Qid q;
	char *n;

	if(s == DEVDOTDOT){
		mkqid(&q, QID(0, Qtopdir), 0, QTDIR);
		n = "#F";
		if(c->dev != 0){
			snprint(up->genbuf, sizeof up->genbuf, "#F%ld", c->dev);
			n = up->genbuf;
		}
		devdir(c, q, n, 0, eve, 0555, dp);
		return 1;
	}
	switch(TYPE(c->qid.path)){
	case Qtopdir:
		if(s != 0)
			break;
		mkqid(&q, QID(0, Qflashdir), 0, QTDIR);
		n = "flash";
		if(c->dev != 0){
			snprint(up->genbuf, sizeof up->genbuf, "flash%ld",
				c->dev);
			n = up->genbuf;
		}
		devdir(c, q, n, 0, eve, 0555, dp);
		return 1;
	case Qflashdir:
		if(s >= 2*nelem(flash.card[c->dev]->part))
			return -1;
		return flash2gen(c, QID(s>>1, s&1?Qctl:Qdata), dp);
	case Qctl:
	case Qdata:
		return flash2gen(c, (ulong)c->qid.path, dp);
	}
	return -1;
}
		
static void
flashreset(void)
{
	Flash *f;
	Flashtype *t;
	char *e;
	int bank;

	for(bank = 0; bank < Nbanks; bank++){
		f = malloc(sizeof(*f));
		if(f == nil){
			print("#F%d: can't allocate Flash data\n", bank);
			return;
		}
		f->cmask = ~(ulong)0;
		if(archflashreset(bank, f) < 0 || f->type == nil ||
		    f->addr == nil){
			free(f);
			return;
		}
		for(t = flash.types; t != nil; t = t->next)
			if(strcmp(f->type, t->name) == 0)
				break;
		if(t == nil){
			iprint("#F%d: no flash driver for type %s (addr %p)\n",
				bank, f->type, f->addr);
			free(f);
			return;
		}
		f->reset = t->reset;
		f->protect = 1;
		if(f->reset(f) == 0){
			flash.card[bank] = f;
			iprint("#F%d: %s addr %#p len %lud width %d interleave %d\n",
//				bank, f->type, PADDR(f->addr), f->size,
				bank, f->type, f->addr, f->size,
				f->width, f->interleave);
			e = flashnewpart(f, "flash", 0, f->size);
			if(e != nil)
				panic("#F%d: couldn't init table: %s", bank, e);
		}else
			iprint("#F%d: %#p: reset failed (%s)\n",
				bank, f->addr, f->type);
	}
}

static Chan*
flashattach(char *spec)
{
	Flash *f;
	int bank;
	Chan *c;

	bank = strtol(spec, nil, 0);
	if(bank < 0 || bank >= Nbanks ||
	   (f = flash.card[bank]) == nil ||
	   f->attach != nil && f->attach(f) < 0)
		error(Enodev);
	c = devattach('F', spec);
	c->dev = bank;
	return c;
}

static Walkqid*
flashwalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, nil, 0, flashgen);
}

static int
flashstat(Chan *c, uchar *dp, int n)
{
	return devstat(c, dp, n, nil, 0, flashgen);
}

static Chan*
flashopen(Chan *c, int omode)
{
	omode = openmode(omode);
	switch(TYPE(c->qid.path)){
	case Qdata:
	case Qctl:
		if(flash.card[c->dev] == nil)
			error(Enodev);
		break;
	}
	return devopen(c, omode, nil, 0, flashgen);
}

static void	 
flashclose(Chan*)
{
}

static long	 
flashread(Chan *c, void *buf, long n, vlong offset)
{
	Flash *f;
	Flashpart *fp;
	Flashregion *r;
	int i;
	ulong start, end;
	char *s, *o;

	if(c->qid.type & QTDIR)
		return devdirread(c, buf, n, nil, 0, flashgen);

	f = flash.card[c->dev];
	fp = &f->part[PART(c->qid.path)];
	if(fp->name == nil)
		error(Egreg);
	switch(TYPE(c->qid.path)){
	case Qdata:
		offset += fp->start;
		if(offset >= fp->end)
			return 0;
		if(offset+n > fp->end)
			n = fp->end - offset;
		n = readflash(f, buf, offset, n);
		if(n < 0)
			error(Eio);
		return n;
	case Qctl:
		s = malloc(READSTR);
		if(s == nil)
			error(Enomem);
		if(waserror()){
			free(s);
			nexterror();
		}
		o = seprint(s, s+READSTR, "%#2.2ux %#4.4ux %d %q\n",
			f->id, f->devid, f->width, f->sort!=nil? f->sort: "nor");
		for(i=0; i<f->nr; i++){
			r = &f->regions[i];
			if(r->start < fp->end && fp->start < r->end){
				start = r->start;
				if(fp->start > start)
					start = fp->start;
				end = r->end;
				if(fp->end < end)
					end = fp->end;
				o = seprint(o, s+READSTR, "%#8.8lux %#8.8lux %#8.8lux",
					start, end, r->erasesize);
				if(r->pagesize)
					o = seprint(o, s+READSTR, " %#8.8lux",
						r->pagesize);
				o = seprint(o, s+READSTR, "\n");
			}
		}
		n = readstr(offset, buf, n, s);
		poperror();
		free(s);
		return n;
	}
	error(Egreg);
	return 0;		/* not reached */
}

enum {
	CMerase,
	CMadd,
	CMremove,
	CMsync,
	CMprotectboot,
};

static Cmdtab flashcmds[] = {
	{CMerase,	"erase",	2},
	{CMadd,		"add",		0},
	{CMremove,	"remove",	2},
	{CMsync,	"sync",		0},
	{CMprotectboot,	"protectboot",	0},
};

static long	 
flashwrite(Chan *c, void *buf, long n, vlong offset)
{
	Cmdbuf *cb;
	Cmdtab *ct;
	ulong addr, start, end;
	char *e;
	Flashpart *fp;
	Flashregion *r;
	Flash *f;

	f = flash.card[c->dev];
	fp = &f->part[PART(c->qid.path)];
	if(fp->name == nil)
		error(Egreg);
	switch(TYPE(c->qid.path)){
	case Qdata:
		if(f->write == nil)
			error(Eperm);
		offset += fp->start;
		if(offset >= fp->end)
			return 0;
		if(offset+n > fp->end)
			n = fp->end - offset;
		n = writeflash(f, offset, buf, n);
		if(n < 0)
			error(Eio);
		return n;
	case Qctl:
		cb = parsecmd(buf, n);
		if(waserror()){
			free(cb);
			nexterror();
		}
		ct = lookupcmd(cb, flashcmds, nelem(flashcmds));
		switch(ct->index){
		case CMerase:
			if(strcmp(cb->f[1], "all") != 0){
				addr = flashaddr(f, fp, cb->f[1]);
				r = flashregion(f, addr);
				if(r == nil)
					error("nonexistent flash region");
				if(addr%r->erasesize != 0)
					error("invalid erase block address");
				eraseflash(f, r, addr);
			}else if(fp->start == 0 && fp->end == f->size &&
			    f->eraseall != nil){
				eraseflash(f, nil, 0);
			}else{
				for(addr = fp->start; addr < fp->end;
				    addr += r->erasesize){
					r = flashregion(f, addr);
					if(r == nil)
						error("nonexistent flash region");
					if(addr%r->erasesize != 0)
						error("invalid erase block address");
					eraseflash(f, r, addr);
				}
			}
			break;
		case CMadd:
			if(cb->nf < 3)
				error(Ebadarg);
			start = flashaddr(f, fp, cb->f[2]);
			if(cb->nf > 3 && strcmp(cb->f[3], "end") != 0)
				end = flashaddr(f, fp, cb->f[3]);
			else
				end = fp->end;
			if(start > end || start >= fp->end || end > fp->end)
				error(Ebadarg);
			e = flashnewpart(f, cb->f[1], start, end);
			if(e != nil)
				error(e);
			break;
		case CMremove:
			/* TO DO */
			break;
		case CMprotectboot:
			if(cb->nf > 1 && strcmp(cb->f[1], "off") == 0)
				f->protect = 0;
			else
				f->protect = 1;
			break;
		case CMsync:
			/* TO DO? */
			break;
		default:
			error(Ebadarg);
		}
		poperror();
		free(cb);
		return n;
	}
	error(Egreg);
	return 0;		/* not reached */
}

static char*
flashnewpart(Flash *f, char *name, ulong start, ulong end)
{
	Flashpart *fp, *empty;
	int i;

	empty = nil;
	for(i = 0; i < nelem(f->part); i++){
		fp = &f->part[i];
		if(fp->name == nil){
			if(empty == nil)
				empty = fp;
		}else if(strcmp(fp->name, name) == 0)
			return Eexist;
	}
	if((fp = empty) == nil)
		return "partition table full";
//	fp->name = nil;
	kstrdup(&fp->name, name);
	if(fp->name == nil)
		return Enomem;
	fp->start = start;
	fp->end = end;
	return nil;
}

static ulong
flashaddr(Flash *f, Flashpart *fp, char *s)
{
	Flashregion *r;
	ulong addr;

	addr = strtoul(s, &s, 0);
	if(*s)
		error(Ebadarg);
	if(fp->name == nil)
		error("partition removed");
	addr += fp->start;
	r = flashregion(f, addr);
	if(r != nil && addr%r->erasesize != 0)
		error("invalid erase unit address");
	if(addr < fp->start || addr > fp->end || addr > f->size)
		error(Ebadarg);
	return addr;
}

static Flashregion*
flashregion(Flash *f, ulong a)
{
	int i;
	Flashregion *r;

	for(i=0; i<f->nr; i++){
		r = &f->regions[i];
		if(r->start <= a && a < r->end)
			return r;
	}
	return nil;
}

Dev flashdevtab = {
	'F',
	"flash",

	flashreset,
	devinit,
	devshutdown,
	flashattach,
	flashwalk,
	flashstat,
	flashopen,
	devcreate,
	flashclose,
	flashread,
	devbread,
	flashwrite,
	devbwrite,
	devremove,
	devwstat,
};

/*
 * called by flash card types named in link section (eg, flashamd.c)
 */
void
addflashcard(char *name, int (*reset)(Flash*))
{
	Flashtype *f, **l;

	f = (Flashtype*)malloc(sizeof(*f));
	if(f == nil)
		error(Enomem);
	f->name = name;
	f->reset = reset;
	f->next = nil;
	for(l = &flash.types; *l != nil; l = &(*l)->next)
		;
	*l = f;
}

static long
readflash(Flash *f, void *buf, long offset, int n)
{
	int r, width, wmask;
	uchar tmp[16];
	uchar *p;
	ulong o;

	if(offset < 0 || offset+n > f->size)
		error(Ebadarg);
	qlock(f);
	if(waserror()){
		qunlock(f);
		nexterror();
	}
	if(f->read != nil){
		width = f->width;
		wmask = width-1;
		p = buf;
		if(offset & wmask) {
			o = offset & ~wmask;
			if(f->read(f, o, (ulong*)tmp, width) < 0)
				error(Eio);
			memmove(tmp, (uchar*)f->addr + o, width);
			for(; n > 0 && offset & wmask; n--)
				*p++ = tmp[offset++ & wmask];
		}
		r = n & wmask;
		n &= ~wmask;
		if(n){
			if(f->read(f, offset, (ulong*)p, n) < 0)
				error(Eio);
			offset += n;
			p += n;
		}
		if(r){
			if(f->read(f, offset, (ulong*)tmp, width))
				error(Eio);
			memmove(p, tmp, r);
		}
	}else
		/* assumes hardware supports byte access */
		memmove(buf, (uchar*)f->addr+offset, n);
	poperror();
	qunlock(f);
	return n;
}

static long
writeflash(Flash *f, long offset, void *buf, int n)
{
	uchar tmp[16];
	uchar *p;
	ulong o;
	int r, width, wmask;
	Flashregion *rg;

	if(f->write == nil || offset < 0 || offset+n > f->size)
		error(Ebadarg);
	rg = flashregion(f, offset);
	if(f->protect && rg != nil && rg->start == 0 && offset < rg->erasesize)
		error(Eprotect);
	width = f->width;
	wmask = width-1;
	qlock(f);
	archflashwp(f, 0);
	if(waserror()){
		archflashwp(f, 1);
		qunlock(f);
		nexterror();
	}
	p = buf;
	if(offset&wmask){
		o = offset & ~wmask;
		if(f->read != nil){
			if(f->read(f, o, tmp, width) < 0)
				error(Eio);
		}else
			memmove(tmp, (uchar*)f->addr+o, width);
		for(; n > 0 && offset&wmask; n--)
			tmp[offset++&wmask] = *p++;
		if(f->write(f, o, tmp, width) < 0)
			error(Eio);
	}
	r = n&wmask;
	n &= ~wmask;
	if(n){
		if(f->write(f, offset, p, n) < 0)
			error(Eio);
		offset += n;
		p += n;
	}
	if(r){
		if(f->read != nil){
			if(f->read(f, offset, tmp, width) < 0)
				error(Eio);
		}else
			memmove(tmp, (uchar*)f->addr+offset, width);
		memmove(tmp, p, r);
		if(f->write(f, offset, tmp, width) < 0)
			error(Eio);
	}
	poperror();
	archflashwp(f, 1);
	qunlock(f);
	return n;
}

static void
eraseflash(Flash *f, Flashregion *r, ulong addr)
{
	int rv;

	if(f->protect && r != nil && r->start == 0 && addr < r->erasesize)
		error(Eprotect);
	qlock(f);
	archflashwp(f, 0);
	if(waserror()){
		archflashwp(f, 1);
		qunlock(f);
		nexterror();
	}
	if(r == nil){
		if(f->eraseall != nil)
			rv = f->eraseall(f);
		else
			rv = -1;
	}else
		rv = f->erasezone(f, r, addr);
	if(rv < 0)
		error(Eio);
	poperror();
	archflashwp(f, 1);
	qunlock(f);
}

/*
 * flash access taking width and interleave into account
 */
int
flashget(Flash *f, ulong a)
{
	switch(f->width){
	default:
		return ((uchar*)f->addr)[a<<f->bshift];
	case 2:
		return ((ushort*)f->addr)[a];
	case 4:
		return ((ulong*)f->addr)[a];
	}
}

void
flashput(Flash *f, ulong a, int v)
{
	switch(f->width){
	default:
		((uchar*)f->addr)[a<<f->bshift] = v;
		break;
	case 2:
		((ushort*)f->addr)[a] = v;
		break;
	case 4:
		((ulong*)f->addr)[a] = v;
		break;
	}
	coherence();
}
