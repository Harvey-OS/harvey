#define RBUFSIZE	(6*1024)	/* raw buffer size */

#include "../port/portdat.h"

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
