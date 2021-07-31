#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

static ulong padblockcnt;
static ulong concatblockcnt;
static ulong pullupblockcnt;
static ulong copyblockcnt;
static ulong consumecnt;
static ulong producecnt;
static ulong qcopycnt;

static int debugging;

#define QDEBUG	if(0)

/*
 *  IO queues
 */
typedef struct Queue	Queue;

struct Queue
{
	Lock;

	Block*	bfirst;		/* buffer */
	Block*	blast;

	int	len;		/* bytes allocated to queue */
	int	dlen;		/* data bytes in queue */
	int	limit;		/* max bytes in queue */
	int	inilim;		/* initial limit */
	int	state;
	int	noblock;	/* true if writes return immediately when q full */
	int	eof;		/* number of eofs read by user */

	void	(*kick)(void*);	/* restart output */
	void*	arg;		/* argument to kick */

	QLock	rlock;		/* mutex for reading processes */
	Rendez	rr;		/* process waiting to read */
	QLock	wlock;		/* mutex for writing processes */
	Rendez	wr;		/* process waiting to write */

	char	err[ERRLEN];
};

enum
{
	/* Queue.state */
	Qstarve		= (1<<0),	/* consumer starved */
	Qmsg		= (1<<1),	/* message stream */
	Qclosed		= (1<<2),
	Qflow		= (1<<3),
	Qcoalesce	= (1<<4),	/* coallesce packets on read */

	Maxatomic	= 32*1024,
};

void
ixsummary(void)
{
	debugging ^= 1;
	iallocsummary();
	print("pad %lud, concat %lud, pullup %lud, copy %lud\n",
		padblockcnt, concatblockcnt, pullupblockcnt, copyblockcnt);
	print("consume %lud, produce %lud, qcopy %lud\n",
		consumecnt, producecnt, qcopycnt);
}

/*
 *  free a list of blocks
 */
void
freeblist(Block *b)
{
	Block *next;

	for(; b != 0; b = next){
		next = b->next;
		b->next = 0;
		freeb(b);
	}
}

/*
 *  pad a block to the front (or the back if size is negative)
 */
Block*
padblock(Block *bp, int size)
{
	int n;
	Block *nbp;

	QDEBUG checkb(bp, "padblock 1");
	if(size >= 0){
		if(bp->rp - bp->base >= size){
			bp->rp -= size;
			return bp;
		}

		if(bp->next)
			panic("padblock 0x%uX", getcallerpc(&bp));
		n = BLEN(bp);
		padblockcnt++;
		nbp = allocb(size+n);
		nbp->rp += size;
		nbp->wp = nbp->rp;
		memmove(nbp->wp, bp->rp, n);
		nbp->wp += n;
		freeb(bp);
		nbp->rp -= size;
	} else {
		size = -size;

		if(bp->next)
			panic("padblock 0x%uX", getcallerpc(&bp));

		if(bp->lim - bp->wp >= size)
			return bp;

		n = BLEN(bp);
		padblockcnt++;
		nbp = allocb(size+n);
		memmove(nbp->wp, bp->rp, n);
		nbp->wp += n;
		freeb(bp);
	}
	QDEBUG checkb(nbp, "padblock 1");
	return nbp;
}

/*
 *  return count of bytes in a string of blocks
 */
int
blocklen(Block *bp)
{
	int len;

	len = 0;
	while(bp) {
		len += BLEN(bp);
		bp = bp->next;
	}
	return len;
}

/*
 *  copy the  string of blocks into
 *  a single block and free the string
 */
Block*
concatblock(Block *bp)
{
	int len;
	Block *nb, *f;

	if(bp->next == 0)
		return bp;

	nb = allocb(blocklen(bp));
	for(f = bp; f; f = f->next) {
		len = BLEN(f);
		memmove(nb->wp, f->rp, len);
		nb->wp += len;
	}
	concatblockcnt += BLEN(nb);
	freeblist(bp);
	QDEBUG checkb(nb, "concatblock 1");
	return nb;
}

/*
 *  make sure the first block has at least n bytes
 */
