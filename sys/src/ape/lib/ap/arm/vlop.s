#define	UMULL(Rs,Rm,Rhi,Rlo,S)	WORD	$((14<<28)|(4<<21)|(S<<20)|(Rhi<<16)|(Rlo<<12)|(Rs<<8)|(9<<4)|Rm)
#define UMLAL(Rs,Rm,Rhi,Rlo,S)	WORD	$((14<<28)|(5<<21)|(S<<20)|(Rhi<<16)|(Rlo<<12)|(Rs<<8)|(9<<4)|Rm)
#define	MUL(Rs,Rm,Rd,S)	WORD	$((14<<28)|(0<<21)|(S<<20)|(Rd<<16)|(Rs<<8)|(9<<4)|Rm)
arg=0

TEXT	_mulv(SB), $0
	MOVW	8(FP), R9	/* l0 */
	MOVW	4(FP), R10	/* h0 */
	MOVW	16(FP), R4	/* l1 */
	MOVW	12(FP), R5	/* h1 */
	UMULL(4, 9, 7, 6, 0)
	MUL(10, 4, 8, 0)
	ADD	R8, R7
	MUL(9, 5, 8, 0)
	ADD	R8, R7
	MOVW	R6, 4(R(arg))
	MOVW	R7, 0(R(arg))
	RET
