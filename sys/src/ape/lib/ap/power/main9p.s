#define NPRIVATES	16

GLOBL	_tos(SB), $4
GLOBL	_privates(SB), $4
GLOBL	_nprivates(SB), $4

TEXT	_mainp(SB), 1, $(3*4+NPRIVATES*4)

	MOVW	$setSB(SB), R2

	/* _tos = arg */
	MOVW	R3, _tos(SB)
	MOVW	$8(SP), R1
	MOVW	R1, _privates(SB)
	MOVW	$NPRIVATES, R1
	MOVW	R1, _nprivates(SB)

	/* _profmain(); */
	BL	_envsetup(SB)

	/* _tos->prof.pp = _tos->prof.next; */
	MOVW	_tos+0(SB),R1
	MOVW	4(R1),R2
	MOVW	R2,(R1)

	/* main(argc, argv, environ); */
	MOVW	inargc-4(FP), R3
	MOVW	$inargv+0(FP), R4
	MOVW	environ(SB), R5
	MOVW	R3, 4(R1)
	MOVW	R4, 8(R1)
	MOVW	R5, 12(R1)
	BL	main(SB)
loop:
	MOVW	R3, 4(R1)
	BL	exit(SB)
	MOVW	$_profin(SB), R4	/* force loading of profile */
	BR	loop

TEXT	_savearg(SB), 1, $0
	RETURN

TEXT	_callpc(SB), 1, $0
	MOVW	argp+0(FP), R3
	MOVW	4(R3), R3
	RETURN
