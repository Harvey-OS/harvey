/*
 * File system devices.
 * Follows device config in Ken's file server.
 * Builds mirrors, concatenations, interleavings, and partitions
 * of devices out of other (inner) devices.
 * It is ok if inner devices are provided by this driver.
 *
 * Built files are grouped on different directories
 * (called trees, and used to represent disks).
 * The "#k/fs" tree is always available and never goes away.
 * Configuration changes happen only while no I/O is in progress.
 *
 * Default sector size is one byte unless changed by the "disk" ctl.
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "ureg.h"
#include "../port/error.h"

enum
{
	Fnone,
	Fmirror,		/* mirror of others */
	Fcat,			/* catenation of others */
	Finter,			/* interleaving of others */
	Fpart,			/* part of other */
	Fclear,			/* start over */
	Fdel,			/* delete a configure device */
	Fdisk,			/* set default tree and sector sz*/

	Sectorsz = 1,
	Blksize	= 8*1024,	/* for Finter only */

	Incr = 5,		/* Increments for the dev array */

	/*
	 * All qids are decorated with the tree number.
	 * #k/fs is tree number 0, is automatically added and
	 * its first qid is for the ctl file. It never goes away.
	 */
	Qtop	= 0,		/* #k */
	Qdir,			/* directory (#k/fs) */
	Qctl,			/* ctl, only for #k/fs/ctl */
	Qfirst,			/* first qid assigned for device */

	Iswrite = 0,
	Isread,

	Optional = 0,
	Mustexist,

	/* tunable parameters */
	Maxconf	= 4*1024,	/* max length for config */
	Ndevs	= 32,		/* max. inner devs per command */
	Ntrees	= 128,		/* max. number of trees */
	Maxretries = 3,		/* max. retries of i/o errors */
	Retrypause = 5000,	/* ms. to pause between retries */
};

typedef struct Inner Inner;
typedef struct Fsdev Fsdev;
typedef struct Tree Tree;

struct Inner
{
	char	*iname;		/* inner device name */
	vlong	isize;		/* size of inner device */
	Chan	*idev;		/* inner device */
};

struct Fsdev
{
	Ref;			/* one per Chan doing I/O */
	int	gone;		/* true if removed */
	int	vers;		/* qid version for this device */
	int	type;		/* Fnone, Fmirror, ... */
	char	*name;		/* name for this fsdev */
	Tree*	tree;		/* where the device is kept */
	vlong	size;		/* min(inner[X].isize) */
	vlong	start;		/* start address (for Fpart) */
	uint	ndevs;		/* number of inner devices */
	Inner	*inner[Ndevs];	/* inner devices */
};

struct Tree
{
	char	*name;		/* name for #k/<name> */
	Fsdev	**devs;		/* devices in dir. */
	uint	ndevs;		/* number of devices */
	uint	nadevs;		/* number of allocated devices in devs */
};

#define dprint if(debug)print

extern Dev fsdevtab;		/* forward */

static RWlock lck;		/* r: use devices; w: change config  */
static Tree fstree;		/* The main "fs" tree. Never goes away */
static Tree *trees[Ntrees];	/* internal representation of config */
static int ntrees;		/* max number of trees */
static int qidvers;
static char *disk;		/* default tree name used */
static char *source;		/* default inner device used */
static int sectorsz = Sectorsz;	/* default sector size */
static char confstr[Maxconf];	/* textual configuration */

static int debug;

static char cfgstr[] = "fsdev:\n";

static Qid tqid = {Qtop, 0, QTDIR};
static Qid cqid = {Qctl, 0, 0};

static char* tnames[] = {
	[Fmirror]	"mirror",
	[Fcat]		"cat",
	[Finter]	"inter",
	[Fpart]		"part",
};

static Cmdtab configs[] = {
	Fmirror,"mirror",	0,
	Fcat,	"cat",		0,
	Finter,	"inter",	0,
	Fpart,	"part",		0,
	Fclear,	"clear",	1,
	Fdel,	"del",		2,
	Fdisk,	"disk",		0,
};

static char Egone[] = "device is gone";		/* file has been removed */

