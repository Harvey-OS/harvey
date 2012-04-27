#include	"u.h"
#include <errno.h>
#include	"lib.h"
#include	"dat.h"
#include	"fns.h"
#include	"error.h"

#undef pread
#undef pwrite

Chan*
lfdchan(int fd)
{
	Chan *c;
	
	c = newchan();
	c->type = devno('L', 0);
	c->aux = (void*)(uintptr)fd;
	c->name = newcname("fd");
	c->mode = ORDWR;
	c->qid.type = 0;
	c->qid.path = 0;
	c->qid.vers = 0;
	c->dev = 0;
	c->offset = 0;
	return c;
}

int
lfdfd(int fd)
{
	return newfd(lfdchan(fd));
}

static Chan*
lfdattach(char *x)
{
	USED(x);
	
	error(Egreg);
	return nil;
}

static Walkqid*
lfdwalk(Chan *c, Chan *nc, char **name, int nname)
{
	USED(c);
	USED(nc);
	USED(name);
	USED(nname);
	
	error(Egreg);
	return nil;
}

static int
lfdstat(Chan *c, uchar *dp, int n)
{
	USED(c);
	USED(dp);
	USED(n);
	error(Egreg);
	return -1;
}

static Chan*
lfdopen(Chan *c, int omode)
{
	USED(c);
	USED(omode);
	
	error(Egreg);
	return nil;
}

static void
lfdclose(Chan *c)
{
	close((int)(uintptr)c->aux);
}

static long
lfdread(Chan *c, void *buf, long n, vlong off)
{
	USED(off);	/* can't pread on pipes */
	n = read((int)(uintptr)c->aux, buf, n);
	if(n < 0){
		iprint("error %d\n", errno);
		oserror();
	}
	return n;
}

static long
lfdwrite(Chan *c, void *buf, long n, vlong off)
{
	USED(off);	/* can't pread on pipes */

	n = write((int)(uintptr)c->aux, buf, n);
	if(n < 0){
		iprint("error %d\n", errno);
		oserror();
	}
	return n;
}

Dev lfddevtab = {
	'L',
	"lfd",
	
	devreset,
	devinit,
	devshutdown,
	lfdattach,
	lfdwalk,
	lfdstat,
	lfdopen,
	devcreate,
	lfdclose,
	lfdread,
	devbread,
	lfdwrite,
	devbwrite,
	devremove,
	devwstat,
};
