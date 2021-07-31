/*
 * cortex arm arch v7 cache flushing and invalidation
 * shared by l.s and rebootcode.s
 */

TEXT cacheiinv(SB), $-4				/* I invalidate */
	MOVW	$0, R0
	MCR	CpSC, 0, R0, C(CpCACHE), C(CpCACHEinvi), CpCACHEall /* ok on cortex */
	ISB
	RET

/*
 * set/way operators, passed a suitable set/way value in R0.
 */
TEXT cachedwb_sw(SB), $-4
	MCR	CpSC, 0, R0, C(CpCACHE), C(CpCACHEwb), CpCACHEsi
	RET

TEXT cachedwbinv_sw(SB), $-4
	MCR	CpSC, 0, R0, C(CpCACHE), C(CpCACHEwbi), CpCACHEsi
	RET

TEXT cachedinv_sw(SB), $-4
	MCR	CpSC, 0, R0, C(CpCACHE), C(CpCACHEinvd), CpCACHEsi
	RET

	/* set cache size select */
TEXT setcachelvl(SB), $-4
	MCR	CpSC, CpIDcssel, R0, C(CpID), C(CpIDid), 0
	ISB
	RET

	/* return cache sizes */
TEXT getwayssets(SB), $-4
	MRC	CpSC, CpIDcsize, R0, C(CpID), C(CpIDid), 0
	RET

/*
 * l1 cache operations.
 * l1 and l2 ops are intended to be called from C, thus need save no
 * caller's regs, only those we need to preserve across calls.
 */

TEXT cachedwb(SB), $-4
	MOVW.W	R14, -8(R13)
	MOVW	$cachedwb_sw(SB), R0
	MOVW	$1, R8
	BL	wholecache(SB)
	MOVW.P	8(R13), R15

TEXT cachedwbinv(SB), $-4
	MOVW.W	R14, -8(R13)
	MOVW	$cachedwbinv_sw(SB), R0
	MOVW	$1, R8
	BL	wholecache(SB)
	MOVW.P	8(R13), R15

TEXT cachedinv(SB), $-4
	MOVW.W	R14, -8(R13)
	MOVW	$cachedinv_sw(SB), R0
	MOVW	$1, R8
	BL	wholecache(SB)
	MOVW.P	8(R13), R15

TEXT cacheuwbinv(SB), $-4
	MOVM.DB.W [R14], (R13)	/* save lr on stack */
	MOVW	CPSR, R1
	CPSID			/* splhi */

	MOVM.DB.W [R1], (R13)	/* save R1 on stack */

	BL	cachedwbinv(SB)
	BL	cacheiinv(SB)

	MOVM.IA.W (R13), [R1]	/* restore R1 (saved CPSR) */
	MOVW	R1, CPSR
	MOVM.IA.W (R13), [R14]	/* restore lr */
	RET

/*
 * l2 cache operations
 */

TEXT l2cacheuwb(SB), $-4
	MOVW.W	R14, -8(R13)
	MOVW	$cachedwb_sw(SB), R0
	MOVW	$2, R8
	BL	wholecache(SB)
	MOVW.P	8(R13), R15

TEXT l2cacheuwbinv(SB), $-4
	MOVW.W	R14, -8(R13)
	MOVW	CPSR, R1
	CPSID			/* splhi */

	MOVM.DB.W [R1], (R13)	/* save R1 on stack */

	MOVW	$cachedwbinv_sw(SB), R0
	MOVW	$2, R8
	BL	wholecache(SB)
	BL	l2cacheuinv(SB)

	MOVM.IA.W (R13), [R1]	/* restore R1 (saved CPSR) */
	MOVW	R1, CPSR
	MOVW.P	8(R13), R15

TEXT l2cacheuinv(SB), $-4
	MOVW.W	R14, -8(R13)
	MOVW	$cachedinv_sw(SB), R0
	MOVW	$2, R8
	BL	wholecache(SB)
	MOVW.P	8(R13), R15

