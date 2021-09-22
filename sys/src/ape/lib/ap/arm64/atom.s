/*
 * int cas(ulong *p, ulong ov, ulong nv);
 */

TEXT	cas+0(SB),0,$0		/* r0 holds p */
TEXT	casp+0(SB),0,$0		/* r0 holds p */
	MOV	ov+8(FP), R1
	MOV	nv+16(FP), R2
spincas:
	LDXR	0(RARG), R3
	CMP	R3, R1
	BNE	fail
	STXR	R2,0(RARG),R4
	CBNZ	R4, spincas
	MOV	$1, R0
	RETURN
fail:
	CLREX
	MOV	$0, R0
	RETURN

TEXT ainc(SB), $0		/* long ainc(long *); */
spinainc:
	LDXR	0(RARG), R3
	ADD	$1,R3
	STXR	R3,0(RARG),R4
	CBNZ	R4, spinainc
	MOV	R3, R0
	RETURN

TEXT adec(SB), $0		/* long adec(long *); */
spinadec:
	LDXR	0(RARG), R3
	SUB	$1,R3
	STXR	R3,0(RARG),R4
	CBNZ	R4, spinadec
	MOV	R3, R0
	RETURN

TEXT loadlinked(SB), $0		/* long loadlinked(long *); */
	LDXR	0(RARG), R0
	RETURN

TEXT storecond(SB), $0		/* int storecond(long *, long); */
	MOV	ov+8(FP), R3
	STXR	R3,0(RARG),R0
	SUB	$1, R0
	RETURN