static char*
seprintdev(char *s, char *e, Fsdev *mp)
{
	int i;

	if(mp == nil)
		return seprint(s, e, "<null Fsdev>");
	if(mp->type < 0 || mp->type >= nelem(tnames) || tnames[mp->type] == nil)
		return seprint(s, e, "bad device type %d\n", mp->type);

	s = strecpy(s, e, tnames[mp->type]);
	if(mp->tree != &fstree)
		s = seprint(s, e, " %s/%s", mp->tree->name, mp->name);
	else
		s = seprint(s, e, " %s", mp->name);
	for(i = 0; i < mp->ndevs; i++)
		s = seprint(s, e, " %s", mp->inner[i]->iname);
	switch(mp->type){
	case Fmirror:
	case Fcat:
	case Finter:
		s = strecpy(s, e, "\n");
		break;
	case Fpart:
		s = seprint(s, e, " %ulld %ulld\n", mp->start, mp->size);
		break;
	default:
		panic("#k: seprintdev bug");
	}
	return s;
}

static vlong
mkpath(int tree, int devno)
{
	return (tree&0xFFFF)<<16 | devno&0xFFFF;
}

static int
path2treeno(int q)
{
	return q>>16 & 0xFFFF;
}

static int
path2devno(int q)
{
	return q & 0xFFFF;
}

static Tree*
gettree(int i, int mustexist)
{
	dprint("gettree %d\n", i);
	if(i < 0)
		panic("#k: bug: bad tree index %d in gettree", i);
	if(i >= ntrees || trees[i] == nil)
		if(mustexist)
			error(Enonexist);
		else
			return nil;
	return trees[i];
}

static Fsdev*
getdev(Tree *t, int i, int mustexist)
{
	dprint("getdev %d\n", i);
	if(i < 0)
		panic("#k: bug: bad dev index %d in getdev", i);
	if(i >= t->nadevs || t->devs[i] == nil)
		if(mustexist)
			error(Enonexist);
		else
			return nil;
	return t->devs[i];
}

static Fsdev*
path2dev(int q)
{
	Tree	*t;

	dprint("path2dev %ux\n", q);
	t = gettree(path2treeno(q), Mustexist);
	return getdev(t, path2devno(q) - Qfirst, Mustexist);
}

static Tree*
treealloc(char *name)
{
	int	i;
	Tree	*t;

	dprint("treealloc %s\n", name);
	for(i = 0; i < nelem(trees); i++)
		if(trees[i] == nil)
			break;
	if(i == nelem(trees))
		return nil;
	t = trees[i] = mallocz(sizeof(Tree), 1);
	if(t == nil)
		return nil;
	if(i == ntrees)
		ntrees++;
	kstrdup(&t->name, name);
	return t;
}

static Tree*
lookuptree(char *name)
{
	int i;

	dprint("lookuptree %s\n", name);
	for(i = 0; i < ntrees; i++)
		if(trees[i] != nil && strcmp(trees[i]->name, name) == 0)
			return trees[i];
	return nil;
}

static Fsdev*
devalloc(Tree *t, char *name)
{
	int	i, ndevs;
	Fsdev	*mp, **devs;

	dprint("devalloc %s %s\n", t->name, name);
	mp = mallocz(sizeof(Fsdev), 1);
	if(mp == nil)
		return nil;
	for(i = 0; i < t->nadevs; i++)
		if(t->devs[i] == nil)
			break;
	if(i >= t->nadevs){
		if(t->nadevs % Incr == 0){
			ndevs = t->nadevs + Incr;
			devs = realloc(t->devs, ndevs * sizeof(Fsdev*));
			if(devs == nil){
				free(mp);
				return nil;
			}
			t->devs = devs;
		}
		t->devs[t->nadevs] = nil;
		t->nadevs++;
	}
	kstrdup(&mp->name, name);
	mp->vers = ++qidvers;
	mp->tree = t;
	t->devs[i] = mp;
	t->ndevs++;
	return mp;
}

static void
deltree(Tree *t)
{
	int i;

	dprint("deltree %s\n", t->name);
	for(i = 0; i < ntrees; i++)
		if(trees[i] == t){
			if(i > 0){		/* "fs" never goes away */
				free(t->name);
				free(t->devs);
				free(t);
				trees[i] = nil;
			}
			return;
		}
	panic("#k: deltree: bug: tree not found");
}

