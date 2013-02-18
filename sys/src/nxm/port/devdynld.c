#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include	<a.out.h>
#include	<dynld.h>
#include	<kernel.h>

/*
 * TO DO
 *	- dynamic allocation of Dev.dc
 *	- inter-module dependencies
 *	- reference count on Dev to allow error("inuse") [or how else to do it]
 *	- is Dev.shutdown the right function to call?  Dev.config perhaps?
 */

#define	DBG	if(1) print
#define	NATIVE


extern ulong ndevs;

enum
{
	Qdir,
	Qdynld,
	Qdynsyms,

	DEVCHAR	= 'L',
};

static Dirtab	dltab[] =
{
	".",			{Qdir, 0, QTDIR},	0,	DMDIR|0555,
	"dynld",		{Qdynld},	0,	0644,
	"dynsyms",	{Qdynsyms},	0,	0444,
};

typedef struct Dyndev Dyndev;

struct Dyndev
{
	char*	name;	/* device name (eg, "dynld") */
	char*	tag;	/* version tag (eg, MD5 or SHA1 hash of content) */
	char*	path;	/* file from whence it came */
	Dynobj*	o;
	Dev*	dev;
	Dyndev*	next;
};

static	Dyndev	*loaded;
static	QLock	dllock;

static	Dyndev**	finddyndev(char*);
static	int	matched(Dyndev*, char*, char*);

extern	Dynobj*	kdynloadfd(int, Dynsym*, int, ulong);

static void
dlfree(Dyndev *l)
{
	if(l != nil){
		free(l->tag);
		free(l->name);
		free(l->path);
		dynobjfree(l->o);
		free(l);
	}
}

static Dyndev*
dlload(char *path, Dynsym *tab, int ntab)
{
	Dyndev *l;
	int fd;

	/* in Plan 9, would probably use Chan* interface here */
	fd = kopen(path, OREAD);
	if(fd < 0)
		error("cannot open");
	if(waserror()){
		kclose(fd);
		nexterror();
	}
	l = mallocz(sizeof(Dyndev), 1);
	if(l == nil)
		error(Enomem);
	if(waserror()){
		dlfree(l);
		nexterror();
	}
	l->path = strdup(path);
	if(l->path == nil)
		error(Enomem);
	l->o = kdynloadfd(fd, tab, ntab, 0);
	if(l->o == nil)
		error(up->env->errstr);
	poperror();
	poperror();
	kclose(fd);
	return l;
}

static void
devload(char *name, char *path, char *tag)
{
	int i;
	Dyndev *l, **lp;
	Dev *dev;
	char tabname[32];

	lp = finddyndev(name);
	if(*lp != nil)
		error("already loaded");	/* i'm assuming the name (eg, "cons") is to be unique */
	l = dlload(path, _exporttab, dyntabsize(_exporttab));
	if(waserror()){
		dlfree(l);
		nexterror();
	}
	snprint(tabname, sizeof(tabname), "%sdevtab", name);
	dev = dynimport(l->o, tabname, signof(*dev));
	if(dev == nil)
		errorf("can't find %sdevtab in module", name);
	kstrdup(&l->name, name);
	kstrdup(&l->tag, tag != nil? tag: "");
	if(dev->name == nil)
		dev->name = l->name;
	else if(strcmp(dev->name, l->name) != 0)
		errorf("module file has device %s", dev->name);
	/* TO DO: allocate dev->dc dynamically (cf. brucee's driver) */
	if(devno(dev->dc, 1) >= 0)
		errorf("devchar %C already used", dev->dc);
	for(i = 0; devtab[i] != nil; i++)
		;
	if(i >= ndevs || devtab[i+1] != nil)
		error("device table full");
#ifdef NATIVE
	i = splhi();
	dev->reset();
	splx(i);
#endif
	dev->init();
	l->dev = devtab[i] = dev;
	l->next = loaded;
	loaded = l;			/* recently loaded ones first: good unload order? */
	poperror();
}

static Dyndev**
finddyndev(char *name)
{
	Dyndev *l, **lp;

	for(lp = &loaded; (l = *lp) != nil; lp = &l->next)
		if(strcmp(l->name, name) == 0)
			break;
	return lp;
}

static int
matched(Dyndev *l, char *path, char *tag)
{
	if(path != nil && strcmp(l->path, path) != 0)
		return 0;
	if(tag != nil && strcmp(l->tag, tag) != 0)
		return 0;
	return 1;
}

