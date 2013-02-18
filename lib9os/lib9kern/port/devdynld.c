#include	"dat.h"
#include	"fns.h"
#include	"error.h"
#include	<a.out.h>
#include	<dynld.h>

#define	DBG	if(1) print

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

enum
{
	DLdev,
	DLudev,
};

static Cmdtab	dlcmd[] =
{
	DLdev,	"dev",	2,
	DLudev,	"udev",	2,
};

typedef struct Dyncmd Dyncmd;

struct Dyncmd
{
	char *path;
	int argc;
	char **argv;
};


typedef struct Dyndev Dyndev;

struct Dyndev
{
	char	*path;
	Dynobj	*o;
	Dyncmd	*cmd;
	Dev	*dev;
	Dyndev	*next;
};

static	Dyndev	*loaded;
static	QLock	dllock;

typedef struct Fd Fd;
struct Fd {
	int	fd;
};

static long
readfd(void *a, void *buf, long nbytes)
{
	return kread(((Fd*)a)->fd, buf, nbytes);
}

static vlong
seekfd(void *a, vlong off, int t)
{
	return kseek(((Fd*)a)->fd, off, t);
}

static void
errfd(char *s)
{
	kstrcpy(up->env->errstr, s, ERRMAX);
}

static void
dlfree(Dyndev *l)
{
	if(l != nil){
		free(l->path);
		dynobjfree(l->o);
		free(l);
	}
}

static Dyndev*
dlload(char *path, Dynsym *tab, int ntab)
{
	Fd f;
	Dyndev *l;
	
	f.fd = kopen(path, OREAD);
	if(f.fd < 0)
		p9error("cannot open");
	if(waserror()){
		kclose(f.fd);
		// XXX: fixme
		panic("undefined symbol");
		//nexterror();
	}
	l = mallocz(sizeof(Dyndev), 1);
	if(l == nil)
		p9error(Enomem);
	if(waserror()){
		dlfree(l);
		nexterror();
	}
	l->path = strdup(path);
	if(l->path == nil)
		p9error(Enomem);
	l->o = dynloadgen(&f, readfd, seekfd, errfd, tab, ntab, 0);
	if(l->o == nil)
		p9error(up->env->errstr);
	poperror();
	poperror();
	
	kclose(f.fd);
	return l;
}


extern void libosmain(int argc, char *argv[]);

//static
void
cmdload(char *path, int argc, char *argv[])
{
	int i;
	Dyndev *l;
	void (*lom)(int argc, char *argv[]);	

	l = dlload(path, _exporttab, dyntabsize(_exporttab));
	if(waserror()){
		dlfree(l);
		panic("cmdload");
		nexterror();
	}

	lom = dynimport(l->o, "libosmain", signof(libosmain));
	if(lom == nil)
		p9error("no libosmain");
	l->cmd = malloc(sizeof(Dyncmd));
	l->cmd->path = strdup(path);
	l->cmd->argc = argc;
	l->cmd->argv = malloc(argc*sizeof(char*));
	for(i=0; i<argc; i++)
		l->cmd->argv[i]=strdup(argv[i]);
	l->next = loaded;
	loaded = l;		
// XXX: exits should clean this up?

	print("lom = %#p\n", lom);
	lom(argc, argv);
	print("lomed\n");


	poperror();
}

static void
devload(char *path)
{
	int i;
	Dyndev *l;
	Dev *dev;
	char devname[32];

	l = dlload(path, _exporttab, dyntabsize(_exporttab));
	if(waserror()){
		dlfree(l);
		nexterror();
	}
	snprint(devname, sizeof(devname), "%sdevtab", "XXX");	/* TO DO */
	dev = dynimport(l->o, devname, signof(*dev));
	if(dev == nil)
		p9error("no devtab");
	if(devno(dev->dc, 1) >= 0)
		p9error("device loaded");
	for(i = 0; devtab[i] != nil; i++)
		;
	if(i >= ndevs || devtab[i+1] != nil)
		p9error("device table full");
	l->dev = devtab[i] = dev;
	dev->init();
	l->next = loaded;
	loaded = l;
	poperror();
}

static void
devunload(char *path)
{
	int i, dc;
	Dyndev *l, **ll;

	dc = 0;
	if(strlen(path) == 1)
		dc = path[0];
	for(ll = &loaded; *ll != nil; ll = &(*ll)->next){
		if(path != nil && strcmp(path, (*ll)->path) == 0)
			break;
		if(dc != 0 && (*ll)->dev && dc == (*ll)->dev->dc)
			break;
	}
	if((l = *ll) != nil){
		for(i = 0; i < ndevs; i++)
			if(l->dev == devtab[i]){
				devtab[i] = nil;
				break;
			}
/*
		if(l->dev)
			l->dev->shutdown();
*/
		*ll = l->next;
		dlfree(l);
	}
}

// static
long
readdl(void *a, ulong n, ulong offset)
{
	int i;
	char *p;
	Dyndev *l;
	int m, len;

	m = 0;
	for(l = loaded; l != nil; l = l->next)
		m++;
	m *= 48;
	p = malloc(m);
	if(p == nil)
		p9error(Enomem);
	if(waserror()){
		free(p);
		nexterror();
	}
	*p = 0;
	len = 0;
	for(l = loaded; l != nil; l = l->next){
		if(l->dev)
			len += snprint(p+len, m-len, "#%C\t%.8p\t%.8lux\t%s\n",
					l->dev->dc, l->o->base, l->o->size, l->dev->name);
		if(l->cmd){
			len += snprint(p+len, m-len, "%s\t%.8p\t%.8lux\n",l->cmd->path, l->o->base, l->o->size);
		}
	}
	n = readstr(offset, a, n, p);
	poperror();
	free(p);
	return n;
}

//static 
long
readsyms(char *a, ulong n, ulong offset)
{
	char *p;
	Dynsym *t;
	long l, nr;

	p = malloc(READSTR);
	if(p == nil)
		p9error(Enomem);
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
		return readdl(a, n, (ulong)voffset);
	case Qdynsyms:
		return readsyms(a, n, (ulong)voffset);
	default:
		p9error(Egreg);
	}
	return n;
}

static long
dlwrite(Chan *c, void *a, long n, vlong voffset)
{
	Cmdbuf *cmd;
	Cmdtab *ct;

	USED(voffset);
	switch((ulong)c->qid.path){
	case Qdynld:
		cmd = parsecmd(a, n);
		qlock(&dllock);
		if(waserror()){
			qunlock(&dllock);
			free(cmd);
			nexterror();
		}
		ct = lookupcmd(cmd, dlcmd, nelem(dlcmd));
		switch(ct->index){
		case DLdev:
			devload(cmd->f[1]);
			break;
		case DLudev:
			devunload(cmd->f[1]);
			break;
		}
		poperror();
		qunlock(&dllock);
		free(cmd);
		break;
	default:
		p9error(Egreg);
	}
	return n;
}

Dev dynlddevtab = {
	DEVCHAR,
	"dynld",

	devinit,
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

/* auxiliary routines for dynamic loading of C modules */

Dynobj*
dynld(int fd)
{
	Fd f;
	
	f.fd = fd;
	return dynloadgen(&f, readfd, seekfd, errfd, _exporttab, dyntabsize(_exporttab), 0);
}

int
dynldable(int fd)
{
	Fd f;

	f.fd = fd;
	return dynloadable(&f, readfd, seekfd);
}
