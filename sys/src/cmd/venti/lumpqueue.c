#include "stdinc.h"
#include "dat.h"
#include "fns.h"

typedef struct LumpQueue	LumpQueue;
typedef struct WLump		WLump;

enum
{
	MaxLumpQ	= 1 << 3	/* max. lumps on a single write queue, must be pow 2 */
};

struct WLump
{
	Lump	*u;
	Packet	*p;
	int	creator;
};

struct LumpQueue
{
	VtLock		*lock;
	VtRendez	*full;
	VtRendez	*empty;
	WLump		q[MaxLumpQ];
	int		w;
	int		r;
};

static LumpQueue	*lumpqs;
static int		nqs;

static void	doQueue(void *vq);

int
initLumpQueues(int nq)
{
	LumpQueue *q;

	int i;
	nqs = nq;

	lumpqs = MKNZ(LumpQueue, nq);

	for(i = 0; i < nq; i++){
		q = &lumpqs[i];
		q->lock = vtLockAlloc();
		q->full = vtRendezAlloc(q->lock);
		q->empty = vtRendezAlloc(q->lock);

		if(vtThread(doQueue, q) < 0){
			setErr(EOk, "can't start write queue slave: %R");
			return 0;
		}
	}

	return 1;
}

/*
 * queue a lump & it's packet data for writing
 */
int
queueWrite(Lump *u, Packet *p, int creator)
{
	LumpQueue *q;
	int i;

	i = indexSect(mainIndex, u->score);
	if(i < 0 || i >= nqs){
		setErr(EBug, "internal error: illegal index section in queueWrite");
		return 0;
	}

	q = &lumpqs[i];

	vtLock(q->lock);
	while(q->r == ((q->w + 1) & (MaxLumpQ - 1)))
		vtSleep(q->full);

	q->q[q->w].u = u;
	q->q[q->w].p = p;
	q->q[q->w].creator = creator;
	q->w = (q->w + 1) & (MaxLumpQ - 1);

	vtWakeup(q->empty);

	vtUnlock(q->lock);

	return 1;
}

static void
doQueue(void *vq)
{
	LumpQueue *q;
	Lump *u;
	Packet *p;
	int creator;

	q = vq;
	for(;;){
		vtLock(q->lock);
		while(q->w == q->r)
			vtSleep(q->empty);
		u = q->q[q->r].u;
		p = q->q[q->r].p;
		creator = q->q[q->r].creator;
		q->r = (q->r + 1) & (MaxLumpQ - 1);

		vtWakeup(q->full);

		vtUnlock(q->lock);

		if(!writeQLump(u, p, creator))
			fprint(2, "failed to write lump for %V: %R", u->score);

		putLump(u);
	}
}