/*
 * these shift values are for the Cortex-A8 L1 cache (A=2, L=6) and
 * the Cortex-A8 L2 cache (A=3, L=6).
 * A = log2(# of ways), L = log2(bytes per cache line).
 * see armv7 arch ref p. 1403.
 */
#define L1WAYSH 30
#define L1SETSH 6
#define L2WAYSH 29
#define L2SETSH 6

/*
 * callers are assumed to be the above l1 and l2 ops.
 * R0 is the function to call in the innermost loop.
 * R8 is the cache level (one-origin: 1 or 2).
 *
 * initial translation by 5c, then massaged by hand.
 */
TEXT wholecache+0(SB), $-4
	MOVW	R0, R1		/* save argument for inner loop in R1 */
	SUB	$1, R8		/* convert cache level to zero origin */

	/* we may not have the MMU on yet, so map R1 to PC's space */
	BIC	$KSEGM,	R1	/* strip segment from address */
	MOVW	PC, R2		/* get PC's segment ... */
	AND	$KSEGM, R2
	CMP	$0, R2		/* PC segment should be non-zero on omap */
	BEQ	buggery
	ORR	R2, R1		/* combine them */

	/* drain write buffers */
	BARRIERS
	MCR	CpSC, 0, R0, C(CpCACHE), C(CpCACHEwb), CpCACHEwait
	ISB

	MOVW	CPSR, R2
	MOVM.DB.W [R2,R14], (SP) /* save regs on stack */
	CPSID			/* splhi to make entire op atomic */

	/* get cache sizes */
	SLL	$1, R8, R0	/* R0 = (cache - 1) << 1 */
	MCR	CpSC, CpIDcssel, R0, C(CpID), C(CpIDid), 0 /* set cache size select */
	ISB
	MRC	CpSC, CpIDcsize, R0, C(CpID), C(CpIDid), 0 /* get cache sizes */

	/* compute # of ways and sets for this cache level */
	SRA	$3, R0, R5	/* R5 (ways) = R0 >> 3 */
	AND	$1023, R5	/* R5 = (R0 >> 3) & MASK(10) */
	ADD	$1, R5		/* R5 (ways) = ((R0 >> 3) & MASK(10)) + 1 */

	SRA	$13, R0, R2	/* R2 = R0 >> 13 */
	AND	$32767, R2	/* R2 = (R0 >> 13) & MASK(15) */
	ADD	$1, R2		/* R2 (sets) = ((R0 >> 13) & MASK(15)) + 1 */

	/* precompute set/way shifts for inner loop */
	CMP	$0, R8		/* cache == 1? */
	MOVW.EQ	$L1WAYSH, R3 	/* yes */
	MOVW.EQ	$L1SETSH, R4
	MOVW.NE	$L2WAYSH, R3	/* no */
	MOVW.NE	$L2SETSH, R4

	/* iterate over ways */
	MOVW	$0, R7		/* R7: way */
outer:
	/* iterate over sets */
	MOVW	$0, R6		/* R6: set */
inner:
	/* compute set/way register contents */
	SLL	R3, R7, R0 	/* R0 = way << R3 (L?WAYSH) */
	ORR	R8<<1, R0	/* R0 = way << L?WAYSH | (cache - 1) << 1 */
	ORR	R6<<R4, R0 	/* R0 = way<<L?WAYSH | (cache-1)<<1 |set<<R4 */

	BL	(R1)		/* call set/way operation with R0 */

	ADD	$1, R6		/* set++ */
	CMP	R2, R6		/* set >= sets? */
	BLT	inner		/* no, do next set */

	ADD	$1, R7		/* way++ */
	CMP	R5, R7		/* way >= ways? */
	BLT	outer		/* no, do next way */

	MOVM.IA.W (SP), [R2,R14] /* restore regs */
	MOVW	R2, CPSR	/* splx */

	/* drain write buffers */
	MCR	CpSC, 0, R0, C(CpCACHE), C(CpCACHEwb), CpCACHEwait
	ISB
	RET

buggery:
WAVE('?')
	MOVW	PC, R0
//	B	pczeroseg(SB)
	RET