/*
 * A device is gone and we know that all its users are gone.
 * A tree is gone when all its devices are gone ("fs" is never gone).
 * Must close devices outside locks, so we could nest our own devices.
 */
static void
mdeldev(Fsdev *mp)
{
	int	i;
	Inner	*in;
	Tree	*t;

	dprint("deldev %s gone %d ref %uld\n", mp->name, mp->gone, mp->ref);

	mp->gone = 1;
	mp->vers = ++qidvers;

	wlock(&lck);
	t = mp->tree;
	for(i = 0; i < t->nadevs; i++)
		if(t->devs[i] == mp){
			t->devs[i] = nil;
			t->ndevs--;
			if(t->ndevs == 0)
				deltree(t);
			break;
		}
	wunlock(&lck);

	free(mp->name);
	for(i = 0; i < mp->ndevs; i++){
		in = mp->inner[i];
		if(in->idev != nil)
			cclose(in->idev);
		free(in->iname);
		free(in);
	}
	if(debug)
		memset(mp, 9, sizeof *mp);	/* poison */
	free(mp);
}

/*
 * Delete one or all devices in one or all trees.
 */
static void
mdelctl(char *tname, char *dname)
{
	int i, alldevs, alltrees, some;
	Fsdev *mp;
	Tree *t;

	dprint("delctl %s\n", dname);
	alldevs = strcmp(dname, "*") == 0;
	alltrees = strcmp(tname, "*") == 0;
	some = 0;
Again:
	wlock(&lck);
	for(i = 0; i < ntrees; i++){
		t = trees[i];
		if(t == nil)
			continue;
		if(alltrees == 0 && strcmp(t->name, tname) != 0)
			continue;
		for(i = 0; i < t->nadevs; i++){
			mp = t->devs[i];
			if(t->devs[i] == nil)
				continue;
			if(alldevs == 0 && strcmp(mp->name, dname) != 0)
				continue;
			/*
			 * Careful: must close outside locks and that
			 * may change the file tree we are looking at.
			 */
			some++;
			mp->gone = 1;
			if(mp->ref == 0){
				incref(mp);	/* keep it there */
				wunlock(&lck);
				mdeldev(mp);
				goto Again;	/* tree can change */
			}
		}
	}
	wunlock(&lck);
	if(some == 0 && alltrees == 0)
		error(Enonexist);
}

static void
setdsize(Fsdev* mp, vlong *ilen)
{
	int	i;
	vlong	inlen;
	Inner	*in;

	dprint("setdsize %s\n", mp->name);
	for (i = 0; i < mp->ndevs; i++){
		in = mp->inner[i];
		in->isize = ilen[i];
		inlen = in->isize;
		switch(mp->type){
		case Finter:
			/* truncate to multiple of Blksize */
			inlen &= ~(Blksize-1);
			in->isize = inlen;
			/* fall through */
		case Fmirror:
			/* use size of smallest inner device */
			if (mp->size == 0 || mp->size > inlen)
				mp->size = inlen;
			break;
		case Fcat:
			mp->size += inlen;
			break;
		case Fpart:
			if(mp->start > inlen)
				error("partition starts after device end");
			if(inlen < mp->start + mp->size){
				print("#k: %s: partition truncated from "
					"%lld to %lld bytes\n", mp->name,
					mp->size, inlen - mp->start);
				mp->size = inlen - mp->start;
			}
			break;
		}
	}
	if(mp->type == Finter)
		mp->size *= mp->ndevs;
}

static void
validdevname(Tree *t, char *dname)
{
	int i;

	for(i = 0; i < t->nadevs; i++)
		if(t->devs[i] != nil && strcmp(t->devs[i]->name, dname) == 0)
			error(Eexist);
}

