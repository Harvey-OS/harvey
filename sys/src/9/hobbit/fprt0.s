#include "machine.h"

TEXT fprt0(SB), $32
	MOV	%PSW, R28
	MOV	R32, R24
	MOV	%MSP, R20
	MOVA	R32, R16

	MOVA	R16, R4
	CALL	fpi(SB)
	CMPEQ	R24, R32
	JMPFY	_success
	KRET
_success:
	MOV	R24, R32

_psw_c:
	AND3	$PSW_C, R28
	CMPEQ	$0, R4
	JMPTY	_clear_c
_set_c:
	ADD3	$0xFFFFFFFF, $1
	JMP	_psw_v
_clear_c:
	TESTC

_psw_v:
	AND3	$PSW_V, R28
	CMPEQ	$0, R4
	JMPTY	_clear_v
_set_v:
	DIV3	$-1, $0x80000000
	JMP	_psw_f
_clear_v:
	TESTV

_psw_f:
	AND3	$PSW_F, R28
	CMPHI	R4, $0
	JMPTY	_set_f
_clear_f:
	CMPEQ	$0, $1
_set_f:
	RETURN
