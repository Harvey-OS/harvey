arg=0
sp=13
sb=12

TEXT	_mainp(SB), 1, $16
	MOVW	$setR12(SB), R(sb)
	MOVW	R(arg), _clock(SB)

	BL	_profmain(SB)
	MOVW	__prof+4(SB), R(arg)
	MOVW	R(arg), __prof+0(SB)
	BL	_envsetup(SB)
	MOVW	$inargv+0(FP), R(arg)
	MOVW	R(arg), 8(R(sp))
	MOVW	inargc-4(FP), R(arg)
	MOVW	R(arg), 4(R(sp))
	BL	main(SB)
loop:
	MOVW	R(arg), 4(R(sp))
	BL	exits(SB)
	MOVW	$_div(SB), R(arg)	/* force loading of div */
	MOVW	$_profin(SB), R(arg)	/* force loading of profile */
	B	loop

TEXT	_savearg(SB), 1, $0
	RET

TEXT	_callpc(SB), 1, $0
	MOVW	argp-4(FP), R(arg)
	RET
