#include "x16.h"

TEXT uartinit(SB), $0
	MOVL c+8(SP), AX
	MOVB $0x00, AH
	JMP _uartbios
	
TEXT uartputc(SB), $0
	MOVL c+8(SP), AX
	MOVB $0x01, AH
	JMP _uartbios

TEXT uartgetc(SB), $0
	MOVL p+4(SP), DX
	CALL rmode16(SB)
	STI
	MOVB $0x03, AH
	BIOSCALL(0x14)
	CALL16(pmode32(SB))
	ANDL $0x8100, AX
	MOVL $0x0100, BX
	CMPL BX, AX
	JE _uartread
	XORL AX, AX
	RET
_uartread:
	MOVB $0x02, AH
_uartbios:
	MOVL p+4(SP), DX
	CALL rmode16(SB)
	STI
	BIOSCALL(0x14)
	CALL16(pmode32(SB))
	ANDL $0xFF, AX
	RET
