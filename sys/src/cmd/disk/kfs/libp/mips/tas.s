/*
 * locks for the magnum
 */
#define	NOOP		WORD	$0x27
#define COP3		WORD	$(023<<26)

TEXT	_tas(SB), $0
	MOVW	R1, keyaddr+0(FP)
again:
	MOVW	keyaddr+0(FP), R1
	MOVB	R0, 1(R1)		/* BUG: might never fault in 0(R1) */
	NOOP
	COP3
	BLTZ	R1, again
	RET
