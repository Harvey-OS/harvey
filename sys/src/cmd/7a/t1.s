TEXT	_main(SB), $0

	ADD		R1,R4
	ADD		R1,R3,R4
	ADD		R1<<24,R3,R4	/* logical left */
	ADD		R1>>24,R3,R4	/* logical right */
	ADD		R1->24,R3,R4	/* arithmetic right */
 	ADD		R1@>24,R3,R4	/* rotate right */
	ADD		$10,R1,R4

	ADD		R3.UXTB << 2,R3,R4	/* extending register */
	ADD		R3.SXTX << 2, R3, R4
	ADDW	R3.UXTW << 2, R3, R4
	ADDW	R3.UXTW, R3, R4

	MOV		$0x11111111, R3
	MOVW	$0x11111111, R3

	CMP		$7, R3
	CMPW	$7, R3

l1:
	ADD	R1, R4
	SUBS	$1, R1, R1
	CBNZ	R1, l1

	ADDS	R1, R4
	CSET	NE, R2

	ADR	$name(SB), R5

loop:
	ADDS	R1,R4
	BNE		loop

	MOV	R0, R5
	MOVW		R3, (R2)
	MOVW		R3, 10(R2)
	MOVW		R3, name(SB)
	MOVW		R3, name(SB)(R2)
	MOVW		R3, name(SB)(R2)
	MOVW		R3, (R2)
	MOVW		R3, (R2)[R1]
	MOV			R3, (R2)[R1]
	MOV			R3, (R2)(R1)
	MOVW		R3, (R2)(R1)
	MOVW		R3, (R2)[R1.SXTX]
	MOV	(RSP)8!, LR		/* pop LR: post-increment RSP (R31) */
	MOV	LR, -8(RSP)!		/* push LR: pre-decrement RSP */

	CMN	$4096,R0,

	RET
