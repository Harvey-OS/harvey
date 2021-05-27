#define DMB	MCR 15, 0, R0, C7, C10, 5
#define	STREX(f,tp,r) WORD $(0xe<<28|0x01800f90 | (tp)<<16 | (r)<<12 | (f)<<0)
#define	LDREX(fp,t)   WORD $(0xe<<28|0x01900f9f | (fp)<<16 | (t)<<12)
#define	CLREX	WORD	$0xf57ff01f

TEXT	tas(SB), $-4			/* tas(ulong *) */
	/* returns old (R0) after modifying (R0) */
	MOVW	R0,R5
	MOVW	$0, R0
	DMB

	MOVW	$1,R2		/* new value of (R0) */
tas1:
	LDREX(5,1)		/* LDREX 0(R5),R1 */
	CMP.S	$0, R1		/* old value non-zero (lock taken)? */
	BNE	lockbusy	/* we lose */
	STREX(2,5,4)		/* STREX R2,(R5),R4 */
	CMP.S	$0, R4
	BNE	tas1		/* strex failed? try again */
	MOVW	$0, R0
	DMB
	MOVW	R1, R0
	RET
lockbusy:
	CLREX
	MOVW	$0, R0
	DMB
	MOVW	R1, R0
	RET
