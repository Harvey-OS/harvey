#include <u.h>
#include <libc.h>
#include <stdio.h>
#include "../system.h"
#ifdef HIRES
#include "../njerq.h"
#include "runlen.h"

#include "runtab.h"
extern unsigned char bytemap[];
Scan *
runlen(unsigned char *src, int width, Scan *sp)
{
	int dx, c, out, wid, parity, right, total;
	unsigned char *cpt;
	short *rp;

	wid = (width + 7)/8;
	if (right = width % 8)
		right = 8 - right;
	rp = sp->run;
	out = 0; parity = 0; total = 0;
	while (--wid >= 0) {
#ifdef VAX
		if ((c = bytemap[*src++&0377]) & 0x80) {
#else
		if ((c = *src++) & 0x80) {
#endif
			cpt = runtab + runs[0xff - c];
			if (parity)
				out += *cpt++;
		} else {
			cpt = runtab + runs[c];
			if (parity == 0)
				out += *cpt++;
		}
		while (*cpt) {
			total += out;
			*rp++ = total;
			out = *cpt++;
			parity ^= 1;
		}
	}
	if (out -= right)
		*rp++ = total+out;
	c = rp - sp->run - 1;
	sp->runN = sp->run + c;
	return sp;
}
#endif
