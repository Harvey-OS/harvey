TEXT ainc(SB), 1, $0	/* long ainc(long *); */
ainclp:
	MOVL	(RARG), AX	/* exp */
	MOVL	AX, BX
	INCL	BX		/* new */
	LOCK; CMPXCHGL BX, (RARG)
	JNZ	ainclp
	MOVL	BX, AX
	RET

TEXT adec(SB), 1, $0	/* long adec(long*); */
adeclp:
	MOVL	(RARG), AX
	MOVL	AX, BX
	DECL	BX
	LOCK; CMPXCHGL BX, (RARG)
	JNZ	adeclp
	MOVL	BX, AX
	RET

/*
 * int cas32(u32int *p, u32int ov, u32int nv);
 * int cas(uint *p, int ov, int nv);
 * int casul(ulong *p, ulong ov, ulong nv);
 */

TEXT cas32(SB), 1, $0
TEXT cas(SB), 1, $0
TEXT casul(SB), 1, $0
TEXT casl(SB), 1, $0			/* back compat */
	MOVL	exp+8(FP), AX
	MOVL	new+16(FP), BX
	LOCK; CMPXCHGL BX, (RARG)
	MOVL	$1, AX				/* use CMOVLEQ etc. here? */
	JNZ	_cas32r0
_cas32r1:
	RET
_cas32r0:
	DECL	AX
	RET

/*
 * int cas64(u64int *p, u64int ov, u64int nv);
 * int casp(void **p, void *ov, void *nv);
 */

TEXT cas64(SB), 1, $0
TEXT casp(SB), 1, $0
	MOVQ	exp+8(FP), AX
	MOVQ	new+16(FP), BX
	LOCK; CMPXCHGQ BX, (RARG)
	MOVL	$1, AX				/* use CMOVLEQ etc. here? */
	JNZ	_cas64r0
_cas64r1:
	RET
_cas64r0:
	DECL	AX
	RET

/*
 * void mfence(void);
 */
TEXT mfence(SB),0,$0
	MFENCE
	RET

