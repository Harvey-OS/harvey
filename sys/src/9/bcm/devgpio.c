/*
 * Raspberry Pi (BCM2835) GPIO
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

enum {
	// GPIO registers
	GPLEV = VIRTIO + 0x200034,
};

enum{
	Qdir = 0,
	Qgpio,
};

Dirtab gpiodir[]={
	".",	{Qdir, 0, QTDIR},	0,	0555,
	"gpio",	{Qgpio, 0},	0,	0664,
};

enum {
	// commands
	CMfunc,
	CMset,
	CMpullup,
	CMpulldown,
	CMfloat,
};

static Cmdtab gpiocmd[] = {
	{CMfunc, "function", 3},
	{CMset, "set", 3},
	{CMpullup, "pullup", 2},
	{CMpulldown, "pulldown", 2},
	{CMfloat, "float", 2},
};

static char *funcs[] = { "in", "out", "alt5", "alt4", "alt0",
	"alt1", "alt2", "alt3", "pulse"};
static int ifuncs[] = { Input, Output, Alt5, Alt4, Alt0,
	Alt1, Alt2, Alt3, -1};

static Chan*
gpioattach(char* spec)
{
	return devattach('G', spec);
}

static Walkqid*	 
gpiowalk(Chan* c, Chan *nc, char** name, int nname)
{
	return devwalk(c, nc, name, nname, gpiodir, nelem(gpiodir), devgen);
}

static int	 
gpiostat(Chan* c, uchar* dp, int n)
{
	return devstat(c, dp, n, gpiodir, nelem(gpiodir), devgen);
}

static Chan*
gpioopen(Chan* c, int omode)
{
	return devopen(c, omode, gpiodir, nelem(gpiodir), devgen);
}

static void	 
gpioclose(Chan*)
{
}

static long	 
gpioread(Chan* c, void *buf, long n, vlong)
{
	char lbuf[20];
	char *e;

	USED(c);
	if(c->qid.path == Qdir)
		return devdirread(c, buf, n, gpiodir, nelem(gpiodir), devgen);
	e = lbuf + sizeof(lbuf);
	seprint(lbuf, e, "%08ulx%08ulx", ((ulong *)GPLEV)[1], ((ulong *)GPLEV)[0]);
	return readstr(0, buf, n, lbuf);
}

static long	 
gpiowrite(Chan* c, void *buf, long n, vlong)
{
	Cmdbuf *cb;
	Cmdtab *ct;
	int pin, i;

	if(c->qid.type & QTDIR)
		error(Eperm);
	cb = parsecmd(buf, n);
	if(waserror()) {
		free(cb);
		nexterror();
	}
	ct = lookupcmd(cb, gpiocmd, nelem(gpiocmd));
	pin = atoi(cb->f[1]);
	switch(ct->index) {
	case CMfunc:
		for(i = 0; i < nelem(funcs); i++)
			if(strcmp(funcs[i], cb->f[2]) == 0)
				break;
		if(i >= nelem(funcs))
			error(Ebadctl);
		if(ifuncs[i] == -1) {
			gpiosel(pin, Output);
			microdelay(2);
			gpiosel(pin, Input);
		}
		else {
			gpiosel(pin, ifuncs[i]);
		}
		break;
	case CMset:
		gpioout(pin, atoi(cb->f[2]));
		break;
	case CMpullup:
		gpiopullup(pin);
		break;
	case CMpulldown:
		gpiopulldown(pin);
		break;
	case CMfloat:
		gpiopulloff(pin);
		break;
	}
	free(cb);
	poperror();
	return n;
}

Dev gpiodevtab = {
	'G',
	"gpio",

	devreset,
	devinit,
	devshutdown,
	gpioattach,
	gpiowalk,
	gpiostat,
	gpioopen,
	devcreate,
	gpioclose,
	gpioread,
	devbread,
	gpiowrite,
	devbwrite,
	devremove,
	devwstat,
};
