/*
 *	RV64A atomic operations
 *	LR/SC only work on cached regions
 */
#include <atom.h>

#define ARG	8

TEXT adec(SB), 1, $-4			/* long adec(long*); */
	MOV	$-1, R9
	JMP	ainctop
TEXT ainc(SB), 1, $-4			/* long ainc(long *); */
	MOV	$1, R9
ainctop:
#ifdef NEWWAY
	/*
	 * this results in negative values for up->nlocks in deccnt, under load.
	 * I thought I understood the atomic memory operations, but
	 * apparently I have missed something.
	 */
	/* after: old value in R10, updated value in memory */
	AMOW(Amoadd, AQ|RL, 9, ARG, 10)
	ADDW	R9, R10, R(ARG)		/* old value +/- 1 for ainc/adec */
#else
	MOV	R(ARG), R12		/* address of counter */
	FENCE
loop:
	LRW(12, ARG)			/* (R12) -> R(ARG) */
	ADD	R9, R(ARG)		/* return new value */
	SCW(ARG, 12, 14)		/* R(ARG) -> (R12) maybe, R14=0 if ok */
	BNE	R14, loop
	FENCE
#endif
	SEXT_W(R(ARG))
	RET

/*
 * int cas(uint* p, int ov, int nv);
 *
 * compare-and-swap: atomically set *addr to nv only if it contains ov,
 * and returns the old value.  this version returns 0 on failure, 1 on success
 * instead.
 */
TEXT cas(SB), 1, $-4
	MOVWU	ov+XLEN(FP), R12
	MOVWU	nv+(XLEN+4)(FP), R13
	FENCE
spincas:
	LRW(ARG, 14)		/* (R(ARG)) -> R14 */
	SLL	$32, R14
	SRL	$32, R14	/* don't sign extend */
	BNE	R12, R14, fail
	SCW(13, ARG, 14)	/* R13 -> (R(ARG)) maybe, R14=0 if ok */
	BNE	R14, spincas	/* R14 != 0 means store failed */
	FENCE
	MOV	$1, R(ARG)
	RET
fail:
	FENCE
	MOV	R0, R(ARG)
	RET

/*
 * int	casp(void **p, void *ov, void *nv);
 * int	casv(uvlong *p, uvlong ov, uvlong nv);
 */
TEXT casp(SB), 1, $-4
TEXT casv(SB), 1, $-4
	MOV	ov+XLEN(FP), R12
	MOV	nv+(2*XLEN)(FP), R13
	FENCE
spincasp:
	LRD(ARG, 14)		/* (R(ARG)) -> R14 */
	BNE	R12, R14, failp
	SCD(13, ARG, 14)	/* R13 -> (R(ARG)) maybe, R14=0 if ok */
	BNE	R14, spincasp	/* R14 != 0 means store failed */
	MOV	$1, R(ARG)
	RET
failp:
	MOV	R0, R(ARG)
	RET
