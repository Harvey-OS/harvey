#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

#include	"netif.h"

typedef struct Pipe	Pipe;
struct Pipe
{
	QLock;
	Pipe	*next;
	int	ref;
	ulong	path;
	Queue	*q[2];
	int	qref[2];
};

struct
{
	Lock;
	ulong	path;
} pipealloc;

enum
{
	Qdir,
	Qdata0,
	Qdata1,
};

Dirtab pipedir[] =
{
	"data",		{Qdata0},	0,			0600,
	"data1",	{Qdata1},	0,			0600,
};
#define NPIPEDIR 2

static void
pipeinit(void)
{
	if(conf.pipeqsize == 0){
		if(conf.nmach > 1)
			conf.pipeqsize = 256*1024;
		else
			conf.pipeqsize = 32*1024;
	}
}

/*
 *  create a pipe, no streams are created until an open
 */
static Chan*
pipeattach(char *spec)
{
	Pipe *p;
	Chan *c;

	c = devattach('|', spec);
	p = malloc(sizeof(Pipe));
	if(p == 0)
		exhausted("memory");
	p->ref = 1;

	p->q[0] = qopen(conf.pipeqsize, 0, 0, 0);
	if(p->q[0] == 0){
		free(p);
		exhausted("memory");
	}
	p->q[1] = qopen(conf.pipeqsize, 0, 0, 0);
	if(p->q[1] == 0){
		free(p->q[0]);
		free(p);
		exhausted("memory");
	}

	lock(&pipealloc);
	p->path = ++pipealloc.path;
	unlock(&pipealloc);

	c->qid = (Qid){CHDIR|NETQID(2*p->path, Qdir), 0};
	c->aux = p;
	c->dev = 0;
	return c;
}

static Chan*
pipeclone(Chan *c, Chan *nc)
{
	Pipe *p;

	p = c->aux;
	nc = devclone(c, nc);
	qlock(p);
	p->ref++;
	if(c->flag & COPEN){
		switch(NETTYPE(c->qid.path)){
		case Qdata0:
			p->qref[0]++;
			break;
		case Qdata1:
			p->qref[1]++;
			break;
		}
	}
	qunlock(p);
	return nc;
}

static int
pipegen(Chan *c, Dirtab *tab, int ntab, int i, Dir *dp)
{
	int id;
	int len;
	Pipe *p;

	if(i == DEVDOTDOT){
		devdir(c, c->qid, "#|", 0, eve, CHDIR|0555, dp);
		return 1;
	}

	id = NETID(c->qid.path);
	if(tab==0 || i>=ntab)
		return -1;
	tab += i;
	p = c->aux;
	switch(tab->qid.path){
	case Qdata0:
		len = qlen(p->q[0]);
		break;
	case Qdata1:
		len = qlen(p->q[1]);
		break;
	default:
		len = tab->length;
		break;
	}
	devdir(c, (Qid){NETQID(id, tab->qid.path),0}, tab->name, len, eve, tab->perm, dp);
	return 1;
}


static int
pipewalk(Chan *c, char *name)
{
	return devwalk(c, name, pipedir, NPIPEDIR, pipegen);
}

static void
pipestat(Chan *c, char *db)
{
	Pipe *p;
	Dir dir;

	p = c->aux;

	switch(NETTYPE(c->qid.path)){
	case Qdir:
		devdir(c, c->qid, ".", 2*DIRLEN, eve, CHDIR|0555, &dir);
		break;
	case Qdata0:
		devdir(c, c->qid, "data", qlen(p->q[0]), eve, 0660, &dir);
		break;
	case Qdata1:
		devdir(c, c->qid, "data1", qlen(p->q[1]), eve, 0660, &dir);
		break;
	default:
		panic("pipestat");
	}
	convD2M(&dir, db);
}

/*
 *  if the stream doesn't exist, create it
 */
