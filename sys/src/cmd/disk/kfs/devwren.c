#include "all.h"

enum{
	MAXWREN = 7,
};

#define WMAGIC	"kfs wren device\n"

typedef struct Wren	Wren;

struct Wren{
	QLock;
	Device	dev;
	uvlong	size;
	int	fd;
};

static Wren	*wrens;
static int	maxwren;
char		*wrenfile;
int		nwren;
int		badmagic;

static Wren *
wren(Device dev)
{
	int i;

	for(i = 0; i < maxwren; i++)
		if(devcmp(dev, wrens[i].dev) == 0)
			return &wrens[i];
	panic("can't find wren for %D", dev);
	return 0;
}

void
wreninit(Device dev)
{
	char buf[MAXBUFSIZE];
	Wren *w;
	Dir *d;
	int fd, i;

	if(wrens == 0)
		wrens = ialloc(MAXWREN * sizeof *wrens);
	w = &wrens[maxwren];
	fd = open(wrenfile, ORDWR);
	if(fd < 0)
		panic("can't open %s", wrenfile);
	if((d = dirfstat(fd)) == nil)
		panic("can't stat %s\n", wrenfile);
	seek(fd, 0, 0);
	i = read(fd, buf, sizeof buf);
	if(i < sizeof buf)
		panic("can't read %s", wrenfile);
	badmagic = 0;
	RBUFSIZE = 1024;
	if(strncmp(buf+256, WMAGIC, strlen(WMAGIC)) == 0){
		RBUFSIZE = atol(buf+256+strlen(WMAGIC));
		if(RBUFSIZE % 512){
			fprint(2, "kfs: bad buffersize(%d): assuming 1k blocks\n", RBUFSIZE);
			RBUFSIZE = 1024;
		}
	}else
		badmagic = 1;
	w->dev = dev;
	w->size = d->length;
	free(d);
	w->fd = fd;
	maxwren++;
}

void
wrenream(Device dev)
{
	Wren *w;
	char buf[MAXBUFSIZE];
	int fd, i;

	if(RBUFSIZE % 512)
		panic("kfs: bad buffersize(%d): restart a multiple of 512\n", RBUFSIZE);
	if(RBUFSIZE > sizeof(buf))
		panic("kfs: bad buffersize(%d): must be at most %d\n", RBUFSIZE, sizeof(buf));

	print("kfs: reaming the file system using %d byte blocks\n", RBUFSIZE);
	w = wren(dev);
	fd = w->fd;
	memset(buf, 0, sizeof buf);
	sprint(buf+256, "%s%d\n", WMAGIC, RBUFSIZE);
	qlock(w);
	i = seek(fd, 0, 0) < 0 || write(fd, buf, RBUFSIZE) != RBUFSIZE;
	qunlock(w);
	if(i < 0)
		panic("can't ream disk");
}

int
wrentag(char *p, int tag, long qpath)
{
	Tag *t;

	t = (Tag*)(p+BUFSIZE);
	return t->tag != tag || (qpath&~QPDIR) != t->path;
}

int
wrencheck(Device dev)
{
	char buf[MAXBUFSIZE];

	if(badmagic)
		return 1;
	if(RBUFSIZE > sizeof(buf))
		panic("kfs: bad buffersize(%d): must be at most %d\n", RBUFSIZE, sizeof(buf));

	if(wrenread(dev, wrensuper(dev), buf) || wrentag(buf, Tsuper, QPSUPER)
	|| wrenread(dev, wrenroot(dev), buf) || wrentag(buf, Tdir, QPROOT))
		return 1;
	if(((Dentry *)buf)[0].mode & DALLOC)
		return 0;
	return 1;
}

long
wrensize(Device dev)
{
	return wren(dev)->size / RBUFSIZE;
}

long
wrensuper(Device dev)
{
	USED(dev);
	return 1;
}

long
wrenroot(Device dev)
{
	USED(dev);
	return 2;
}

int
wrenread(Device dev, long addr, void *b)
{
	Wren *w;
	int fd, i;

	w = wren(dev);
	fd = w->fd;
	qlock(w);
	i = seek(fd, (vlong)addr*RBUFSIZE, 0) == -1 || read(fd, b, RBUFSIZE) != RBUFSIZE;
	qunlock(w);
	if(i)
		print("wrenread failed: %r\n");
	return i;
}

int
wrenwrite(Device dev, long addr, void *b)
{
	Wren *w;
	int fd, i;

	w = wren(dev);
	fd = w->fd;
	qlock(w);
	i = seek(fd, (vlong)addr*RBUFSIZE, 0) == -1 || write(fd, b, RBUFSIZE) != RBUFSIZE;
	qunlock(w);
	if(i)
		print("wrenwrite failed: %r\n");
	return i;
}
