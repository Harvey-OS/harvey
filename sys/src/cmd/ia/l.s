TEXT	_main(SB), $0

	ADD		R1,R4
	ADD		R1,R3,R4
	ADD		$10,R1,R4

loop:
	BNE		R1,R4,loop

	MOVW		R3, (R2)
	MOVW		R3, 10(R2)
	MOVW		R3, _main(SB)
	MOVW		R3, (R2)

	RET
