
/*
 * int swp(int r, int *p);
 * uchar swpb(uchar r, uchar *p);
 *
 * int cas(uintptr *p, uintptr ov, uintptr nv);
 */

#define	LDREX(a,r)	WORD	$(0xe<<28|0x01900f9f | (a)<<16 | (r)<<12)
#define	STREX(a,v,r)	WORD	$(0xe<<28|0x01800f90 | (a)<<16 | (r)<<12 | (v)<<0)

TEXT	cas+0(SB),0,$12		/* r0 holds p */
	MOVW	ov+4(FP), R1
	MOVW	nv+8(FP), R2
spin:
/*	LDREX	0(R0),R3	*/
	LDREX(0,3)
	CMP.S	R3, R1
	BNE	fail
/*	STREX	0(R0),R2,R4	*/
	STREX(0,2,4)
	CMP.S	$0, R4
	BNE	spin
	MOVW	$1, R0
	RET
fail:
	MOVW	$0, R0
	RET
