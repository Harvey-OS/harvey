/*
 * sheevaplug machine assist, definitions
 * arm926ej-s processor at 1.2GHz
 *
 * loader uses R11 as scratch.
 */
#include "mem.h"
#include "arm.h"

#undef B					/* B is for 'botch' */

#define PADDR(a)	((a) & ~KZERO)
#define KADDR(a)	(KZERO|(a))

#define L1X(va)		(((((va))>>20) & 0x0fff)<<2)

#define MACHADDR	(L1-MACHSIZE)

#define PTEDRAM		(Dom0|L1AP(Krw)|Section|Cached|Buffered)
#define PTEIO		(Dom0|L1AP(Krw)|Section)

/* wave at the user; clobbers R1 & R7; needs R12 (SB) set */
#define WAVE(c) \
	BARRIERS; \
	MOVW	$PHYSCONS, R7; \
	MOVW	$(c), R1; \
	MOVW	R1, (R7); \
	BARRIERS

#define DMB	\
	MCR	CpSC, 0, R0, C(CpCACHE), C(CpCACHEwb), CpCACHEdmbarr
/*
 * data synchronisation barrier (formerly drain write buffer).
 * waits for cache, tlb, branch-prediction, memory; the lot.
 * on sheeva, also flushes L2 eviction buffer.
 * zeroes R0.
 */
#define DSB	\
	MOVW	$0, R0; \
	MCR	CpSC, 0, R0, C(CpCACHE), C(CpCACHEwb), CpCACHEwait
/* prefetch flush; zeroes R0 */
#define ISB	\
	MOVW	$0, R0; \
	MCR	CpSC, 0, R0, C(CpCACHE), C(CpCACHEinvi), CpCACHEwait; \
	MCR	CpSC, CpL2, PC, C(CpTESTCFG), C(CpTCl2inv), CpTCl2seva

/* zeroes R0 */
#define	BARRIERS	DSB; ISB

/*
 * invoked with PTE bits in R2, pa in R3, PTE pointed to by R4.
 * fill PTE pointed to by R4 and increment R4 past it.
 * increment R3 by a MB.  clobbers R1.
 */
#define FILLPTE() \
	ORR	R3, R2, R1;			/* pte bits in R2, pa in R3 */ \
	MOVW	R1, (R4); \
	ADD	$4, R4;				/* bump PTE address */ \
	ADD	$MiB, R3;			/* bump pa */ \

/* zero PTE pointed to by R4 and increment R4 past it. assumes R0 is 0. */
#define ZEROPTE() \
	MOVW	R0, (R4); \
	ADD	$4, R4;				/* bump PTE address */
