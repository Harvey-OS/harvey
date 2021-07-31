#include "mem.h"
/*
 *  Entered here from Compaq's bootldr.  First relocate to
 *  the location we're linked for and then copy back the
 *  decompressed kernel.
 *
 *  All 
 */
TEXT _start(SB), $-4
	MOVW	$setR12(SB), R12		/* load the SB */
	MOVW	$1, R0		/* dance to make 5l think that the magic */
	MOVW	$1, R1		/* numbers in WORDs below are being used */
	CMP.S	R0, R1		/* and to align them to where bootldr wants */
	BEQ	_start2
	WORD	$0x016f2818	/* magic number to say we are a kernel */
	WORD	$0xc0008000	/* entry point address */
	WORD	$0		/* size?, or end of data? */

_start2:

	/* SVC mode, interrupts disabled */
	MOVW	$(PsrDirq|PsrDfiq|PsrMsvc), R1
	MOVW	R1, CPSR

	/* disable the MMU */
	MOVW	$0x130, R1
	MCR     CpMMU, 0, R1, C(CpControl), C(0x0)

	/* enable caches */
	MRC	CpMMU, 0, R0, C(CpControl), C(0x0)
	ORR	$(CpCdcache|CpCicache|CpCwb), R0
	MCR     CpMMU, 0, R0, C(CpControl), C(0x0)

	/* flush caches */
	MCR	CpMMU, 0, R0, C(CpCacheFlush), C(0x7), 0
	/* drain prefetch */
	MOVW	R0,R0						
	MOVW	R0,R0
	MOVW	R0,R0
	MOVW	R0,R0

	/* drain write buffer */
	MCR	CpMMU, 0, R0, C(CpCacheFlush), C(0xa), 4

	/* relocate to where we expect to be */
	MOVW	$(512*1024),R3
	MOVW	$0xC0008000,R1
	MOVW	$0xC0200000,R2
	ADD	R1,R3
_relloop:
	MOVW	(R1),R0
	MOVW	R0,(R2)
	ADD	$4,R1
	ADD	$4,R2
	CMP.S	R1,R3
	BNE	_relloop

	MOVW	$(MACHADDR+BY2PG), R13		/* stack */
	SUB	$4, R13				/* link */

	/* jump to where we've been relocated */
	MOVW	$_relocated(SB),R15

TEXT _relocated(SB),$-4
	BL	main(SB)
	BL	exit(SB)
	/* we shouldn't get here */
_mainloop:
	B	_mainloop
	BL	_div(SB)			/* hack to get _div etc loaded */

TEXT mypc(SB),$-4
	MOVW	R14,R0
	RET

TEXT draincache(SB),$-4
	/* write back any dirty data */
	MOVW	$0xe0000000,R0
	ADD	$(8*1024),R0,R1
_cfloop:
	MOVW.P	32(R0),R2
	CMP.S	R0,R1
	BNE	_cfloop
	
	/* drain write buffer and invalidate i&d cache contents */
	MCR	CpMMU, 0, R0, C(CpCacheFlush), C(0xa), 4
	MCR	CpMMU, 0, R0, C(CpCacheFlush), C(0x7), 0

	/* drain prefetch */
	MOVW	R0,R0						
	MOVW	R0,R0
	MOVW	R0,R0
	MOVW	R0,R0

	/* disable caches */
	MRC	CpMMU, 0, R0, C(CpControl), C(0x0)
	BIC	$(CpCdcache|CpCicache|CpCwb), R0
	MCR     CpMMU, 0, R0, C(CpControl), C(0x0)
	RET
