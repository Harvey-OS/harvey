#define	BDNZ	BC	16,0,
	TEXT	memmove(SB), $0
	BR	move

	TEXT	memcpy(SB), $0
move:

/*
 * performance:
 * (tba)
 */

	MOVW	R3, s1+0(FP)
	MOVW	n+8(FP), R9		/* R9 is count */
	MOVW	R3, R10			/* R10 is to-pointer */
	CMP	R9, $0
	BEQ	ret
	BLT	trap
	MOVW	s2+4(FP), R11		/* R11 is from-pointer */

/*
 * if no more than 16 bytes, just use one lsw/stsw
 */
	CMP	R9, $16
	BLE	fout

	ADD	R9,R11, R13		/* R13 is end from-pointer */
	ADD	R9,R10, R12		/* R12 is end to-pointer */

/*
 * easiest test is copy backwards if
 * destination string has higher mem address
 */
	CMPU	R10, R11
	BGT	back

/*
 * test if both pointers
 * are similarly word aligned
 */
	XOR	R10,R11, R7
	ANDCC	$3,R7
	BNE	fbad

/*
 * move a few bytes to align pointers
 */
	ANDCC	$3,R10,R7
	BEQ	f2
	SUBC	R7, $4, R7
	SUB	R7, R9
	MOVW	R7, XER
	LSW	(R11), R16
	ADD	R7, R11
	STSW	R16, (R10)
	ADD	R7, R10

/*
 * turn R14 into doubleword count
 * copy 16 bytes at a time while there's room.
 */
f2:
	SRAWCC	$4, R9, R14
	BLE	fout
	MOVW	R14, CTR
	SUB	$4, R11
	SUB	$4, R10
f3:
	MOVWU	4(R11), R16
	MOVWU	R16, 4(R10)
	MOVWU	4(R11), R17
	MOVWU	R17, 4(R10)
	MOVWU	4(R11), R16
	MOVWU	R16, 4(R10)
	MOVWU	4(R11), R17
	MOVWU	R17, 4(R10)
	BDNZ	f3
	RLWNMCC	$0, R9, $15, R9	/* residue */
	BEQ	ret
	ADD	$4, R11
	ADD	$4, R10

/*
 * move up to 16 bytes through R16 .. R19; aligned and unaligned
 */
fout:
	MOVW	R9, XER
	LSW	(R11), R16
	STSW	R16, (R10)
	BR	ret

/*
 * loop for unaligned copy, then copy up to 15 remaining bytes
 */
fbad:
	SRAWCC	$4, R9, R14
	BLE	f6
	MOVW	R14, CTR
f5:
	LSW	(R11), $16, R16
	ADD	$16, R11
	STSW	R16, $16, (R10)
	ADD	$16, R10
	BDNZ	f5
	RLWNMCC	$0, R9, $15, R9	/* residue */
	BEQ	ret
f6:
	MOVW	R9, XER
	LSW	(R11), R16
	STSW	R16, (R10)
	BR	ret

/*
 * whole thing repeated for backwards
 */
back:
	CMP	R9, $4
	BLT	bout

	XOR	R12,R13, R7
	ANDCC	$3,R7
	BNE	bout
b1:
	ANDCC	$3,R13, R7
	BEQ	b2
	MOVBZU	-1(R13), R16
	MOVBZU	R16, -1(R12)
	SUB	$1, R9
	BR	b1
b2:
	SRAWCC	$4, R9, R14
	BLE	b4
	MOVW	R14, CTR
b3:
	MOVWU	-4(R13), R16
	MOVWU	R16, -4(R12)
	MOVWU	-4(R13), R17
	MOVWU	R17, -4(R12)
	MOVWU	-4(R13), R16
	MOVWU	R16, -4(R12)
	MOVWU	-4(R13), R17
	MOVWU	R17, -4(R12)
	BDNZ	b3
	RLWNMCC	$0, R9, $15, R9	/* residue */
	BEQ	ret
b4:
	SRAWCC	$2, R9, R14
	BLE	bout
	MOVW	R14, CTR
b5:
	MOVWU	-4(R13), R16
	MOVWU	R16, -4(R12)
	BDNZ	b5
	RLWNMCC	$0, R9, $3, R9	/* residue */
	BEQ	ret

bout:
	CMPU	R13, R11
	BLE	ret
	MOVBZU	-1(R13), R16
	MOVBZU	R16, -1(R12)
	BR	bout

trap:
	MOVW	$0, R0
	MOVW	0(R0), R0

ret:
	MOVW	s1+0(FP), R3
	RETURN
