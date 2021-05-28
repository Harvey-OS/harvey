/*
 * minimal reboot code
 */
#include "mem.h"
#include "spim.s"

TEXT	_main(SB), $0
	MOVW	$setR30(SB), R30
	JMP		main(SB)

TEXT	setsp(SB), $-4
	MOVW	R1, R29
	RET

TEXT	coherence(SB), $-4
	SYNC
	RET

TEXT	go(SB), $-4		/* go(kernel) */
	MOVW	_argc(SB), R4
	CONST	(CONFARGV, R5)
	MOVW	_env(SB), R6
	MOVW	R0, R7
	JMP		(R1)

	JMP		0(PC)

/*
 *  cache manipulation
 */
TEXT	cleancache(SB), $-4
	MOVW	M(STATUS), R10			/* old status -> R10 */
	MOVW	R0, M(STATUS)			/* intrs off */
	EHB
	MOVW	$KSEG0, R1			/* index, not address, kseg0 avoids tlb */
	MOVW	$(SCACHESIZE/4), R9		/* 4-way cache */
ccache:
	CACHE	SD+IWBI, 0(R1)		/* flush & invalidate thru L2 by index */
	CACHE	SD+IWBI, 1(R1)		/* ways are least significant bits */
	CACHE	SD+IWBI, 2(R1)
	CACHE	SD+IWBI, 3(R1)
	SUBU	$CACHELINESZ, R9
	ADDU	$CACHELINESZ, R1
	BGTZ	R9, ccache
	SYNC
	MOVW	R10, M(STATUS)
	EHB
	RET
