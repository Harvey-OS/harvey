#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"

#define	splicedebug	1

#define	DPRINT	if(splicedebug)kprint

/*
 * Splice line discipline.
 */

static void spliceopen(Queue*, Stream*);
static void spliceclose(Queue*);
static void spliceoput(Queue*, Block*);
static void spliceiput(Queue*, Block*);
Qinfo spliceinfo =
{
	spliceiput,
	spliceoput,
	spliceopen,
	spliceclose,
	"splice"
};

static void
spliceopen(Queue *q, Stream *s)
{
	DPRINT("spliceopen q=0x%ux s=0x%ux\n", q, s);

	WR(q)->ptr = s;		/* pointer to ourselves */
	RD(q)->ptr = 0;		/* pointer to other side */
}

static void
spliceclose(Queue *q)
{
	DPRINT("spliceclose q=0x%ux us=0x%ux them=0x%ux\n",
		q, WR(q)->ptr, RD(q)->ptr);

	RD(q)->ptr = 0;
	WR(q)->ptr = 0;
}

static void
spliceoput(Queue *q, Block *bp)
{
	int fd;
	Chan *c;
	Stream *s;

	if(bp->type != M_CTL){
		PUTNEXT(q, bp);
		return;
	}
	fd = strtol((char *)bp->rptr, 0, 0);
	freeb(bp);
	if(RD(q)->ptr)
		error("stream already spliced");
	c = fdtochan(fd, ORDWR, 0, 0);
	s = c->stream;
	if(s == 0)
		error("splice attempt on non-stream");
	pushq(s, &spliceinfo);
	RD(q)->ptr = s;
	RD(s->procq->next)->ptr = WR(q)->ptr;
}

static void
spliceiput(Queue *q, Block *bp)
{
	if(bp->type == M_HANGUP)
		PUTNEXT(q, bp);
	else
		FLOWCTL(((Stream *)q->ptr)->procq, bp);
}
