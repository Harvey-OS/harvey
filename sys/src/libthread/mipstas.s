/*
 *	magnum user level lock code
 */

#define	LL(base, rt)	WORD	$((060<<26)|((base)<<21)|((rt)<<16))
#define	SC(base, rt)	WORD	$((070<<26)|((base)<<21)|((rt)<<16))
#define	NOOP		WORD	$0x27

TEXT	tas(SB), $0

	MOVW	R1, R2		/* address of key */
	MOVW	$1, R3
	LL(2, 1)
	NOOP
	SC(2, 3)		/* one is success, zero failure */
	NOOP
	MOVW	$1, R1
	SUB	R1,R3,R1	/* zero is success, one failure */
	RET
