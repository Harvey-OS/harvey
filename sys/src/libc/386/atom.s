TEXT ainc(SB), $0	/* long ainc(long *); */
	MOVL	addr+0(FP), BX
ainclp:
	MOVL	(BX), AX
	MOVL	AX, CX
	INCL	CX
	LOCK
	BYTE	$0x0F; BYTE $0xB1; BYTE $0x0B	/* CMPXCHGL CX, (BX) */
	JNZ	ainclp
	MOVL	CX, AX
	RET

TEXT adec(SB), $0	/* long adec(long*); */
	MOVL	addr+0(FP), BX
adeclp:
	MOVL	(BX), AX
	MOVL	AX, CX
	DECL	CX
	LOCK
	BYTE	$0x0F; BYTE $0xB1; BYTE $0x0B	/* CMPXCHGL CX, (BX) */
	JNZ	adeclp
	MOVL	CX, AX
	RET

/*
 * int cas32(u32int *p, u32int ov, u32int nv);
 * int cas(uint *p, int ov, int nv);
 * int casp(void **p, void *ov, void *nv);
 * int casl(ulong *p, ulong ov, ulong nv);
 */

/*
 * CMPXCHG (CX), DX: 0000 1111 1011 000w oorr rmmm,
 * mmm = CX = 001; rrr = DX = 010
 */

#define CMPXCHG		BYTE $0x0F; BYTE $0xB1; BYTE $0x11

TEXT	cas32+0(SB),0,$0
TEXT	cas+0(SB),0,$0
TEXT	casp+0(SB),0,$0
TEXT	casl+0(SB),0,$0
	MOVL	p+0(FP), CX
	MOVL	ov+4(FP), AX
	MOVL	nv+8(FP), DX
	LOCK
	CMPXCHG
	JNE	fail
	MOVL	$1,AX
	RET
fail:
	MOVL	$0,AX
	RET

/*
 * int cas64(u64int *p, u64int ov, u64int nv);
 */

/*
 * CMPXCHG64 (DI): 0000 1111 1100 0111 0000 1110,
 */

#define CMPXCHG64		BYTE $0x0F; BYTE $0xC7; BYTE $0x0F

TEXT	cas64+0(SB),0,$0
	MOVL	p+0(FP), DI
	MOVL	ov+0x4(FP), AX
	MOVL	ov+0x8(FP), DX
	MOVL	nv+0xc(FP), BX
	MOVL	nv+0x10(FP), CX
	LOCK
	CMPXCHG64
	JNE	fail
	MOVL	$1,AX
	RET
