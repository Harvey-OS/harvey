/*
 * File system devices.
 * '#k'. 
 * Follows device config in Ken's file server.
 * Builds mirrors, device cats, interleaving, and partition of devices out of
 * other (inner) devices.
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "ureg.h"
#include "../port/error.h"

enum {
	Fmirror,	// mirror of others
	Fcat,		// catenation of others
	Finter,		// interleaving of others
	Fpart,		// part of others

	Blksize	= 8*1024,	// for Finter only
	Maxconf	= 1024,		// max length for config

	Nfsdevs = 64,
	Ndevs	= 8,

	Qtop	= 0,	// top dir (contains "fs")
	Qdir	= 1,	// actual dir
	Qctl	= 2,	// ctl file
	Qfirst	= 3,	// first fs file
};

#define	Cfgstr	"fsdev:\n"

typedef struct Fsdev Fsdev;

struct Fsdev
{
	int	type;
	char	*name;		// name for this fsdev
	vlong	start;		// start address (for Fpart)
	vlong	size;		// min(idev sizes)
	int	ndevs;		// number of inner devices
	char	*iname[Ndevs];	// inner device names
	Chan	*idev[Ndevs];	// inner devices
	vlong	isize[Ndevs];	// sizes for inneer devices
};

/*
 * Once configured, a fsdev is never removed. The name of those
 * configured is never nil. We have no locks here.
 */
static Fsdev	fsdev[Nfsdevs];

static Qid	tqid = {Qtop, 0, QTDIR};
static Qid	dqid = {Qdir, 0, QTDIR};
static Qid	cqid = {Qctl, 0, 0};

static Cmdtab configs[] = {
	Fmirror,"mirror",	0,
	Fcat,	"cat",		0,
	Finter,	"inter",	0,
	Fpart,	"part",		5,
};

static char	confstr[Maxconf];
static int	configed;


static Fsdev*
path2dev(int i, int mustexist)
{
	if (i < 0 || i >= nelem(fsdev))
		error("bug: bad index in devfsdev");
	if (mustexist && fsdev[i].name == nil)
		error(Enonexist);

	if (fsdev[i].name == nil)
		return nil;
	else
		return &fsdev[i];
}

static Fsdev*
devalloc(void)
{
	int	i;

	for (i = 0; i < nelem(fsdev); i++)
		if (fsdev[i].name == nil)
			break;
	if (i == nelem(fsdev))
		error(Enodev);

	return &fsdev[i];
}

static void
setdsize(Fsdev* mp)
{
	uchar	buf[128];	/* old DIRLEN plus a little should be plenty */
	int	i;
	Chan	*mc;
	Dir	d;
	long	l;

	if (mp->type != Fpart){
		mp->start= 0;
		mp->size = 0LL;
	}
	for (i = 0; i < mp->ndevs; i++){
		mc = mp->idev[i];
		l = devtab[mc->type]->stat(mc, buf, sizeof(buf));
		convM2D(buf, l, &d, nil);
		mp->isize[i] = d.length;
		switch(mp->type){
		case Fmirror:
			if (mp->size == 0LL || mp->size > d.length)
				mp->size = d.length;
			break;
		case Fcat:
			mp->size += d.length;
			break;
		case Finter:
			// truncate to multiple of Blksize
			d.length = (d.length & ~(Blksize-1));
			mp->isize[i] = d.length;
			mp->size += d.length;
			break;
		case Fpart:
			// should raise errors here?
			if (mp->start > d.length)
				mp->start = d.length;
			if (d.length < mp->start + mp->size)
				mp->size = d.length - mp->start;
			break;
		}
	}
}

static void
mpshut(Fsdev *mp)
{
	int	i;
	char	*nm;

	nm = mp->name;
	mp->name = nil;		// prevent others from using this.
	if (nm)
		free(nm);
	for (i = 0; i < mp->ndevs; i++){
		if (mp->idev[i] != nil)
			cclose(mp->idev[i]);
		if (mp->iname[i])
			free(mp->iname[i]);
	}
	memset(mp, 0, sizeof(*mp));
}