static void
devunload(char *name, char *path, char *tag)
{
	int i;
	Dyndev *l, **lp;

	lp = finddyndev(name);
	l = *lp;
	if(l == nil)
		error("not loaded");
	if(!matched(l, path, tag))
		error("path/tag mismatch");
	for(i = 0; i < ndevs; i++)
		if(l->dev == devtab[i]){
			devtab[i] = nil;		/* TO DO: ensure driver is not currently in use */
			break;
		}
#ifdef NATIVE
	l->dev->shutdown();
#endif
	*lp = l->next;
	dlfree(l);
}

static long
readdynld(void *a, ulong n, ulong offset)
{
	char *p;
	Dyndev *l;
	int m, len;

	m = 0;
	for(l = loaded; l != nil; l = l->next)
		m += 48 + strlen(l->name) + strlen(l->path) + strlen(l->tag);
	p = malloc(m);
	if(p == nil)
		error(Enomem);
	if(waserror()){
		free(p);
		nexterror();
	}
	*p = 0;
	len = 0;
	for(l = loaded; l != nil; l = l->next)
		if(l->dev)
			len += snprint(p+len, m-len, "#%C\t%.8p\t%.8lud\t%q\t%q\t%q\n",
					l->dev->dc, l->o->base, l->o->size, l->name, l->path, l->tag);
	n = readstr(offset, a, n, p);
	poperror();
	free(p);
	return n;
}

static long
readsyms(char *a, ulong n, ulong offset)
{
	char *p;
	Dynsym *t;
	long l, nr;

	p = malloc(READSTR);
	if(p == nil)
		error(Enomem);
	if(waserror()){
		free(p);
		nexterror();
	}
	nr = 0;
	for(t = _exporttab; n > 0 && t->name != nil; t++){
		l = snprint(p, READSTR, "%.8lux %.8lux %s\n", t->addr, t->sig, t->name);
		if(offset >= l){
			offset -= l;
			continue;
		}
		l = readstr(offset, a, n, p);
		offset = 0;
		n -= l;
		a += l;
		nr += l;
	}
	poperror();
	free(p);
	return nr;
}

static Chan*
dlattach(char *spec)
{
	return devattach(DEVCHAR, spec);
}

static Walkqid*
dlwalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, dltab, nelem(dltab), devgen);
}

static int
dlstat(Chan *c, uchar *db, int n)
{
	return devstat(c, db, n, dltab, nelem(dltab), devgen);
}

static Chan*
dlopen(Chan *c, int omode)
{
	return devopen(c, omode, dltab, nelem(dltab), devgen);
}

static void
dlclose(Chan *c)
{
	USED(c);
}

static long
dlread(Chan *c, void *a, long n, vlong voffset)
{
	switch((ulong)c->qid.path){
	case Qdir:
		return devdirread(c, a, n, dltab, nelem(dltab), devgen);
	case Qdynld:
		return readdynld(a, n, voffset);
	case Qdynsyms:
		return readsyms(a, n, voffset);
	default:
		error(Egreg);
	}
	return n;
}

static long
dlwrite(Chan *c, void *a, long n, vlong voffset)
{
	Cmdbuf *cb;
	char *name, *tag, *path;

	USED(voffset);
	switch((ulong)c->qid.path){
	case Qdynld:
		cb = parsecmd(a, n);
		qlock(&dllock);
		if(waserror()){
			qunlock(&dllock);
			free(cb);
			nexterror();
		}
		if(cb->nf < 3 || strcmp(cb->f[1], "dev") != 0)	/* only do devices */
			cmderror(cb, Ebadctl);
		name = cb->f[2];
		path = nil;
		if(cb->nf > 3)
			path = cb->f[3];
		tag = nil;
		if(cb->nf > 4)
			tag = cb->f[4];
		if(strcmp(cb->f[0], "load") == 0){
			if(path == nil)
				cmderror(cb, "missing load path");
			devload(name, path, tag);	/* TO DO: remaining parameters might be dependencies? */
		}else if(strcmp(cb->f[0], "unload") == 0)
			devunload(name, path, tag);
		else
			cmderror(cb, Ebadctl);
		poperror();
		qunlock(&dllock);
		free(cb);
		break;
	default:
		error(Egreg);
	}
	return n;
}

Dev dynlddevtab = {
	DEVCHAR,
	"dynld",

	devreset,
	devinit,
	devshutdown,	/* TO DO */
	dlattach,
	dlwalk,
	dlstat,
	dlopen,
	devcreate,
	dlclose,
	dlread,
	devbread,
	dlwrite,
	devbwrite,
	devremove,
	devwstat,
};
