/*
 * armv6 machine assist, definitions
 *
 * loader uses R11 as scratch.
 */

#include "mem.h"
#include "arm.h"

#define PADDR(va)	(PHYSDRAM | ((va) & ~KSEGM))

#define L1X(va)		(((((va))>>20) & 0x0fff)<<2)

#define PTEDRAM		(Dom0|L1AP(Krw)|Section|Cached|Buffered)

/*
 * new instructions
 */

#define ISB	\
	MOVW	$0, R0; \
	MCR	CpSC, 0, R0, C(CpCACHE), C(CpCACHEinvi), CpCACHEwait

#define DSB \
	MOVW	$0, R0; \
	MCR	CpSC, 0, R0, C(CpCACHE), C(CpCACHEwb), CpCACHEwait

#define	BARRIERS	ISB; DSB

#define MCRR(coproc, op, rd, rn, crm) \
	WORD $(0xec400000|(rn)<<16|(rd)<<12|(coproc)<<8|(op)<<4|(crm))

#define OKAY \
	MOVW	$0x7E200028,R2; \
	MOVW	$0x10000,R3; \
	MOVW	R3,(R2)
