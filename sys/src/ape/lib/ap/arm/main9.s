arg=0
sp=13
sb=12

TEXT	_main(SB), 1, $16
	MOVW	$setR12(SB), R(sb)
	BL	_envsetup(SB)
	MOVW	$inargv+0(FP), R(arg)
	MOVW	R(arg), 8(R(sp))
	MOVW	inargc-4(FP), R(arg)
	MOVW	R(arg), 4(R(sp))
	BL	main(SB)
loop:
	MOVW	R(arg), 4(R(sp))
	BL	exit(SB)
	BL	_div(SB)
	B	loop
