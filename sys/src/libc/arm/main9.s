arg=0
sp=13
sb=12

TEXT	_main(SB), 1, $16
	MOVW	$setR12(SB), R(sb)
	MOVW	R(arg), _clock(SB)

	MOVW	$inargv+0(FP), R(arg)
	MOVW	R(arg), 8(R(sp))
	MOVW	inargc-4(FP), R(arg)
	MOVW	R(arg), 4(R(sp))
	BL	main(SB)
loop:
	MOVW	$_exitstr<>(SB), R(arg)
	MOVW	R(arg), 4(R(sp))
	BL	exits(SB)
	BL	_div(SB)
	B	loop

DATA	_exitstr<>+0(SB)/4, $"main"
GLOBL	_exitstr<>+0(SB), $5
