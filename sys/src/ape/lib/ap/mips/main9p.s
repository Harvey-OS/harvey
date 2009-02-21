#define NPRIVATES	16

GLOBL	_tos(SB), $4
GLOBL	_privates(SB), $4
GLOBL	_nprivates(SB), $4

TEXT	_mainp(SB), 1, $(3*4+NPRIVATES*4)
	MOVW	$setR30(SB), R30

	/* _tos = arg */
	MOVW	R1, _tos(SB)
/*
	MOVW	$0,FCR31
	NOR	R0,R0
	MOVD	$0.5, F26
	SUBD	F26, F26, F24
	ADDD	F26, F26, F28
	ADDD	F28, F28, F30
*/
	MOVW	$8(SP), R1
	MOVW	R1, _privates(SB)
	MOVW	$NPRIVATES, R1
	MOVW	R1, _nprivates(SB)

	/* _profmain(); */
	JAL	_profmain(SB)

	/* _tos->prof.pp = _tos->prof.next; */
	MOVW	_tos+0(SB),R1
	MOVW	4(R1),R2
	MOVW	R2,(R1)

	JAL	_envsetup(SB)

	/* main(argc, argv, environ); */
	MOVW	inargc-4(FP), R1
	MOVW	$inargv+0(FP), R2
	MOVW	environ(SB), R3
	MOVW	R1, 4(R29)
	MOVW	R2, 8(R29)
	MOVW	R3, 12(R29)
	JAL	main(SB)
loop:
	MOVW	R1, 4(R29)
	JAL	exit(SB)
	MOVW	$_profin(SB), R0	/* force loading of profile */
	JMP	loop

TEXT	_savearg(SB), 1, $0
	RET

TEXT	_callpc(SB), 1, $0
	MOVW	argp-4(FP), R1
	RET
