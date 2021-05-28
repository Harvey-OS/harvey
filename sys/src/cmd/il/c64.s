/*
	../ia/8.out -j c64.s && 8.out  -jla c64.j
	../ia/8.out c64.s && 8.out  -la c64.i
	win db j.out
	win db i.out
*/
TEXT	_main(SB), 0, $0
	MOVW	R9, R9
	MOVW	$0x11, R9
	MOVW	0x10(SP), R10
	MOVW	R12, 0x18(SP) 
	ADDW		$0x12, R11
	RET
