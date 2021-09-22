TEXT ainc(SB), 1, $8	/* long ainc(long *); */
ainclp:
	MOVL	(RARG), AX	/* exp */
	MOVL	AX, BX
	INCL	BX		/* new */
	LOCK; CMPXCHGL BX, (RARG)
	JNZ	ainclp
	MOVL	BX, AX
	RET

TEXT adec(SB), 1, $8	/* long adec(long*); */
adeclp:
	MOVL	(RARG), AX
	MOVL	AX, BX
	DECL	BX
	LOCK; CMPXCHGL BX, (RARG)
	JNZ	adeclp
	MOVL	BX, AX
	RET

TEXT	_xinc(SB),$8	/* void _xinc(long *); */
	LOCK; INCL 0(RARG)
	RET

TEXT	_xdec(SB),$8	/* long _xdec(long *); */
	MOVL	$0, AX
	MOVL	$1, BX
	LOCK; DECL 0(RARG)
	CMOVLNE	BX, AX
	RET

/*
 * int cas(uint *p, int ov, int nv);
 */

TEXT cas(SB), 1, $0
	MOVL	exp+8(FP), AX
	MOVL	new+16(FP), BX
	LOCK; CMPXCHGL BX, (RARG)
	MOVL	$1, AX				/* use CMOVLEQ etc. here? */
	JNZ	_casr0
_casr1:
	RET
_casr0:
	DECL	AX
	RET

/*
 * int casv(u64int *p, u64int ov, u64int nv);
 * int casp(void **p, void *ov, void *nv);
 */

TEXT casv(SB), 1, $0
TEXT casp(SB), 1, $0
	MOVQ	exp+8(FP), AX
	MOVQ	new+16(FP), BX
	LOCK; CMPXCHGQ BX, (RARG)
	MOVL	$1, AX				/* use CMOVLEQ etc. here? */
	JNZ	_casvr0
_casvr1:
	RET
_casvr0:
	DECL	AX
	RET