Block*
pullupblock(Block *bp, int n)
{
	int i;
	Block *nbp;

	/*
	 *  this should almost always be true, it's
	 *  just to avoid every caller checking.
	 */
	if(BLEN(bp) >= n)
		return bp;

	/*
	 *  if not enough room in the first block,
	 *  add another to the front of the list.
	 */
	if(bp->lim - bp->rp < n){
		nbp = allocb(n);
		nbp->next = bp;
		bp = nbp;
	}

	/*
	 *  copy bytes from the trailing blocks into the first
	 */
	n -= BLEN(bp);
	while(nbp = bp->next){
		i = BLEN(nbp);
		if(i > n) {
			memmove(bp->wp, nbp->rp, n);
			pullupblockcnt++;
			bp->wp += n;
			nbp->rp += n;
			QDEBUG checkb(bp, "pullupblock 1");
			return bp;
		}
		else {
			memmove(bp->wp, nbp->rp, i);
			pullupblockcnt++;
			bp->wp += i;
			bp->next = nbp->next;
			nbp->next = 0;
			freeb(nbp);
			n -= i;
			if(n == 0){
				QDEBUG checkb(bp, "pullupblock 2");
				return bp;
			}
		}
	}
	freeb(bp);
	return 0;
}

/*
 *  trim to len bytes starting at offset
 */
Block *
trimblock(Block *bp, int offset, int len)
{
	ulong l;
	Block *nb, *startb;

	QDEBUG checkb(bp, "trimblock 1");
	if(blocklen(bp) < offset+len) {
		freeblist(bp);
		return nil;
	}

	while((l = BLEN(bp)) < offset) {
		offset -= l;
		nb = bp->next;
		bp->next = nil;
		freeb(bp);
		bp = nb;
	}

	startb = bp;
	bp->rp += offset;

	while((l = BLEN(bp)) < len) {
		len -= l;
		bp = bp->next;
	}

	bp->wp -= (BLEN(bp) - len);

	if(bp->next) {
		freeblist(bp->next);
		bp->next = nil;
	}

	return startb;
}

/*
 *  copy 'count' bytes into a new block
 */
Block*
copyblock(Block *bp, int count)
{
	int l;
	Block *nbp;

	QDEBUG checkb(bp, "copyblock 0");
	nbp = allocb(count);
	for(; count > 0 && bp != 0; bp = bp->next){
		l = BLEN(bp);
		if(l > count)
			l = count;
		memmove(nbp->wp, bp->rp, l);
		nbp->wp += l;
		count -= l;
	}
	if(count > 0){
		memset(nbp->wp, 0, count);
		nbp->wp += count;
	}
	copyblockcnt++;
	QDEBUG checkb(nbp, "copyblock 1");

	return nbp;
}

Block*
adjustblock(Block* bp, int len)
{
	int n;
	Block *nbp;

	if(len < 0){
		freeb(bp);
		return nil;
	}

	if(bp->rp+len > bp->lim){
		nbp = copyblock(bp, len);
		freeblist(bp);
		QDEBUG checkb(nbp, "adjustblock 1");

		return nbp;
	}

	n = BLEN(bp);
	if(len > n)
		memset(bp->wp, 0, len-n);
	bp->wp = bp->rp+len;
	QDEBUG checkb(bp, "adjustblock 2");

	return bp;
}


/*
 *  throw away up to count bytes from a
 *  list of blocks.  Return count of bytes
 *  thrown away.
 */
int
pullblock(Block **bph, int count)
{
	Block *bp;
	int n, bytes;

	bytes = 0;
	if(bph == nil)
		return 0;

	while(*bph != nil && count != 0) {
		bp = *bph;
		n = BLEN(bp);
		if(count < n)
			n = count;
		bytes += n;
		count -= n;
		bp->rp += n;
		QDEBUG checkb(bp, "pullblock ");
		if(BLEN(bp) == 0) {
			*bph = bp->next;
			bp->next = nil;
			freeb(bp);
		}
	}
	return bytes;
}

/*
 *  get next block from a queue, return null if nothing there
 */