static void
parseconfig(char *a, long n, Cmdbuf **cbp, Cmdtab **ctp)
{
	Cmdbuf	*cb;
	Cmdtab	*ct;

	*cbp = cb = parsecmd(a, n);
	*ctp = ct = lookupcmd(cb, configs, nelem(configs));

	cb->f++;			/* skip command */
	cb->nf--;
	switch(ct->index){
	case Fmirror:
	case Fcat:
	case Finter:
		if(cb->nf < 2)
			error("too few arguments for ctl");
		if(cb->nf - 1 > Ndevs)
			error("too many devices in ctl");
		break;
	case Fdisk:
		if(cb->nf < 1 || cb->nf > 3)
			error("ctl usage: disk name [sz dev]");
		break;
	case Fpart:
		if(cb->nf != 4 && (cb->nf != 3 || source == nil))
			error("ctl usage: part new [file] off len");
		break;
	}
}

static void
parsename(char *name, char *disk, char **tree, char **dev)
{
	char *slash;

	slash = strchr(name, '/');
	if(slash == nil){
		if(disk != nil)
			*tree = disk;
		else
			*tree = "fs";
		*dev = name;
	}else{
		*tree = name;
		*slash++ = 0;
		*dev = slash;
	}
	validname(*tree, 0);
	validname(*dev, 0);
}

static vlong
getlen(Chan *c)
{
	uchar	buf[128];	/* old DIRLEN plus a little should be plenty */
	Dir	d;
	long	l;

	l = devtab[c->type]->stat(c, buf, sizeof buf);
	convM2D(buf, l, &d, nil);
	return d.length;
}

/*
 * Process a single line of configuration,
 * often of the form "cmd newname idev0 idev1".
 * locking is tricky, because we need a write lock to
 * add/remove devices yet adding/removing them may lead
 * to calls to this driver that require a read lock (when
 * inner devices are also provided by us).
 */
static void
mconfig(char* a, long n)
{
	int	i;
	vlong	size, start;
	vlong	*ilen;
	char	*tname, *dname, *fakef[4];
	Chan	**idev;
	Cmdbuf	*cb;
	Cmdtab	*ct;
	Fsdev	*mp;
	Inner	*inprv;
	Tree	*t;

	/* ignore comments & empty lines */
	if (*a == '\0' || *a == '#' || *a == '\n')
		return;

	dprint("mconfig\n");
	size = 0;
	start = 0;
	mp = nil;
	cb = nil;
	idev = nil;
	ilen = nil;

	if(waserror()){
		free(cb);
		nexterror();
	}

	parseconfig(a, n, &cb, &ct);
	switch (ct->index) {
	case Fdisk:
		kstrdup(&disk, cb->f[0]);
		if(cb->nf >= 2)
			sectorsz = strtoul(cb->f[1], 0, 0);
		else
			sectorsz = Sectorsz;
		if(cb->nf == 3)
			kstrdup(&source, cb->f[2]);
		else{
			free(source);
			source = nil;
		}
		poperror();
		free(cb);
		return;
	case Fclear:
		poperror();
		free(cb);
		mdelctl("*", "*");		/* del everything */
		return;
	case Fpart:
		if(cb->nf == 3){
			/*
			 * got a request in the format of sd(3),
			 * pretend we got one in our format.
			 * later we change end to be len.
			 */
			fakef[0] = cb->f[0];
			fakef[1] = source;
			fakef[2] = cb->f[1];
			fakef[3] = cb->f[2];
			cb->f = fakef;
			cb->nf = 4;
		}
		start = strtoll(cb->f[2], nil, 10);
		size =  strtoll(cb->f[3], nil, 10);
		if(cb->f == fakef)
			size -= start;		/* it was end */
		cb->nf -= 2;
		break;
	}
	parsename(cb->f[0], disk, &tname, &dname);
	for(i = 1; i < cb->nf; i++)
		validname(cb->f[i], 1);

	if(ct->index == Fdel){
		mdelctl(tname, dname);
		poperror();
		free(cb);
		return;
	}

	/*
	 * Open all inner devices while we have only a read lock.
	 */
	poperror();
	rlock(&lck);
	if(waserror()){
		runlock(&lck);
Fail:
		for(i = 1; i < cb->nf; i++)
			if(idev != nil && idev[i-1] != nil)
				cclose(idev[i]);
		if(mp != nil)
			mdeldev(mp);
		free(idev);
		free(ilen);
		free(cb);
		nexterror();
	}
	idev = smalloc(sizeof(Chan*) * Ndevs);
	ilen = smalloc(sizeof(vlong) * Ndevs);
	for(i = 1; i < cb->nf; i++){
		idev[i-1] = namec(cb->f[i], Aopen, ORDWR, 0);
		ilen[i-1] = getlen(idev[i-1]);
	}
	poperror();
	runlock(&lck);

	/*
	 * Get a write lock and add the device if we can.
	 */
	wlock(&lck);
	if(waserror()){
		wunlock(&lck);
		goto Fail;
	}

	t = lookuptree(tname);
	if(t != nil)
		validdevname(t, dname);
	else
		t = treealloc(tname);
	if(t == nil)
		error("no more trees");
	mp = devalloc(t, dname);
	if(mp == nil){
		if(t->ndevs == 0)	/* it was created for us */
			deltree(t);	/* but we will not mdeldev() */
		error(Enomem);
	}

	mp->type = ct->index;
	if(mp->type == Fpart){
		mp->start = start * sectorsz;
		mp->size = size * sectorsz;
	}
	for(i = 1; i < cb->nf; i++){
		inprv = mp->inner[i-1] = mallocz(sizeof(Inner), 1);
		if(inprv == nil)
			error(Enomem);
		mp->ndevs++;
		kstrdup(&inprv->iname, cb->f[i]);
		inprv->idev = idev[i-1];
		idev[i-1] = nil;
	}
	setdsize(mp, ilen);

	poperror();
	wunlock(&lck);
	free(idev);
	free(ilen);
	free(cb);
}

