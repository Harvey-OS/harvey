#define RBUFSIZE	(4*1024)	/* raw buffer size */

#include "../port/portdat.h"

typedef struct KMap	KMap;
typedef struct Mconf	Mconf;

struct Mconf
{
	ulong	monitor;	/* graphics monitor id; 0 for none */
	int	usec_delay;	/* count for delay loop */
	char	ss2;		/* is a sparcstation 2 */
	char	ss2cachebug;	/* has sparcstation2 cache bug */
	int	ncontext;	/* in mmu */
	int	npmeg;
	int	vacsize;	/* size of virtual address cache, in bytes */
	int	vaclinesize;	/* size of cache line */

	ulong	npage0;		/* total physical pages of memory, bank 0 */
	ulong	npage1;		/* total physical pages of memory, bank 1 */
	ulong	base0;		/* base of bank 0 */
	ulong	base1;		/* base of bank 1 */
	ulong	npage;		/* total physical pages of memory */

	Lock	iallock;	/* ialloc arena */
	ulong	vfract;		/* fractionally allocated page below vbase */
	ulong	vbase;		/* start of free virtual memory */
	ulong	vlimit;		/* end of free virtual memory */
};

extern	Mconf	mconf;

struct KMap
{
	KMap	*next;
	ulong	pa;
	ulong	va;
};

extern	Mach	mach0;

typedef	struct	Lance	Lance;
typedef	struct	Lanmem	Lanmem;

/*
 * LANCE CSR3 (bus control bits)
 */
#define BSWP	0x4
#define ACON	0x2
#define BCON	0x1

/*
 *  system dependent lance stuff
 *  filled by lancesetup() 
 */
struct	Lance
{
	uchar	ea[Easize];	/* our ether address */
	ushort	lognrrb;	/* log2 number of receive ring buffers */
	ushort	logntrb;	/* log2 number of xmit ring buffers */
	ushort	nrrb;		/* number of receive ring buffers */
	ushort	ntrb;		/* number of xmit ring buffers */
	ushort*	rap;		/* lance address register */
	ushort*	rdp;		/* lance data register */
	ushort	busctl;		/* bus control bits */
	int	sep;		/* separation between shorts in lance ram
				    as seen by host */
	ushort*	lanceram;	/* start of lance ram as seen by host */
	Lanmem* lm;		/* start of lance ram as seen by lance */
	Enpkt*	rp;		/* receive buffers (host address) */
	Enpkt*	tp;		/* transmit buffers (host address) */
	Enpkt*	lrp;		/* receive buffers (lance address) */
	Enpkt*	ltp;		/* transmit buffers (lance address) */
};

