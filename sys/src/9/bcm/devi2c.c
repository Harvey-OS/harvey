/*
 * i2c
 *
 * Copyright © 1998, 2003 Vita Nuova Limited.
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"

typedef struct I2Cdir I2Cdir;

enum{
	Qdir,
	Qdata,
	Qctl,
};

static
Dirtab i2ctab[]={
	".",	{Qdir, 0, QTDIR},	0,	0555,
	"i2cdata",		{Qdata, 0},	256,	0660,
	"i2cctl",		{Qctl, 0},		0,	0660,
};

struct I2Cdir {
	Ref;
	I2Cdev;
	Dirtab	tab[nelem(i2ctab)];
};

static void
i2creset(void)
{
	i2csetup(0);
}

static Chan*
i2cattach(char* spec)
{
	char *s;
	ulong addr;
	I2Cdir *d;
	Chan *c;

	addr = strtoul(spec, &s, 16);
	if(*spec == 0 || *s || addr >= (1<<10))
		error("invalid i2c address");
	d = malloc(sizeof(I2Cdir));
	if(d == nil)
		error(Enomem);
	d->ref = 1;
	d->addr = addr;
	d->salen = 0;
	d->tenbit = addr >= 128;
	memmove(d->tab, i2ctab, sizeof(d->tab));
	sprint(d->tab[1].name, "i2c.%lux.data", addr);
	sprint(d->tab[2].name, "i2c.%lux.ctl", addr);

	c = devattach('J', spec);
	c->aux = d;
	return c;
}

static Walkqid*
i2cwalk(Chan* c, Chan *nc, char **name, int nname)
{
	Walkqid *wq;
	I2Cdir *d;

	d = c->aux;
	wq = devwalk(c, nc, name, nname, d->tab, nelem(d->tab), devgen);
	if(wq != nil && wq->clone != nil && wq->clone != c)
		incref(d);
	return wq;
}

static int
i2cstat(Chan* c, uchar *dp, int n)
{
	I2Cdir *d;

	d = c->aux;
	return devstat(c, dp, n, d->tab, nelem(d->tab), devgen);
}

static Chan*
i2copen(Chan* c, int omode)
{
	I2Cdir *d;

	d = c->aux;
	return devopen(c, omode, d->tab, nelem(d->tab), devgen);
}

static void
i2cclose(Chan *c)
{
	I2Cdir *d;

	d = c->aux;
	if(decref(d) == 0)
		free(d);
}

static long
i2cread(Chan *c, void *a, long n, vlong offset)
{
	I2Cdir *d;
	char *s, *e;
	ulong len;

	d = c->aux;
	switch((ulong)c->qid.path){
	case Qdir:
		return devdirread(c, a, n, d->tab, nelem(d->tab), devgen);
	case Qdata:
		len = d->tab[1].length;
		if(offset+n >= len){
			n = len - offset;
			if(n <= 0)
				return 0;
		}
		n = i2crecv(d, a, n, offset);
		break;
	case Qctl:
		s = smalloc(READSTR);
		if(waserror()){
			free(s);
			nexterror();
		}
		e = seprint(s, s+READSTR, "size %lud\n", (ulong)d->tab[1].length);
		if(d->salen)
			e = seprint(e, s+READSTR, "subaddress %d\n", d->salen);
		if(d->tenbit)
			seprint(e, s+READSTR, "a10\n");
		n = readstr(offset, a, n, s);
		poperror();
		free(s);
		return n;
	default:
		n=0;
		break;
	}
	return n;
}

static long
i2cwrite(Chan *c, void *a, long n, vlong offset)
{
	I2Cdir *d;
	long len;
	Cmdbuf *cb;

	USED(offset);
	switch((ulong)c->qid.path){
	case Qdata:
		d = c->aux;
		len = d->tab[1].length;
		if(offset+n >= len){
			n = len - offset;
			if(n <= 0)
				return 0;
		}
		n = i2csend(d, a, n, offset);
		break;
	case Qctl:
		cb = parsecmd(a, n);
		if(waserror()){
			free(cb);
			nexterror();
		}
		if(cb->nf < 1)
			error(Ebadctl);
		d = c->aux;
		if(strcmp(cb->f[0], "subaddress") == 0){
			if(cb->nf > 1){
				len = strtol(cb->f[1], nil, 0);
				if(len <= 0)
					len = 0;
				if(len > 4)
					cmderror(cb, "subaddress too long");
			}else
				len = 1;
			d->salen = len;
		}else if(cb->nf > 1 && strcmp(cb->f[0], "size") == 0){
			len = strtol(cb->f[1], nil, 0);
			if(len < 0)
				cmderror(cb, "size is negative");
			d->tab[1].length = len;
		}else if(strcmp(cb->f[0], "a10") == 0)
			d->tenbit = 1;
		else
			cmderror(cb, "unknown control request");
		poperror();
		free(cb);
		break;
	default:
		error(Ebadusefd);
	}
	return n;
}

Dev i2cdevtab = {
	'J',
	"i2c",

	i2creset,
	devinit,
	devshutdown,
	i2cattach,
	i2cwalk,
	i2cstat,
	i2copen,
	devcreate,
	i2cclose,
	i2cread,
	devbread,
	i2cwrite,
	devbwrite,
	devremove,
	devwstat,
};
