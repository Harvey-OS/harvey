#define	NOOP	BRA	FALSE,(R0)

TEXT	memcpy(SB), $-4
	JMP	mm

TEXT	memmove(SB), $-4
mm:
	NOSCHED
	MOVW	$p2+4(FP), R2
	MOVW	(R2), R4
	MOVW	$n+8(FP), R2
	MOVW	(R2), R5

/*
 * check for backwards
 */
	CMP	R3, R4
	BRA	HI, back

/*
 * check if at least 8 bytes
 */
forward:
	CMP	R5, $8
	BRA	LT, bytef

/*
 * try to work align
 */
	MOVW	R3, R7
	AND	$3, R7
	BRA	EQ, fa
	MOVW	R5, R6

falign:
	MOVB	(R4)+, R10
	SUB	$1, R7
	BRA	NE, falign
	MOVB	R10, (R3)+

fa:
	BIT	$3, R4
	BRA	NE, bytef

	SRL	$2, R6
	BRA	EQ, bytef
	SUB	$1, R6

/*
 * copy longs forward
 */
longf:
	BMOVW	R6, (R4)+, (R3)+
	SUB	$2048, R6
	BRA	GT, longf
	AND	$3, R5

bytef:
	CMP	R5, $0
	BRA	GT, bytef1
	SUB	$2, R5

	JMP	(R18)
	NOOP

bytef1:
	MOVB	(R4)+, R10
	DBRA	R5, bytef1
	MOVB	R10, (R3)+

	JMP	(R18)
	NOOP

/*
 * same thing only backwards
 */
back:
	ADD	$-1, R5, R10
	ADD	R10, R3, R8
	CMP	R8, R4
	BRA	CS, forward
	NOOP

	ADD	R10, R4

/*
 * check if at least 8 bytes
	CMP	R5, $8
	BRA	LT, byteb
*/
	JMP	byteb

/*
 * try to work align
 */
	MOVW	R8, R7
	AND	$3, R7
	BRA	EQ, fa
	MOVW	R5, R6

balign:
	MOVB	(R4)-, R10
	SUB	$1, R7
	BRA	NE, balign
	MOVB	R10, (R8)-

ba:
	BIT	$3, R4
	BRA	NE, byteb

	SRL	$2, R6
	BRA	EQ, byteb
	SUB	$1, R6

/*
 * copy longs forward
 */
longb:
	BMOVW	R6, (R4)-, (R8)-
	SUB	$2048, R6
	BRA	GT, longb
	AND	$3, R5

byteb:
	CMP	R5, $0
	BRA	GT, byteb1
	SUB	$2, R5

	JMP	(R18)
	NOOP

byteb1:
	MOVB	(R4)-, R10
	DBRA	R5, byteb1
	MOVB	R10, (R8)-

	JMP	(R18)
	NOOP