Block*
qget(Queue *q)
{
	int dowakeup;
	Block *b;

	/* sync with qwrite */
	ilock(q);

	b = q->bfirst;
	if(b == 0){
		q->state |= Qstarve;
		iunlock(q);
		return 0;
	}
	q->bfirst = b->next;
	b->next = 0;
	q->len -= BALLOC(b);
	q->dlen -= BLEN(b);
	QDEBUG checkb(b, "qget");

	/* if writer flow controlled, restart */
	if((q->state & Qflow) && q->len < q->limit/2){
		q->state &= ~Qflow;
		dowakeup = 1;
	} else
		dowakeup = 0;

	iunlock(q);

	if(dowakeup)
		wakeup(&q->wr);

	return b;
}

/*
 *  throw away the next 'len' bytes in the queue
 */
int
qdiscard(Queue *q, int len)
{
	Block *b;
	int dowakeup, n, sofar;

	ilock(q);
	for(sofar = 0; sofar < len; sofar += n){
		b = q->bfirst;
		if(b == nil)
			break;
		QDEBUG checkb(b, "qdiscard");
		n = BLEN(b);
		if(n <= len - sofar){
			q->bfirst = b->next;
			b->next = 0;
			q->len -= BALLOC(b);
			q->dlen -= BLEN(b);
			freeb(b);
		} else {
			n = len - sofar;
			b->rp += n;
			q->dlen -= n;
		}
	}

	/* if writer flow controlled, restart */
	if((q->state & Qflow) && q->len < q->limit/2){
		q->state &= ~Qflow;
		dowakeup = 1;
	} else
		dowakeup = 0;

	iunlock(q);

	if(dowakeup)
		wakeup(&q->wr);

	return sofar;
}

/*
 *  Interrupt level copy out of a queue, return # bytes copied.
 */
int
qconsume(Queue *q, void *vp, int len)
{
	Block *b;
	int n, dowakeup;
	uchar *p = vp;
	Block *tofree = nil;

	/* sync with qwrite */
	ilock(q);

	for(;;) {
		b = q->bfirst;
		if(b == 0){
			q->state |= Qstarve;
			iunlock(q);
			return -1;
		}
		QDEBUG checkb(b, "qconsume 1");

		n = BLEN(b);
		if(n > 0)
			break;
		q->bfirst = b->next;
		q->len -= BALLOC(b);

		/* remember to free this */
		b->next = tofree;
		tofree = b;
	};

	if(n < len)
		len = n;
	memmove(p, b->rp, len);
	consumecnt += n;
	if((q->state & Qmsg) || len == n)
		q->bfirst = b->next;
	b->rp += len;
	q->dlen -= len;

	/* discard the block if we're done with it */
	if((q->state & Qmsg) || len == n){
		b->next = 0;
		q->len -= BALLOC(b);
		q->dlen -= BLEN(b);

		/* remember to free this */
		b->next = tofree;
		tofree = b;
	}

	/* if writer flow controlled, restart */
	if((q->state & Qflow) && q->len < q->limit/2){
		q->state &= ~Qflow;
		dowakeup = 1;
	} else
		dowakeup = 0;

	iunlock(q);

	if(dowakeup)
		wakeup(&q->wr);

	if(tofree != nil)
		freeblist(tofree);

	return len;
}

int
qpass(Queue *q, Block *b)
{
	int dlen, len, dowakeup;

	/* sync with qread */
	dowakeup = 0;
	ilock(q);
	if(q->len >= q->limit){
		freeblist(b);
		iunlock(q);
		return -1;
	}
	if(q->state & Qclosed){
		freeblist(b);
		iunlock(q);
		return BALLOC(b);
	}

	/* add buffer to queue */
	if(q->bfirst)
		q->blast->next = b;
	else
		q->bfirst = b;
	len = BALLOC(b);
	dlen = BLEN(b);
	QDEBUG checkb(b, "qpass");
	while(b->next){
		b = b->next;
		QDEBUG checkb(b, "qpass");
		len += BALLOC(b);
		dlen += BLEN(b);
	}
	q->blast = b;
	q->len += len;
	q->dlen += dlen;

	if(q->len >= q->limit/2)
		q->state |= Qflow;

	if(q->state & Qstarve){
		q->state &= ~Qstarve;
		dowakeup = 1;
	}
	iunlock(q);

	if(dowakeup)
		wakeup(&q->rr);

	return len;
}

