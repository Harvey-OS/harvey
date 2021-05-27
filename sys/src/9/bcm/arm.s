/*
 * armv6/v7 machine assist, definitions
 *
 * loader uses R11 as scratch.
 */

#include "mem.h"
#include "arm.h"

#define PADDR(va)	(PHYSDRAM | ((va) & ~KSEGM))

#define L1X(va)		(((((va))>>20) & 0x0fff)<<2)

/*
 * new instructions
 */

#define ISB	\
	MOVW	$0, R0; \
	MCR	CpSC, 0, R0, C(CpCACHE), C(CpCACHEinvi), CpCACHEwait

#define DSB \
	MOVW	$0, R0; \
	MCR	CpSC, 0, R0, C(CpCACHE), C(CpCACHEwb), CpCACHEwait

#define	BARRIERS	DSB; ISB

#define MCRR(coproc, op, rd, rn, crm) \
	WORD $(0xec400000|(rn)<<16|(rd)<<12|(coproc)<<8|(op)<<4|(crm))
#define MRRC(coproc, op, rd, rn, crm) \
	WORD $(0xec500000|(rn)<<16|(rd)<<12|(coproc)<<8|(op)<<4|(crm))
#define MSR(R, rn, m, m1) \
	WORD $(0xe120f200|(R)<<22|(m1)<<16|(m)<<8|(rn))

#define	LDREX(fp,t)   WORD $(0xe<<28|0x01900f9f | (fp)<<16 | (t)<<12)
/* `The order of operands is from left to right in dataflow order' - asm man */
#define	STREX(f,tp,r) WORD $(0xe<<28|0x01800f90 | (tp)<<16 | (r)<<12 | (f)<<0)
#define	CLREX	WORD	$0xf57ff01f

#define CPSIE	WORD	$0xf1080080	/* intr enable: zeroes I bit */
#define CPSID	WORD	$0xf10c0080	/* intr disable: sets I bit */

#define OKAY \
	MOVW	$0x7E200028,R2; \
	MOVW	$0x10000,R3; \
	MOVW	R3,(R2)

#define PUTC(s)

/*
 * get cpu id, or zero if armv6
 */
#define CPUID(r) \
	MRC	CpSC, 0, r, C(CpID), C(CpIDfeat), 7; \
	CMP	$0, r; \
	B.EQ	2(PC); \
	MRC	CpSC, 0, r, C(CpID), C(CpIDidct), CpIDmpid; \
	AND.S	$(MAXMACH-1), r

