#include	"dat.h"
#include	"fns.h"
#include	"error.h"

#define QDEBUG if(0)

/*
 *  IO queues
 */

struct Queue
{
	Lock	l;

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
	void	(*bypass)(void*, Block*);	/* bypass queue altogether */
	void*	arg;		/* argument to kick */

	QLock	rlock;		/* mutex for reading processes */
	Rendez	rr;		/* process waiting to read */
	QLock	wlock;		/* mutex for writing processes */
	Rendez	wr;		/* process waiting to write */

	char	err[ERRMAX];
	int	want;
};

enum
{
	Maxatomic	= 32*1024
};

uint	qiomaxatomic = Maxatomic;

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
p9freeb(Block *b)
{
	if(b == nil)
		return;

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
		b->next = 0;
		p9freeb(b);
	}
}

ulong padblockoverhead;

/*
 *  pad a block to the front (or the back if size is negative)
 */
Block*
padblock(Block *bp, int size)
{
	int n;
	Block *nbp;

	if(size >= 0){
		if(bp->rp - bp->base >= size){
			bp->rp -= size;
			return bp;
		}

		if(bp->next)
			panic("padblock 0x%luX", getcallerpc(&bp));
		n = BLEN(bp);
		padblockoverhead += n;
		nbp = p9allocb(size+n);
		nbp->rp += size;
		nbp->wp = nbp->rp;
		memmove(nbp->wp, bp->rp, n);
		nbp->wp += n;
		p9freeb(bp);
		nbp->rp -= size;
	} else {
		size = -size;

		if(bp->next)
			panic("padblock 0x%luX", getcallerpc(&bp));

		if(bp->lim - bp->wp >= size)
			return bp;

		n = BLEN(bp);
		padblockoverhead += n;
		nbp = p9allocb(size+n);
		memmove(nbp->wp, bp->rp, n);
		nbp->wp += n;
		p9freeb(bp);
	}
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
 * return count of space in blocks
 */
int
blockalloclen(Block *bp)
{
	int len;

	len = 0;
	while(bp) {
		len += BALLOC(bp);
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

	nb = p9allocb(blocklen(bp));
	for(f = bp; f; f = f->next) {
		len = BLEN(f);
		memmove(nb->wp, f->rp, len);
		nb->wp += len;
	}
	freeblist(bp);
	return nb;
}

/*
 *  make sure the first block has at least n bytes.  If we started with
 *  less than n bytes, make sure we have exactly n bytes.  devssl.c depends
 *  on this.
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
		nbp = p9allocb(n);
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
			bp->wp += n;
			nbp->rp += n;
			return bp;
		}
		else {
			memmove(bp->wp, nbp->rp, i);
			bp->wp += i;
			bp->next = nbp->next;
			nbp->next = 0;
			p9freeb(nbp);
			n -= i;
			if(n == 0)
				return bp;
		}
	}
	freeblist(bp);
	return 0;
}

/*
 *  make sure the first block has at least n bytes
 */
Block*
pullupqueue(Queue *q, int n)
{
	Block *b;

	if(BLEN(q->bfirst) >= n)
		return q->bfirst;
	q->bfirst = pullupblock(q->bfirst, n);
	for(b = q->bfirst; b != nil && b->next != nil; b = b->next)
		;
	q->blast = b;
	return q->bfirst;
}

/*
 *  trim to len bytes starting at offset
 */
Block *
trimblock(Block *bp, int offset, int len)
{
	ulong l;
	Block *nb, *startb;

	if(blocklen(bp) < offset+len) {
		freeblist(bp);
		return nil;
	}

	while((l = BLEN(bp)) < offset) {
		offset -= l;
		nb = bp->next;
		bp->next = nil;
		p9freeb(bp);
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

	nbp = p9allocb(count);
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

Block*
adjustblock(Block* bp, int len)
{
	int n;
	Block *nbp;

	if(len < 0){
		p9freeb(bp);
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
		if(BLEN(bp) == 0) {
			*bph = bp->next;
			bp->next = nil;
			p9freeb(bp);
		}
	}
	return bytes;
}

/*
 *  allocate queues and blocks (round data base address to 64 bit boundary)
 */
Block*
iallocb(int size)
{
	Block *b;
	ulong addr;

	b = kmalloc(sizeof(Block)+size);
	if(b == 0)
		return 0;
	memset(b, 0, sizeof(Block));

	addr = (ulong)b + sizeof(Block);
	b->base = (uchar*)addr;
	b->lim = b->base + size;
	b->rp = b->base;
	b->wp = b->rp;

	return b;
}


/*
  *  call error if iallocb fails
 */
Block*
p9allocb(int size)
{
	Block *b;

	b = iallocb(size);
	if(b == 0)
		exhausted("allocb");
	return b;
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
	lock(&q->l);

	b = q->bfirst;
	if(b == 0){
		q->state |= Qstarve;
		unlock(&q->l);
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

	unlock(&q->l);

	if(dowakeup)
		Wakeup(&q->wr);

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

	lock(&q->l);
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
			p9freeb(b);
		} else {
			n = len - sofar;
			b->rp += n;
			q->dlen -= n;
		}
	}

	/* if writer flow controlled, restart */
	if((q->state & Qflow) && q->len < q->limit){
		q->state &= ~Qflow;
		dowakeup = 1;
	} else
		dowakeup = 0;

	unlock(&q->l);

	if(dowakeup)
		Wakeup(&q->wr);

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
		q->len -= BALLOC(b);

		/* remember to free this */
		b->next = tofree;
		tofree = b;
	}

	if(n < len)
		len = n;
	memmove(p, b->rp, len);
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

	unlock(&q->l);

	if(dowakeup)
		Wakeup(&q->wr);

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
	lock(&q->l);
	if(q->len >= q->limit){
		unlock(&q->l);
		freeblist(b);
		return -1;
	}
	if(q->state & Qclosed){
		unlock(&q->l);
		len = blocklen(b);
		freeblist(b);
		return len;
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
	unlock(&q->l);

	if(dowakeup)
		Wakeup(&q->rr);

	return len;
}

int
qpassnolim(Queue *q, Block *b)
{
	int dlen, len, dowakeup;

	/* sync with qread */
	dowakeup = 0;
	lock(&q->l);

	len = BALLOC(b);
	if(q->state & Qclosed){
		unlock(&q->l);
		freeblist(b);
		return len;
	}

	/* add buffer to queue */
	if(q->bfirst)
		q->blast->next = b;
	else
		q->bfirst = b;
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
	unlock(&q->l);

	if(dowakeup)
		Wakeup(&q->rr);

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
			*l = p9allocb(n);
			memmove((*l)->wp, nbp->rp, n);
			(*l)->wp += n;
			(*l)->next = nbp->next;
			p9freeb(nbp);
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
	lock(&q->l);

	if(q->state & Qclosed){
		unlock(&q->l);
		return -1;
	}

	/* no waiting receivers, room in buffer? */
	if(q->len >= q->limit){
		q->state |= Qflow;
		unlock(&q->l);
		return -1;
	}

	/* save in buffer */
	b = iallocb(len);
	if(b == 0){
		unlock(&q->l);
		print("qproduce: iallocb failed\n");
		return -1;
	}
	memmove(b->wp, p, len);
	b->wp += len;
	if(q->bfirst)
		q->blast->next = b;
	else
		q->bfirst = b;
	q->blast = b;
	/* b->next = 0; done by p9allocb() */
	q->len += BALLOC(b);
	q->dlen += BLEN(b);
	QDEBUG checkb(b, "qproduce");

	if(q->state & Qstarve){
		q->state &= ~Qstarve;
		dowakeup = 1;
	}

	if(q->len >= q->limit)
		q->state |= Qflow;
	unlock(&q->l);


	if(dowakeup)
		Wakeup(&q->rr);

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

	nb = p9allocb(len);

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

	q = kmalloc(sizeof(Queue));
	if(q == 0)
		return 0;

	q->limit = q->inilim = limit;
	q->kick = kick;
	q->arg = arg;
	q->state = msg;
	q->state |= Qstarve;
	q->eof = 0;
	q->noblock = 0;

	return q;
}

/* open a queue to be bypassed */
Queue*
qbypass(void (*bypass)(void*, Block*), void *arg)
{
	Queue *q;

	q = malloc(sizeof(Queue));
	if(q == 0)
		return 0;

	q->limit = 0;
	q->arg = arg;
	q->bypass = bypass;
	q->state = 0;

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
			if(*q->err && strcmp(q->err, Ehungup) != 0)
				return -1;
			return 0;
		}

		q->state |= Qstarve;	/* flag requesting producer to wake me */
		unlock(&q->l);
		Sleep(&q->rr, notempty, q);
		lock(&q->l);
	}
	return 1;
}

/*
 * add a block list to a queue
 */
void
qaddlist(Queue *q, Block *b)
{
	/* queue the block */
	if(q->bfirst)
		q->blast->next = b;
	else
		q->bfirst = b;
	q->len += blockalloclen(b);
	q->dlen += blocklen(b);
	while(b->next)
		b = b->next;
	q->blast = b;
}

/*
 *  called with q locked
 */
Block*
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
 *  copy the contents of a string of blocks into
 *  memory.  emptied blocks are freed.  return
 *  pointer to first unconsumed block.
 */
Block*
bl2mem(uchar *p, Block *b, int n)
{
	int i;
	Block *next;

	for(; b != nil; b = next){
		i = BLEN(b);
		if(i > n){
			memmove(p, b->rp, n);
			b->rp += n;
			return b;
		}
		memmove(p, b->rp, i);
		n -= i;
		p += i;
		b->rp += i;
		next = b->next;
		p9freeb(b);
	}
	return nil;
}

/*
 *  copy the contents of memory into a string of blocks.
 *  return nil on error.
 */
Block*
mem2bl(uchar *p, int len)
{
	int n;
	Block *b, *first, **l;

	first = nil;
	l = &first;
	if(waserror()){
		freeblist(first);
		nexterror();
	}
	do {
		n = len;
		if(n > Maxatomic)
			n = Maxatomic;

		*l = b = p9allocb(n);
		setmalloctag(b, (up->text[0]<<24)|(up->text[1]<<16)|(up->text[2]<<8)|up->text[3]);
		memmove(b->wp, p, n);
		b->wp += n;
		p += n;
		len -= n;
		l = &b->next;
	} while(len > 0);
	poperror();

	return first;
}

/*
 *  put a block back to the front of the queue
 *  called with q ilocked
 */
void
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
 *  called with q locked
 */
static void
qwakeup_unlock(Queue *q)
{
	int dowakeup = 0;

	/* if writer flow controlled, restart */
	if((q->state & Qflow) && q->len < q->limit/2){
		q->state &= ~Qflow;
		dowakeup = 1;
	}

	unlock(&q->l);

	/* wakeup flow controlled writers */
	if(dowakeup){
		if(q->kick)
			q->kick(q->arg);
		Wakeup(&q->wr);
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

	lock(&q->l);
	switch(qwait(q)){
	case 0:
		/* queue closed */
		unlock(&q->l);
		poperror();
		qunlock(&q->rlock);
		return nil;
	case -1:
		/* multiple reads on a closed queue */
		unlock(&q->l);
		p9error(q->err);
	}

	/* if we get here, there's at least one block in the queue */
	b = qremove(q);
	n = BLEN(b);

	/* split block if it's too big and this is not a message oriented queue */
	nb = b;
	if(n > len){
		if((q->state&Qmsg) == 0){
			n -= len;
			b = p9allocb(n);
			memmove(b->wp, nb->rp+len, n);
			b->wp += n;
			qputback(q, b);
		}
		nb->wp = nb->rp + len;
	}

	/* restart producer */
	qwakeup_unlock(q);

	poperror();
	qunlock(&q->rlock);
	return nb;
}

/*
 *  read a queue.  if no data is queued, wait on its Rendez
 */
long
qread(Queue *q, void *vp, int len)
{
	Block *b, *first, **l;
	int m, n;

	qlock(&q->rlock);
	if(waserror()){
		qunlock(&q->rlock);
		nexterror();
	}

	lock(&q->l);
again:
	switch(qwait(q)){
	case 0:
		/* queue closed */
		unlock(&q->l);
		poperror();
		qunlock(&q->rlock);
		return 0;
	case -1:
		/* multiple reads on a closed queue */
		unlock(&q->l);
		p9error(q->err);
	}

	/* if we get here, there's at least one block in the queue */
	if(q->state & Qcoalesce){
		/* when coalescing, 0 length blocks just go away */
		b = q->bfirst;
		if(BLEN(b) <= 0){
			p9freeb(qremove(q));
			goto again;
		}

		/*  grab the first block plus as many
		 *  following blocks as will completely
		 *  fit in the read.
		 */
		n = 0;
		l = &first;
		m = BLEN(b);
		for(;;) {
			*l = qremove(q);
			l = &b->next;
			n += m;

			b = q->bfirst;
			if(b == nil)
				break;
			m = BLEN(b);
			if(n+m > len)
				break;
		}
	} else {
		first = qremove(q);
		n = BLEN(first);
	}

	/* copy to user space outside of the ilock */
	unlock(&q->l);
	b = bl2mem(vp, first, len);
	lock(&q->l);

	/* take care of any left over partial block */
	if(b != nil){
		n -= BLEN(b);
		if(q->state & Qmsg)
			p9freeb(b);
		else
			qputback(q, b);
	}

	/* restart producer */
	qwakeup_unlock(q);

	poperror();
	qunlock(&q->rlock);
	return n;
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
	volatile struct {Block *b;} cb;

	dowakeup = 0;
	n = BLEN(b);
	if(q->bypass){
		(*q->bypass)(q->arg, b);
		return n;
	}
	cb.b = b;

	qlock(&q->wlock);
	if(waserror()){
		if(cb.b != nil)
			p9freeb(cb.b);
		qunlock(&q->wlock);
		nexterror();
	}

	lock(&q->l);

	/* give up if the queue is closed */
	if(q->state & Qclosed){
		unlock(&q->l);
		p9error(q->err);
	}

	/* if nonblocking, don't queue over the limit */
	if(q->len >= q->limit){
		if(q->noblock){
			unlock(&q->l);
			p9freeb(b);
			poperror();
			qunlock(&q->wlock);
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
	cb.b = nil;

	if(q->state & Qstarve){
		q->state &= ~Qstarve;
		dowakeup = 1;
	}

	unlock(&q->l);

	/*  get output going again */
	if(q->kick && (dowakeup || (q->state&Qkick)))
		q->kick(q->arg);

	if(dowakeup)
		Wakeup(&q->rr);

	/*
	 *  flow control, wait for queue to get below the limit
	 *  before allowing the process to continue and queue
	 *  more.  We do this here so that postnote can only
	 *  interrupt us after the data has been queued.  This
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

		lock(&q->l);
		q->state |= Qflow;
		unlock(&q->l);
		Sleep(&q->wr, qnotfull, q);
	}

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

	sofar = 0;
	do {
		n = len-sofar;
		if(n > Maxatomic)
			n = Maxatomic;

		b = p9allocb(n);
		setmalloctag(b, getcallerpc(&q));
		if(waserror()){
			p9freeb(b);
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

		b = iallocb(n);
		if (b == 0) {
			print("qiwrite: iallocb failed\n");
			break;
		}
		memmove(b->wp, p+sofar, n);
		b->wp += n;

		lock(&q->l);

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

		unlock(&q->l);

		if(dowakeup){
			if(q->kick)
				q->kick(q->arg);
			Wakeup(&q->rr);
		}

		sofar += n;
	} while(sofar < len && (q->state & Qmsg) == 0);

	return sofar;
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
	lock(&q->l);
	q->state |= Qclosed;
	q->state &= ~(Qflow|Qstarve);
	strcpy(q->err, Ehungup);
	bfirst = q->bfirst;
	q->bfirst = 0;
	q->len = 0;
	q->dlen = 0;
	q->noblock = 0;
	unlock(&q->l);

	/* free queued blocks */
	freeblist(bfirst);

	/* wake up readers/writers */
	Wakeup(&q->rr);
	Wakeup(&q->wr);
}

/*
 *  Mark a queue as closed.  Wakeup any readers.  Don't remove queued
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
		kstrcpy(q->err, msg, sizeof q->err);
	unlock(&q->l);

	/* wake up readers/writers */
	Wakeup(&q->rr);
	Wakeup(&q->wr);
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
	lock(&q->l);
	q->state &= ~Qclosed;
	q->state |= Qstarve;
	q->eof = 0;
	q->limit = q->inilim;
	unlock(&q->l);
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
	lock(&q->l);
	bfirst = q->bfirst;
	q->bfirst = 0;
	q->len = 0;
	q->dlen = 0;
	unlock(&q->l);

	/* free queued blocks */
	freeblist(bfirst);

	/* wake up readers/writers */
	Wakeup(&q->wr);
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
