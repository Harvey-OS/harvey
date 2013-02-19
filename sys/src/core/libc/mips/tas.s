/*
 *	magnum user level lock code
 */

#define	LL(base, rt)	WORD	$((060<<26)|((base)<<21)|((rt)<<16))
#define	SC(base, rt)	WORD	$((070<<26)|((base)<<21)|((rt)<<16))
#define	NOOP		WORD	$0x27
#define COP3		WORD	$(023<<26)

	TEXT	C_3ktas(SB),$0
	MOVW	R1, R21
btas:
	MOVW	R21, R1
	MOVB	R0, 1(R1)
	NOOP
	COP3
	BLTZ	R1, btas
	RET

	TEXT	C_4ktas(SB), $0
	MOVW	R1, R2		/* address of key */
tas1:
	MOVW	$1, R3
	LL(2, 1)
	NOOP
	SC(2, 3)
	NOOP
	BEQ	R3, tas1
	RET

	TEXT	C_fcr0(SB), $0
	MOVW	FCR0, R1
	RET
