#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"
#include	"devtab.h"

/*
 *  Part 1) Blocks
 */

/*
 *  Allocate a block.  Put the data portion at the end of the smalloc'd
 *  chunk so that it can easily grow from the front to add protocol
 *  headers.  Thank Larry Peterson for the suggestion.
 */
Block *
allocb(ulong size)
{
	Block *bp;
	uchar *base, *lim;

	bp = smalloc(sizeof(Block)+size);

	base = (uchar*)bp + sizeof(Block);
	lim = (uchar*)bp + msize(bp);
	bp->wptr = bp->rptr = lim - size;
	bp->base = base;
	bp->lim = lim;
	bp->flags = 0;
	bp->next = 0;
	bp->list = 0;
	bp->type = M_DATA;
	return bp;
}

/*
 *  Free a block (or list of blocks).  Poison its pointers so that
 *  someone trying to access it after freeing will cause a panic.
 */
void
freeb(Block *bp)
{
	Block *next;

	while(bp){
		bp->rptr = 0;
		bp->wptr = 0;
		next = bp->next;
		free(bp);
		bp = next;	
	}
}

/*
 *  Pad a block to the front with n bytes.  This is used to add protocol
 *  headers to the front of blocks.
 */
Block *
padb(Block *bp, int n)
{
	Block *nbp;

	if(bp->base && bp->rptr-bp->base>=n){
		bp->rptr -= n;
		return bp;
	} else {
		nbp = allocb(n);
		nbp->wptr = nbp->lim;
		nbp->rptr = nbp->wptr - n;
		nbp->next = bp;
		return nbp;
	}
} 

/*
 *  make sure the first block has n bytes
 */