int
qpassnolim(Queue *q, Block *b)
{
	int dlen, len, dowakeup;

	/* sync with qread */
	dowakeup = 0;
	ilock(q);

	if(q->state & Qclosed){
		freeblist(b);
		iunlock(q);
		return BALLOC(b);
	}

	/* add buffer to queue */
	if(q->bfirst)
		q->blast->next = b;
	else
		q->bfirst = b;
	len = BALLOC(b);
	dlen = BLEN(b);
	QDEBUG checkb(b, "qpass");
	while(b->next){
		b = b->next;
		QDEBUG checkb(b, "qpass");
		len += BALLOC(b);
		dlen += BLEN(b);
	}
	q->blast = b;
	q->len += len;
	q->dlen += dlen;

	if(q->len >= q->limit/2)
		q->state |= Qflow;

	if(q->state & Qstarve){
		q->state &= ~Qstarve;
		dowakeup = 1;
	}
	iunlock(q);

	if(dowakeup)
		wakeup(&q->rr);

	return len;
}

/*
 *  if the allocated space is way out of line with the used
 *  space, reallocate to a smaller block
 */
Block*
packblock(Block *bp)
{
	Block **l, *nbp;
	int n;

	for(l = &bp; *l; l = &(*l)->next){
		nbp = *l;
		n = BLEN(nbp);
		if((n<<2) < BALLOC(nbp)){
			*l = allocb(n);
			memmove((*l)->wp, nbp->rp, n);
			(*l)->wp += n;
			(*l)->next = nbp->next;
			freeb(nbp);
		}
	}

	return bp;
}

int
qproduce(Queue *q, void *vp, int len)
{
	Block *b;
	int dowakeup;
	uchar *p = vp;

	/* sync with qread */
	dowakeup = 0;
	ilock(q);

	/* no waiting receivers, room in buffer? */
	if(q->len >= q->limit){
		q->state |= Qflow;
		iunlock(q);
		return -1;
	}

	/* save in buffer */
	b = iallocb(len);
	if(b == 0){
		iunlock(q);
		return 0;
	}
	memmove(b->wp, p, len);
	producecnt += len;
	b->wp += len;
	if(q->bfirst)
		q->blast->next = b;
	else
		q->bfirst = b;
	q->blast = b;
	/* b->next = 0; done by iallocb() */
	q->len += BALLOC(b);
	q->dlen += BLEN(b);
	QDEBUG checkb(b, "qproduce");

	if(q->state & Qstarve){
		q->state &= ~Qstarve;
		dowakeup = 1;
	}

	if(q->len >= q->limit)
		q->state |= Qflow;
	iunlock(q);

	if(dowakeup)
		wakeup(&q->rr);

	return len;
}

/*
 *  copy from offset in the queue
 */
Block*
qcopy(Queue *q, int len, ulong offset)
{
	int sofar;
	int n;
	Block *b, *nb;
	uchar *p;

	nb = allocb(len);

	ilock(q);

	/* go to offset */
	b = q->bfirst;
	for(sofar = 0; ; sofar += n){
		if(b == nil){
			iunlock(q);
			return nb;
		}
		n = BLEN(b);
		if(sofar + n > offset){
			p = b->rp + offset - sofar;
			n -= offset - sofar;
			break;
		}
		QDEBUG checkb(b, "qcopy");
		b = b->next;
	}

	/* copy bytes from there */
	for(sofar = 0; sofar < len;){
		if(n > len - sofar)
			n = len - sofar;
		memmove(nb->wp, p, n);
		qcopycnt += n;
		sofar += n;
		nb->wp += n;
		b = b->next;
		if(b == nil)
			break;
		n = BLEN(b);
		p = b->rp;
	}
	iunlock(q);

	return nb;
}

/*
 *  called by non-interrupt code
 */
Queue*
qopen(int limit, int msg, void (*kick)(void*), void *arg)
{
	Queue *q;

	q = malloc(sizeof(Queue));
	if(q == 0)
		return 0;

	ilock(q);
	q->limit = q->inilim = limit;
	q->kick = kick;
	q->arg = arg;
	q->state = 0;
	if(msg > 0)
		q->state |= Qmsg;
	else if(msg < 0)
		q->state |= Qcoalesce;
	
	q->state |= Qstarve;
	q->eof = 0;
	q->noblock = 0;
	iunlock(q);

	return q;
}

