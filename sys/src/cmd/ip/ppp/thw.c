#include <u.h>
#include <libc.h>
#include <ip.h>
#include <auth.h>
#include "ppp.h"
#include "thwack.h"

typedef struct Cstate Cstate;
struct Cstate
{
	ulong		seq;
	Thwack		th;
	ulong		stats[ThwStats];
};

typedef struct Uncstate Uncstate;
struct Uncstate
{
	QLock		ackl;			/* lock for acks sent back to compressor */
	int		doack;			/* send an ack? */
	int		badpacks;		/* bad packets seen in a row */
	ulong		ackseq;			/* packets to ack */
	int		ackmask;

	int		active;			/* 0 => waiting for resetack */
	int		resetid;		/* id of most recent reset */
	Unthwack	ut;
};

enum
{
	ThwAcked	= 1UL << 23,
	ThwCompMask	= 3UL << 21,
	ThwCompressed	= 0UL << 21,
	ThwUncomp	= 1UL << 21,
	ThwUncompAdd	= 2UL << 21,		/* uncompressed, but add to decompression buffer */
	ThwSeqMask	= 0x0fffff,
	ThwSmallPack	= 96,
};

static	void		*compinit(PPP*);
static	Block*		comp(PPP*, ushort, Block*, int*);
static	Block		*compresetreq(void*, Block*);
static	void		compcompack(void*, Block*);
static	void		compfini(void*);

static	void		*uncinit(PPP*);
static	Block*		uncomp(PPP*, Block*, int *protop, Block**);
static	void		uncfini(void*);
static	void		uncresetack(void*, Block*);

Comptype cthwack = {
	compinit,
	comp,
	compresetreq,
	compfini
};

Uncomptype uncthwack = {
	uncinit,
	uncomp,
	uncresetack,
	uncfini
};

static void *
compinit(PPP *)
{
	Cstate *cs;

	cs = mallocz(sizeof(Cstate), 1);
	thwackinit(&cs->th);
	return cs;
}

static void
compfini(void *as)
{
	Cstate *cs;

	cs = as;
	thwackcleanup(&cs->th);
	free(cs);
}


static Block *
compresetreq(void *as, Block *b)
{
	Cstate *cs;
	Lcpmsg *m;
	int id;

	cs = as;
	m = (Lcpmsg*)b->rptr;
	id = m->id;

	thwackinit(&cs->th);

	freeb(b);

	netlog("thwack resetreq id=%d \n", id);

	b = alloclcp(Lresetack, id, 4, &m);
	hnputs(m->len, 4);

	return b;
}

static Block*
comp(PPP *ppp, ushort proto, Block *b, int *protop)
{
	Uncstate *uncs;
	Cstate *cs;
	Block *bb;
	ulong seq, acked;
	int n, nn, mustadd;

	cs = ppp->cstate;
	*protop = 0;

	/* put ack and protocol into b */
	n = BLEN(b);
	if(b->rptr - (2+4) < b->base)
		sysfatal("thwack: not enough header in block");
	acked = 0;
	if(ppp->unctype == &uncthwack){
		uncs = ppp->uncstate;
		qlock(&uncs->ackl);
		if(uncs->doack){
			uncs->doack = 0;
			b->rptr -= 4;
			b->rptr[0] = uncs->ackseq >> 16;
			b->rptr[1] = uncs->ackseq >> 8;
			b->rptr[2] = uncs->ackseq;
			b->rptr[3] = uncs->ackmask;
			acked = ThwAcked;
		}
		qunlock(&uncs->ackl);
	}
	if(proto > 0xff){
		b->rptr -= 2;
		b->rptr[0] = proto >> 8;
		b->rptr[1] = proto;
	}else{
		b->rptr--;
		b->rptr[0] = proto;
	}

	bb = allocb(BLEN(b) + 3);

	seq = cs->seq;
	if(n <= 3){
		mustadd = 0;
		nn = -1;
	}else{
		mustadd = n < ThwSmallPack;
		nn = thwack(&cs->th, mustadd, bb->wptr + 3, n - 3, b, seq, cs->stats);
	}
	if(nn < 0 && !mustadd){
		if(!acked || BLEN(b) + 1 > ppp->mtu){
			freeb(bb);
			if(acked)
				b->rptr += 4;
			if(proto > 0xff)
				b->rptr += 2;
			else
				b->rptr++;
			*protop = proto;
			return b;
		}
		bb->wptr[0] = (ThwUncomp | ThwAcked) >> 16;

		memmove(bb->wptr + 1, b->rptr, BLEN(b));

		bb->wptr += BLEN(b) + 1;
		freeb(b);
	}else{
		cs->seq = (seq + 1) & ThwSeqMask;
		if(nn < 0){
			nn = BLEN(b);
			memmove(bb->wptr + 3, b->rptr, nn);
			seq |= ThwUncompAdd;
		}else
			seq |= ThwCompressed;
		seq |= acked;
		bb->wptr[0] = seq>>16;
		bb->wptr[1] = seq>>8;
		bb->wptr[2] = seq;

		bb->wptr += nn + 3;
	}

	*protop = Pcdata;
	return bb;
}