static void
rdconf(void)
{
	int mustrd;
	char *c, *e, *p, *s;
	Chan *cc;
	static int configed;

	/* only read config file once */
	if (configed)
		return;
	configed = 1;

	dprint("rdconf\n");
	/* add the std "fs" tree */
	trees[0] = &fstree;
	ntrees++;
	fstree.name = "fs";

	/* identify the config file */
	s = getconf("fsconfig");
	if (s == nil){
		mustrd = 0;
		s = "/dev/sdC0/fscfg";
	} else
		mustrd = 1;

	/* read it */
	cc = nil;
	c = nil;
	if (waserror()){
		if (cc != nil)
			cclose(cc);
		if (c)
			free(c);
		if (!mustrd)
			return;
		nexterror();
	}
	cc = namec(s, Aopen, OREAD, 0);
	devtab[cc->type]->read(cc, confstr, sizeof confstr, 0);
	cclose(cc);
	cc = nil;

	/* validate, copy and erase config; mconfig will repopulate confstr */
	if (strncmp(confstr, cfgstr, sizeof cfgstr - 1) != 0)
		error("bad #k config, first line must be: 'fsdev:\\n'");
	kstrdup(&c, confstr + sizeof cfgstr - 1);
	memset(confstr, 0, sizeof confstr);

	/* process config copy one line at a time */
	for (p = c; p != nil && *p != '\0'; p = e){
		e = strchr(p, '\n');
		if (e == nil)
			e = p + strlen(p);
		else
			e++;
		mconfig(p, e - p);
	}
	USED(cc);		/* until now, can be used in waserror clause */
	poperror();
}

