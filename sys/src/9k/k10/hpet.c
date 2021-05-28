#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

typedef struct Hpet Hpet;
typedef struct Tn Tn;

struct Hpet {					/* Event Timer Block */
	u32int	cap;				/* General Capabilities */
	u32int	period;				/* Main Counter Tick Period */
	u32int	_8_[2];
	u32int	cnf;				/* General Configuration */
	u32int	_20_[3];
	u32int	sts;				/* General Interrupt Status */
	u32int	_36_[51];
	union {					/* Main Counter Value */
		u32int	u32[2];
		u64int	u64;
	} counter;
	u32int	_248[2];
	Tn	tn[];				/* Timers */
};

struct Tn {					/* Timer */
	u32int	cnf;				/* Configuration */
	u32int	cap;				/* Capabilities */
	union {					/* Comparator */
		u32int	u32[2];
		u64int	u64;
	} comparator;
	u32int	val;				/* FSB Interrupt Value */
	u32int	addr;				/* FSB Interrupt Address */
	u32int	_24_[2];
};

static Hpet* etb[8];				/* Event Timer Blocks */

void
hpetinit(int seqno, uintptr pa, int minticks)
{
	Tn *tn;
	int i, n;
	Hpet *hpet;
	u64int val;

	DBG("hpet: seqno %d pa %#p minticks %d\n", seqno, pa, minticks);
	if(seqno < 0 || seqno > nelem(etb) || (hpet = vmap(pa, 1024)) == nil)
		return;
	etb[seqno] = hpet;

	DBG("HPET: cap %#8.8ux period %#8.8ux\n", hpet->cap, hpet->period);
	DBG("HPET: cnf %#8.8ux sts %#8.8ux\n",hpet->cnf, hpet->sts);
	DBG("HPET: counter %#16.16llux\n",
		(((u64int)hpet->counter.u32[1])<<32)|hpet->counter.u32[0]);

	n = ((hpet->cap>>8) & 0x0F) + 1;
	for(i = 0; i < n; i++){
		tn = &hpet->tn[i];
		DBG("Tn%d: cnf %#8.8ux cap %#8.8ux\n", i, tn->cnf, tn->cap);
		DBG("Tn%d: comparator %#16.16llux\n", i,
			(((u64int)tn->comparator.u32[1])<<32)|tn->comparator.u32[0]);
		DBG("Tn%d: val %#8.8ux addr %#8.8ux\n", i, tn->val, tn->addr);
	}

	/*
	 * hpet->period is the number of femtoseconds per counter tick.
	 */
}
