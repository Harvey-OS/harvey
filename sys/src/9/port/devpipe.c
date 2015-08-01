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

typedef struct Pipe	Pipe;
struct Pipe
{
	QLock;
	Pipe	*next;
	int	ref;
	uint32_t	path;
	Queue	*q[2];
	int	qref[2];
};

struct
{
	Lock;
	uint32_t	path;
} pipealloc;

enum
{
	Qdir,
	Qdata0,
	Qdata1,
};

Dirtab pipedir[] =
{
	".",		{Qdir,0,QTDIR},	0,		DMDIR|0500,
	"data",		{Qdata0},	0,		0600,
	"data1",	{Qdata1},	0,		0600,
};
#define NPIPEDIR 3

#define PIPETYPE(x)	(((unsigned)x)&0x1f)
#define PIPEID(x)	((((unsigned)x))>>5)
#define PIPEQID(i, t)	((((unsigned)i)<<5)|(t))


enum
{
	/* Plan 9 default for nmach > 1 */
	Pipeqsize = 256*1024
};

static void
pipeinit(void)
{
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

	p->q[0] = qopen(Pipeqsize, 0, 0, 0);
	if(p->q[0] == 0){
		free(p);
		exhausted("memory");
	}
	p->q[1] = qopen(Pipeqsize, 0, 0, 0);
	if(p->q[1] == 0){
		free(p->q[0]);
		free(p);
		exhausted("memory");
	}

	lock(&pipealloc);
	p->path = ++pipealloc.path;
	unlock(&pipealloc);

	mkqid(&c->qid, PIPEQID(2*p->path, Qdir), 0, QTDIR);
	c->aux = p;
	c->devno = 0;
	return c;
}

static int
pipegen(Chan *c, char* d, Dirtab *tab, int ntab, int i, Dir *dp)
{
	Qid q;
	int len;
	Pipe *p;

	if(i == DEVDOTDOT){
		devdir(c, c->qid, "#|", 0, eve, DMDIR|0555, dp);
		return 1;
	}
	i++;	/* skip . */
	if(tab==0 || i>=ntab)
		return -1;

	tab += i;
	p = c->aux;
	switch((uint32_t)tab->qid.path){
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
	mkqid(&q, PIPEQID(PIPEID(c->qid.path), tab->qid.path), 0, QTFILE);
	devdir(c, q, tab->name, len, eve, tab->perm, dp);
	return 1;
}


static Walkqid*
pipewalk(Chan *c, Chan *nc, char **name, int nname)
{
	Walkqid *wq;
	Pipe *p;

	wq = devwalk(c, nc, name, nname, pipedir, NPIPEDIR, pipegen);
	if(wq != nil && wq->clone != nil && wq->clone != c){
		p = c->aux;
		qlock(p);
		p->ref++;
		if(c->flag & COPEN){
			print("channel open in pipewalk\n");
			switch(PIPETYPE(c->qid.path)){
			case Qdata0:
				p->qref[0]++;
				break;
			case Qdata1:
				p->qref[1]++;
				break;
			}
		}
		qunlock(p);
	}
	return wq;
}

static int32_t
pipestat(Chan *c, uint8_t *db, int32_t n)
{
	Pipe *p;
	Dir dir;

	p = c->aux;

	switch(PIPETYPE(c->qid.path)){
	case Qdir:
		devdir(c, c->qid, ".", 0, eve, DMDIR|0555, &dir);
		break;
	case Qdata0:
		devdir(c, c->qid, "data", qlen(p->q[0]), eve, 0600, &dir);
		break;
	case Qdata1:
		devdir(c, c->qid, "data1", qlen(p->q[1]), eve, 0600, &dir);
		break;
	default:
		panic("pipestat");
	}
	n = convD2M(&dir, db, n);
	if(n < BIT16SZ)
		error(Eshortstat);
	return n;
}

/*
 *  if the stream doesn't exist, create it
 */
static Chan*
pipeopen(Chan *c, int omode)
{
	Pipe *p;

	if(c->qid.type & QTDIR){
		if(omode != OREAD)
			error(Ebadarg);
		c->mode = omode;
		c->flag |= COPEN;
		c->offset = 0;
		return c;
	}

	p = c->aux;
	qlock(p);
	switch(PIPETYPE(c->qid.path)){
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
	c->iounit = qiomaxatomic;
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
		switch(PIPETYPE(c->qid.path)){
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

static int32_t
piperead(Chan *c, void *va, int32_t n, int64_t m)
{
	Pipe *p;

	p = c->aux;

	switch(PIPETYPE(c->qid.path)){
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
pipebread(Chan *c, int32_t n, int64_t offset)
{
	Pipe *p;

	p = c->aux;

	switch(PIPETYPE(c->qid.path)){
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
static int32_t
pipewrite(Chan *c, void *va, int32_t n, int64_t mm)
{
	Proc *up = machp()->externup;
	Pipe *p;

	if(0)if(!islo())
		print("pipewrite hi %#p\n", getcallerpc(&c)); // devmnt?

	if(waserror()) {
		/* avoid notes when pipe is a mounted queue */
		if((c->flag & CMSG) == 0)
			postnote(up, 1, "sys: write on closed pipe", NUser);
		nexterror();
	}

	p = c->aux;

	switch(PIPETYPE(c->qid.path)){
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

static int32_t
pipebwrite(Chan *c, Block *bp, int64_t mm)
{
	Proc *up = machp()->externup;
	int32_t n;
	Pipe *p;

	if(waserror()) {
		/* avoid notes when pipe is a mounted queue */
		if((c->flag & CMSG) == 0)
			postnote(up, 1, "sys: write on closed pipe", NUser);
		nexterror();
	}

	p = c->aux;
	switch(PIPETYPE(c->qid.path)){
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
	devshutdown,
	pipeattach,
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
