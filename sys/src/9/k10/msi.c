/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "apic.h"

enum {
	Dpcicap		= 1<<0,
	Dmsicap		= 1<<1,
	Dvec		= 1<<2,
	Debug		= 0,
};

enum {
	/* address */
	Msiabase		= 0xfee00000u,
	Msiadest		= 1<<12,		/* same as 63:56 of apic vector */
	Msiaedest	= 1<<4,		/* same as 55:48 of apic vector */
	Msialowpri	= 1<<3,		/* redirection hint */
	Msialogical	= 1<<2,

	/* data */
	Msidlevel	= 1<<15,
	Msidassert	= 1<<14,
	Msidlogical	= 1<<11,
	Msidmode	= 1<<8,		/* 3 bits; delivery mode */
	Msidvector	= 0xff<<0,
};

enum{
	/* msi capabilities */
	Vmask		= 1<<8,
	Cap64		= 1<<7,
	Mmesgmsk	= 7<<4,
	Mmcap		= 7<<1,
	Msienable	= 1<<0,
};

static int
msicap(Pcidev *p)
{
	int c;

	c = pcicap(p, PciCapMSI);
	if(c == -1)
		return 0;
	return c;
}

static int
blacklist(Pcidev *p)
{
	switch(p->vid<<16 | p->did){
	case 0x11ab<<16 | 0x6485:
		return -1;
	}
	return 0;
}

int
pcimsienable(Pcidev *p, uvlong vec)
{
	char *s;
	uint c, f, d, datao, lopri, dmode, logical;

	c = msicap(p);
	if(c == 0)
		return -1;

	f = pcicfgr16(p, c + 2) & ~Mmesgmsk;

	if(blacklist(p) != 0)
		return -1;
	datao = 8;
	d = vec>>48;
	lopri = (vec & 0x700) == MTlp;
	logical = (vec & Lm) != 0;
	pcicfgw32(p, c + 4, Msiabase | Msiaedest * d
		| Msialowpri * lopri | Msialogical * logical);
	if(f & Cap64){
		datao += 4;
		pcicfgw32(p, c + 8, 0);
	}
	dmode = (vec >> 8) & 7;
	pcicfgw16(p, c + datao, Msidassert | Msidlogical * logical
		| Msidmode * dmode | (uint)vec & 0xff);
	if(f & Vmask)
		pcicfgw32(p, c + datao + 4, 0);

	/* leave vectors configured but disabled for debugging */
	if((s = getconf("*nomsi")) != nil && atoi(s) != 0)
		return -1;

	pcicfgw16(p, c + 2, f);
	return 0;
}

int
pcimsimask(Pcidev *p, int mask)
{
	uint c, f;

	c = msicap(p);
	if(c == 0)
		return -1;
	f = pcicfgr16(p, c + 2) & ~Msienable;
	if(mask){
		pcicfgw16(p, c + 2, f & ~Msienable);
//		pciclrbme(p);		cheeze
	}else{
		pcisetbme(p);
		pcicfgw16(p, c + 2, f | Msienable);
	}
	return 0;
}
