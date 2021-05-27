#include "x16.h"
#include "mem.h"

TEXT e820(SB), $0
	MOVL bx+4(SP), BX
	MOVL p+8(SP), DI
	MOVL $0xE820, AX
	MOVL $0x534D4150, DX
	CALL rmode16(SB)
	LWI(24, rCX)
	BIOSCALL(0x15)
	JC _bad
	CALL16(pmode32(SB))
	CMPB CL, $24
	JZ _ret
	MOVL $1, AX
	MOVL p+8(SP), DI
	MOVL AX, 20(DI)
_ret:
	MOVL BX, AX
	RET
_bad:
	CALL16(pmode32(SB))
	XORL AX, AX
	MOVL p+8(SP), DI
	MOVL AX, 0(DI)
	MOVL AX, 4(DI)
	MOVL AX, 8(DI)
	MOVL AX, 12(DI)
	MOVL AX, 16(DI)
	MOVL AX, 20(DI)
	RET
