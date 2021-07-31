/*
 *  keyboard scan code input from outside the kernel.
 *  to avoid duplication of keyboard map processing for usb.
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

extern	void kbdputsc(int, int);

enum {
	Qdir,
	Qkbd,
};

Dirtab kbintab[] = {
	".",	{Qdir, 0, QTDIR},	0,	0555,
	"kbin",	{Qkbd, 0},		0,	0200,
};

Lock	kbinlck;
int	kbinbusy;

static Chan *
kbinattach(char *spec)
{
	return devattach(L'Ι', spec);
}

static Walkqid*
kbinwalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, kbintab, nelem(kbintab), devgen);
}

static int
kbinstat(Chan *c, uchar *dp, int n)
{
	return devstat(c, dp, n, kbintab, nelem(kbintab), devgen);
}

static Chan*
kbinopen(Chan *c, int omode)
{
	if(!iseve())
		error(Eperm);
	if(c->qid.path == Qkbd){
		lock(&kbinlck);
		if(kbinbusy){
			unlock(&kbinlck);
			error(Einuse);
		}
		kbinbusy++;
		unlock(&kbinlck);
	}
	return devopen(c, omode, kbintab, nelem(kbintab), devgen);
}

static void
kbinclose(Chan *c)
{
	if(c->aux){
		free(c->aux);
		c->aux = nil;
	}
	if(c->qid.path == Qkbd)
		kbinbusy = 0;
}

static long
kbinread(Chan *c, void *a, long n, vlong )
{
	if(c->qid.type == QTDIR)
		return devdirread(c, a, n, kbintab, nelem(kbintab), devgen);
	return 0;
}

static long
kbinwrite(Chan *c, void *a, long n, vlong)
{
	int i;
	uchar *p = a;

	if(c->qid.type == QTDIR)
		error(Eisdir);
	switch((int)c->qid.path){
	case Qkbd:
		for(i = 0; i < n; i++)
			kbdputsc(*p++, 1);	/* external source */
		break;
	default:
		error(Egreg);
	}
	return n;
}

Dev kbindevtab = {
	L'Ι',
	"kbin",

	devreset,
	devinit,
	devshutdown,
	kbinattach,
	kbinwalk,
	kbinstat,
	kbinopen,
	devcreate,
	kbinclose,
	kbinread,
	devbread,
	kbinwrite,
	devbwrite,
	devremove,
	devwstat,
};
