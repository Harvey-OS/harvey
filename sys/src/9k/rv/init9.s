TEXT	main(SB),0,$XLEN
	MOV	$setSB(SB), R3
	MOV	R8, argv0+0(FP)
	MOV	$argv0+0(FP), R9
	MOV	R9, argv+0(SP)
	JAL	R1, startboot(SB)
	RET
