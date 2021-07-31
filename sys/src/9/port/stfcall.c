#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

enum
{
	Twritehdr	= 16,	/* Min bytes for Twrite */
	Rreadhdr	= 8,	/* Min bytes for Rread */
	Twritecnt	= 13,	/* Offset in byte stream of write count */
	Rreadcnt	= 5,	/* Offset for Readcnt */
};

static void fcalliput(Queue*, Block*);
static void fcalloput(Queue*, Block*);
static void fcallopen(Queue*, Stream*);
static void fcallclose(Queue*);
static void fcallreset(void);
Qinfo fcallinfo = { fcalliput, fcalloput, fcallopen, fcallclose, "fcall", fcallreset };

static uchar msglen[256] =
{
	[Tnop]		3,
	[Rnop]		3,
	[Tsession]	3,
	[Rsession]	3,
	[Terror]	0,
	[Rerror]	67,
	[Tflush]	5,
	[Rflush]	3,
	[Tattach]	89,
	[Rattach]	13,
	[Tclone]	7,
	[Rclone]	5,
	[Twalk]		33,
	[Rwalk]		13,
	[Topen]		6,
	[Ropen]		13,
	[Tcreate]	38,
	[Rcreate]	13,
	[Tread]		15,
	[Rread]		8,
	[Twrite]	16,
	[Rwrite]	7,
	[Tclunk]	5,
	[Rclunk]	5,
	[Tremove]	5,
	[Rremove]	5,
	[Tstat]		5,
	[Rstat]		121,
	[Twstat]	121,
	[Rwstat]	5,
	[Tclwalk]	35,
	[Rclwalk]	13,
	[Tauth]		69,
	[Rauth]		35,
};

static void
fcallreset(void)
{
}

static void
fcallopen(Queue *q, Stream *s)
{
	USED(q, s);
}

static void
fcallclose(Queue * q)
{
	USED(q);
}

void
fcalloput(Queue *q, Block *bp)
{
	PUTNEXT(q, bp);
}

void
upstream(Queue *q, ulong len)
{
	Block *bl, **tail, *bp;
	ulong l;

	tail = &bl;
	while(len) {
		l = BLEN(q->first);
		if(l > len)
			break;
		bp = getq(q);			/* Consume all of block */
		*tail = bp;
		tail = &bp->next;
		len -= l;
	}
	if(len) {				/* Consume partial block */
		lock(q);
		*tail = copyb(q->first, len);
		q->first->rptr += len;
		q->len -= len;
		unlock(q);
	}
	for(bp = bl; bp->next; bp = bp->next)
		;
	bp->flags |= S_DELIM;
	PUTNEXT(q, bl);
}

static void
fcalliput(Queue *q, Block *bp)
{
	ulong len, need, off;

	if(bp->type != M_DATA) {
		PUTNEXT(q, bp);
		return;
	}
	if(BLEN(bp) == 0) {
		freeb(bp);
		return;
	}

	/* Stash the data */
	bp->flags &= ~S_DELIM;
	putq(q, bp);

	for(;;) {
		bp = q->first;
		if(bp == 0)
			return;
		switch(bp->rptr[0]) {		/* This is the type */
		default:
			len = msglen[bp->rptr[0]];
			if(len == 0){
				bp = allocb(0);
				bp->type = M_HANGUP;
				PUTNEXT(q, bp);
				return;
			}
			if(q->len < len)
				return;
	
			upstream(q, len);
			continue;

		case Twrite:			/* Fmt: TGGFFOOOOOOOOCC */
			len = Twritehdr;	/* T = type, G = tag, F = fid */
			off = Twritecnt;	/* O = offset, C = count */
			break;

		case Rread:			/* Fmt: TGGFFCC */
			len = Rreadhdr;
			off = Rreadcnt;
			break;
		}
	
		if(q->len < len)
			return;
	
		pullup(q->first, len);
		bp = q->first;
		need = len+bp->rptr[off]+(bp->rptr[off+1]<<8);
		if(q->len < need)
			return;
	
		upstream(q, need);
	}
}
