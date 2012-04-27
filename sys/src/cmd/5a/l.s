TEXT	main(SB), $0

	ADD		R1,R4
	ADD		R1,R3,R4
	ADD		R1<<1,R3,R4	/* logical left */
	ADD		R1>>1,R3,R4	/* logical right */
	ADD		R1->1,R3,R4	/* arithmetic right */
 	ADD		R1@>1,R3,R4	/* rotate right */

	ADD		R1<<R2,R3,R4
	ADD		$10,R1,R4

loop:
	ADD.S.NE	R1,R4
	BNE		loop

	MOVW		R3, CPSR
	MOVW		R3, SPSR
	MOVW		R3, F10
	MOVW		R3, (R2)
	MOVW		R3, 10(R2)
	MOVW		R3, name(SB)
	MOVW		R3, name(SB)(R2)
	MOVW		R3, name(SB)(R2)
	MOVW		R3, (R2)
	MOVW		R3, R1<<2(R2)

	MRC.EQ		3,9,R3,C5,C6,2
	MRC		3,9,R3,C5,C6,2

	MOVM.IA		[R0,SP,R4], (R2)
	MOVM.DB.W	(R0), [R6-R11]
	MOVM.DB.W	(R0), [R0-R11]
	MOVM.S		(R0), [R0-R11]	// .S is supervisor space

	CMN	$4096,R0,

	RET
