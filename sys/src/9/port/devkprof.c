#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

#include	"devtab.h"

#define	LRES	3		/* log of PC resolution */
#define	SZ	4		/* sizeof of count cell; well known as 4 */

struct
{
	int	minpc;
	int	maxpc;
	int	nbuf;
	int	time;
	ulong	*buf;
}kprof;

enum{
	Kprofdirqid,
	Kprofdataqid,
	Kprofctlqid,
	Nkproftab=Kprofctlqid,
	Kprofmaxqid,
};
Dirtab kproftab[Nkproftab]={
	"kpdata",	{Kprofdataqid},		0,	0600,
	"kpctl",	{Kprofctlqid},		0,	0600,
};

void kproftimer(ulong);

void
kprofreset(void)
{
}

void
kprofinit(void)
{
	if(SZ != sizeof kprof.buf[0])
		panic("kprof size");
}

Chan *
kprofattach(char *spec)
{
	ulong n;

	/* allocate when first used */
	kprof.minpc = KTZERO;
	kprof.maxpc = (ulong)etext;
	kprof.nbuf = (kprof.maxpc-kprof.minpc) >> LRES;
	n = kprof.nbuf*SZ;
	if(kprof.buf == 0) {
		kprof.buf = xalloc(n);
		if(kprof.buf == 0)
			error(Enomem);
	}
	kproftab[0].length = n;
	return devattach('T', spec);
}
Chan *
kprofclone(Chan *c, Chan *nc)
{
	return devclone(c, nc);
}

int
kprofwalk(Chan *c, char *name)
{
	return devwalk(c, name, kproftab, (long)Nkproftab, devgen);
}

void
kprofstat(Chan *c, char *db)
{
	devstat(c, db, kproftab, (long)Nkproftab, devgen);
}

Chan *
kprofopen(Chan *c, int omode)
{
	if(c->qid.path == CHDIR){
		if(omode != OREAD)
			error(Eperm);
	}
	c->mode = openmode(omode);
	c->flag |= COPEN;
	c->offset = 0;
	return c;
}

void
kprofcreate(Chan *c, char *name, int omode, ulong perm)
{
	USED(c, name, omode, perm);
	error(Eperm);
}

void
kprofremove(Chan *c)
{
	USED(c);
	error(Eperm);
}

void
kprofwstat(Chan *c, char *dp)
{
	USED(c, dp);
	error(Eperm);
}

void
kprofclose(Chan *c)
{
	USED(c);
}

long
kprofread(Chan *c, void *va, long n, ulong offset)
{
	ulong tabend;
	ulong w, *bp;
	uchar *a, *ea;

	switch(c->qid.path & ~CHDIR){
	case Kprofdirqid:
		return devdirread(c, va, n, kproftab, Nkproftab, devgen);

	case Kprofdataqid:
		tabend = kprof.nbuf*SZ;
		if(offset & (SZ-1))
			error(Ebadarg);
		if(offset >= tabend){
			n = 0;
			break;
		}
		if(offset+n > tabend)
			n = tabend-offset;
		n &= ~(SZ-1);
		a = va;
		ea = a + n;
		bp = kprof.buf + offset/SZ;
		while(a < ea){
			w = *bp++;
			*a++ = w>>24;
			*a++ = w>>16;
			*a++ = w>>8;
			*a++ = w>>0;
		}
		break;

	default:
		n = 0;
		break;
	}
	return n;
}

long
kprofwrite(Chan *c, char *a, long n, ulong offset)
{
	USED(offset);

	switch((int)(c->qid.path&~CHDIR)){
	case Kprofctlqid:
		if(strncmp(a, "startclr", 8) == 0){
			memset((char *)kprof.buf, 0, kprof.nbuf*SZ);
			kprof.time = 1;
		}else if(strncmp(a, "start", 5) == 0)
			kprof.time = 1;
		else if(strncmp(a, "stop", 4) == 0)
			kprof.time = 0;
		else
			error(Ebadctl);
		break;
	default:
		error(Ebadusefd);
	}
	return n;
}

void
kproftimer(ulong pc)
{
	extern void spldone(void);

	if(kprof.time == 0)
		return;
	/*
	 *  if the pc is coming out of spllo or splx,
	 *  use the pc saved when we went splhi.
	 */
	if(pc>=(ulong)splx && pc<=(ulong)spldone)
		pc = m->splpc;

	kprof.buf[0] += TK2MS(1);
	if(kprof.minpc<=pc && pc<kprof.maxpc){
		pc -= kprof.minpc;
		pc >>= LRES;
		kprof.buf[pc] += TK2MS(1);
	}else
		kprof.buf[1] += TK2MS(1);
}
