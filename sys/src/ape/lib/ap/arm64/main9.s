#define NPRIVATES	16

TEXT	_main(SB), 1, $(2*8 + NPRIVATES*8)
	MOV	$setSB(SB), R28
	/* _tos = arg */
	MOVW	RARG, _tos(SB)
	BL	_envsetup(SB)

	MOV	$p-(NPRIVATES*8)(SP), R1
	MOV	R1, _privates(SB)
	MOVW	$NPRIVATES, R1
	MOVW	R1, _nprivates(SB)

	MOV	$inargv+0(FP), RARG
	MOV	RARG, 16(RSP)
	MOVW	inargc-8(FP), RARG
	MOVW	RARG, 8(RSP)
	BL	main(SB)
loop:
	MOVW	RARG, 4(RSP)
	BL	exit(SB)
//	BL	_div(SB)
	B	loop
