	TEXT	_main(SB), 1, $(3*XLEN)

	MOV	$setSB(SB), R3
	JAL	R1, _envsetup(SB)

	MOVW	inargc-XLEN(FP), R8
	MOV 	R8, XLEN(R2)
	MOV 	$inargv+0(FP), R9
	MOV 	R9, (2*XLEN)(R2)
	MOV 	environ(SB), R9
	MOV 	R9, (3*XLEN)(R2)
	JAL	R1, main(SB)

	MOVW	R8, XLEN(R2)
	JAL	R1, exit(SB)
	RET
