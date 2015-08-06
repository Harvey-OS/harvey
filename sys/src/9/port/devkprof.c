/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include        "ureg.h"
#include        "../port/portfns.h"


#define LRES	3		/* log of PC resolution */
#define SZ	4		/* sizeof of count cell; well known as 4 */

struct
{
	uintptr	minpc;
	uintptr	maxpc;
	int	nbuf;
	int	time;
	uint32_t	*buf;
	Lock lock;
}kprof;

enum{
	Kprofdirqid,
	Kprofdataqid,
	Kprofctlqid,
	Kprofoprofileqid,
};

Dirtab kproftab[]={
	".",		{Kprofdirqid, 0, QTDIR},0,	DMDIR|0550,
	"kpdata",	{Kprofdataqid},		0,	0600,
	"kpctl",	{Kprofctlqid},		0,	0600,
	"kpoprofile",	{Kprofoprofileqid},	0,	0600,
};

void oprof_alarm_handler(Ureg *u)
{
	//int coreid = machp()->machno;
	if (u->ip > KTZERO)
		oprofile_add_backtrace(u->ip, u->bp);
	else
		oprofile_add_userpc(u->ip);
}

static Chan*
kprofattach(char *spec)
{
	uint32_t n;

	/* allocate when first used */
	kprof.minpc = KTZERO;
	kprof.maxpc = PTR2UINT(etext);
	kprof.nbuf = (kprof.maxpc-kprof.minpc) >> LRES;
	n = kprof.nbuf*SZ;
	if(kprof.buf == 0) {
		kprof.buf = malloc(n);
		if(kprof.buf == 0)
			error(Enomem);
	}
	kproftab[1].length = n;
	alloc_cpu_buffers();
	print("Kprof attached. Buf is %p, %d bytes, minpc %p maxpc %p\n",
			kprof.buf, n, (void *)kprof.minpc, (void *)kprof.maxpc);
	return devattach('K', spec);
}

static void
_kproftimer(uintptr_t pc)
{
	if(kprof.time == 0)
		return;

	/*
	 * if the pc corresponds to the idle loop, don't consider it.
	if(m->inidle)
		return;
	 */
	/*
	 *  if the pc is coming out of spllo or splx,
	 *  use the pc saved when we went splhi.
	 */
	if(pc>=PTR2UINT(spllo) && pc<=PTR2UINT(spldone))
		pc = machp()->splpc;

	ilock(&(&kprof.l)->lock);
	kprof.buf[0] += TK2MS(1);
	if(kprof.minpc<=pc && pc<kprof.maxpc){
		pc -= kprof.minpc;
		pc >>= LRES;
		kprof.buf[pc] += TK2MS(1);
	}else
		kprof.buf[1] += TK2MS(1);
	iunlock(&(&kprof.l)->lock);
}

static void
kprofinit(void)
{
	if(SZ != sizeof kprof.buf[0])
		panic("kprof size");
	kproftimer = _kproftimer;
}

static Walkqid*
kprofwalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, kproftab, nelem(kproftab), devgen);
}

static int32_t
kprofstat(Chan *c, uint8_t *db, int32_t n)
{
	return devstat(c, db, n, kproftab, nelem(kproftab), devgen);
}

static Chan*
kprofopen(Chan *c, int omode)
{
	if(c->qid.type & QTDIR){
		if(omode != OREAD)
			error(Eperm);
	}
	c->mode = openmode(omode);
	c->flag |= COPEN;
	c->offset = 0;
	return c;
}

static void
kprofclose(Chan* c)
{
}

static int32_t
kprofread(Chan *c, void *va, int32_t n, int64_t off)
{
	uint32_t end;
	uint32_t w, *bp;
	uint8_t *a, *ea;
	uint32_t offset = off;

	switch((int)c->qid.path){
	case Kprofdirqid:
		return devdirread(c, va, n, kproftab, nelem(kproftab), devgen);

	case Kprofdataqid:
		end = kprof.nbuf*SZ;
		if(offset & (SZ-1))
			error(Ebadarg);
		if(offset >= end){
			n = 0;
			break;
		}
		if(offset+n > end)
			n = end-offset;
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

	case Kprofoprofileqid:
		n = oprofread(va,n);
		break;

	default:
		n = 0;
		break;
	}
	return n;
}

static int32_t
kprofwrite(Chan *c, void *a, int32_t n, int64_t m)
{
	switch((int)(c->qid.path)){
	case Kprofctlqid:
		if(strncmp(a, "startclr", 8) == 0){
			memset((char *)kprof.buf, 0, kprof.nbuf*SZ);
			kprof.time = 1;
		}else if(strncmp(a, "start", 5) == 0)
			kprof.time = 1;
		else if(strncmp(a, "stop", 4) == 0) {
			print("kprof stopped. %d ms\n", kprof.buf[0]);
			kprof.time = 0;
		} else if (!strcmp(a, "opstart")) {
			oprofile_control_trace(1);
		} else if (!strcmp(a, "opstop")) {
			oprofile_control_trace(0);
		} else {
			error("startclr|start|stop|opstart|opstop");
		}
		break;
	default:
		error(Ebadusefd);
	}
	return n;
}

Dev kprofdevtab = {
	'K',
	"kprof",

	devreset,
	kprofinit,
	devshutdown,
	kprofattach,
	kprofwalk,
	kprofstat,
	kprofopen,
	devcreate,
	kprofclose,
	kprofread,
	devbread,
	kprofwrite,
	devbwrite,
	devremove,
	devwstat,
};
