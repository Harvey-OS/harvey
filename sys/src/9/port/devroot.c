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

enum
{
	Qdir = 0,
	Qboot = 0x1000,

	Nrootfiles = 32,
	Nbootfiles = 32,
};

typedef struct Dirlist Dirlist;
struct Dirlist
{
	uint base;
	Dirtab *dir;
	uint8_t **data;
	int ndir;
	int mdir;
};

static Dirtab rootdir[Nrootfiles] = {
	{"#/",		{Qdir, 0, QTDIR},	0,		DMDIR|0555},
	{"boot",	{Qboot, 0, QTDIR},	0,		DMDIR|0555},
};
static uint8_t *rootdata[Nrootfiles];
static Dirlist rootlist =
{
	0,
	rootdir,
	rootdata,
	2,
	Nrootfiles
};

static Dirtab bootdir[Nbootfiles] = {
	{"boot",	{Qboot, 0, QTDIR},	0,		DMDIR|0555},
};
static uint8_t *bootdata[Nbootfiles];
static Dirlist bootlist =
{
	Qboot,
	bootdir,
	bootdata,
	1,
	Nbootfiles
};

/*
 *  add a file to the list
 */
static void
addlist(Dirlist *l, char *name, uint8_t *contents, uint32_t len,
        int perm)
{
	Dirtab *d;

	if(l->ndir >= l->mdir)
		panic("too many root files");
	l->data[l->ndir] = contents;
	d = &l->dir[l->ndir];
	strcpy(d->name, name);
	d->length = len;
	d->perm = perm;
	d->qid.type = 0;
	d->qid.vers = 0;
	d->qid.path = ++l->ndir + l->base;
	if(perm & DMDIR)
		d->qid.type |= QTDIR;
}

/*
 *  add a root file
 */
void
addbootfile(char *name, uint8_t *contents, uint32_t len)
{
	addlist(&bootlist, name, contents, len, 0555);
}

/*
 *  add a root directory
 */
static void
addrootdir(char *name)
{
	addlist(&rootlist, name, nil, 0, DMDIR|0555);
}

static void
rootreset(void)
{
	addrootdir("bin");
	addrootdir("dev");
	addrootdir("env");
	addrootdir("fd");
	addrootdir("mnt");
	addrootdir("n");
	addrootdir("net");
	addrootdir("net.alt");
	addrootdir("proc");
	addrootdir("root");
	addrootdir("srv");
}

static Chan*
rootattach(char *spec)
{
	return devattach('/', spec);
}

static int
rootgen(Chan *c, char *name, Dirtab* dir, int j, int s, Dir *dp)
{
	int t;
	Dirtab *d;
	Dirlist *l;

	switch((int)c->qid.path){
	case Qdir:
		if(s == DEVDOTDOT){
			devdir(c, (Qid){Qdir, 0, QTDIR}, "#/", 0, eve, 0555, dp);
			return 1;
		}
		return devgen(c, name, rootlist.dir, rootlist.ndir, s, dp);
	case Qboot:
		if(s == DEVDOTDOT){
			devdir(c, (Qid){Qdir, 0, QTDIR}, "#/", 0, eve, 0555, dp);
			return 1;
		}
		return devgen(c, name, bootlist.dir, bootlist.ndir, s, dp);
	default:
		if(s == DEVDOTDOT){
			if((int)c->qid.path < Qboot)
				devdir(c, (Qid){Qdir, 0, QTDIR}, "#/", 0, eve, 0555, dp);
			else
				devdir(c, (Qid){Qboot, 0, QTDIR}, "#/", 0, eve, 0555, dp);
			return 1;
		}
		if(s != 0)
			return -1;
		if((int)c->qid.path < Qboot){
			t = c->qid.path-1;
			l = &rootlist;
		}else{
			t = c->qid.path - Qboot - 1;
			l = &bootlist;
		}
		if(t >= l->ndir)
			return -1;
if(t < 0){
print("rootgen %#llx %d %d\n", c->qid.path, s, t);
panic("whoops");
}
		d = &l->dir[t];
		devdir(c, d->qid, d->name, d->length, eve, d->perm, dp);
		return 1;
	}
}

static Walkqid*
rootwalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c,  nc, name, nname, nil, 0, rootgen);
}

static int32_t
rootstat(Chan *c, uint8_t *dp, int32_t n)
{
	return devstat(c, dp, n, nil, 0, rootgen);
}

static Chan*
rootopen(Chan *c, int omode)
{
	return devopen(c, omode, nil, 0, devgen);
}

/*
 * sysremove() knows this is a nop
 */
static void
rootclose(Chan* c)
{
}

static int32_t
rootread(Chan *c, void *buf, int32_t n, int64_t off)
{
	uint32_t t;
	Dirtab *d;
	Dirlist *l;
	uint8_t *data;
	uint32_t offset = off;

	t = c->qid.path;
	switch(t){
	case Qdir:
	case Qboot:
		return devdirread(c, buf, n, nil, 0, rootgen);
	}

	if(t<Qboot)
		l = &rootlist;
	else{
		t -= Qboot;
		l = &bootlist;
	}

	t--;
	if(t >= l->ndir)
		error(Egreg);

	d = &l->dir[t];
	data = l->data[t];
	if(offset >= d->length)
		return 0;
	if(offset+n > d->length)
		n = d->length - offset;
	memmove(buf, data+offset, n);
	return n;
}

static int32_t
rootwrite(Chan* c, void* v, int32_t n, int64_t m)
{
	error(Egreg);
	return 0;
}

Dev rootdevtab = {
	.dc = '/',
	.name = "root",

	.reset = rootreset,
	.init = devinit,
	.shutdown = devshutdown,
	.attach = rootattach,
	.walk = rootwalk,
	.stat = rootstat,
	.open = rootopen,
	.create = devcreate,
	.close = rootclose,
	.read = rootread,
	.bread = devbread,
	.write = rootwrite,
	.bwrite = devbwrite,
	.remove = devremove,
	.wstat = devwstat,
};
