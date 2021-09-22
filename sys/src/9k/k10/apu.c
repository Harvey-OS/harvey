/*
 * PC Engines's APU2 support
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "io.h"

enum {						/* amd family 16h proc funcs */
	Dramcfglow	= 0x90,			/* f2 pci cfg reg */
	Dimmeccen	= 1<<19,

	Nbcap		= 0xe8,			/* f3 pci cfg reg */
	Ecccap		= 1<<3,
	Chipkillcap	= 1<<4,

	Nbcfg1		= 0x88,			/* f3 pci cfg reg */
	Disdramscrub	= 1<<27,

	NBarraddr	= 0xb8,			/* f3 pci cfg reg */
	Rsrvd		= ~0xf000003f,		/* bits 10-27 reserved */

	Eccexclbaselow	= 0x240,		/* f5 pci cfg reg */
	Eccexclen	= 1<<0,
};

/*
 * modified strscan & strsearch from mp.c
 */

static void*
strscan(uchar* address, int length, char* signature)
{
	uchar *e, *p;
	int siglength, first;

	e = address+length;
	siglength = strlen(signature);
	first = *signature;
	for(p = address; p+siglength < e; p++) {
		if (*p != first) {
			p = memchr(p, first, e - p);
			if (p == nil)
				return p;
		}
		if(memcmp(p, signature, siglength) == 0)
			return p;
	}
	return nil;
}

uintptr	freebasemem(void);

void*
strsearch(char* signature)
{
	uintptr p;
	void *r;

	/*
	 * Search for the data structure:
	 * 1) within the first KiB of the Extended BIOS Data Area (EBDA), or
	 * 2) within the last KiB of system base memory if the EBDA segment
	 *    is undefined, or
	 * 3) within the BIOS ROM address space between 0xf0000 and 0xfffff
	 *    (but will actually check 0xe0000 to 0xfffff).
	 */
	if((p = (uintptr)BIOSSEG(L16GET((uchar *)EBDAADDR))) != 0)
		if((r = strscan((void *)p, LOWMEMEND - p, signature)) != nil)
			return r;

	p = freebasemem();
	if(p == 0)
		/* hack for virtualbox: look in last KiB below 0xa0000 */
		p = LOWMEMEND;
	else
		p += KZERO;
	if((r = strscan((uchar *)p - KB, KB, signature)) != nil)
		return r;

	r = strscan(BIOSSEG(0xe000), 0x20000, signature);
	if (r)
		return r;

	/* desperate last attempt for vbox; found at 0x9fc00 = 654336 */
	r = strscan(KADDR(0), MB, signature);
	if (r) {
		iprint("strsearch's last resort match at %#p\n", r);
		vmbotch(Virtualbox, "strsearch's last resort match");
	}
	return r;
}

int
isapu(void)
{
	int apu;

	apu = strsearch("PC Enginapu2") != nil;	/* works as of bios v4.11 */
	if (!apu)
		apu = strsearch("PCEngines ap") != nil; /* for bios v2 */
	return apu;
}

/*
 * derived from coreboot code from
 * github.com/pcengines/coreboot/pull/207/files
 * see also AMD's BKDG for Family 16h (Models 30h-3fh).
 *
 * 0.24.3:	brg  06.00.00 1022/1583   0
 *	AMD Family 16h (Models 30h-3fh) Processor Function 3
 * 0.24.5:	brg  06.00.00 1022/1585   0
 *	AMD Family 16h (Models 30h-3fh) Processor Function 5
 */
void
apueccon(void)
{
	ulong scrubbits, rsrvbits, exclbits;
	Pcidev *f2, *f3, *f5;

	f2 = pcimatch(nil, Vamd, 0x1582); /* Family 16h Processor Function 2 */
	f3 = pcimatch(nil, Vamd, 0x1583); /* Family 16h Processor Function 3 */
	f5 = pcimatch(nil, Vamd, 0x1585); /* Family 16h Processor Function 5 */
	if(f2 == nil || f3 == nil || f5 == nil)
		return;				/* not an apu2 */

	scrubbits = pcicfgr32(f3, Nbcfg1);
	rsrvbits = pcicfgr32(f3, NBarraddr);
	exclbits = pcicfgr32(f5, Eccexclbaselow);
	if (scrubbits == ~0 && rsrvbits == ~0 && exclbits == ~0) {
		print("can't read amd ecc pci-e config space registers\n");
		return;
	}

	print("apu2 dram dimm ecc: %s, %s, %s; ",
		pcicfgr32(f3, Nbcap) & Ecccap? "capable": "incapable",
		pcicfgr32(f2, Dramcfglow) & Dimmeccen? "enabled": "disabled",
		pcicfgr32(f3, Nbcap) & Chipkillcap?
			"can chipkill": "can't chipkill");
	if (!(scrubbits & Disdramscrub) && (rsrvbits & Rsrvd) == 0 &&
	    !(exclbits & Eccexclen)) {
		print("already on\n");
		return;
	}

	print("was off because ");
	if (scrubbits & Disdramscrub)
		print("ecc scrub was off, ");
	if (exclbits & Eccexclen)
		print("ecc exclusion was on, ");
	if (rsrvbits & Rsrvd)
		print("reserved bits set in %#lux", rsrvbits & Rsrvd);
	print("\n\t");

	/* dram ecc scrubbing off */
	pcisetcfgbit(f3, Nbcfg1, Disdramscrub, "scrubbing was already off; ");

	/* clear reserved bits */
	pciclrcfgbit(f3, NBarraddr, Rsrvd, "");

	/* turn ecc exclusion range off */
	pciclrcfgbit(f5, Eccexclbaselow, Eccexclen, "");

	/* dram ecc scrubbing back on */
	pciclrcfgbit(f3, Nbcfg1, Disdramscrub, "scrubbing already on; ");

	print("ecc now enabled\n");
}
