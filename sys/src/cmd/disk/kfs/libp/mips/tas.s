/*
 * locks for the mips
 */
#define	LL(base, rt)	WORD	$((060<<26)|((base)<<21)|((rt)<<16))
#define	SC(base, rt)	WORD	$((070<<26)|((base)<<21)|((rt)<<16))
#define	NOOP		WORD	$0x27
#define COP3		WORD	$(023<<26)

GLOBL	_tassub(SB), $4

TEXT	_tas(SB), $0
	MOVW	_tassub(SB), R2
	BEQ	R2, choose
	JMP	(R2)
choose:
	MOVW	FCR0, R2
	MOVW	$_3ktas(SB), R3
	SGT	$0x0500, R2, R5
	MOVW	$_4ktas(SB), R4
	BNE	R5, is3k
	MOVW	R4, _tassub(SB)
	JMP	(R4)
is3k:
	MOVW	R3, _tassub(SB)
	JMP	(R3)

/*
 * single processor r3000 -- kernel emulates tas with cop3 instruction
 */
TEXT	_3ktas(SB), $0
	MOVW	R1, keyaddr+0(FP)
again:
	MOVW	keyaddr+0(FP), R1
	MOVB	R0, 1(R1)		/* BUG: might never fault in 0(R1) */
	NOOP
	COP3
	BLTZ	R1, again
	RET

/*
 * r4000 -- load linked / store conditional
 */
TEXT	_4ktas(SB), $0
	MOVW	R1, R2		/* address of key */
loop:
	MOVW	$1, R3
	LL(2, 1)
	NOOP
	SC(2, 3)
	NOOP
	BEQ	R3, loop
	RET