static int
mgen(Chan *c, char*, Dirtab*, int, int i, Dir *dp)
{
	int	treeno;
	Fsdev	*mp;
	Qid	qid;
	Tree	*t;

	dprint("mgen %#ullx %d\n", c->qid.path, i);
	qid.type = QTDIR;
	qid.vers = 0;
	if(c->qid.path == Qtop){
		if(i == DEVDOTDOT){
			devdir(c, tqid, "#k", 0, eve, DMDIR|0775, dp);
			return 1;
		}
		t = gettree(i, Optional);
		if(t == nil){
			dprint("no\n");
			return -1;
		}
		qid.path = mkpath(i, Qdir);
		devdir(c, qid, t->name, 0, eve, DMDIR|0775, dp);
		return 1;
	}

	treeno = path2treeno(c->qid.path);
	t = gettree(treeno, Optional);
	if(t == nil){
		dprint("no\n");
		return -1;
	}
	if((c->qid.type & QTDIR) != 0){
		if(i == DEVDOTDOT){
			devdir(c, tqid, "#k", 0, eve, DMDIR|0775, dp);
			return 1;
		}
		if(treeno == 0){
			/* take care of #k/fs/ctl */
			if(i == 0){
				devdir(c, cqid, "ctl", 0, eve, 0664, dp);
				return 1;
			}
			i--;
		}
		mp = getdev(t, i, Optional);
		if(mp == nil){
			dprint("no\n");
			return -1;
		}
		qid.type = QTFILE;
		qid.vers = mp->vers;
		qid.path = mkpath(treeno, Qfirst+i);
		devdir(c, qid, mp->name, mp->size, eve, 0664, dp);
		return 1;
	}

	if(i == DEVDOTDOT){
		qid.path = mkpath(treeno, Qdir);
		devdir(c, qid, t->name, 0, eve, DMDIR|0775, dp);
		return 1;
	}
	dprint("no\n");
	return -1;
}

static Chan*
mattach(char *spec)
{
	dprint("mattach\n");
	return devattach(fsdevtab.dc, spec);
}

static Walkqid*
mwalk(Chan *c, Chan *nc, char **name, int nname)
{
	Walkqid *wq;

	rdconf();

	dprint("mwalk %llux\n", c->qid.path);
	rlock(&lck);
	if(waserror()){
		runlock(&lck);
		nexterror();
	}
	wq = devwalk(c, nc, name, nname, 0, 0, mgen);
	poperror();
	runlock(&lck);
	return wq;
}

static int
mstat(Chan *c, uchar *db, int n)
{
	int	p;
	Dir	d;
	Fsdev	*mp;
	Qid	q;
	Tree	*t;

	dprint("mstat %llux\n", c->qid.path);
	rlock(&lck);
	if(waserror()){
		runlock(&lck);
		nexterror();
	}
	p = c->qid.path;
	memset(&d, 0, sizeof d);
	switch(p){
	case Qtop:
		devdir(c, tqid, "#k", 0, eve, DMDIR|0775, &d);
		break;
	case Qctl:
		devdir(c, cqid, "ctl", 0, eve, 0664, &d);
		break;
	default:
		t = gettree(path2treeno(p), Mustexist);
		if(c->qid.type & QTDIR)
			devdir(c, c->qid, t->name, 0, eve, DMDIR|0775, &d);
		else{
			mp = getdev(t, path2devno(p) - Qfirst, Mustexist);
			q = c->qid;
			q.vers = mp->vers;
			devdir(c, q, mp->name, mp->size, eve, 0664, &d);
		}
	}
	n = convD2M(&d, db, n);
	if (n == 0)
		error(Ebadarg);
	poperror();
	runlock(&lck);
	return n;
}

static Chan*
mopen(Chan *c, int omode)
{
	int	q;
	Fsdev	*mp;

	dprint("mopen %llux\n", c->qid.path);
	if((c->qid.type & QTDIR) && omode != OREAD)
		error(Eperm);
	if(c->qid.path != Qctl && (c->qid.type&QTDIR) == 0){
		rlock(&lck);
		if(waserror()){
			runlock(&lck);
			nexterror();
		}
		q = c->qid.path;
		mp = path2dev(q);
		if(mp->gone)
			error(Egone);
		incref(mp);
		poperror();
		runlock(&lck);
	}
	/*
	 * Our mgen does not return the info for the qid
	 * but only for its children. Don't use devopen here.
	 */
	c->offset = 0;
	c->mode = openmode(omode & ~OTRUNC);
	c->flag |= COPEN;
	return c;
}

static void
mclose(Chan *c)
{
	int	mustdel, q;
	Fsdev	*mp;

	dprint("mclose %llux\n", c->qid.path);
	if(c->qid.type & QTDIR || !(c->flag & COPEN))
		return;
	rlock(&lck);
	if(waserror()){
		runlock(&lck);
		nexterror();
	}
	mustdel = 0;
	mp = nil;
	q = c->qid.path;
	if(q == Qctl){
		free(disk);
		disk = nil;	/* restore defaults */
		free(source);
		source = nil;
		sectorsz = Sectorsz;
	}else{
		mp = path2dev(q);
		if(mp->gone != 0 && mp->ref == 1)
			mustdel = 1;
		else
			decref(mp);
	}
	poperror();
	runlock(&lck);
	if(mustdel)
		mdeldev(mp);
}

