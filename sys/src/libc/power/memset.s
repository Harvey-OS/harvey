	TEXT	memset(SB),$0
#define	BDNZ	BC	16,0,
	MOVW R3, p+0(FP)		/* R3 is pointer */

/*
 * performance:
 *	about 100mbytes/sec (8k blocks) on a 603/105 without L2 cache
 *	drops to 40mbytes/sec (10k blocks) and 28mbytes/sec with 32k blocks
 */

	MOVW	n+8(FP), R4		/* R4 is count */
	CMP	R4, $0
	BLE	ret
	MOVW	c+4(FP), R5		/* R5 is char */

/*
 * create 16 copies of c in R5 .. R8
 */
	RLWNM	$0, R5, $0xff, R5
	RLWMI	$8, R5, $0xff00, R5
	RLWMI	$16, R5, $0xffff0000, R5
	MOVW	R5, R6
	MOVW	R5, R7
	MOVW	R5, R8

/*
 * let STSW do the work for 16 characters or less; aligned and unaligned
 */
	CMP	R4, $16
	BLE	out

/*
 * store enough bytes to align pointer
 */
	ANDCC	$7,R3, R9
	BEQ	l2
	SUBC	R9, $8, R9
	MOVW	R9, XER
	STSW	R5, (R3)
	ADD	R9, R3
	SUB	R9, R4

/*
 * store 16 at a time while there's room
 * STSW was used here originally, but it's `completion serialised'
 */
l2:
	SRAWCC	$4, R4, R9
	BLE	out
	MOVW	R9, CTR
l3:
	MOVW	R5, 0(R3)
	ADD	$8, R3, R10
	MOVW	R6, 4(R3)
	MOVW	R7, 0(R10)
	ADD	$8, R10, R3
	MOVW	R8, 4(R10)
	BDNZ	l3
	RLWNMCC	$0, R4, $15, R4	/* residue */
	BEQ	ret

/*
 * store up to 16 bytes from R5 .. R8; aligned and unaligned
 */

out:
	MOVW	R4, XER
	STSW	R5, (R3)

ret:
	MOVW	0(FP), R3
	RETURN
	END