static int
notempty(void *a)
{
	Queue *q = a;

	return (q->state & Qclosed) || q->bfirst != 0;
}

/*
 *  wait for the queue to be non-empty or closed.
 *  called with q ilocked.
 */
static int
qwait(Queue *q)
{
	/* wait for data */
	for(;;){
		if(q->bfirst != nil)
			break;

		if(q->state & Qclosed){
			if(++q->eof > 3)
				return -1;
			return 0;
		}

		q->state |= Qstarve;	/* flag requesting producer to wake me */
		iunlock(q);
		sleep(&q->rr, notempty, q);
		ilock(q);
	}
	return 1;
}

/*
 *  called with q ilocked
 */
static Block*
qremove(Queue *q)
{
	Block *b;

	b = q->bfirst;
	if(b == nil)
		return nil;
	q->bfirst = b->next;
	b->next = nil;
	q->dlen -= BLEN(b);
	q->len -= BALLOC(b);
	QDEBUG checkb(b, "qremove");
	return b;
}

/*
 *  put a block back to the front of the queue
 *  called with q ilocked
 */
static void
qputback(Queue *q, Block *b)
{
	b->next = q->bfirst;
	if(q->bfirst == nil)
		q->blast = b;
	q->bfirst = b;
	q->len += BALLOC(b);
	q->dlen += BLEN(b);
}

/*
 *  flow control, get producer going again
 *  called with q ilocked
 */
static void
qwakeup_iunlock(Queue *q)
{
	int dowakeup = 0;

	/* if writer flow controlled, restart */
	if((q->state & Qflow) && q->len < q->limit/2){
		q->state &= ~Qflow;
		dowakeup = 1;
	}

	iunlock(q);

	/* wakeup flow controlled writers */
	if(dowakeup){
		if(q->kick)
			q->kick(q->arg);
		wakeup(&q->wr);
	}
}

/*
 *  get next block from a queue (up to a limit)
 */
Block*
qbread(Queue *q, int len)
{
	Block *b, *nb;
	int n;

	qlock(&q->rlock);
	if(waserror()){
		qunlock(&q->rlock);
		nexterror();
	}

	ilock(q);
	switch(qwait(q)){
	case 0:
		/* queue closed */
		iunlock(q);
		qunlock(&q->rlock);
		poperror();
		return nil;
	case -1:
		/* multiple reads on a closed queue */
		iunlock(q);
		error(q->err);
	}

	/* if we get here, there's at least one block in the queue */
	b = qremove(q);
	n = BLEN(b);

	/* split block if it's too big and this is not a message queue */
	nb = b;
	if(n > len){
		if((q->state&Qmsg) == 0){
			n -= len;
			b = allocb(n);
			memmove(b->wp, nb->rp+len, n);
			b->wp += n;
			qputback(q, b);
		}
		nb->wp = nb->rp + len;
	}

	/* restart producer */
	qwakeup_iunlock(q);

	poperror();
	qunlock(&q->rlock);
	return nb;
}

/*
 *  read a queue.  if no data is queued, post a Block
 *  and wait on its Rendez.
 */
