#include	"lib9.h"
#include	"sys.h"
#include	"error.h"

#define QDEBUG if(0)

/*
 *  IO queues
 */
/* typedef is in sys.h */
struct Queue
{
	Lock	l;

	Block*	bfirst;		/* buffer */
	Block*	blast;

	int	len;		/* bytes in queue */
	int	limit;		/* max bytes in queue */
	int	inilim;		/* initial limit */
	int	state;
	int	noblock;	/* true if writes return immediately when q full */
	int	eof;		/* number of eofs read by user */

	void	(*kick)(void*);	/* restart output */
	void*	arg;		/* argument to kick */

	Qlock	rlock;		/* mutex for reading processes */
	Rendez	rr;		/* process waiting to read */
	Qlock	wlock;		/* mutex for writing processes */
	Rendez	wr;		/* process waiting to write */

	char	err[ERRLEN];
	int	want;
};

enum
{
	/* Queue.state */	
	Qstarve		= (1<<0),	/* consumer starved */
	Qmsg		= (1<<1),	/* message stream */
	Qclosed		= (1<<2),
	Qflow		= (1<<3)
};

void
checkb(Block *b, char *msg)
{
	if(b->base > b->lim)
		panic("checkb 0 %s %lux %lux", msg, b->base, b->lim);
	if(b->rp < b->base)
		panic("checkb 1 %s %lux %lux", msg, b->base, b->rp);
	if(b->wp < b->base)
		panic("checkb 2 %s %lux %lux", msg, b->base, b->wp);
	if(b->rp > b->lim)
		panic("checkb 3 %s %lux %lux", msg, b->rp, b->lim);
	if(b->wp > b->lim)
		panic("checkb 4 %s %lux %lux", msg, b->wp, b->lim);
}

void
freeb(Block *b)
{
	/*
	 * drivers which perform non cache coherent DMA manage their own buffer 
	 * pool of uncached buffers and provide their own free routine.
	 */
	if(b->free) {
		b->free(b);
		return;
	}

	/* poison the block in case someone is still holding onto it */
	b->next = (void*)0xdeadbabe;
	b->rp = (void*)0xdeadbabe;
	b->wp = (void*)0xdeadbabe;
	b->lim = (void*)0xdeadbabe;
	b->base = (void*)0xdeadbabe;

	free(b);
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
		freeb(b);
	}
}

ulong padblockoverhead;

/*
 *  pad a block to the front
 */