static	void *
uncinit(PPP *)
{
	Uncstate *s;

	s = mallocz(sizeof(Uncstate), 1);

	s->active = 1;

	unthwackinit(&s->ut);

	return s;
}

static	void
uncfini(void *as)
{
	free(as);
}

static	void
uncresetack(void *as, Block *b)
{
	Uncstate *s;
	Lcpmsg *m;

	s = as;
	m = (Lcpmsg*)b->rptr;

	/*
	 * rfc 1962 says we must reset every message
	 * we don't since we may have acked some messages
	 * which the compressor will use in the future.
	 */
	netlog("unthwack resetack id=%d resetid=%d active=%d\n", m->id, s->resetid, s->active);
	if(m->id == (uchar)s->resetid && !s->active){
		s->active = 1;
		unthwackinit(&s->ut);
	}
}

static	Block*
uncomp(PPP *ppp, Block *bb, int *protop, Block **reply)
{
	Lcpmsg *m;
	Cstate *cs;
	Uncstate *uncs;
	Block *b, *r;
	ulong seq, mseq;
	ushort proto;
	uchar mask;
	int n;

	*reply = nil;
	*protop = 0;
	uncs = ppp->uncstate;

	if(BLEN(bb) < 4){
		syslog(0, "ppp", ": thwack: short packet\n");
		freeb(bb);
		return nil;
	}

	if(!uncs->active){
		netlog("unthwack: inactive, killing packet\n");
		freeb(bb);
		r = alloclcp(Lresetreq, uncs->resetid, 4, &m);
		hnputs(m->len, 4);
		*reply = r;
		return nil;
	}

	seq = bb->rptr[0] << 16;
	if((seq & ThwCompMask) == ThwUncomp){
		bb->rptr++;
		b = bb;
	}else{
		seq |= (bb->rptr[1]<<8) | bb->rptr[2];
		bb->rptr += 3;
		if((seq & ThwCompMask) == ThwCompressed){
			b = allocb(ThwMaxBlock);
			n = unthwack(&uncs->ut, b->wptr, ThwMaxBlock, bb->rptr, BLEN(bb), seq & ThwSeqMask);
			freeb(bb);
			if(n < 2){
				syslog(0, "ppp", ": unthwack: short or corrupted packet %d seq=%ld\n", n, seq);
				netlog("unthwack: short or corrupted packet n=%d seq=%ld: %s\n", n, seq, uncs->ut.err);
				freeb(b);

				r = alloclcp(Lresetreq, ++uncs->resetid, 4, &m);
				hnputs(m->len, 4);
				*reply = r;
				uncs->active = 0;
				return nil;
			}
			b->wptr += n;
		}else{
			unthwackadd(&uncs->ut, bb->rptr, BLEN(bb), seq & ThwSeqMask);
			b = bb;
		}

		/*
		 * update ack state
		 */
		mseq = unthwackstate(&uncs->ut, &mask);
		qlock(&uncs->ackl);
		uncs->ackseq = mseq;
		uncs->ackmask = mask;
		uncs->doack = 1;
		qunlock(&uncs->ackl);
	}

	/*
	 * grab the compressed protocol field
	 */
	proto = *b->rptr++;
	if((proto & 1) == 0)
		proto = (proto << 8) | *b->rptr++;
	*protop = proto;

	/*
	 * decode the ack, and forward to compressor
	 */
	if(seq & ThwAcked){
		if(ppp->ctype == &cthwack){
			cs = ppp->cstate;
			mseq = (b->rptr[0]<<16) | (b->rptr[1]<<8) | b->rptr[2];
			mask = b->rptr[3];
			thwackack(&cs->th, mseq, mask);
		}
		b->rptr += 4;
	}
	return b;
}
