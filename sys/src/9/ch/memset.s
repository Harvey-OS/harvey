/*
 * R4000 double-word version.  Only works in kernel mode.
 * Requires low-core support to save R5 in a 64-bit hole.
 */

/*
 *  R4000 instructions
 */
#define	LD(base, rt)		WORD	$((067<<26)|((base)<<21)|((rt)<<16))
#define	STD(rt, offset, base)		WORD	$((077<<26)|((base)<<21)|((rt)<<16)|((offset)&0xFFFF))

	TEXT	memset(SB),$16	/* $16 for hole to build temporary */
	MOVW R1, 0(FP)

	MOVW	n+8(FP), R3		/* R3 is count */
	MOVW	p+0(FP), R4		/* R4 is pointer */
	MOVW	c+4(FP), R5		/* R5 is char */
	ADDU	R3,R4, R6		/* R6 is end pointer */

/*
 * if not at least 8 chars,
 * don't even mess around.
 * 7 chars to guarantee any
 * rounding up to a doubleword
 * boundary and 8 characters
 * to get at least maybe one
 * full doubleword store.
 */
	SGT	$8,R3, R1
	BNE	R1, out

/*
 * turn R5 into a doubleword of characters.
 * build doubleword on stack; can't use OR on 64-bit registers
 */
	AND	$0xff, R5
	SLL	$8,R5, R1
	OR	R1, R5
	SLL	$16,R5, R1
	OR	R1, R5
	MOVW	R29, R1
	ADD	$8, R1
	AND	$~7, R1
	MOVW	R5, 0(R1)
	MOVW	R5, 4(R1)
	LD		(1, 5)
	
/*
 * store one byte at a time until pointer
 * is aligned on a doubleword boundary
 */
l1:
	AND	$7,R4, R1
	BEQ	R1, l2
	MOVB	R5, 0(R4)
	ADDU	$1, R4
	JMP	l1

/*
 * turn R3 into end pointer-31
 * store 32 at a time while there's room
 */
l2:
	ADDU	$-31,R6, R3
l3:
	SGTU	R3,R4, R1
	BEQ	R1, l4
	STD	(5, 0, 4)
	STD	(5, 8, 4)
	ADDU	$32, R4
	STD	(5, -16, 4)
	STD	(5, -8, 4)
	JMP	l3

/*
 * turn R3 into end pointer-3
 * store 4 at a time while there's room
 */
l4:
	ADDU	$-3,R6, R3
l5:
	SGTU	R3,R4, R1
	BEQ	R1, out
	MOVW	R5, 0(R4)
	ADDU	$4, R4
	JMP	l5

/*
 * last loop, store byte at a time
 */
out:
	SGTU	R6,R4 ,R1
	BEQ	R1, ret
	MOVB	R5, 0(R4)
	ADDU	$1, R4
	JMP	out

ret:
	MOVW	s1+0(FP), R1
	RET
	END
