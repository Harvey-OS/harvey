TEXT	main(SB), $0

	ADD	R1,R4
	ADD	R1,R3,R4
	ADD	R1<<1,R3,R4	/* logical left */
	ADD	R1>>1,R3,R4	/* logical right */
	ADD	R1->1,R3,R4	/* arithmetic right */
 	ADD	R1@>1,R3,R4	/* rotate right */

	ADD	R1<<R2,R3,R4
	MOVW	R1<<R2,R4
	ADD	$10,R1,R4

loop:
	ADD.S.NE	R1,R4
	BNE	loop

	MRC.EQ	3,9,R3,C5,C6,2
	MRC	3,9,R3,C5,C6,2

	MOVW	$(0xf<<28), CPSR
	MOVW.F	R3, SPSR

	SWI	123

	SWPW	R1,(R2),R3
	SWPBU.NE	(R2),R3
	SWPBU	R1,(R2)

	MOVM.IA.S.W	(R13),[R15]
	RFE
