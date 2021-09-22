#define LGSTRIDE 2
#define STRIDE (1<<LGSTRIDE)

	TEXT	memset(SB),$0
	CLD
	MOVQ	RARG, DI
	MOVBQZX	c+8(FP), AX
	MOVQ	n+16(FP), BX
/*
 * if not enough bytes, just set bytes
 */
	CMPQ	BX, $(2*STRIDE+1)
	JLS	c3
/*
 * if dest not aligned, just set bytes
 */
	MOVQ	RARG, CX
	ANDQ	$(STRIDE-1), CX
	JNE	c3
/*
 * build word in AX
 */
	CMPQ	AX, $0
	JEQ	c1
	MOVB	AL, AH
	MOVQ	AX, CX
	SHLQ	$16, CX
	ORQ	CX, AX			/* word of pattern in AX */
/*
 * set multiple aligned words
 */
c1:
	MOVQ	BX, CX
	SHRQ	$LGSTRIDE, CX
	ANDQ	$(STRIDE-1), BX		/* remaining bytes */
	REP;	STOSL			/* CX loops of *DI++ = AX */
/*
 * set the rest, by bytes
 */
c3:
	MOVQ	BX, CX
	REP;	STOSB

	MOVQ	RARG, AX
	RET
