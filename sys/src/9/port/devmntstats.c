#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

void (*mntstats)(int, Chan*, uvlong, ulong);

enum
{
	Qmntstat = 1<<12,

	Nhash=	31,
	Nms=	256,
	Nrpc=	(Tmax-Tnop)/2,
};

typedef struct Mntstats Mntstats;
struct Mntstats
{
	Mntstats *next;
	int	inuse;
	Chan	c;
	uvlong	hi[Nrpc];		/* high water time spent */
	uvlong	tot[Nrpc];		/* cumulative time spent */
	uvlong	bytes[Nrpc];		/* cumulative bytes xfered */
	ulong	n[Nrpc];		/* number of messages/msg type */
	uvlong	bigtot[Nrpc];		/* cumulative time spent in big messages */
	uvlong	bigbytes[Nrpc];		/* cumulative bytes xfered in big messages */
	ulong	bign[Nrpc];		/* number of big messages */
};

static struct
{
	Lock;
	Mntstats	*hash[Nhash];
	Mntstats	all[Nms];
	int		n;
} msalloc;

static void
_mntstats(int type, Chan *c, uvlong start, ulong bytes)
{
	uint h;
	Mntstats **l, *m;
	uvlong elapsed;

	elapsed = fastticks(nil) - start;
	type -= Tnop;
	type >>= 1;

	h = (c->dev<<4)+(c->type<<2)+c->qid.path;
	h %= Nhash;
	for(l = &msalloc.hash[h]; *l; l = &(*l)->next)
		if(eqchan(&(*l)->c, c, 0))
			break;
	m = *l;

	if(m == nil){
		lock(&msalloc);
		for(m = msalloc.all; m < &msalloc.all[Nms]; m++)
			if(m->inuse && eqchan(&m->c, c, 0))
				break;
		if(m == &msalloc.all[Nms])
			for(m = msalloc.all; m < &msalloc.all[Nms]; m++){
				if(m->inuse == 0){
					m->inuse = 1;
					m->c = *c;
					*l = m;
					msalloc.n++;
					break;
				}
			}
		unlock(&msalloc);
		if(m >= &msalloc.all[Nms])
			return;
	}

	if(m->hi[type] < elapsed)
		m->hi[type] = elapsed;
	m->tot[type] += elapsed;
	m->n[type]++;
	m->bytes[type] += bytes;

	if(bytes >= 8*1024){
		m->bigtot[type] += elapsed;
		m->bign[type]++;
		m->bigbytes[type] += bytes;
	}
}

static int
mntstatsgen(Chan *c, Dirtab*, int, int i, Dir *dp)
{
	Qid q;
	Mntstats *m;

	if(i == DEVDOTDOT){
		devdir(c, (Qid){CHDIR,0}, "#z", 0, eve, 0555, dp);
		return 1;
	}

	m = &msalloc.all[i];
	if(i > Nms || m->inuse == 0)
		return -1;

	q = (Qid){Qmntstat+i, 0};
	snprint(up->genbuf, sizeof up->genbuf, "%C%lud.%lux", devtab[m->c.type]->dc, m->c.dev, m->c.qid.path);
	devdir(c, q, up->genbuf, 0, eve, 0666, dp);

	return 1;
}

static void
mntstatsinit(void)
{
	mntstats = _mntstats;
}

static Chan*
mntstatsattach(char *spec)
{
	return devattach('z', spec);
}

static int
mntstatswalk(Chan *c, char *name)
{
	return devwalk(c, name, 0, msalloc.n, mntstatsgen);
}

static void
mntstatsstat(Chan *c, char *dp)
{
	devstat(c, dp, 0, msalloc.n, mntstatsgen);
}

static Chan*
mntstatsopen(Chan *c, int omode)
{
	return devopen(c, omode, 0, msalloc.n, mntstatsgen);
}

static void
mntstatsclose(Chan*)
{
}

enum
{
	Nline=	136,
};

char *rpcname[Nrpc] =
{
	"nop",
	"osession",
	"error",
	"flush",
	"oattach",
	"clone",
	"walk",
	"open",
	"create",
	"read",
	"write",
	"clunk",
	"remove",
	"stat",
	"wstat",
	"clwalk",
	"auth",
	"session",
	"attach",
};

static long
mntstatsread(Chan *c, void *buf, long n, vlong off)
{
	char *a, *start;
	ulong o;
	char xbuf[Nline+1];
	Mntstats *m;

	start = a = buf;

	if(n <= 0)
		return n;

	if(c->qid.path & CHDIR)
		return devdirread(c, buf, n, 0, msalloc.n, mntstatsgen);

	m = &msalloc.all[c->qid.path - Qmntstat];
	o = off;
	if((o % Nline) != 0)
		error(Ebadarg);
	n = n/Nline;
	o = o/Nline;
	while(n > 0 && o < Nrpc){
		snprint(xbuf, sizeof(xbuf), "%-8.8s\t%20.0llud\n\t%20.0llud %20.0llud %9.0lud\n\t%20.0llud %20.0llud %9.0lud\n",
			rpcname[o], m->hi[o],
			m->tot[o], m->bytes[o], m->n[o],
			m->bigtot[o], m->bigbytes[o], m->bign[o]);
		memmove(a, xbuf, Nline);
		a += Nline;
		o++;
		n--;
	}
	return a - start;
}

static long
mntstatswrite(Chan*, void*, long, vlong)
{
	lock(&msalloc);
	memset(msalloc.all, 0, sizeof(msalloc.all));
	msalloc.n = 0;
	unlock(&msalloc);
	return 0;
}

Dev mntstatsdevtab = {
	'z',
	"mntstats",

	devreset,
	mntstatsinit,
	mntstatsattach,
	devclone,
	mntstatswalk,
	mntstatsstat,
	mntstatsopen,
	devcreate,
	mntstatsclose,
	mntstatsread,
	devbread,
	mntstatswrite,
	devbwrite,
	devremove,
	devwstat,
};
