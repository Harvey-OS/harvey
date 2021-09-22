#ifdef EXPAND
/* expand doesn't run in high KZERO space */
#undef	PADDR
#define	PADDR(v) ((uintptr)(v) & ~KSEGM)
#undef	KZERO
#define	KZERO 0
#else
#include	"u.h"
#include	"mem.h"
#include	"../port/lib.h"
#include	"dat.h"
#include	"fns.h"

int v_flag;
#endif
#include "../pc/multiboot.h"

Mbi	mbhdr;			/* dummy in case warp64 is called directly */
int	nmmap;

/* these need to be copied to low memory */
Mbi	*multibootheader = &mbhdr;  /* `multibootheader' name known to l64p.s */
MMap	mmap[Maxmmap+1];	/* has to fit at 0x500 before 0x1200 */

void
mkmultiboot(void)
{
	MMap *lmmap;

#ifdef EXPAND
	multibootheader = (Mbi *)PADDR(MBOOTTAB);
#else
	multibootheader = (Mbi *)MBOOTTAB;
#endif
	memset(multibootheader, 0, sizeof *multibootheader);

	lmmap = (MMap *)(multibootheader + 1);
	memmove(lmmap, mmap, sizeof mmap);

	multibootheader->cmdline = PADDR(BOOTLINE);
	multibootheader->flags |= Fcmdline;
	if(nmmap != 0){
		multibootheader->mmapaddr = PADDR(lmmap);
		multibootheader->mmaplength = nmmap*sizeof(MMap);
		multibootheader->flags |= Fmmap;
	}

	multibootheader = (Mbi *)PADDR(multibootheader);
	if(v_flag)
		print("PADDR(&multibootheader) %#p\n", multibootheader);
}
