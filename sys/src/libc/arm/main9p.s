#define NPRIVATES	16

arg=0
sp=13
sb=12

TEXT	_mainp(SB), 1, $(16 + NPRIVATES*4)
	MOVW	$setR12(SB), R(sb)
	MOVW	R(arg), _tos(SB)

	MOVW	$p-64(SP), R1
	MOVW	R1, _privates(SB)
	MOVW	$NPRIVATES, R1
	MOVW	R1, _nprivates(SB)

	BL	_profmain(SB)
	MOVW	_tos(SB), R1
	MOVW	4(R1), R0
	MOVW	R0, 0(R1)

	MOVW	$inargv+0(FP), R(arg)
	MOVW	R(arg), 8(R(sp))
	MOVW	inargc-4(FP), R(arg)
	MOVW	R(arg), 4(R(sp))
	BL	main(SB)
loop:
	MOVW	$_exitstr<>(SB), R(arg)
	MOVW	R(arg), 4(R(sp))
	BL	exits(SB)
	MOVW	$_div(SB), R(arg)	/* force loading of div */
	MOVW	$_profin(SB), R(arg)	/* force loading of profile */
	B	loop

TEXT	_savearg(SB), 1, $0
	RET

TEXT	_callpc(SB), 1, $-4
	MOVW	0(R13), R(arg)
	RET

DATA	_exitstr<>+0(SB)/4, $"main"
GLOBL	_exitstr<>+0(SB), $5