static long
io(Fsdev *mp, Inner *in, int isread, void *a, long l, vlong off)
{
	long wl;
	Chan	*mc;

	mc = in->idev;
	if(mc == nil)
		error(Egone);
	if (waserror()) {
		print("#k: %s: byte %,lld count %ld (of #k/%s): %s error: %s\n",
			in->iname, off, l, mp->name, (isread? "read": "write"),
			(up && up->errstr? up->errstr: ""));
		nexterror();
	}
	if (isread)
		wl = devtab[mc->type]->read(mc, a, l, off);
	else
		wl = devtab[mc->type]->write(mc, a, l, off);
	poperror();
	return wl;
}

/* NB: a transfer could span multiple inner devices */
static long
catio(Fsdev *mp, int isread, void *a, long n, vlong off)
{
	int	i;
	long	l, res;
	Inner	*in;

	if(debug)
		print("catio %d %p %ld %lld\n", isread, a, n, off);
	res = n;
	for (i = 0; n > 0 && i < mp->ndevs; i++){
		in = mp->inner[i];
		if (off >= in->isize){
			off -= in->isize;
			continue;		/* not there yet */
		}
		if (off + n > in->isize)
			l = in->isize - off;
		else
			l = n;
		if(debug)
			print("\tdev %d %p %ld %lld\n", i, a, l, off);

		if (io(mp, in, isread, a, l, off) != l)
			error(Eio);

		a = (char*)a + l;
		off = 0;
		n -= l;
	}
	if(debug)
		print("\tres %ld\n", res - n);
	return res - n;
}

static long
interio(Fsdev *mp, int isread, void *a, long n, vlong off)
{
	int	i;
	long	boff, res, l, wl, wsz;
	vlong	woff, blk, mblk;

	blk  = off / Blksize;
	boff = off % Blksize;
	wsz  = Blksize - boff;
	res = n;
	while(n > 0){
		mblk = blk / mp->ndevs;
		i    = blk % mp->ndevs;
		woff = mblk*Blksize + boff;
		if (n > wsz)
			l = wsz;
		else
			l = n;

		wl = io(mp, mp->inner[i], isread, a, l, woff);
		if (wl != l)
			error(Eio);

		blk++;
		boff = 0;
		wsz = Blksize;
		a = (char*)a + l;
		n -= l;
	}
	return res;
}

static char*
seprintconf(char *s, char *e)
{
	int	i, j;
	Tree	*t;

	*s = 0;
	for(i = 0; i < ntrees; i++){
		t = trees[i];
		if(t != nil)
			for(j = 0; j < t->nadevs; j++)
				if(t->devs[j] != nil)
					s = seprintdev(s, e, t->devs[j]);
	}
	return s;
}