long
qread(Queue *q, void *vp, int len)
{
	Block *b, *first, *next, **l;
	int m;
	uchar *s, *e, *p;

	s = p = vp;
	e = s+len;

	qlock(&q->rlock);
	if(waserror()){
		qunlock(&q->rlock);
		nexterror();
	}

	ilock(q);
again:
	switch(qwait(q)){
	case 0:
		/* queue closed */
		iunlock(q);
		qunlock(&q->rlock);
		poperror();
		return 0;
	case -1:
		/* multiple reads on a closed queue */
		iunlock(q);
		error(q->err);
	}

	/* if we get here, there's at least one block in the queue */
	if(q->state & Qcoalesce){
		/* when coalescing, 0 length blocks just go away */
		b = q->bfirst;
		if(BLEN(b) <= 0){
			freeb(qremove(q));
			goto again;
		}

		/*  grab the first block plus as many
		 *  following blocks as will completely
		 *  fit in the read.
		 */
		l = &first;
		m = BLEN(b);
		for(;;) {
			*l = qremove(q);
			l = &b->next;
			p += m;

			b = q->bfirst;
			if(b == nil)
				break;
			m = BLEN(b);
			if(p+m > e)
				break;
		}
	} else {
		first = qremove(q);
	}

	/* copy to user space outside of the ilock */
	iunlock(q);
	p = s;
	for(b = first; b != nil; b = next){
		m = BLEN(b);
		if(m > e - p){
			m = e - p;
			memmove(p, b->rp, m);
			p += m;
			b->rp += m;
			break;
		}
		memmove(p, b->rp, m);
		p += m;
		next = b->next;
		b->next = nil;
		freeb(b);
	}
	ilock(q);

	/* take care of any left over partial block */
	if(b != nil){
		if(q->state & Qmsg)
			freeb(b);
		else
			qputback(q, b);
	}

	/* restart producer */
	qwakeup_iunlock(q);

	poperror();
	qunlock(&q->rlock);
	return p-s;
}

static int
qnotfull(void *a)
{
	Queue *q = a;

	return q->len < q->limit || (q->state & Qclosed);
}

/*
 *  add a block to a queue obeying flow control
 */
long
qbwrite(Queue *q, Block *b)
{
	int n, dowakeup;
	Proc *p;

	dowakeup = 0;
	n = BLEN(b);
	qlock(&q->wlock);
	if(waserror()){
		if(b != nil)
			freeb(b);
		qunlock(&q->wlock);
		nexterror();
	}

	ilock(q);

	/* give up if the queue is closed */
	if(q->state & Qclosed){
		iunlock(q);
		error(q->err);
	}

	/* if nonblocking, don't queue over the limit */
	if(q->len >= q->limit){
		if(q->noblock){
			iunlock(q);
			freeb(b);
			qunlock(&q->wlock);
			poperror();
			return n;
		}
	}

	/* queue the block */
	if(q->bfirst)
		q->blast->next = b;
	else
		q->bfirst = b;
	q->blast = b;
	b->next = 0;
	q->len += BALLOC(b);
	q->dlen += n;
	QDEBUG checkb(b, "qbwrite");
	b = nil;

	/* make sure other end gets awakened */
	if(q->state & Qstarve){
		q->state &= ~Qstarve;
		dowakeup = 1;
	}
	iunlock(q);

	if(dowakeup){
		if(q->kick)
			q->kick(q->arg);
		p = wakeup(&q->rr);

		/* if we just wokeup a higher priority process, let it run */
		if(p != nil && p->priority > up->priority)
			sched();
	}

	/*
	 *  flow control, wait for queue to get below the limit
	 *  before allowing the process to continue and queue
	 *  more.  We do this here so that postnote can only
	 *  interrupt us after the data has been quued.  This
	 *  means that things like 9p flushes and ssl messages
	 *  will not be disrupted by software interrupts.
	 *
	 *  Note - this is moderately dangerous since a process
	 *  that keeps getting interrupted and rewriting will
	 *  queue infinite crud.
	 */
	for(;;){
		if(q->noblock || qnotfull(q))
			break;

		ilock(q);
		q->state |= Qflow;
		iunlock(q);
		sleep(&q->wr, qnotfull, q);
	}
	USED(b);

	qunlock(&q->wlock);
	poperror();
	return n;
}

/*
 *  write to a queue.  only Maxatomic bytes at a time is atomic.
 */
int
qwrite(Queue *q, void *vp, int len)
{
	int n, sofar;
	Block *b;
	uchar *p = vp;

	QDEBUG if(islo() == 0)
		print("qwrite hi %lux\n", getcallerpc(&q));

	sofar = 0;
	do {
		n = len-sofar;
		if(n > Maxatomic)
			n = Maxatomic;

		b = allocb(n);
		setmalloctag(b, (up->text[0]<<24)|(up->text[1]<<16)|(up->text[2]<<8)|up->text[3]);
		if(waserror()){
			freeb(b);
			nexterror();
		}
		memmove(b->wp, p+sofar, n);
		poperror();
		b->wp += n;

		qbwrite(q, b);

		sofar += n;
	} while(sofar < len && (q->state & Qmsg) == 0);

	return len;
}