static Chan*
pipeopen(Chan *c, int omode)
{
	Pipe *p;

	if(c->qid.path & CHDIR){
		if(omode != OREAD)
			error(Ebadarg);
		c->mode = omode;
		c->flag |= COPEN;
		c->offset = 0;
		return c;
	}

	p = c->aux;
	qlock(p);
	switch(NETTYPE(c->qid.path)){
	case Qdata0:
		p->qref[0]++;
		break;
	case Qdata1:
		p->qref[1]++;
		break;
	}
	qunlock(p);

	c->mode = openmode(omode);
	c->flag |= COPEN;
	c->offset = 0;
	return c;
}

static void
pipeclose(Chan *c)
{
	Pipe *p;

	p = c->aux;
	qlock(p);

	if(c->flag & COPEN){
		/*
		 *  closing either side hangs up the stream
		 */
		switch(NETTYPE(c->qid.path)){
		case Qdata0:
			p->qref[0]--;
			if(p->qref[0] == 0){
				qhangup(p->q[1], 0);
				qclose(p->q[0]);
			}
			break;
		case Qdata1:
			p->qref[1]--;
			if(p->qref[1] == 0){
				qhangup(p->q[0], 0);
				qclose(p->q[1]);
			}
			break;
		}
	}


	/*
	 *  if both sides are closed, they are reusable
	 */
	if(p->qref[0] == 0 && p->qref[1] == 0){
		qreopen(p->q[0]);
		qreopen(p->q[1]);
	}

	/*
	 *  free the structure on last close
	 */
	p->ref--;
	if(p->ref == 0){
		qunlock(p);
		free(p->q[0]);
		free(p->q[1]);
		free(p);
	} else
		qunlock(p);
}

static long
piperead(Chan *c, void *va, long n, vlong)
{
	Pipe *p;

	p = c->aux;

	switch(NETTYPE(c->qid.path)){
	case Qdir:
		return devdirread(c, va, n, pipedir, NPIPEDIR, pipegen);
	case Qdata0:
		return qread(p->q[0], va, n);
	case Qdata1:
		return qread(p->q[1], va, n);
	default:
		panic("piperead");
	}
	return -1;	/* not reached */
}

static Block*
pipebread(Chan *c, long n, ulong offset)
{
	Pipe *p;

	p = c->aux;

	switch(NETTYPE(c->qid.path)){
	case Qdata0:
		return qbread(p->q[0], n);
	case Qdata1:
		return qbread(p->q[1], n);
	}

	return devbread(c, n, offset);
}

/*
 *  a write to a closed pipe causes a note to be sent to
 *  the process.
 */
static long
pipewrite(Chan *c, void *va, long n, vlong)
{
	Pipe *p;

	if(!islo())
		print("pipewrite hi %lux\n", getcallerpc(&c));
	if(waserror()) {
		/* avoid notes when pipe is a mounted queue */
		if((c->flag & CMSG) == 0)
			postnote(up, 1, "sys: write on closed pipe", NUser);
		nexterror();
	}

	p = c->aux;

	switch(NETTYPE(c->qid.path)){
	case Qdata0:
		n = qwrite(p->q[1], va, n);
		break;

	case Qdata1:
		n = qwrite(p->q[0], va, n);
		break;

	default:
		panic("pipewrite");
	}

	poperror();
	return n;
}

static long
pipebwrite(Chan *c, Block *bp, ulong)
{
	long n;
	Pipe *p;

	if(waserror()) {
		/* avoid notes when pipe is a mounted queue */
		if((c->flag & CMSG) == 0)
			postnote(up, 1, "sys: write on closed pipe", NUser);
		nexterror();
	}

	p = c->aux;
	switch(NETTYPE(c->qid.path)){
	case Qdata0:
		n = qbwrite(p->q[1], bp);
		break;

	case Qdata1:
		n = qbwrite(p->q[0], bp);
		break;

	default:
		n = 0;
		panic("pipebwrite");
	}

	poperror();
	return n;
}

Dev pipedevtab = {
	'|',
	"pipe",

	devreset,
	pipeinit,
	pipeattach,
	pipeclone,
	pipewalk,
	pipestat,
	pipeopen,
	devcreate,
	pipeclose,
	piperead,
	pipebread,
	pipewrite,
	pipebwrite,
	devremove,
	devwstat,
};
