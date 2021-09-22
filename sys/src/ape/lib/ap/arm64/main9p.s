#define NPRIVATES	16

GLOBL	_tos(SB), $8
GLOBL	_privates(SB), $8
GLOBL	_nprivates(SB), $4

TEXT	_mainp(SB), 1, $(3*8+NPRIVATES*8)

	MOV	$setSB(SB), R28

	/* _tos = arg */
	MOVW	RARG, _tos(SB)

	MOV	$private+8(SP), R1
	MOV	R1, _privates(SB)
	MOVW	$NPRIVATES, R1
	MOVW	R1, _nprivates(SB)

	/* _profmain(); */
	BL	_profmain(SB)

	/* _tos->prof.pp = _tos->prof.next; */
	MOV	_tos+0(SB),R1
	MOV	8(R1), R2
	MOV	R2, 0(R1)

	BL	_envsetup(SB)

	/* main(argc, argv, environ); */
	MOV	$inargv+0(FP), RARG
	MOV	RARG, 16(RSP)
	MOVW	inargc-8(FP), RARG
	MOVW	RARG, 4(RSP)
	MOV	environ(SB), RARG
	MOV	RARG, 8(RSP)
	BL	main(SB)
loop:
	MOVW	RARG, 4(RSP)
	BL	exit(SB)
//	MOV	$_div(SB), RARG	/* force loading of div */
	MOV	$_profin(SB), RARG	/* force loading of profile */
	B	loop

TEXT	_savearg(SB), 1, $0
	RETURN

TEXT	_callpc(SB), 1, $0
	MOV	argp-8(FP), RARG
	RETURN