static void
mconfig(char* a, long n)	// "name idev0 idev1"
{
	static	QLock	lck;
	Cmdbuf	*cb;
	Cmdtab	*ct;
	Fsdev	*mp;
	int	i;
	char	*oldc;
	char	*c;
	vlong	size, start;

	size = 0;
	start = 0;
	if (confstr[0] == 0)
		seprint(confstr, confstr+sizeof(confstr), Cfgstr);
	mp = nil;
	cb = nil;
	oldc = confstr + strlen(confstr);
	qlock(&lck);
	if (waserror()){
		*oldc = 0;
		if (mp != nil)
			mpshut(mp);
		qunlock(&lck);
		if (cb)
			free(cb);
		nexterror();
	}
	cb = parsecmd(a, n);
	c = oldc;
	for (i = 0; i < cb->nf; i++)
		c = seprint(c, confstr+sizeof(confstr), "%s ", cb->f[i]);
	*(c-1) = '\n';
	ct = lookupcmd(cb, configs, nelem(configs));
	cb->f++;	// skip command
	cb->nf--;
	if (ct->index == Fpart){
		size = strtoll(cb->f[3], nil, 10);
		cb->nf--;
		start = strtoll(cb->f[2], nil, 10);
		cb->nf--;
	}
	for (i = 0; i < nelem(fsdev); i++)
		if (fsdev[i].name != nil && strcmp(fsdev[i].name, cb->f[0])==0)
			error(Eexist);
	if (cb->nf - 1 > Ndevs)
		error("too many devices; fix me");
	for (i = 0; i < cb->nf; i++)
		validname(cb->f[i], (i != 0));
	mp = devalloc();
	mp->type = ct->index;
	if (mp->type == Fpart){
		mp->size = size;
		mp->start = start;
	}
	kstrdup(&mp->name, cb->f[0]);
	for (i = 1; i < cb->nf; i++){
		kstrdup(&mp->iname[i-1], cb->f[i]);
		mp->idev[i-1] = namec(mp->iname[i-1], Aopen, ORDWR, 0);
		if (mp->idev[i-1] == nil)
			error(Egreg);
		mp->ndevs++;
	}
	setdsize(mp);
	poperror();
	configed = 1;
	qunlock(&lck);
	free(cb);
	
}

static void
rdconf(void)
{
	int	mustrd;
	char	*s;
	char	*c;
	char	*p;
	char	*e;
	Chan *cc;
	Chan **ccp;

	s = getconf("fsconfig");
	if (s == nil){
		mustrd = 0;
		s = "/dev/sdC0/fscfg";
	} else
		mustrd = 1;
	ccp = &cc;
	*ccp = nil;
	c = nil;
	if (waserror()){
		configed = 1;
		if (*ccp != nil)
			cclose(*ccp);
		if (c)
			free(c);
		if (!mustrd)
			return;
		nexterror();
	}
	*ccp = namec(s, Aopen, OREAD, 0);
	devtab[(*ccp)->type]->read(*ccp, confstr, sizeof(confstr), 0);
	cclose(*ccp);
	*ccp = nil;
	if (strncmp(confstr, Cfgstr, strlen(Cfgstr)) != 0)
		error("Bad config, first line must be: 'fsdev:\\n'");
	kstrdup(&c, confstr + strlen(Cfgstr));
	memset(confstr, 0, sizeof(confstr));
	for (p = c; p != nil && *p != 0; p = e){
		e = strchr(p, '\n');
		if (e == nil)
			e = p + strlen(p);
		if (e == p) {
			e++;
			continue;
		}
		mconfig(p, e - p);
	}
	poperror();
}


