#define DMB		WORD	$0xf57ff05f /* data mem. barrier; last f = SY */

#define	CLREX		WORD	$0xf57ff01f
#define	LDREX(a,r)	WORD	$(0xe<<28|0x01900f9f | (a)<<16 | (r)<<12)
/* `The order of operands is from left to right in dataflow order' - asm man */
#define	STREX(v,a,r)	WORD	$(0xe<<28|0x01800f90 | (a)<<16 | (r)<<12 | (v)<<0)

/*
 * erratum 754327: no automatic store buffer drain; in cortex-a9 prior to r2p0.
 * stores can linger in the store buffer.  a dmb will flush them so that other
 * processors or peripherals (`external agents') can see them.
 */

/*
 * int cas(ulong *p, ulong ov, ulong nv);
 */

TEXT	cas+0(SB),0,$0		/* r0 holds p */
TEXT	casp+0(SB),0,$0		/* r0 holds p */
	MOVW	ov+4(FP), R1
	MOVW	nv+8(FP), R2
	DMB
spincas:
	LDREX(0,3)		/* LDREX 0(R0),R3 */
	CMP.S	R3, R1
	BNE	fail
	STREX(2,0,4)		/* STREX 0(R0),R2,R4 */
	CMP.S	$0, R4
	BNE	spincas
	DMB			/* required by erratum 754327 */
	MOVW	$1, R0
	RET
fail:
	CLREX
	MOVW	$0, R0
	RET

TEXT adec(SB), $0		/* long adec(long *); */
	MOVW	$-1, R5
	B	start
TEXT ainc(SB), $0		/* long ainc(long *); */
	MOVW	$1, R5
start:
	DMB
spinainc:
	LDREX(0,3)		/* LDREX 0(R0),R3 */
	ADD	R5, R3		/* inc or dec */
	STREX(3,0,4)		/* STREX 0(R0),R3,R4 */
	CMP.S	$0, R4
	BNE	spinainc
	DMB			/* required by erratum 754327 */
	MOVW	R3, R0
	RET

TEXT loadlinked(SB), $0		/* long loadlinked(long *); */
	LDREX(0,0)		/* LDREX 0(R0),R0 */
	RET

TEXT storecond(SB), $0		/* int storecond(long *, long); */
	MOVW	ov+4(FP), R3
	STREX(3,0,0)		/* STREX 0(R0),R3,R0 */
	DMB			/* required by erratum 754327 */
	RSB	$1, R0
	RET
