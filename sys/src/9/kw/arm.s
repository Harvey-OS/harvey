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
	ISB; \
	MOVW	$PHYSCONS, R7; \
	MOVW	$(c), R1; \
	MOVW	R1, (R7); \
	ISB

/* new instructions */
#define CLZ(s, d) WORD	$(0xe16f0f10 | (d) << 12 | (s))	/* count leading 0s */

#define DMB	\
	MCR	CpSC, 0, R0, C(CpCACHE), C(CpCACHEwb), CpCACHEdmbarr
/*
 * data synchronisation barrier (formerly drain write buffer).
 * waits for cache flushes, eviction buffer drain, tlb flushes,
 * branch-prediction flushes, writes to memory; the lot.
 * on sheeva, also flushes L2 eviction buffer.
 * zeroes R0.
 */
#define DSB	\
	MOVW	$0, R0; \
	MCR	CpSC, 0, R0, C(CpCACHE), C(CpCACHEwb), CpCACHEwait
/*
 * prefetch flush; zeroes R0.
 * arm926ej-s manual says we need to sync with l2 cache in isb,
 * and uncached load is the easiest way.  doesn't seem to matter.
 */
#define ISB	\
	MOVW	$0, R0; \
	MCR	CpSC, 0, R0, C(CpCACHE), C(CpCACHEinvi), CpCACHEwait
//	MOVW	(R0), R0; MOVW $0, R0

/* zeroes R0 */
#define	BARRIERS	ISB; DSB

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