static int
mgen(Chan *c, char*, Dirtab*, int, int i, Dir *dp)
{
	Qid	qid;
	Fsdev	*mp;

	if (c->qid.path == Qtop){
		switch(i){
		case DEVDOTDOT:
			devdir(c, tqid, "#k", 0, eve, DMDIR|0775, dp);
			return 1;
		case 0:
			devdir(c, dqid, "fs", 0, eve, DMDIR|0775, dp);
			return 1;
		default:
			return -1;
		}
	}
	if (c->qid.path != Qdir){
		switch(i){
		case DEVDOTDOT:
			devdir(c, dqid, "fs", 0, eve, DMDIR|0775, dp);
			return 1;
		default:
			return -1;
		}
	}
	switch(i){
	case DEVDOTDOT:
		devdir(c, tqid, "#k", 0, eve, DMDIR|0775, dp);
		return 1;
	case 0:
		devdir(c, cqid, "ctl", 0, eve, 0664, dp);
		return 1;
	}
	i--;	// for ctl
	qid.path = Qfirst + i;
	qid.vers = 0;
	qid.type = 0;
	mp = path2dev(i, 0);
	if (mp == nil)
		return -1;
	kstrcpy(up->genbuf, mp->name, sizeof(up->genbuf));
	devdir(c, qid, up->genbuf, mp->size, eve, 0664, dp);
	return 1;
}

static Chan*
mattach(char *spec)
{
	*confstr = 0;
	return devattach(L'k', spec);
}

static Walkqid*
mwalk(Chan *c, Chan *nc, char **name, int nname)
{
	if (!configed)
		rdconf();
	return devwalk(c, nc, name, nname, 0, 0, mgen);
}

static int
mstat(Chan *c, uchar *db, int n)
{
	Dir	d;
	Fsdev	*mp;
	int	p;

	p = c->qid.path;
	memset(&d, 0, sizeof(d));
	switch(p){
	case Qtop:
		devdir(c, tqid, "#k", 0, eve, DMDIR|0775, &d);
		break;
	case Qdir:
		devdir(c, dqid, "fs", 0, eve, DMDIR|0775, &d);
		break;
	case Qctl:
		devdir(c, cqid, "ctl", 0, eve, 0664, &d);
		break;
	default:
		mp = path2dev(p - Qfirst, 1);
		devdir(c, c->qid, mp->name, mp->size, eve, 0664, &d);
	}
	n = convD2M(&d, db, n);
	if (n == 0)
		error(Ebadarg);
	return n;
}

static Chan*
mopen(Chan *c, int omode)
{
	if((c->qid.type & QTDIR) && omode != OREAD)
		error(Eperm);
	if (omode & OTRUNC)
		omode &= ~OTRUNC;
	c->mode = openmode(omode);
	c->flag |= COPEN;
	c->offset = 0;
	return c;
}

static void
mclose(Chan*)
{
	// that's easy
}

static long
catio(Fsdev *mp, int isread, void *a, long n, vlong off)
{
	int	i;
	Chan*	mc;
	long	l, wl, res;
	//print("catio %d %p %ld %lld\n", isread, a, n, off);
	res = n;
	for (i = 0; n >= 0 && i < mp->ndevs ; i++){
		mc = mp->idev[i];
		if (off > mp->isize[i]){
			off -= mp->isize[i];
			continue;
		}
		if (off + n > mp->isize[i])
			l = mp->isize[i] - off;
		else
			l = n;
		//print("\tdev %d %p %ld %lld\n", i, a, l, off);

		if (isread)
			wl = devtab[mc->type]->read(mc, a, l, off);
		else
			wl = devtab[mc->type]->write(mc, a, l, off);
		if (wl != l)
			error("#k: write failed");
		a = (char*)a + l;
		off = 0;
		n -= l;
	}
	//print("\tres %ld\n", res - n);
	return res - n;
}