Block *
pullup(Block *bp, int n)
{
	Block *nbp;
	int i;

	/*
	 *  this should almost always be true, the rest it
	 *  just for to avoid every caller checking.
	 */
	if(BLEN(bp) >= n)
		return bp;

	/*
	 *  if not enough room in the first block,
	 *  add another to the front of the list.
	 */
	if(bp->lim - bp->rptr < n){
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
		if(i >= n) {
			memmove(bp->wptr, nbp->rptr, n);
			bp->wptr += n;
			nbp->rptr += n;
			return bp;
		} else {
			memmove(bp->wptr, nbp->rptr, i);
			bp->wptr += i;
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
 *  return the number of data bytes of a list of blocks
 */
int
blen(Block *bp)
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
 *  round a block chain to some even number of bytes.  Used
 *  by devip.c becuase all IP packets must have an even number
 *  of bytes.
 *
 *  The last block in the returned chain will have S_DELIM set.
 */
int
bround(Block *bp, int amount)
{
	Block *last;
	int len, pad;

	len = 0;
	SET(last);	/* Ken's magic */

	while(bp) {
		len += BLEN(bp);
		last = bp;
		bp = bp->next;
	}

	pad = ((len + amount) & ~amount) - len;
	if(pad) {
		if(last->lim - last->wptr >= pad){
			memset(last->wptr, 0, pad);
			last->wptr += pad;
		} else {
			last->next = allocb(pad);
			last->flags &= ~S_DELIM;
			last = last->next;
			last->wptr += pad;
			last->flags |= S_DELIM;
		}
	}

	return len + pad;
}

/*
 *  expand a block list to be one block, len bytes long.  used by
 *  ethernet routines.
 */
Block*
expandb(Block *bp, int len)
{
	Block *nbp, *new;
	int i;
	ulong delim = 0;

	new = allocb(len);
	if(new == 0){
		freeb(bp);
		return 0;
	}

	/*
	 *  copy bytes into new block
	 */
	for(nbp = bp; len>0 && nbp; nbp = nbp->next){
		delim = nbp->flags & S_DELIM;
		i = BLEN(nbp);
		if(i > len) {
			memmove(new->wptr, nbp->rptr, len);
			new->wptr += len;
			break;
		} else {
			memmove(new->wptr, nbp->rptr, i);
			new->wptr += i;
			len -= i;
		}
	}
	if(len){
		memset(new->wptr, 0, len);
		new->wptr += len;
	}
	new->flags |= delim;
	freeb(bp);
	return new;

}

/*
 *  make a copy of the first 'count' bytes of a block chain.  Use
 *  by transport protocols.
 */
Block *
copyb(Block *bp, int count)
{
	Block *nb, *head, **p;
	int l;

	p = &head;
	while(count) {
		l = BLEN(bp);
		if(count < l)
			l = count;
		nb = allocb(l);
		if(nb == 0)
			panic("copyb.1");
		memmove(nb->wptr, bp->rptr, l);
		nb->wptr += l;
		count -= l;
		if(bp->flags & S_DELIM)
			nb->flags |= S_DELIM;
		*p = nb;
		p = &nb->next;
		bp = bp->next;
		if(bp == 0)
			break;
	}
	if(count) {
		nb = allocb(count);
		if(nb == 0)
			panic("copyb.2");
		memset(nb->wptr, 0, count);
		nb->wptr += count;
		nb->flags |= S_DELIM;
		*p = nb;
	}
	if(blen(head) == 0)
		print("copyb: zero length\n");

	return head;
}

/*
 *  Part 2) Queues
 */

/*
 *  process end line discipline
 */
static Streamput stputq;
Qinfo procinfo =
{
	stputq,
	nullput,
	0,
	0,
	"process"
};

/*
 *  line disciplines that can be pushed
 */
static Qinfo *lds;

/*
 *  make known a stream module and call its initialization routine, if
 *  it has one.
 */
void
newqinfo(Qinfo *qi)
{
	if(qi->next)
		panic("newqinfo: already configured");

	qi->next = lds;
	lds = qi;
	if(qi->reset)
		(*qi->reset)();
}

/*
 *  find the info structure for line discipline 'name'
 */
Qinfo *
qinfofind(char *name)
{
	Qinfo *qi;

	if(name == 0)
		return 0;
	for(qi = lds; qi; qi = qi->next)
		if(strcmp(qi->name, name)==0)
			return qi;
	return 0;
}

/*
 *  allocate a pair of queues.  flavor them with the requested put routines.
 *  the `QINUSE' flag on the read side is the only one used.
 */
static Queue *
allocq(Qinfo *qi)
{
	Queue *q, *wq;

	q = smalloc(2*sizeof(Queue));

	q->flag = QINUSE;
	q->r.p = 0;
	q->info = qi;
	q->put = qi->iput;
	q->len = q->nb = 0;
	q->ptr = 0;
	q->rp = &q->r;
	wq = q->other = q + 1;

	wq->flag = QINUSE;
	wq->r.p = 0;
	wq->info = qi;
	wq->put = qi->oput;
	wq->other = q;
	wq->ptr = 0;
	wq->len = wq->nb = 0;
	wq->rp = &wq->r;

	return q;
}

/*
 *  free a queue
 */
static void
freeq(Queue *q)
{
	Block *bp;

	q = RD(q);
	while(bp = getq(q))
		freeb(bp);
	q = WR(q);
	while(bp = getq(q))
		freeb(bp);
	free(RD(q));
}

/*
 *  flush a queue
 */
static void
flushq(Queue *q)
{
	Block *bp;

	q = RD(q);
	while(bp = getq(q))
		freeb(bp);
	q = WR(q);
	while(bp = getq(q))
		freeb(bp);
}

/*
 *  push a queue onto a stream referenced by the proc side write q
 */
Queue *
pushq(Stream* s, Qinfo *qi)
{
	Queue *q;
	Queue *nq;

	q = RD(s->procq);

	/*
	 *  make the new queue
	 */
	nq = allocq(qi);

	/*
	 *  push
	 */
	qlock(s);
	RD(nq)->next = q;
	RD(WR(q)->next)->next = RD(nq);
	WR(nq)->next = WR(q)->next;
	WR(q)->next = WR(nq);
	qunlock(s);

	if(qi->open)
		(*qi->open)(RD(nq), s);

	return WR(nq)->next;
}

/*
 *  pop off the top line discipline
 */
static void
popq(Stream *s)
{
	Queue *q;

	if(waserror()){
		qunlock(s);
		nexterror();
	}
	qlock(s);
	if(s->procq->next == WR(s->devq))
		error(Ebadld);
	q = s->procq->next;
	if(q->info->close)
		(*q->info->close)(RD(q));
	s->procq->next = q->next;
	RD(q->next)->next = RD(s->procq);
	qunlock(s);
	freeq(q);
}

/*
 *  add a block (or list of blocks) to the end of a queue.  return true
 *  if one of the blocks contained a delimiter. 
 */
int
putq(Queue *q, Block *bp)
{
	int delim;

	lock(q);
	if(q->first)
		q->last->next = bp;
	else
		q->first = bp;
	q->len += BLEN(bp);
	q->nb++;
	delim = bp->flags & S_DELIM;
	while(bp->next) {
		bp = bp->next;
		q->len += BLEN(bp);
		q->nb++;
		delim |= bp->flags & S_DELIM;
	}
	q->last = bp;
	if(q->len >= Streamhi || q->nb >= Streambhi)
		q->flag |= QHIWAT;
	unlock(q);
	return delim;
}

int
putb(Blist *q, Block *bp)
{
	int delim;

	if(q->first)
		q->last->next = bp;
	else
		q->first = bp;
	q->len += BLEN(bp);
	delim = bp->flags & S_DELIM;
	while(bp->next) {
		bp = bp->next;
		q->len += BLEN(bp);
		delim |= bp->flags & S_DELIM;
	}
	q->last = bp;
	return delim;
}

/*
 *  add a block to the start of a queue 
 */
void
putbq(Blist *q, Block *bp)
{
	lock(q);
	if(q->first)
		bp->next = q->first;
	else
		q->last = bp;
	q->first = bp;
	q->len += BLEN(bp);
	q->nb++;
	unlock(q);
}

/*
 *  remove the first block from a queue
 */
Block *
getq(Queue *q)
{
	Block *bp;

	lock(q);
	bp = q->first;
	if(bp) {
		q->first = bp->next;
		if(q->first == 0)
			q->last = 0;
		q->len -= BLEN(bp);
		q->nb--;
		if((q->flag&QHIWAT) && q->len<Streamhi/2 && q->nb<Streambhi/2 &&q->other){
			wakeup(q->other->next->other->rp);
			q->flag &= ~QHIWAT;
		}
		bp->next = 0;
	}
	unlock(q);
	return bp;
}

/*
 *  grab all the blocks in a queue
 */
Block *
grabq(Queue *q)
{
	Block *bp;

	lock(q);
	bp = q->first;
	if(bp){
		q->first = 0;
		q->last = 0;
		q->len = 0;
		q->nb = 0;
		if(q->flag&QHIWAT){
			wakeup(q->other->next->other->rp);
			q->flag &= ~QHIWAT;
		}
	}
	unlock(q);
	return bp;
}

/*
 *  remove the first block from a list of blocks
 */
Block *
getb(Blist *q)
{
	Block *bp;

	bp = q->first;
	if(bp) {
		q->first = bp->next;
		if(q->first == 0)
			q->last = 0;
		q->len -= BLEN(bp);
		bp->next = 0;
	}
	return bp;
}


/*
 *  put a block into the bit bucket
 */
void
nullput(Queue *q, Block *bp)
{
	USED(q);
	if(bp->type == M_HANGUP)
		freeb(bp);
	else {
		freeb(bp);
		error(Ehungup);
	}
}

/*
 *  Part 3) Streams
 */

/*
 *  the per stream directory structure
 */
Dirtab streamdir[]={
	"data",		{Sdataqid},	0,			0600,
	"ctl",		{Sctlqid},	0,			0600,
};

/*
 *  hash buckets containing all streams
 */
enum
{
	Nbits=	5,
	Nhash=	1<<Nbits,
	Nmask=	Nhash-1,
};
typedef struct Sthash Sthash;
struct Sthash
{
	QLock;
	Stream	*s;
};
static Sthash ht[Nhash];

static void	hangup(Stream*);

/*
 *  A stream device consists of the contents of streamdir plus
 *  any directory supplied by the actual device.
 *
 *  values of s:
 * 	0 to ntab-1 apply to the auxiliary directory.
 *	ntab to ntab+Shighqid-Slowqid+1 apply to streamdir.
 */
int
streamgen(Chan *c, Dirtab *tab, int ntab, int s, Dir *dp)
{
	if(s < ntab)
		tab = &tab[s];
	else if(s < ntab + Shighqid - Slowqid + 1)
		tab = &streamdir[s - ntab];
	else
		return -1;

	devdir(c, (Qid){STREAMQID(STREAMID(c->qid.path),tab->qid.path), 0}, 
		tab->name, tab->length, eve, tab->perm, dp);
	return 1;
}

/*
 *  return a hash bucket for a stream
 */
static Sthash*
hash(int type, int dev, int id)
{
	return &ht[(type*7*7 + dev*7 + id) & Nmask];
}

/*
 *  create a new stream, if noopen is non-zero, don't increment the open count
 */
Stream *
streamnew(ushort type, ushort dev, ushort id, Qinfo *qi, int noopen)
{
	Stream *s;
	Queue *q;
	Sthash *hb;

	hb = hash(type, dev, id);

	/*
	 *  if the stream already exists, just increment the reference counts.
	 */
	qlock(hb);
	for(s = hb->s; s; s = s->next) {
		if(s->type == type && s->dev == dev && s->id == id){
			s->inuse++;
			qunlock(hb);
			if(noopen == 0){
				qlock(s);
				s->opens++;
				qunlock(s);
			}
			return s;
		}
	}

	/*
	 *  create and init a new stream
	 */
	s = smalloc(sizeof(Stream));
	s->inuse = 1;
	s->type = type;
	s->dev = dev;
	s->id = id;
	s->err = 0;
	s->hread = 0;
	s->next = hb->s;
	hb->s = s;

	/*
	 *  The ordering of these 2 instructions is very important.
	 *  It makes sure we finish the stream initialization before
	 *  anyone else can access it.
	 */
	qlock(s);
	qunlock(hb);

	if(waserror()){
		qunlock(s);
		streamclose1(s);
		nexterror();
	}

	/*
 	 *  hang a device and process q off the stream
	 */
	if(noopen)
		s->opens = 0;
	else
		s->opens = 1;
	q = allocq(&procinfo);
	WR(q)->ptr = s;
	RD(q)->ptr = s;
	s->procq = WR(q);
	q = allocq(qi);
	s->devq = RD(q);
	WR(s->procq)->next = WR(s->devq);
	RD(s->procq)->next = 0;
	RD(s->devq)->next = RD(s->procq);
	WR(s->devq)->next = 0;

	if(qi->open)
		(*qi->open)(RD(s->devq), s);

	qunlock(s);
	poperror();
	return s;
}

/*
 *  Associate a stream with a channel
 */
void
streamopen(Chan *c, Qinfo *qi)
{
	c->stream = streamnew(c->type, c->dev, STREAMID(c->qid.path), qi, 0);
}

/*
 *  Enter a stream only if the stream exists and is open.  Increment the
 *  reference count so it can't disappear under foot.
 *
 *  Return -1 if the stream no longer exists or is not opened.
 */
int
streamenter(Stream *s)
{
	Sthash *hb;
	Stream *ns;

	hb = hash(s->type, s->dev, s->id);
	qlock(hb);
	for(ns = hb->s; ns; ns = ns->next)
		if(s->type == ns->type && s->dev == ns->dev && s->id == ns->id){
			s->inuse++;
			qunlock(hb);
			if(s->opens == 0){
				streamexit(s);
				return -1;
			}
			return 0;
		}
	qunlock(hb);
	return -1;
}

/*
 *  Decrement the reference count on a stream.  If the count is
 *  zero, free the stream.
 */
void
streamexit(Stream *s)
{
	Queue *q;
	Queue *nq;
	Sthash *hb;
	Stream **l, *ns;

	hb = hash(s->type, s->dev, s->id);
	qlock(hb);
	if(s->inuse-- == 1){
		if(s->opens != 0)
			panic("streamexit %d %s\n", s->opens, s->devq->info->name);

		/*
		 *  ascend the stream freeing the queues
		 */
		for(q = s->devq; q; q = nq){
			nq = q->next;
			freeq(q);
		}
		if(s->err)
			freeb(s->err);

		/*
		 *  unchain it from the hash bucket and free
		 */
		l = &hb->s;
		for(ns = hb->s; ns; ns = ns->next){
			if(s == ns){
				*l = s->next;
				break;
			}
			l = &ns->next;
		}
		free(s);
	}
	qunlock(hb);
}

/*
 *  nail down a stream so that it can't be closed
 */
void
naildownstream(Stream *s)
{
	s->opens++;
	s->inuse++;
}

/*
 *  Decrement the open count.  When it goes to zero, call the close
 *  routines for each queue in the stream.
 */
int
streamclose1(Stream *s)
{
	Queue *q, *nq;
	int rv;

	/*
	 *  decrement the open count
	 */
	qlock(s);
	if(s->opens-- == 1){
		/*
		 *  descend the stream closing the queues
		 */
		for(q = s->procq; q; q = q->next){
			if(!waserror()){
				if(q->info->close)
					(*q->info->close)(q->other);
				poperror();
			}
			WR(q)->put = nullput;

			/*
			 *  this may be 2 streams joined device end to device end
			 */
			if(q == s->devq->other)
				break;
		}
	
		/*
		 *  ascend the stream flushing the queues
		 */
		for(q = s->devq; q; q = nq){
			nq = q->next;
			flushq(q);
		}
	}
	rv = s->opens;
	qunlock(s);

	/*
	 *  leave it and free it
	 */
	streamexit(s);
	return rv;
}
int
streamclose(Chan *c)
{
	/*
	 *  if no stream, ignore it
	 */
	if(!c->stream)
		return 0;
	return streamclose1(c->stream);
}

/*
 *  put a block to be read into the queue.  wakeup any waiting reader
 */
void
stputq(Queue *q, Block *bp)
{
	int awaken;
	Stream *s;

	if(bp->type == M_HANGUP){
		s = q->ptr;
		if(bp->rptr<bp->wptr && s->err==0)
			s->err = bp;
		else
			freeb(bp);
		q->flag |= QHUNGUP;
		q->other->flag |= QHUNGUP;
		wakeup(q->other->rp);
		awaken = 1;
	} else {
		lock(q);
		if(q->first)
			q->last->next = bp;
		else
			q->first = bp;
		q->len += BLEN(bp);
		q->nb++;
		awaken = bp->flags & S_DELIM;
		while(bp->next) {
			bp = bp->next;
			q->len += BLEN(bp);
			q->nb++;
			awaken |= bp->flags & S_DELIM;
		}
		q->last = bp;
		if(q->len >= Streamhi || q->nb >= Streambhi){
			q->flag |= QHIWAT;
			awaken = 1;
		}
		unlock(q);
	}
	if(awaken)
		wakeup(q->rp);
}

/*
 *  return the stream id
 */
long
streamctlread(Chan *c, void *vbuf, long n)
{
	char *buf = vbuf;
	char num[32];
	Stream *s;

	s = c->stream;
	if(STREAMTYPE(c->qid.path) == Sctlqid){
		sprint(num, "%d", s->id);
		return readstr(c->offset, buf, n, num);
	} else {
		if(CHDIR & c->qid.path)
			return devdirread(c, vbuf, n, 0, 0, streamgen);
		else
			panic("streamctlread");
	}
	return 0;	/* not reached */
}

/*
 *  return true if there is an output buffer available
 */
static int
isinput(void *x)
{
	Queue *q;

	q = (Queue *)x;
	return (q->flag&QHUNGUP) || q->first!=0;
}

/*
 *  read until we fill the buffer or until a DELIM is encountered
 */
long
streamread(Chan *c, void *vbuf, long n)
{
	Block *bp;
	Block *tofree;
	Stream *s;
	Queue *q;
	int left, i;
	uchar *buf = vbuf;

	if(STREAMTYPE(c->qid.path) != Sdataqid)
		return streamctlread(c, vbuf, n);

	/*
	 *  one reader at a time
	 */
	s = c->stream;
	left = n;
	qlock(&s->rdlock);
	tofree = 0;
	q = 0;
	if(waserror()){
		/*
		 *  put any partially read message back into the
		 *  queue
		 */
		while(tofree){
			bp = tofree;
			tofree = bp->next;
			bp->next = 0;
			putbq(q, bp);
		}
		qunlock(&s->rdlock);
		nexterror();
	}

	/*
	 *  sleep till data is available
	 */
	q = RD(s->procq);
	while(left){
		bp = getq(q);
		if(bp == 0){
			if(q->flag & QHUNGUP){
				if(s->err)
					error((char*)s->err->rptr);
				else if(s->hread++<3)
					break;
				else
					error(Ehungup);
			}
			q->rp = &q->r;
			sleep(q->rp, isinput, (void *)q);
			continue;
		}

		i = BLEN(bp);
		if(i <= left){
			memmove(buf, bp->rptr, i);
			left -= i;
			buf += i;
			bp->next = tofree;
			tofree = bp;
			if(bp->flags & S_DELIM)
				break;
		} else {
			memmove(buf, bp->rptr, left);
			bp->rptr += left;
			putbq(q, bp);
			left = 0;
		}
	}

	/*
	 *  free completely read blocks
	 */
	if(tofree)
		freeb(tofree);

	qunlock(&s->rdlock);
	poperror();
	return n - left;	
}

/*
 *  look for an instance of the line discipline `name' on
 *  the stream `s'
 */
void
qlook(Stream *s, char *name)
{
	Queue *q;

	for(q = s->procq; q; q = q->next){
		if(strcmp(q->info->name, name) == 0)
			return;

		/*
		 *  this may be 2 streams joined device end to device end
		 */
		if(q == s->devq->other)
			break;
	}
	error(Ebadarg);
}

/*
 *  Handle a ctl request.  Streamwide requests are:
 *
 *	hangup			-- send an M_HANGUP up the stream
 *	push ldname		-- push the line discipline named ldname
 *	pop			-- pop a line discipline
 *	look ldname		-- look for a line discipline
 *
 *  This routing is entered with s->wrlock'ed and must unlock.
 */
static long
streamctlwrite(Chan *c, void *a, long n)
{
	Qinfo *qi;
	Block *bp;
	Stream *s;

	if(STREAMTYPE(c->qid.path) != Sctlqid)
		panic("streamctlwrite %lux", c->qid);
	s = c->stream;

	/*
	 *  package
	 */
	bp = allocb(n+1);
	memmove(bp->wptr, a, n);
	bp->wptr[n] = 0;
	bp->wptr += n + 1;

	/*
	 *  check for standard requests
	 */
	if(streamparse("hangup", bp)){
		hangup(s);
		freeb(bp);
	} else if(streamparse("push", bp)){
		qi = qinfofind((char *)bp->rptr);
		if(qi == 0)
			error(Ebadld);
		pushq(s, qi);
		freeb(bp);
	} else if(streamparse("pop", bp)){
		popq(s);
		freeb(bp);
	} else if(streamparse("look", bp)){
		qlook(s, (char *)bp->rptr);
		freeb(bp);
	} else {
		bp->type = M_CTL;
		bp->flags |= S_DELIM;
		PUTNEXT(s->procq, bp);
	}

	return n;
}

/*
 *  wait till there's room in the next stream
 */
static int
notfull(void *arg)
{
	return !QFULL((Queue *)arg);
}
void
flowctl(Queue *q, Block *bp)
{
	if(bp->type != M_HANGUP){
		qlock(&q->rlock);
		if(waserror()){
			qunlock(&q->rlock);
			freeb(bp);
			nexterror();
		}
		q->rp = &q->r;
		sleep(q->rp, notfull, q->next);
		qunlock(&q->rlock);
		poperror();
	}
	PUTNEXT(q, bp);
}

/*
 *  send the request as a single delimited block
 */
long
streamwrite(Chan *c, void *a, long n, int docopy)
{
	Stream *s;
	Queue *q;
	long rem;
	int i;
	Block *bp;
	char *va;

	/*
	 *  docopy will get used if I ever figure out when to avoid copying
	 *  data. -- presotto
	 */
	USED(docopy);

	s = c->stream;

	/*
	 *  decode the qid
	 */
	if(STREAMTYPE(c->qid.path) != Sdataqid)
		return streamctlwrite(c, a, n);

	/*
	 *  No writes allowed on hungup channels
	 */
	q = s->procq;
	if(q->other->flag & QHUNGUP){
		if(s->err)
			error((char*)(s->err->rptr));
		else
			error(Ehungup);
	}

	/*
	 *  Write the message using blocks <= Streamhi bytes longs
	 */
	va = a;
	rem = n;
	for(;;){
		if(rem > Streamhi)
			i = Streamhi;
		else
			i = rem;
		bp = allocb(i);
		memmove(bp->wptr, va, i);
		bp->wptr += i;
		va += i;
		rem -= i;
		if(rem > 0){
			FLOWCTL(q, bp);
		} else {
			bp->flags |= S_DELIM;
			FLOWCTL(q, bp);
			break;
		}
	}
	return n;
}

/*
 *  stat a stream.  the length is the number of bytes up to the
 *  first delimiter.
 */
void
streamstat(Chan *c, char *db, char *name, long perm)
{
	Dir dir;
	Stream *s;
	Queue *q;
	Block *bp;
	long n;

	s = c->stream;
	if(s == 0)
		n = 0;
	else {
		q = RD(s->procq);
		lock(q);
		for(n=0, bp=q->first; bp; bp = bp->next){
			n += BLEN(bp);
			if(bp->flags&S_DELIM)
				break;
		}
		unlock(q);
	}

	devdir(c, c->qid, name, n, eve, perm, &dir);
	convD2M(&dir, db);
}

/*
 *  send a hangup up a stream
 */
static void
hangup(Stream *s)
{
	Block *bp;

	bp = allocb(0);
	bp->type = M_HANGUP;
	(*s->devq->put)(s->devq, bp);
}

/*
 *  parse a string and return a pointer to the second element if the 
 *  first matches name.  bp->rptr will be updated to point to the
 *  second element.
 *
 *  return 0 if no match.
 *
 *  it is assumed that the block data is null terminated.  streamwrite
 *  guarantees this.
 */
int
streamparse(char *name, Block *bp)
{
	int len;

	len = strlen(name);
	if(BLEN(bp) < len)
		return 0;
	if(strncmp(name, (char *)bp->rptr, len)==0){
		if(bp->rptr[len] == ' ')
			bp->rptr += len+1;
		else if(bp->rptr[len])
			return 0;
		else
			bp->rptr += len;
		while(*bp->rptr==' ' && bp->wptr>bp->rptr)
			bp->rptr++;
		return 1;
	}
	return 0;
}

/*
 *  like andrew's getmfields but no hidden state
 */
int
getfields(char *lp, char **fields, int n, char sep)
{
	int i;

	for(i=0; lp && *lp && i<n; i++){
		while(*lp == sep)
			*lp++=0;
		if(*lp == 0)
			break;
		fields[i]=lp;
		while(*lp && *lp != sep)
			lp++;
	}
	return i;
}
