/*
 *	mips user level lock code
 */

#define	LL(base, rt)	WORD	$((060<<26)|((base)<<21)|((rt)<<16))
#define	SC(base, rt)	WORD	$((070<<26)|((base)<<21)|((rt)<<16))
#define	NOOP		WORD	$0x27

	TEXT	_tas(SB), $0
	TEXT	C_4ktas(SB), $0
	MOVV	R1, R2		/* address of key */
tas1:
	MOVW	$1, R3
	LL(2, 1)
	NOOP
	SC(2, 3)
	NOOP
	BEQ	R3, tas1
	RET