Block*
padblock(Block *bp, int size)
{
	int n;
	Block *nbp;

	if(bp->rp - bp->base > size){
		bp->rp -= size;
		return bp;
	}

	n = bp->wp - bp->rp;
	padblockoverhead += n;
	nbp = allocb(size+n);
	nbp->rp += size;
	nbp->wp = nbp->rp;
	memmove(nbp->wp, bp->rp, n);
	nbp->wp += n;
	freeb(bp);
	nbp->rp -= size;
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

	nb = allocb(blocklen(bp));
	for(f = bp; f; f = f->next) {
		len = BLEN(f);
		memmove(nb->wp, f->rp, len);
		nb->wp += len;
	}
	freeblist(bp);
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
	 *  this should almost always be true, the rest it
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
	while((nbp = bp->next) != nil){
		i = BLEN(nbp);
		if(i >= n) {
			memmove(bp->wp, nbp->rp, n);
			bp->wp += n;
			nbp->rp += n;
			return bp;
		}
		else {
			memmove(bp->wp, nbp->rp, i);
			bp->wp += i;
			bp->next = nbp->next;
			nbp->next = 0;
			freeb(nbp);
			n -= i;
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
	long l;
	Block *nb, *startb;

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

	return nbp;
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
		if(BLEN(bp) == 0) {
			*bph = bp->next;
			bp->next = nil;
			freeb(bp);
		}
	}
	return bytes;
}

/*
 *  allocate queues and blocks
 */
Block*
allocb(int size)
{
	Block *b;

	b = mallocz(sizeof(Block)+size);
	if(b == 0)
		panic("allocb");

	b->base = (uchar*)b + sizeof(Block);
	b->lim = b->base + size;
	b->rp = b->base;
	b->wp = b->rp;

	return b;
}

/*
 *  get next block from a queue, return null of nothing there
 */
Block*
qget(Queue *q)
{
	int dowakeup;
	Block *b;

	/* sync with qwrite */
	lock(&q->l);

	b = q->bfirst;
	if(b == 0){
		q->state |= Qstarve;
		unlock(&q->l);
		return 0;
	}
	q->bfirst = b->next;
	b->next = 0;
	q->len -= BLEN(b);

	/* if writer flow controlled, restart */
	if((q->state & Qflow) && q->len < q->limit/2){
		q->state &= ~Qflow;
		dowakeup = 1;
	} else
		dowakeup = 0;

	unlock(&q->l);

	if(dowakeup)
		rendwakeup(&q->wr);

	return b;
}

/*
 *  throw away the next 'len' bytes in the queue
 */
void
qdiscard(Queue *q, int len)
{
	Block *b;
	int n;

	lock(&q->l);
	while((b = q->bfirst) != nil){
		n = BLEN(b);
		if(n > len){
			b->rp += len;
			break;
		}
		q->bfirst = b->next;
		q->len -= n;
		len -= n;
		freeb(b);
	}
	unlock(&q->l);
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

	/* sync with qwrite */
	lock(&q->l);

	for(;;) {
		b = q->bfirst;
		if(b == 0){
			q->state |= Qstarve;
			unlock(&q->l);
			return -1;
		}
		QDEBUG checkb(b, "qconsume 1");

		n = BLEN(b);
		if(n > 0)
			break;
		q->bfirst = b->next;
		freeb(b);
	};

	if(n < len)
		len = n;
	memmove(p, b->rp, len);
	if((q->state & Qmsg) || len == n)
		q->bfirst = b->next;
	b->rp += len;
	q->len -= len;

	/* if writer flow controlled, restart */
	if((q->state & Qflow) && q->len < q->limit/2){
		q->state &= ~Qflow;
		dowakeup = 1;
	} else
		dowakeup = 0;

	unlock(&q->l);

	if(dowakeup)
		rendwakeup(&q->wr);

	QDEBUG checkb(b, "qconsume 2");
	/* discard the block if we're done with it */
	if((q->state & Qmsg) || len == n)
		freeb(b);
	return len;
}

int
qpass(Queue *q, Block *b)
{
	int len, dowakeup;

	len = BLEN(b);
	/* sync with qread */
	dowakeup = 0;
	lock(&q->l);

	/* save in buffer */
	if(q->bfirst)
		q->blast->next = b;
	else
		q->bfirst = b;
	q->blast = b;
	q->len += len;
	QDEBUG checkb(b, "qpass");

	if(q->len >= q->limit/2)
		q->state |= Qflow;

	if(q->state & Qstarve){
		q->state &= ~Qstarve;
		dowakeup = 1;
	}
	unlock(&q->l);

	if(dowakeup)
		rendwakeup(&q->rr);

	return len;
}

int
qproduce(Queue *q, void *vp, int len)
{
	Block *b;
	int dowakeup;
	uchar *p = vp;

	/* sync with qread */
	dowakeup = 0;
	lock(&q->l);

	/* no waiting receivers, room in buffer? */
	if(q->len >= q->limit){
		q->state |= Qflow;
		unlock(&q->l);
		return -1;
	}

	/* save in buffer */
	b = allocb(len);
	if(b == 0){
		unlock(&q->l);
		return 0;
	}
	memmove(b->wp, p, len);
	b->wp += len;
	if(q->bfirst)
		q->blast->next = b;
	else
		q->bfirst = b;
	q->blast = b;
	q->len += len;
	QDEBUG checkb(b, "qproduce");

	if(q->state & Qstarve){
		q->state &= ~Qstarve;
		dowakeup = 1;
	}
	unlock(&q->l);

	if(dowakeup)
		rendwakeup(&q->rr);

	return len;
}

/*
 *  copy from offset in the queue
 */
Block*
qcopy(Queue *q, int len, long offset)
{
	int sofar;
	int n;
	Block *b, *nb;
	uchar *p;

	nb = allocb(len);

	lock(&q->l);

	/* go to offset */
	b = q->bfirst;
	for(sofar = 0; ; sofar += n){
		if(b == nil){
			unlock(&q->l);
			return nb;
		}
		n = BLEN(b);
		if(sofar + n > offset){
			p = b->rp + offset - sofar;
			n -= offset - sofar;
			break;
		}
		b = b->next;
	}

	/* copy bytes from there */
	for(sofar = 0; sofar < len;){
		if(n > len - sofar)
			n = len - sofar;
		memmove(nb->wp, p, n);
		sofar += n;
		nb->wp += n;
		b = b->next;
		if(b == nil)
			break;
		n = BLEN(b);
		p = b->rp;
	}
	unlock(&q->l);

	return nb;
}

/*
 *  called by non-interrupt code
 */
Queue*
qopen(int limit, int msg, void (*kick)(void*), void *arg)
{
	Queue *q;

	q = mallocz(sizeof(Queue));
	if(q == 0)
		return 0;

	q->limit = q->inilim = limit;
	q->kick = kick;
	q->arg = arg;
	q->state = msg ? Qmsg : 0;
	q->state |= Qstarve;
	q->eof = 0;

	return q;
}

static int
notempty(void *a)
{
	Queue *q = a;

	return (q->state & Qclosed) || q->bfirst != 0;
}

/*
 *  get next block from a queue (up to a limit)
 */
Block*
qbread(Queue *q, int len)
{
	Block *b, *nb;
	int n, dowakeup;

	qlock(&q->rlock);
	if(waserror()){
		qunlock(&q->rlock);
		nexterror();
	}

	/* wait for data */
	for(;;){
		/* sync with qwrite/qproduce */
		lock(&q->l);

		b = q->bfirst;
		if(b)
			break;

		if(q->state & Qclosed){
			unlock(&q->l);
			poperror();
			qunlock(&q->rlock);
			if(++q->eof > 3)
				error(q->err);
			return 0;
		}

		q->state |= Qstarve;	/* flag requesting producer to wake me */
		unlock(&q->l);
		rendsleep(&q->rr, notempty, q);
	}

	/* remove a buffered block */
	q->bfirst = b->next;
	n = BLEN(b);
	q->len -= n;

	/* if writer flow controlled, restart */
	if((q->state & Qflow) && q->len < q->limit/2){
		q->state &= ~Qflow;
		dowakeup = 1;
	} else
		dowakeup = 0;

	/* split block if its too big and this is not a message oriented queue */
	nb = b;
	if(n > len){
		if((q->state&Qmsg) == 0){
			unlock(&q->l);

			n -= len;
			b = allocb(n);
			memmove(b->wp, nb->rp+len, n);
			b->wp += n;

			lock(&q->l);
			b->next = q->bfirst;
			if(q->bfirst == 0)
				q->blast = b;
			q->bfirst = b;
			q->len += n;
		}
		nb->wp = nb->rp + len;
	}

	unlock(&q->l);

	/* wakeup flow controlled writers */
	if(dowakeup){
		if(q->kick)
			(*q->kick)(q->arg);
		rendwakeup(&q->wr);
	}

	poperror();
	qunlock(&q->rlock);
	return nb;
}

/*
 *  read a queue
 */
long
qread(Queue *q, void *vp, int len)
{
	Block *b;

	b = qbread(q, len);
	if(b == 0)
		return 0;

	len = BLEN(b);
	memmove(vp, b->rp, len);
	freeb(b);
	return len;
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

	dowakeup = 0;
	n = BLEN(b);

	if(waserror()){
		qunlock(&q->wlock);
		nexterror();
	}
	qlock(&q->wlock);

	/* flow control */
	while(!qnotfull(q)){
		if(q->noblock){
			freeb(b);
			qunlock(&q->wlock);
			poperror();
			return n;
		}
		q->state |= Qflow;
		rendsleep(&q->wr, qnotfull, q);
	}

	lock(&q->l);

	if(q->state & Qclosed){
		freeb(b);
		unlock(&q->l);
		error(q->err);
	}

	if(q->bfirst)
		q->blast->next = b;
	else
		q->bfirst = b;
	q->blast = b;
	q->len += n;

	if(q->state & Qstarve){
		q->state &= ~Qstarve;
		dowakeup = 1;
	}

	unlock(&q->l);

	if(dowakeup){
		if(q->kick)
			(*q->kick)(q->arg);
		rendwakeup(&q->rr);
	}

	qunlock(&q->wlock);
	poperror();
	return n;
}

/*
 *  write to a queue.  only 128k at a time is atomic.
 */
long
qwrite(Queue *q, void *vp, int len)
{
	int n, sofar;
	Block *b;
	uchar *p = vp;

	sofar = 0;
	do {
		n = len-sofar;
		if(n > 128*1024)
			n = 128*1024;

		b = allocb(n);
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

long
qiwrite(Queue *q, void *vp, int len)
{
	int n, sofar, dowakeup;
	Block *b;
	uchar *p = vp;

	dowakeup = 0;

	sofar = 0;
	do {
		n = len-sofar;
		if(n > 128*1024)
			n = 128*1024;

		b = allocb(n);
		memmove(b->wp, p+sofar, n);
		b->wp += n;

		lock(&q->l);

		QDEBUG checkb(b, "qiwrite");
		if(q->bfirst)
			q->blast->next = b;
		else
			q->bfirst = b;
		q->blast = b;
		q->len += n;

		if(q->state & Qstarve){
			q->state &= ~Qstarve;
			dowakeup = 1;
		}

		unlock(&q->l);

		if(dowakeup){
			if(q->kick)
				(*q->kick)(q->arg);
			rendwakeup(&q->rr);
		}

		sofar += n;
	} while(sofar < len && (q->state & Qmsg) == 0);

	return len;
}

/*
 *  Mark a queue as closed.  No further IO is permitted.
 *  All blocks are released.
 */
void
qclose(Queue *q)
{
	Block *bfirst;

	/* mark it */
	lock(&q->l);
	q->state |= Qclosed;
	strcpy(q->err, Ehungup);
	bfirst = q->bfirst;
	q->bfirst = 0;
	q->len = 0;
	q->noblock = 0;
	unlock(&q->l);

	/* free queued blocks */
	freeblist(bfirst);

	/* wake up readers/writers */
	rendwakeup(&q->rr);
	rendwakeup(&q->wr);
}

/*
 *  Mark a queue as closed.  wakeup any readers.  Don't remove queued
 *  blocks.
 */
void
qhangup(Queue *q, char *msg)
{
	/* mark it */
	lock(&q->l);
	q->state |= Qclosed;
	if(msg == 0 || *msg == 0)
		strcpy(q->err, Ehungup);
	else
		strncpy(q->err, msg, ERRLEN-1);
	unlock(&q->l);

	/* wake up readers/writers */
	rendwakeup(&q->rr);
	rendwakeup(&q->wr);
}

/*
 *  mark a queue as no longer hung up
 */
void
qreopen(Queue *q)
{
	q->state &= ~Qclosed;
	q->state |= Qstarve;
	q->eof = 0;
	q->limit = q->inilim;
}

/*
 *  return bytes queued
 */
int
qlen(Queue *q)
{
	return q->len;
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
	lock(&q->l);
	bfirst = q->bfirst;
	q->bfirst = 0;
	q->len = 0;
	unlock(&q->l);

	/* free queued blocks */
	freeblist(bfirst);

	/* wake up readers/writers */
	rendwakeup(&q->wr);
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

int
qclosed(Queue *q)
{
	return q->state & Qclosed;
}
