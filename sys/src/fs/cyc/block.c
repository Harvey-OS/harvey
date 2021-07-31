#include	<u.h>
#include	"comm.h"
#include	"all.h"
#include	"fns.h"
#include	"io.h"

/*
 * copy vme to fiber.
 */

void
vmexmit(ulong a, long count)
{
	ulong ha, ea;
	int c;

	ha = (a & 0x0fffffff) | 0x40000000;
	while(count > 0) {
		while(FIFOXMTBLK)
			runsql();

		c = count;
		if(c >= CHUNK) {
			c = ha & (ALIGNQM-1);
			if(c == 0) {
				bmovevf(ha, &(SQL->txfifo));
				count -= CHUNK;
				ha += CHUNK*HPP;
				runsql();
				continue;
			}
			c = (ALIGNQM - c)/HPP;
		}
		ea = ha + c*HPP;
		while(ha < ea) {
			XMIT(*(ulong*)ha);
			ha += HPP;
		}
		runsql();
		count -= c;
	}
}

/*
 * copy fiber to vme using
 * vmeblk parameters.
 * pseudo dma.
 */
void
recvvme(void)
{
	ulong a, ha, cnt, u;

	a = b.vmeaddr;
	ha = (a & 0x0fffffff) | 0x40000000;

	u = ha;
	cnt = b.vmecount;		/* Word count */

	while(cnt) {
		if((ha & (ALIGNQM-1)) == 0) {
			if(cnt >= CHUNK) {
				if(FIFORCVBLK)
					goto empty;

				bmovefv(&(SQL->rxfifo), ha);
				ha += CHUNK*HPP;
				cnt -= CHUNK;
				continue;
			}
		}
		if(FIFOE)
			goto empty;

		RECV(*(ulong*)ha);
		ha += HPP;
		cnt--;
	}

	u = b.vmeblk.haddr;
	if(b.vmeblk.type) {
		vmestore(u + 1*HPP, b.vmeblk.u);
		vmestore(u + 5*HPP, b.vmeblk.s);
		vmestore(u + 4*HPP, b.vmeblk.n);
	} else {
		vmestore(u + 3*HPP, b.vmeblk.s);
		vmestore(u + 4*HPP, b.vmeblk.n);
	}

	/*
	 * BOTCH, dont take last slot
	 */
	COMM->replyq[COMM->replyp] = swapl(u);
	COMM->replyp++;
	if(COMM->replyp >= NRQ)
		COMM->replyp = 0;
	irq5(COMM->vmevec);
	b.vmeaddr = 0;
	return;

empty:
	b.vmeaddr += ha-u;
	b.vmecount = cnt;
}
