#include "all.h"

enum{
	MAXWREN = 7,
};

static char WMAGIC[] =	"kfs wren device\n";
static char MMAGIC[] =	"kfs multi-wren device %4d/%4d\n";

typedef struct Wren	Wren;

struct Wren{
	QLock;
	Device	dev;
	ulong	nblocks;
	int	fd;
};

static char	*wmagic = WMAGIC;
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

/*
 * find out the length of a file
 * given the mesg version of a stat buffer
 * we call this because convM2D is different
 * for the file system than in the os
 */
uvlong
statlen(char *ap)
{
	uchar *p;
	ulong ll, hl;

	p = (uchar*)ap;
	p += 3*NAMELEN+5*4;
	ll = p[0] | (p[1]<<8) | (p[2]<<16) | (p[3]<<24);
	hl = p[4] | (p[5]<<8) | (p[6]<<16) | (p[7]<<24);
	return ll | ((uvlong) hl << 32);
}

static void
wrenpartinit(Device dev, int k)
{
	char buf[MAXBUFSIZE], d[DIRREC];
	char file[128], magic[64];
	Wren *w;
	int fd, i, nmagic;

	if(wrens == 0)
		wrens = ialloc(MAXWREN * sizeof *wrens);
	w = &wrens[maxwren];
	if(nwren > 0)
		sprint(file, "%s%d", wrenfile, k);
	else
		strcpy(file, wrenfile);
	fd = open(file, ORDWR);
	if(fd < 0)
		panic("can't open %s", file);
	if(fstat(fd, d) < 0)
		panic("can't stat %s\n", file);
	seek(fd, 0, 0);
	i = read(fd, buf, sizeof buf);
	if(i < sizeof buf)
		panic("can't read %s", file);
	badmagic = 0;
	RBUFSIZE = 1024;
	sprint(magic, wmagic, k, nwren);
	nmagic = strlen(magic);
	if(strncmp(buf+256, magic, nmagic) == 0){
		RBUFSIZE = atol(buf+256+nmagic);
		if(RBUFSIZE % 512){
			fprint(2, "kfs: bad buffersize(%d): assuming 1k blocks\n", RBUFSIZE);
			RBUFSIZE = 1024;
		}
	}else
		badmagic = 1;
	w->dev = dev;
	w->nblocks = statlen(d)/RBUFSIZE;
	if(k > 0)
		w->nblocks -= 1;	/* don't count magic */
	w->fd = fd;
	maxwren++;
}

void
wreninit(Device dev)
{
	int i;

	if(nwren > 0)
		wmagic = MMAGIC;
	i = 0;
	do{
		wrenpartinit(dev, i);
	}while(++i < nwren);
}

static void
wrenpartream(Device dev, int k)
{
	Wren *w;
	char buf[MAXBUFSIZE], magic[64];
	int fd, i;

	if(RBUFSIZE % 512)
		panic("kfs: bad buffersize(%d): restart a multiple of 512\n", RBUFSIZE);
	print("kfs: reaming the file system using %d byte blocks\n", RBUFSIZE);
	w = wren(dev)+k;
	fd = w->fd;
	memset(buf, 0, sizeof buf);
	sprint(magic, wmagic, k, nwren);
	sprint(buf+256, "%s%d\n", magic, RBUFSIZE);
	qlock(w);
	i = seek(fd, 0, 0) < 0 || write(fd, buf, RBUFSIZE) != RBUFSIZE;
	qunlock(w);
	if(i < 0)
		panic("can't ream disk");
}

void
wrenream(Device dev)
{
	int i;

	i = 0;
	do{
		wrenpartream(dev, i);
	}while(++i < nwren);
}

static int
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
	if(RBUFSIZE > sizeof buf)
		panic("bufsize too big");
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
	Wren *w;
	int i, nb;

	w = wren(dev);
	nb = 0;
	i = 0;
	do{
		nb += w[i].nblocks;
	}while(++i < nwren);
	return nb;
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
	for(i=0; i<nwren; i++){
		if(addr < w->nblocks)
			break;
		addr -= w->nblocks;
		++w;
	}
	if(i > 0)
		addr++;
	fd = w->fd;
	qlock(w);
	i = seek(fd, (vlong)addr*RBUFSIZE, 0) == -1 || read(fd, b, RBUFSIZE) != RBUFSIZE;
	qunlock(w);
	return i;
}

int
wrenwrite(Device dev, long addr, void *b)
{
	Wren *w;
	int fd, i;

	w = wren(dev);
	for(i=0; i<nwren; i++){
		if(addr < w->nblocks)
			break;
		addr -= w->nblocks;
		++w;
	}
	if(i > 0)
		addr++;
	fd = w->fd;
	qlock(w);
	i = seek(fd, (vlong)addr*RBUFSIZE, 0) == -1 || write(fd, b, RBUFSIZE) != RBUFSIZE;
	qunlock(w);
	return i;
}