static long
interio(Fsdev *mp, int isread, void *a, long n, vlong off)
{
	int	i;
	Chan*	mc;
	long	l, wl, wsz;
	vlong	woff, blk, mblk;
	long	boff, res;

	blk  = off / Blksize;
	boff = off % Blksize;
	wsz  = Blksize - boff;
	res = n;
	while(n > 0){
		i    = blk % mp->ndevs;
		mc   = mp->idev[i];
		mblk = blk / mp->ndevs;
		woff = mblk * Blksize + boff;
		if (n > wsz)
			l = wsz;
		else
			l = n;
		if (isread)
			wl = devtab[mc->type]->read(mc, a, l, woff);
		else
			wl = devtab[mc->type]->write(mc, a, l, woff);
		if (wl != l || l == 0)
			error(Eio);
		a = (char*)a + l;
		n -= l;
		blk++;
		boff = 0;
		wsz = Blksize;
	}
	return res;
}

static long
mread(Chan *c, void *a, long n, vlong off)
{
	int	i;
	Fsdev	*mp;
	Chan	*mc;
	long	l;
	long	res;

	if (c->qid.type & QTDIR)
		return devdirread(c, a, n, 0, 0, mgen);
	if (c->qid.path == Qctl)
		return readstr((long)off, a, n, confstr + strlen(Cfgstr));
	i = c->qid.path - Qfirst;
	mp = path2dev(i, 1);

	if (off >= mp->size)
		return 0;
	if (off + n > mp->size)
		n = mp->size - off;
	if (n == 0)
		return 0;

	res = -1;
	switch(mp->type){
	case Fmirror:
		for (i = 0; i < mp->ndevs; i++){
			mc = mp->idev[i];
			if (waserror()){
				// if a read fails we let the user know and try
				// another device.
				print("#k: mread: (%llx %d): %s\n",
					c->qid.path, i, up->errstr);
				continue;
			}
			l = devtab[mc->type]->read(mc, a, n, off);
			poperror();
			if (l >=0){
				res = l;
				break;
			}
		}
		if (i == mp->ndevs)
			error(Eio);
		break;
	case Fcat:
		res = catio(mp, 1, a, n, off);
		break;
	case Finter:
		res = interio(mp, 1, a, n, off);
		break;
	case Fpart:
		off += mp->start;
		mc = mp->idev[0];
		res = devtab[mc->type]->read(mc, a, n, off);
		break;
	}
	return res;
}

static long
mwrite(Chan *c, void *a, long n, vlong off)
{
	Fsdev	*mp;
	long	l, res;
	int	i;
	Chan	*mc;

	if (c->qid.type & QTDIR)
		error(Eperm);
	if (c->qid.path == Qctl){
		mconfig(a, n);
		return n;
	}
	mp = path2dev(c->qid.path - Qfirst, 1);

	if (off >= mp->size)
		return 0;
	if (off + n > mp->size)
		n = mp->size - off;
	if (n == 0)
		return 0;
	res = n;
	switch(mp->type){
	case Fmirror:
		for (i = mp->ndevs-1; i >=0; i--){
			mc = mp->idev[i];
			l = devtab[mc->type]->write(mc, a, n, off);
			if (l < res)
				res = l;
		}
		break;
	case Fcat:
		res = catio(mp, 0, a, n, off);
		break;
	case Finter:
		res = interio(mp, 0, a, n, off);
		break;
	case Fpart:
		mc = mp->idev[0];
		off += mp->start;
		l = devtab[mc->type]->write(mc, a, n, off);
		if (l < res)
			res = l;
		break;
	}
	return res;
}

Dev fsdevtab = {
	'k',
	"devfs",

	devreset,
	devinit,
	devshutdown,
	mattach,
	mwalk,
	mstat,
	mopen,
	devcreate,
	mclose,
	mread,
	devbread,
	mwrite,
	devbwrite,
	devremove,
	devwstat,
	devpower,
	devconfig,
};
