TEXT	_xinc(SB),$0	/* void _xinc(long *); */
TEXT	ainc(SB),$0	/* long ainc(long *); */
	MOVW	R3, R4
xincloop:
	LWAR	(R4), R3
	ADD	$1, R3
	DCBT	(R4)				/* fix 405 errata cpu_210 */
	STWCCC	R3, (R4)
	BNE	xincloop
	RETURN

TEXT	_xdec(SB),$0	/* long _xdec(long *); */
TEXT	adec(SB),$0	/* long adec(long *); */
	MOVW	R3, R4
xdecloop:
	LWAR	(R4), R3
	ADD	$-1, R3
	DCBT	(R4)				/* fix 405 errata cpu_210 */
	STWCCC	R3, (R4)
	BNE	xdecloop
	RETURN

TEXT	loadlink(SB), $0

	LWAR	(R3), R3
	RETURN

TEXT	storecond(SB), $0

	MOVW	val+4(FP), R4
	DCBT	(R3)				/* fix 405 errata cpu_210 */
	STWCCC	R4, (R3)
	BNE	storecondfail
	MOVW	$1, R3
	RETURN
storecondfail:
	MOVW	$0, R3
	RETURN

/*
 * int cas(uint *p, int ov, int nv);
 * int casp(void **p, void *ov, void *nv);
 */

TEXT	cas+0(SB),0,$0
TEXT	casp+0(SB),0,$0
	MOVW	ov+4(FP),R4
	MOVW	nv+8(FP),R8
	LWAR	(R3),R5
	CMP	R5,R4
	BNE	fail
	DCBT	(R3)				/* fix 405 errata cpu_210 */
	STWCCC	R8,(R3)
	BNE	fail1
	MOVW	$1,R3
	RETURN
fail:
	DCBT	(R3)				/* fix 405 errata cpu_210 */
	STWCCC	R5,(R3)	/* give up exclusive access */
fail1:
	MOVW	R0,R3
	RETURN
	END
