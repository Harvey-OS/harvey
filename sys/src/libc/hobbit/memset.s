TEXT memset(SB), $32			/* memset(void *ap, int c, size_t n); */
	MOV	ap+0(FP), p-20(SP)	/* p-20(SP) - start address */
	ADD3	n+8(FP), p-20(SP)
	MOV	R4, pe-24(SP)		/* pe-24(SP) - end address */

	CMPHI	n+8(FP), $3		/* is n >= 4 */
	JMPFN	_byte

_dowordalign:
	AND3	$3, p-20(SP)		/* word aligned? */
	CMPEQ	$0, R4
	JMPTY	_iswordalign
	MOV	c+4(FP), *p-20(SP).B
	ADD	$1, p-20(SP)
	JMP	_dowordalign

_iswordalign:
	MOV	c+4(FP), const-16(SP).B
	MOV	const-16(SP).B, const-15(SP).B
	MOV	const-16(SP).H, const-14(SP).H

	SUB3	p-20(SP), pe-24(SP)
	CMPHI	R4, $15			/* >= 16 */
	JMPFN	_word

_doquadalign:
	AND3	$15, p-20(SP)		/* quad aligned? */
	CMPEQ	$0, R4
	JMPTY	_isquadalign
	MOV	const-16(SP), *p-20(SP)
	ADD	$4, p-20(SP)
	JMP	_doquadalign

_isquadalign:
	MOV	const-16(SP), const-12(SP)
	MOV	const-16(SP), const-8(SP)
	MOV	const-16(SP), const-4(SP)

	AND3	$~15, pe-24(SP)
	SUB3	p-20(SP), R4
	CMPGT	R4, $15
	SUB	$16, R4
	ADD	p-20(SP), R4
	JMPFN	_word

	CMPEQ	$0, c+4(FP)		/* common case */
	JMPTY	_common

_quad1:
	CMPHI	R4, p-20(SP)
	DQM	const-16(SP), *p-20(SP)
	ADD	$16, p-20(SP)
	JMPTY	_quad1

_word:
	AND3	$~3, pe-24(SP)
	SUB3	p-20(SP), R4
	CMPGT	R4, $3
	SUB	$4, R4
	ADD	p-20(SP), R4
	JMPFN	_byte
_word1:
	CMPHI	R4, p-20(SP)
	MOV	const-16(SP), *p-20(SP)
	ADD	$4, p-20(SP)
	JMPTY	_word1

_byte:
	SUB3	p-20(SP), pe-24(SP)
	CMPGT	R4, $0
	SUB3	$1, pe-24(SP)
	JMPFN	_return
_byte1:
	CMPHI	R4, p-20(SP)
	MOV	c+4(FP), *p-20(SP).B
	ADD	$1, p-20(SP)
	JMPTY	_byte1
_return:
	RETURN

_common:
	CMPHI	R4, p-20(SP)
	DQM	$0, *p-20(SP)
	ADD	$16, p-20(SP)
	JMPTY	_common

_0word:
	AND3	$~3, pe-24(SP)
	SUB3	p-20(SP), R4
	CMPGT	R4, $3
	SUB	$4, R4
	ADD	p-20(SP), R4
	JMPFN	_0byte
_0word1:
	CMPHI	R4, p-20(SP)
	MOV	$0, *p-20(SP)
	ADD	$4, p-20(SP)
	JMPTY	_0word1

_0byte:
	SUB3	p-20(SP), pe-24(SP)
	CMPGT	R4, $0
	SUB3	$1, pe-24(SP)
	JMPFN	_0return
_0byte1:
	CMPHI	R4, p-20(SP)
	MOV	$0, *p-20(SP).B
	ADD	$1, p-20(SP)
	JMPTY	_0byte1
_0return:
	RETURN