/*
 *  used by print() to write to a queue.  Since we may be splhi or not in
 *  a process, don't qlock.
 */
int
qiwrite(Queue *q, void *vp, int len)
{
	int n, sofar, dowakeup;
	Block *b;
	uchar *p = vp;

	dowakeup = 0;

	sofar = 0;
	do {
		n = len-sofar;
		if(n > Maxatomic)
			n = Maxatomic;

		b = allocb(n);
		memmove(b->wp, p+sofar, n);
		b->wp += n;

		ilock(q);

		QDEBUG checkb(b, "qiwrite");
		if(q->bfirst)
			q->blast->next = b;
		else
			q->bfirst = b;
		q->blast = b;
		q->len += BALLOC(b);
		q->dlen += n;

		if(q->state & Qstarve){
			q->state &= ~Qstarve;
			dowakeup = 1;
		}

		iunlock(q);

		if(dowakeup){
			if(q->kick)
				q->kick(q->arg);
			wakeup(&q->rr);
		}

		sofar += n;
	} while(sofar < len && (q->state & Qmsg) == 0);

	return len;
}

/*
 *  be extremely careful when calling this,
 *  as there is no reference accounting
 */
void
qfree(Queue *q)
{
	qclose(q);
	free(q);
}

/*
 *  Mark a queue as closed.  No further IO is permitted.
 *  All blocks are released.
 */
void
qclose(Queue *q)
{
	Block *bfirst;

	if(q == nil)
		return;

	/* mark it */
	ilock(q);
	q->state |= Qclosed;
	q->state &= ~(Qflow|Qstarve);
	strcpy(q->err, Ehungup);
	bfirst = q->bfirst;
	q->bfirst = 0;
	q->len = 0;
	q->dlen = 0;
	q->noblock = 0;
	iunlock(q);

	/* free queued blocks */
	freeblist(bfirst);

	/* wake up readers/writers */
	wakeup(&q->rr);
	wakeup(&q->wr);
}

/*
 *  Mark a queue as closed.  Wakeup any readers.  Don't remove queued
 *  blocks.
 */
void
qhangup(Queue *q, char *msg)
{
	/* mark it */
	ilock(q);
	q->state |= Qclosed;
	if(msg == 0 || *msg == 0)
		strcpy(q->err, Ehungup);
	else
		strncpy(q->err, msg, ERRLEN-1);
	iunlock(q);

	/* wake up readers/writers */
	wakeup(&q->rr);
	wakeup(&q->wr);
}

/*
 *  return non-zero if the q is hungup
 */
int
qisclosed(Queue *q)
{
	return q->state & Qclosed;
}

/*
 *  mark a queue as no longer hung up
 */
void
qreopen(Queue *q)
{
	ilock(q);
	q->state &= ~Qclosed;
	q->state |= Qstarve;
	q->eof = 0;
	q->limit = q->inilim;
	iunlock(q);
}

/*
 *  return bytes queued
 */
int
qlen(Queue *q)
{
	return q->dlen;
}

/*
 * return space remaining before flow control
 */
int
qwindow(Queue *q)
{
	int l;

	l = q->limit - q->len;
	if(l < 0)
		l = 0;
	return l;
}

/*
 *  return true if we can read without blocking
 */
int
qcanread(Queue *q)
{
	return q->bfirst!=0;
}

/*
 *  change queue limit
 */
void
qsetlimit(Queue *q, int limit)
{
	q->limit = limit;
}

/*
 *  set blocking/nonblocking
 */
void
qnoblock(Queue *q, int onoff)
{
	q->noblock = onoff;
}

/*
 *  flush the output queue
 */
void
qflush(Queue *q)
{
	Block *bfirst;

	/* mark it */
	ilock(q);
	bfirst = q->bfirst;
	q->bfirst = 0;
	q->len = 0;
	q->dlen = 0;
	iunlock(q);

	/* free queued blocks */
	freeblist(bfirst);

	/* wake up readers/writers */
	wakeup(&q->wr);
}

int
qfull(Queue *q)
{
	return q->state & Qflow;
}

int
qstate(Queue *q)
{
	return q->state;
}
