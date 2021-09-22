#undef OLD_INC_DEC

#ifdef OLD_INC_DEC
/* TODO: remove this old code */
TEXT ainc(SB), 1, $0				/* int ainc(int*); */
	MFENCE
	MOVL	$1, AX
	LOCK; XADDL AX, (RARG)
	ADDL	$1, AX
	MFENCE
	RET

/*
 * adec is typically used to release a lock; flush out all stores from this cpu
 * before decrement so that another cpu will see them after it acquires the
 * lock.
 */
TEXT adec(SB), 1, $0				/* int adec(int*); */
	MFENCE
	MOVL	$-1, AX
	LOCK; XADDL AX, (RARG)
	SUBL	$1, AX
	MFENCE
	RET

#else

TEXT ainc(SB), 1, $0		/* long ainc(long *); */
ainclp:
	MFENCE
	MOVL	(RARG), AX	/* exp */
	MOVL	AX, BX
	INCL	BX		/* new */
	LOCK; CMPXCHGL BX, (RARG)
	JNZ	ainclp
	MOVL	BX, AX
	MFENCE
	RET

/*
 * adec is typically used to release a lock; flush out all stores from this cpu
 * before decrement so that another cpu will see them after it acquires the
 * lock.
 */
TEXT adec(SB), 1, $0		/* long adec(long*); */
adeclp:
	MFENCE
	MOVL	(RARG), AX
	MOVL	AX, BX
	DECL	BX
	LOCK; CMPXCHGL BX, (RARG)
	JNZ	adeclp
	MOVL	BX, AX
	MFENCE
	RET
#endif

/*
 * int cas(uint *p, int ov, int nv);
 *
 * compare-and-swap: atomically set *addr to nv only if it contains ov,
 * and returns the old value.  this version returns 0 on success, -1 on failure
 * instead.
 */

TEXT cas(SB), 1, $0
	MOVL	exp+8(FP), AX
	MOVL	new+16(FP), BX
	LOCK; CMPXCHGL BX, (RARG)
_cascset:
	MOVL	$1, AX				/* use CMOVLEQ etc. here? */
	JEQ	_casr1
	DECL	AX				/* return 0 */
_casr1:
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
	JMP	_cascset
