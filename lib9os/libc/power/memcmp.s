	TEXT	memcmp(SB), $0
#define	BDNZ	BC	16,0,
	MOVW R3, s1+0(FP)		/* R3 is pointer1 */

/*
 * performance:
 *	67mb/sec aligned; 16mb/sec unaligned
 */

	MOVW	n+8(FP), R4		/* R4 is count */
	MOVW	s2+4(FP), R5		/* R5 is pointer2 */

/*
 * let LSW do the work for 4 characters or less; aligned and unaligned
 */
	CMP	R4, $0
	BLE	eq
	CMP	R4, $4
	BLE	out

	XOR	R3, R5, R9
	ANDCC	$3, R9
	BNE	l4	/* pointers misaligned; use LSW loop */

/*
 * do enough bytes to align pointers
 */
	ANDCC	$3,R3, R9
	BEQ	l2
	SUBC	R9, $4, R9
	MOVW	R9, XER
	LSW	(R3), R10
	ADD	R9, R3
	LSW	(R5), R14
	ADD	R9, R5
	SUB	R9, R4
	CMPU	R10, R14
	BNE	ne

/*
 * compare 16 at a time
 */
l2:
	SRAWCC	$4, R4, R9
	BLE	l4
	MOVW	R9, CTR
	SUB	$4, R3
	SUB	$4, R5
l3:
	MOVWU	4(R3), R10
	MOVWU	4(R5), R12
	MOVWU	4(R3), R11
	MOVWU	4(R5), R13
	CMPU	R10, R12
	BNE	ne
	MOVWU	4(R3), R10
	MOVWU	4(R5), R12
	CMPU	R11, R13
	BNE	ne
	MOVWU	4(R3), R11
	MOVWU	4(R5), R13
	CMPU	R10, R12
	BNE	ne
	CMPU	R11, R13
	BNE	ne
	BDNZ	l3
	ADD	$4, R3
	ADD	$4, R5
	RLWNMCC	$0, R4, $15, R4	/* residue */
	BEQ	eq

/*
 * do remaining words with LSW; also does unaligned case
 */
l4:
	SRAWCC	$2, R4, R9
	BLE	out
	MOVW	R9, CTR
l5:
	LSW	(R3), $4, R10
	ADD	$4, R3
	LSW	(R5), $4, R11
	ADD	$4, R5
	CMPU	R10, R11
	BNE	ne
	BDNZ	l5
	RLWNMCC	$0, R4, $3, R4	/* residue */
	BEQ	eq

/*
 * do remaining bytes with final LSW
 */
out:
	MOVW	R4, XER
	LSW	(R3), R10
	LSW	(R5), R11
	CMPU	R10, R11
	BNE	ne

eq:
	MOVW	$0, R3
	RETURN

ne:
	MOVW	$1, R3
	BGE	ret
	MOVW	$-1,R3
ret:
	RETURN
	END
