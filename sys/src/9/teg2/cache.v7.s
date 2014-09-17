/*
 * cortex arm arch v7 cache flushing and invalidation
 * included by l.s and rebootcode.s
 */

TEXT cacheiinv(SB), $-4				/* I invalidate */
	MOVW	$0, R0
	MTCP	CpSC, 0, R0, C(CpCACHE), C(CpCACHEinvi), CpCACHEall /* ok on cortex */
	ISB
	RET

/*
 * set/way operators, passed a suitable set/way value in R0.
 */
TEXT cachedwb_sw(SB), $-4
	MTCP	CpSC, 0, R0, C(CpCACHE), C(CpCACHEwb), CpCACHEsi
	RET

TEXT cachedwbinv_sw(SB), $-4
	MTCP	CpSC, 0, R0, C(CpCACHE), C(CpCACHEwbi), CpCACHEsi
	RET

TEXT cachedinv_sw(SB), $-4
	MTCP	CpSC, 0, R0, C(CpCACHE), C(CpCACHEinvd), CpCACHEsi
	RET

	/* set cache size select */
TEXT setcachelvl(SB), $-4
	MTCP	CpSC, CpIDcssel, R0, C(CpID), C(CpIDidct), 0
	ISB
	RET

	/* return cache sizes */
TEXT getwayssets(SB), $-4
	MFCP	CpSC, CpIDcsize, R0, C(CpID), C(CpIDidct), 0
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
 * architectural l2 cache operations
 */

TEXT _l2cacheuwb(SB), $-4
	MOVW.W	R14, -8(R13)
	MOVW	$cachedwb_sw(SB), R0
	MOVW	$2, R8
	BL	wholecache(SB)
	MOVW.P	8(R13), R15	/* return */

TEXT _l2cacheuwbinv(SB), $-4
	MOVW.W	R14, -8(R13)
	MOVW	CPSR, R1
	CPSID			/* splhi */

	MOVM.DB.W [R1], (R13)	/* save R1 on stack */

	MOVW	$cachedwbinv_sw(SB), R0
	MOVW	$2, R8
	BL	wholecache(SB)

	BL	_l2cacheuinv(SB)

	MOVM.IA.W (R13), [R1]	/* restore R1 (saved CPSR) */
	MOVW	R1, CPSR
	MOVW.P	8(R13), R15	/* return */

TEXT _l2cacheuinv(SB), $-4
	MOVW.W	R14, -8(R13)
	MOVW	$cachedinv_sw(SB), R0
	MOVW	$2, R8
	BL	wholecache(SB)
	MOVW.P	8(R13), R15	/* return */

/*
 * callers are assumed to be the above l1 and l2 ops.
 * R0 is the function to call in the innermost loop.
 * R8 is the cache level (1-origin: 1 or 2).
 *
 * R0	func to call at entry
 * R1	func to call after entry
 * R2	nsets
 * R3	way shift (computed from R8)
 * R4	set shift (computed from R8)
 * R5	nways
 * R6	set scratch
 * R7	way scratch
 * R8	cache level, 0-origin
 * R9	extern reg up
 * R10	extern reg m
 *
 * initial translation by 5c, then massaged by hand.
 */
TEXT wholecache+0(SB), $-4
	MOVW	CPSR, R2
	MOVM.DB.W [R2,R14], (SP) /* save regs on stack */

	MOVW	R0, R1		/* save argument for inner loop in R1 */
	SUB	$1, R8		/* convert cache level to zero origin */

	/* we might not have the MMU on yet, so map R1 (func) to R14's space */
	MOVW	R14, R0		/* get R14's segment ... */
	AND	$KSEGM, R0
	BIC	$KSEGM,	R1	/* strip segment from func address */
	ORR	R0, R1		/* combine them */

	/* get cache sizes */
	SLL	$1, R8, R0	/* R0 = (cache - 1) << 1 */
	MTCP	CpSC, CpIDcssel, R0, C(CpID), C(CpIDidct), 0 /* set cache select */
	ISB
	MFCP	CpSC, CpIDcsize, R0, C(CpID), C(CpIDidct), 0 /* get cache sizes */

	/* compute # of ways and sets for this cache level */
	SRA	$3, R0, R5	/* R5 (ways) = R0 >> 3 */
	AND	$((1<<10)-1), R5 /* R5 = (R0 >> 3) & MASK(10) */
	ADD	$1, R5		/* R5 (ways) = ((R0 >> 3) & MASK(10)) + 1 */

	SRA	$13, R0, R2	/* R2 = R0 >> 13 */
	AND	$((1<<15)-1), R2 /* R2 = (R0 >> 13) & MASK(15) */
	ADD	$1, R2		/* R2 (sets) = ((R0 >> 13) & MASK(15)) + 1 */

	/* precompute set/way shifts for inner loop */
	MOVW	$(CACHECONF+0), R3	/* +0 = l1waysh */
	MOVW	$(CACHECONF+4), R4	/* +4 = l1setsh */
	CMP	$0, R8		/* cache == 1? */
	ADD.NE	$(4*2), R3	/* no, assume l2: +8 = l2waysh */
	ADD.NE	$(4*2), R4	/* +12 = l2setsh */

	MOVW	R14, R0		/* get R14's segment ... */
	AND	$KSEGM, R0

	BIC	$KSEGM,	R3	/* strip segment from address */
	ORR	R0, R3		/* combine them */
	BIC	$KSEGM,	R4	/* strip segment from address */
	ORR	R0, R4		/* combine them */
	MOVW	(R3), R3
	MOVW	(R4), R4

	CMP	$0, R3		/* sanity checks */
	BEQ	wbuggery
	CMP	$0, R4
	BEQ	sbuggery

	CPSID			/* splhi to make entire op atomic */
	BARRIERS

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

	BL	(R1)		/* call set/way operation with R0 arg. */

	ADD	$1, R6		/* set++ */
	CMP	R2, R6		/* set >= sets? */
	BLT	inner		/* no, do next set */

	ADD	$1, R7		/* way++ */
	CMP	R5, R7		/* way >= ways? */
	BLT	outer		/* no, do next way */

	MOVM.IA.W (SP), [R2,R14] /* restore regs */
	BARRIERS
	MOVW	R2, CPSR	/* splx */

	RET

wbuggery:
	PUTC('?')
	PUTC('c')
	PUTC('w')
	B	topanic
sbuggery:
	PUTC('?')
	PUTC('c')
	PUTC('s')
topanic:
	MOVW	$.string<>+0(SB), R0
	BIC	$KSEGM,	R0	/* strip segment from address */
	MOVW	R14, R1		/* get R14's segment ... */
	AND	$KSEGM, R1
	ORR	R1, R0		/* combine them */
	SUB	$12, R13	/* not that it matters, since we're panicing */
	MOVW	R14, 8(R13)
	BL	panic(SB)	/* panic("msg %#p", LR) */
bugloop:
	WFI
	B	bugloop

	DATA	.string<>+0(SB)/8,$"bad cach"
	DATA	.string<>+8(SB)/8,$"e params"
	DATA	.string<>+16(SB)/8,$"\073 pc %\043p"
	DATA	.string<>+24(SB)/1,$"\z"
	GLOBL	.string<>+0(SB),$25
