/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <ip.h>
#include <auth.h>
#include "ppp.h"

static	uint16_t	endian	= 1;
static	uint8_t*	aendian	= (uint8_t*)&endian;
#define	LITTLE	*aendian

uint16_t
ptclbsum(uint8_t *addr, int len)
{
	uint32_t losum, hisum, mdsum, x;
	uint32_t t1, t2;

	losum = 0;
	hisum = 0;
	mdsum = 0;

	x = 0;
	if((uintptr)addr & 1) {
		if(len) {
			hisum += addr[0];
			len--;
			addr++;
		}
		x = 1;
	}
	while(len >= 16) {
		t1 = *(uint16_t*)(addr+0);
		t2 = *(uint16_t*)(addr+2);	mdsum += t1;
		t1 = *(uint16_t*)(addr+4);	mdsum += t2;
		t2 = *(uint16_t*)(addr+6);	mdsum += t1;
		t1 = *(uint16_t*)(addr+8);	mdsum += t2;
		t2 = *(uint16_t*)(addr+10);	mdsum += t1;
		t1 = *(uint16_t*)(addr+12);	mdsum += t2;
		t2 = *(uint16_t*)(addr+14);	mdsum += t1;
		mdsum += t2;
		len -= 16;
		addr += 16;
	}
	while(len >= 2) {
		mdsum += *(uint16_t*)addr;
		len -= 2;
		addr += 2;
	}
	if(x) {
		if(len)
			losum += addr[0];
		if(LITTLE)
			losum += mdsum;
		else
			hisum += mdsum;
	} else {
		if(len)
			hisum += addr[0];
		if(LITTLE)
			hisum += mdsum;
		else
			losum += mdsum;
	}

	losum += hisum >> 8;
	losum += (hisum & 0xff) << 8;
	while(hisum = losum>>16)
		losum = hisum + (losum & 0xffff);

	return losum & 0xffff;
}

uint16_t
ptclcsum(Block *bp, int offset, int len)
{
	uint8_t *addr;
	uint32_t losum, hisum;
	uint16_t csum;
	int odd, blen, x;

	/* Correct to front of data area */
	while(bp != nil && offset && offset >= BLEN(bp)) {
		offset -= BLEN(bp);
		bp = bp->next;
	}
	if(bp == nil)
		return 0;

	addr = bp->rptr + offset;
	blen = BLEN(bp) - offset;

	if(bp->next == nil) {
		if(blen < len)
			len = blen;
		return ~ptclbsum(addr, len) & 0xffff;
	}

	losum = 0;
	hisum = 0;

	odd = 0;
	while(len) {
		x = blen;
		if(len < x)
			x = len;

		csum = ptclbsum(addr, x);
		if(odd)
			hisum += csum;
		else
			losum += csum;
		odd = (odd+x) & 1;
		len -= x;

		bp = bp->next;
		if(bp == nil)
			break;
		blen = BLEN(bp);
		addr = bp->rptr;
	}

	losum += hisum>>8;
	losum += (hisum&0xff)<<8;
	while((csum = losum>>16) != 0)
		losum = csum + (losum & 0xffff);

	return ~losum & 0xffff;
}

uint16_t
ipcsum(uint8_t *addr)
{
	int len;
	uint32_t sum;

	sum = 0;
	len = (addr[0]&0xf)<<2;

	while(len > 0) {
		sum += (addr[0]<<8) | addr[1] ;
		len -= 2;
		addr += 2;
	}

	sum = (sum & 0xffff) + (sum >> 16);
	sum = (sum & 0xffff) + (sum >> 16);

	return (sum^0xffff);
}
