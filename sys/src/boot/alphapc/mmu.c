#include	"u.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"lib.h"

static int	debug;

static uvlong	by2pg;		/* hwrpb->by2pg */
static uvlong	pte2pg;		/* by2pg/8 */
static uvlong	pgmask;		/* by2pg-1 */
static uvlong	ptemask;		/* pte2pg-1 */
static uvlong	pgshift;		/* log2(by2pg) */
static uvlong	pteshift;		/* log2(pte2pg) = pgshift - 3 */

#define	L1(va)	(((uvlong)(va)>>3*pteshift+3) & (pte2pg-1))
#define	L2(va)	(((uvlong)(va)>>2*pteshift+3) & (pte2pg-1))
#define	L3(va)	(((uvlong)(va)>>pgshift) & (pte2pg-1))
#define	OFF(va)	(((uvlong)(va)) & (by2pg-1))

#define	V1(l1)	(((vlong)l1<<(64-pteshift)) >> 64-(3*pteshift+3))
#define	V2(l2)	(((uvlong)l2<<(64-pteshift)) >> 64-(2*pteshift+3))
#define	V3(l3)	(((uvlong)l3<<(64-pteshift)) >> 64-pgshift)
#define	VA(l1, l2, l3, off)	(V1(l1) | V2(l2) | V3(l3) | (off))

static int
log2(uvlong x)
{
	int i;

	if ((x & (x-1)) == 0)
		for (i = 0; i < 64; i++)
			if (x & (1<<i))
				return i;
	panic("log2: %llux", x);
	return -1;
}

void
mmuinit(void)
{
	int i;
	uvlong npage, nlvl2, nlvl3;
	uvlong l1p, l2p, lvl2, lvl3;
	extern ulong _main[], edata[];

	/* map entire physical memory at KZERO */
	by2pg = hwrpb->by2pg;
	pte2pg = (by2pg >> 3);
	pgmask = by2pg-1;
	ptemask = pte2pg-1;
	pgshift = log2(by2pg);
	pteshift = pgshift-3;

	l1p = (1LL<<3*pteshift+3)|(1LL<<2*pteshift+3)|(1LL<<pgshift);
	if (rdv(l1p+8*(pte2pg-1)) != 0 || rdv(l1p+8*(pte2pg-2)) != 0)
		panic("KZERO lvl1 already mapped");

	npage = (conf.maxphys+pgmask)>>pgshift;
	nlvl3 = (npage+ptemask)>>pteshift;
	nlvl2 = (nlvl3+ptemask)>>pteshift;
	if (nlvl2 > 1)
		panic("meminit: nlvl2");	/* cannot happen, due to virtual space limitation */
	if (debug)
		print("nlvl1 %llud nlvl2 %llud nlvl3 %llud npage %llud\n", 1LL, nlvl2, nlvl3, npage);

	lvl2 = allocate(nlvl2+nlvl3);
	lvl3 = lvl2 + nlvl2*by2pg;

	wrv(l1p+8*(pte2pg-2), rdv(l1p+8)|PTEASM);
	wrv(l1p+8*(pte2pg-1), (lvl2<<(32-pgshift)) | PTEKVALID | PTEASM);

	l2p = (1LL<<3*pteshift+3)|(1LL<<2*pteshift+3)|((vlong)KZERO >> 2*pteshift)&((1LL<<2*pteshift+3)-1);
	for (i = 0; i < nlvl3; i++)
		stqp(lvl2+(l2p&(by2pg-1))+8*i, ((lvl3+i*by2pg)<<(32-pgshift)) | PTEKVALID | PTEASM);

	for (i = 0; i < npage; i++)
		stqp(lvl3+8*i, ((uvlong)i<<32) | PTEKVALID | PTEASM);

	tlbflush();

	if (debug)
		print("\n");
}

uvlong
paddr(uvlong va)
{
	uvlong ptbr, x, pte;

	ptbr = getptbr();
	pte = ldqp((ptbr<<pgshift)+8*L1(va));
	if ((pte&PTEKVALID) != PTEKVALID)
		return 0;
	x = ((pte>>32)<<pgshift);
	pte = ldqp(x+8*L2(va));
	if ((pte&PTEKVALID) != PTEKVALID)
		return 0;
	x = ((pte>>32)<<pgshift);
	pte = ldqp(x+8*L3(va));
	if ((pte&PTEKVALID) != PTEKVALID)
		return 0;
	x = ((pte>>32)<<pgshift);
	return x;
}

uvlong
pground(uvlong x)
{
	return (x+pgmask) & ~pgmask;
}
