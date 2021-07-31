#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

#include	"devtab.h"

typedef struct Pipe	Pipe;
struct Pipe
{
	Ref;
	QLock;
	Pipe	*next;
	ulong	path;
};

struct
{
	Lock;
	Pipe	*pipe;
	ulong	path;
} pipealloc;

static Pipe *getpipe(ulong);
static void pipeiput(Queue*, Block*);
static void pipeoput(Queue*, Block*);
static void pipestclose(Queue *);

Qinfo pipeinfo =
{
	pipeiput,
	pipeoput,
	0,
	pipestclose,
	"pipe"
};

Dirtab pipedir[] =
{
	"data",		{Sdataqid},	0,			0600,
	"ctl",		{Sctlqid},	0,			0600,
	"data1",	{Sdataqid},	0,			0600,
	"ctl1",		{Sctlqid},	0,			0600,
};
#define NPIPEDIR 4

void
pipeinit(void)
{
}

void
pipereset(void)
{
}

/*
 *  create a pipe, no streams are created until an open
 */
Chan*
pipeattach(char *spec)
{
	Pipe *p;
	Chan *c;

	c = devattach('|', spec);
	p = smalloc(sizeof(Pipe));
	p->ref = 1;

	lock(&pipealloc);
	p->path = ++pipealloc.path;
	p->next = pipealloc.pipe;
	pipealloc.pipe = p;
	unlock(&pipealloc);

	c->qid = (Qid){CHDIR|STREAMQID(2*p->path, 0), 0};
	c->dev = 0;
	return c;
}

Chan*
pipeclone(Chan *c, Chan *nc)
{
	Pipe *p;

	p = getpipe(STREAMID(c->qid.path)/2);
	nc = devclone(c, nc);
	if(incref(p) <= 1)
		panic("pipeclone");
	return nc;
}

int
pipegen(Chan *c, Dirtab *tab, int ntab, int i, Dir *dp)
{
	int id;

	id = STREAMID(c->qid.path);
	if(i > 1)
		id++;
	if(tab==0 || i>=ntab)
		return -1;
	tab += i;
	devdir(c, (Qid){STREAMQID(id, tab->qid.path),0}, tab->name, tab->length, eve, tab->perm, dp);
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
	streamstat(c, db, "pipe", 0666);
}

/*
 *  if the stream doesn't exist, create it
 */
Chan *
pipeopen(Chan *c, int omode)
{
	Pipe *p;
	int other;
	Stream *local, *remote;

	if(c->qid.path & CHDIR){
		if(omode != OREAD)
			error(Ebadarg);
		c->mode = omode;
		c->flag |= COPEN;
		c->offset = 0;
		return c;
	}

	p = getpipe(STREAMID(c->qid.path)/2);
	if(waserror()){
		qunlock(p);
		nexterror();
	}
	qlock(p);
	streamopen(c, &pipeinfo);
	local = c->stream;
	if(local->devq->ptr == 0){
		/*
		 *  first open, create the other end also
		 */
		other = STREAMID(c->qid.path)^1;
		remote = streamnew(c->type, c->dev, other, &pipeinfo,1);

		/*
		 *  connect the device ends of both streams
		 */
		local->devq->ptr = remote;
		remote->devq->ptr = local;
		local->devq->other->next = remote->devq;
		remote->devq->other->next = local->devq;
	} else if(local->opens == 1){
		/*
		 *  keep other side around till last close of this side
		 */
		streamenter(local->devq->ptr);
	}
	qunlock(p);
	poperror();

	c->mode = omode&~OTRUNC;
	c->flag |= COPEN;
	c->offset = 0;
	return c;
}

void
pipecreate(Chan *c, char *name, int omode, ulong perm)
{
	USED(c, name, omode, perm);
	error(Egreg);
}

void
piperemove(Chan *c)
{
	USED(c);
	error(Egreg);
}

void
pipewstat(Chan *c, char *db)
{
	USED(c, db);
	error(Eperm);
}

void
pipeclose(Chan *c)
{
	Pipe *p, *f, **l;
	Stream *remote;

	p = getpipe(STREAMID(c->qid.path)/2);

	/*
	 *  take care of local and remote streams
	 */
	if(c->stream){
		qlock(p);
		remote = c->stream->devq->ptr;
		if(streamclose(c) == 0){
			if(remote)
				streamexit(remote);
		}
		qunlock(p);
	}

	/*
	 *  free the structure
	 */
	if(decref(p) == 0){
		lock(&pipealloc);
		l = &pipealloc.pipe;
		for(f = *l; f; f = f->next) {
			if(f == p) {
				*l = p->next;
				break;
			}
			l = &f->next;
		}
		unlock(&pipealloc);
		free(p);
	}
}

long
piperead(Chan *c, void *va, long n, ulong offset)
{
	USED(offset);
	if(c->qid.path & CHDIR)
		return devdirread(c, va, n, pipedir, NPIPEDIR, pipegen);

	return streamread(c, va, n);
}

/*
 *  a write to a closed pipe causes a note to be sent to
 *  the process.
 */
long
pipewrite(Chan *c, void *va, long n, ulong offset)
{
	USED(offset);

	/* avoid notes when pipe is a mounted stream */
	if(c->flag & CMSG)
		return streamwrite(c, va, n, 0);

	if(waserror()) {
		postnote(u->p, 1, "sys: write on closed pipe", NUser);
		error(Egreg);
	}
	n = streamwrite(c, va, n, 0);
	poperror();
	return n;
}

/*
 *  send a block upstream to the process.
 *  sleep until there's room upstream.
 */
static void
pipeiput(Queue *q, Block *bp)
{
	FLOWCTL(q, bp);
}

/*
 *  send the block to the other side
 */
static void
pipeoput(Queue *q, Block *bp)
{
	PUTNEXT(q, bp);
}

/*
 *  send a hangup and disconnect the streams
 */
static void
pipestclose(Queue *q)
{
	Block *bp;

	/*
	 *  point to the bit-bucket and let any in-progress
	 *  write's finish.
	 */
	q->put = nullput;
	wakeup(&q->r);

	/*
	 *  send a hangup
	 */
	q = q->other;
	if(q->next == 0)
		return;
	bp = allocb(0);
	bp->type = M_HANGUP;
	PUTNEXT(q, bp);
}

Pipe*
getpipe(ulong path)
{
	Pipe *p;

	lock(&pipealloc);
	for(p = pipealloc.pipe; p; p = p->next) {
		if(path == p->path) {
			unlock(&pipealloc);
			return p;
		}
	}
	unlock(&pipealloc);
	panic("getpipe");
	return 0;		/* not reached */
}