static long
mread(Chan *c, void *a, long n, vlong off)
{
	int	i, retry;
	long	l, res;
	Fsdev	*mp;
	Tree	*t;

	dprint("mread %llux\n", c->qid.path);
	rlock(&lck);
	if(waserror()){
		runlock(&lck);
		nexterror();
	}
	res = -1;
	if(c->qid.type & QTDIR){
		res = devdirread(c, a, n, 0, 0, mgen);
		goto Done;
	}
	if(c->qid.path == Qctl){
		seprintconf(confstr, confstr + sizeof(confstr));
		res = readstr((long)off, a, n, confstr);
		goto Done;
	}

	t = gettree(path2treeno(c->qid.path), Mustexist);
	mp = getdev(t, path2devno(c->qid.path) - Qfirst, Mustexist);

	if(off >= mp->size){
		res = 0;
		goto Done;
	}
	if(off + n > mp->size)
		n = mp->size - off;
	if(n == 0){
		res = 0;
		goto Done;
	}

	switch(mp->type){
	case Fcat:
		res = catio(mp, Isread, a, n, off);
		break;
	case Finter:
		res = interio(mp, Isread, a, n, off);
		break;
	case Fpart:
		res = io(mp, mp->inner[0], Isread, a, n, mp->start + off);
		break;
	case Fmirror:
		retry = 0;
		do {
			if (retry > 0) {
				print("#k/%s: retry %d read for byte %,lld "
					"count %ld: %s\n", mp->name, retry, off,
					n, (up && up->errstr? up->errstr: ""));
				/*
				 * pause before retrying in case it's due to
				 * a transient bus or controller problem.
				 */
				tsleep(&up->sleep, return0, 0, Retrypause);
			}
			for (i = 0; i < mp->ndevs; i++){
				if (waserror())
					continue;
				l = io(mp, mp->inner[i], Isread, a, n, off);
				poperror();
				if (l >= 0){
					res = l;
					break;		/* read a good copy */
				}
			}
		} while (i == mp->ndevs && ++retry <= Maxretries);
		if (retry > Maxretries) {
			/* no mirror had a good copy of the block */
			print("#k/%s: byte %,lld count %ld: CAN'T READ "
				"from mirror: %s\n", mp->name, off, n,
				(up && up->errstr? up->errstr: ""));
			error(Eio);
		} else if (retry > 0)
			print("#k/%s: byte %,lld count %ld: retry read OK "
				"from mirror: %s\n", mp->name, off, n,
				(up && up->errstr? up->errstr: ""));
		break;
	}
Done:
	poperror();
	runlock(&lck);
	return res;
}

static long
mwrite(Chan *c, void *a, long n, vlong off)
{
	int	i, allbad, anybad, retry;
	long	l, res;
	Fsdev	*mp;
	Tree	*t;

	dprint("mwrite %llux\n", c->qid.path);
	if (c->qid.type & QTDIR)
		error(Eisdir);
	if (c->qid.path == Qctl){
		mconfig(a, n);
		return n;
	}

	rlock(&lck);
	if(waserror()){
		runlock(&lck);
		nexterror();
	}

	t = gettree(path2treeno(c->qid.path), Mustexist);
	mp = getdev(t, path2devno(c->qid.path) - Qfirst, Mustexist);

	if(off >= mp->size){
		res = 0;
		goto Done;
	}
	if(off + n > mp->size)
		n = mp->size - off;
	if(n == 0){
		res = 0;
		goto Done;
	}
	res = n;
	switch(mp->type){
	case Fcat:
		res = catio(mp, Iswrite, a, n, off);
		break;
	case Finter:
		res = interio(mp, Iswrite, a, n, off);
		break;
	case Fpart:
		res = io(mp, mp->inner[0], Iswrite, a, n, mp->start + off);
		if (res != n)
			error(Eio);
		break;
	case Fmirror:
		retry = 0;
		do {
			if (retry > 0) {
				print("#k/%s: retry %d write for byte %,lld "
					"count %ld: %s\n", mp->name, retry, off,
					n, (up && up->errstr? up->errstr: ""));
				/*
				 * pause before retrying in case it's due to
				 * a transient bus or controller problem.
				 */
				tsleep(&up->sleep, return0, 0, Retrypause);
			}
			allbad = 1;
			anybad = 0;
			for (i = mp->ndevs - 1; i >= 0; i--){
				if (waserror()) {
					anybad = 1;
					continue;
				}
				l = io(mp, mp->inner[i], Iswrite, a, n, off);
				poperror();
				if (l == n)
					allbad = 0;	/* wrote a good copy */
				else
					anybad = 1;
			}
		} while (anybad && ++retry <= Maxretries);
		if (allbad) {
			/* no mirror took a good copy of the block */
			print("#k/%s: byte %,lld count %ld: CAN'T WRITE "
				"to mirror: %s\n", mp->name, off, n,
				(up && up->errstr? up->errstr: ""));
			error(Eio);
		} else if (retry > 0)
			print("#k/%s: byte %,lld count %ld: retry wrote OK "
				"to mirror: %s\n", mp->name, off, n,
				(up && up->errstr? up->errstr: ""));

		break;
	}
Done:
	poperror();
	runlock(&lck);
	return res;
}

Dev fsdevtab = {
	'k',
	"fs",

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
