#include <u.h>
#include <libc.h>
#include <libg.h>
#include "../system.h"
#ifdef HIRES
#include "../stdio.h"
#include "runlen.h"

#define PUTV(length, value) \
	buf |= (value) << (bufct); \
	bufct += length; \
	while (bufct >= 8) { \
		*chpt++ = buf; \
		buf >>= 8; \
		bufct -= 8; \
		if (chpt >= &t->chbuf[sizeof t->chbuf]) \
			chpt = t6flush(t, chpt); \
	}

#include "ccittout.h"

static ushort *hmcodes[] = { wmout-2, bmout-2 };

static ushort *htcodes[] = { wtout, btout };

static uchar *
t6flush(T6state *t, uchar *addr)
{
	long n = addr - t->chbuf;
	if (n > 0)
		write(t->fd, (char *)t->chbuf, n);
	return (t->chpt = t->chbuf);
}

T6state *
t6winit(T6state *t, int fd)
{
	t->fd = fd;
	t->bufct = 0;
	t->buf = 0;
	t->nch = 0;
	t->chpt = t->chbuf;
	return t;
}

void
t6write(T6state *t, Scan *code, Scan *ref)
{
	int a0=0, dx, color=0, pass=0, i;
	short *ib1=ref->run;
	short *ia1=code->run;
	int bufct = t->bufct;
	unsigned long buf = t->buf;
	uchar *chpt = t->chpt; ushort *src;

	for (;;) {
		if (ib1 < ref->runN && *(ib1+1) < *ia1) {
			if (pass++)
				goto Horizontal;
/*fprint(2,"P ");*/
			PUTV(4, 0x08);			/* pass mode */
			a0 = *(ib1+1), ib1 += 2;
		} else if ((dx = *ia1 - *ib1) >= -3 && dx <= 3) {
			src = &vout[(dx+3)*2];
/*fprint(2,"V%d ",dx);*/
			PUTV(src[0], src[1]);		/* vertical mode */
			pass=0;
			a0 = *ia1++, color^=1;
			if (dx >= 0)
				++ib1;
			else if (--ib1 < ref->run)
				ib1 += 2;
		} else {				/* horizontal mode */
	Horizontal:
/*fprint(2,"H ");*/
			PUTV(3, 0x04);
			for (i=2,dx= *ia1-a0; i>0; i--,dx=(ia1<code->runN)?*(ia1+1)-*ia1:0) {
				for (; dx >= 2560; dx -= 2560) {
/*fprint(2,"2560+");*/
					PUTV(12, 0xf80);
				}
				if (dx >= 64) {
					src = hmcodes[color] + ((dx>>5)&0x7e);
/*fprint(2,"%d+",64*(dx/64));*/
					PUTV(src[0], src[1]);
					dx &= 0x3f;
				}
				src = htcodes[color] + (dx<<1);
/*fprint(2,"%d ",dx);*/
				PUTV(src[0], src[1]);
				color ^= 1;
			}
			pass=0;
			if (dx == 0)
				break;
			a0 = *(ia1+1), ia1 += 2;
		}
/*fprint(2,"a0=%d\n", a0);*/
		if (ia1 > code->runN)
			break;
		if (ib1 > ref->runN)
			ib1 = ref->runN;
		else
			while (*ib1 <= a0) {		/* find next ref */
				if ((ib1 += 2) > ref->runN) {
					ib1 = ref->runN;
					break;
				}
			}
	}
	t->bufct = bufct;
	t->buf = buf;
	t->chpt = chpt;
}

void
t6wclose(T6state *t)
{
	if (t->bufct)
		*t->chpt++ = t->buf;
	t6flush(t, t->chpt);
}
#endif
