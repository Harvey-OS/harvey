#include "lib9.h"
#include "sys.h"
#include "error.h"

enum {
	Pipeqsize	= 32*1024
};

typedef struct Pipe	Pipe;
struct Pipe
{
	Qlock	lk;
	Pipe	*next;
	int	ref;
	ulong	path;
	Queue	*q[2];
	int	qref[2];
};

struct
{
	Lock	lk;
	ulong	path;
} pipealloc;

enum
{
	Qdir,
	Qdata0,
	Qdata1
};

static Dirtab pipedir[] =
{
	"data",		{Qdata0},	0,			0600,
	"data1",	{Qdata1},	0,			0600,
};
#define NPIPEDIR 2

#define NETQID(i, t)	(((i)<<5)|(t))
#define	NETTYPE(i)	((i)&0x1f)
#define NETID(i)	(((i)&~CHDIR)>>5)

void
pipeinit(void)
{
}

void
pipereset(void)
{
}

Chan*
pipeattach(void *spec)
{
	Pipe *p;
	Chan *c;

	c = devattach('|', spec);
	p = mallocz(sizeof(Pipe));
	if(p == 0)
		panic("memory");
	p->ref = 1;

	p->q[0] = qopen(Pipeqsize, 0, 0, 0);
	if(p->q[0] == 0){
		free(p);
		panic("memory");
	}
	p->q[1] = qopen(Pipeqsize, 0, 0, 0);
	if(p->q[1] == 0){
		free(p->q[0]);
		free(p);
		panic("memory");
	}

	lock(&pipealloc.lk);
	p->path = ++pipealloc.path;
	unlock(&pipealloc.lk);

	c->qid.path = CHDIR|NETQID(2*p->path, Qdir);
	c->qid.vers = 0;
	c->u.aux = p;
	c->dev = 0;
	return c;
}

Chan*
pipeclone(Chan *c, Chan *nc)
{
	Pipe *p;

	p = c->u.aux;
	nc = devclone(c, nc);
	qlock(&p->lk);
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
	qunlock(&p->lk);
	return nc;
}

int
pipegen(Chan *c, Dirtab *tab, int ntab, int i, Dir *dp)
{
	int id;
	Qid q;

	id = NETID(c->qid.path);
	if(i > 1)
		id++;
	if(tab==0 || i>=ntab)
		return -1;
	tab += i;
	q.path = NETQID(id, tab->qid.path);
	q.vers = 0;
	devdir(c, q, tab->name, tab->length, eve, tab->perm, dp);
	return 1;
}


int
pipewalk(Chan *c, char *name)
{
	return devwalk(c, name, pipedir, NPIPEDIR, pipegen);
}

void
pipestat(Chan *c, char *db)
{
	Pipe *p;
	Dir dir;

	p = c->u.aux;

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

Chan *
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

	p = c->u.aux;
	qlock(&p->lk);
	switch(NETTYPE(c->qid.path)){
	case Qdata0:
		p->qref[0]++;
		break;
	case Qdata1:
		p->qref[1]++;
		break;
	}
	qunlock(&p->lk);

	c->mode = openmode(omode);
	c->flag |= COPEN;
	c->offset = 0;
	return c;
}

void
pipecreate(Chan *c, char *name, int mode, ulong perm)
{
	USED(c);
	USED(name);
	USED(mode);
	USED(perm);

	error(Egreg);
}

void
piperemove(Chan *c)
{
	USED(c);

	error(Egreg);
}

void
pipewstat(Chan *c, char *buf)
{
	USED(c);
	USED(buf);

	error(Eperm);
}

void
pipeclose(Chan *c)
{
	Pipe *p;

	p = c->u.aux;
	qlock(&p->lk);

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
		qunlock(&p->lk);
		free(p->q[0]);
		free(p->q[1]);
		free(p);
	} else
		qunlock(&p->lk);
}

long
piperead(Chan *c, void *va, long n, ulong offset)
{
	Pipe *p;

	USED(offset);

	p = c->u.aux;

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

Block*
pipebread(Chan *c, long n, ulong offset)
{
	Pipe *p;

	p = c->u.aux;

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
long
pipewrite(Chan *c, void *va, long n, ulong offset)
{
	Pipe *p;

	USED(offset);

	if(waserror())
		error(Ehungup);

	p = c->u.aux;

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

long
pipebwrite(Chan *c, Block *bp, ulong offset)
{
	long n;
	Pipe *p;

	USED(offset);

	if(waserror())
		error(Ehungup);

	p = c->u.aux;
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
