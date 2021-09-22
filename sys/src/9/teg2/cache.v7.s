/*
 * cortex arm arch v7 cache flushing and invalidation.
 * included by l.s and rebootcode.s.
 */

TEXT cacheiinv(SB), $-4				/* I invalidate */
	MOVW	$0, R0
	MTCP	CpSC, 0, R0, C(CpCACHE), C(CpCACHEinviis), CpCACHEall
	BARRIERS
	RET

/*
 * set/way operators, passed a suitable set/way value in R0.
 * not required by arm to work on (architected) L2 and beyond.
 *
 * these are unrolled inner loops of wholecache, and the way value
 * in R0 will be incremented by 4 upon return.  R5 is assumed to contain
 * the way increment.
 */
TEXT cachedwb_sw(SB), $-4
	MTCP	CpSC, 0, R0, C(CpCACHE), C(CpCACHEwb), CpCACHEsi
	ADD	R5, R0			/* next way */
	MTCP	CpSC, 0, R0, C(CpCACHE), C(CpCACHEwb), CpCACHEsi
	ADD	R5, R0			/* next way */
	MTCP	CpSC, 0, R0, C(CpCACHE), C(CpCACHEwb), CpCACHEsi
	ADD	R5, R0			/* next way */
	MTCP	CpSC, 0, R0, C(CpCACHE), C(CpCACHEwb), CpCACHEsi
	ADD	R5, R0			/* next way */
	RET

TEXT cachedwbinv_sw(SB), $-4
	MTCP	CpSC, 0, R0, C(CpCACHE), C(CpCACHEwbi), CpCACHEsi
	ADD	R5, R0			/* next way */
	MTCP	CpSC, 0, R0, C(CpCACHE), C(CpCACHEwbi), CpCACHEsi
	ADD	R5, R0			/* next way */
	MTCP	CpSC, 0, R0, C(CpCACHE), C(CpCACHEwbi), CpCACHEsi
	ADD	R5, R0			/* next way */
	MTCP	CpSC, 0, R0, C(CpCACHE), C(CpCACHEwbi), CpCACHEsi
	ADD	R5, R0			/* next way */
	RET

TEXT cachedinv_sw(SB), $-4
	MTCP	CpSC, 0, R0, C(CpCACHE), C(CpCACHEinvd), CpCACHEsi
	ADD	R5, R0			/* next way */
	MTCP	CpSC, 0, R0, C(CpCACHE), C(CpCACHEinvd), CpCACHEsi
	ADD	R5, R0			/* next way */
	MTCP	CpSC, 0, R0, C(CpCACHE), C(CpCACHEinvd), CpCACHEsi
	ADD	R5, R0			/* next way */
	MTCP	CpSC, 0, R0, C(CpCACHE), C(CpCACHEinvd), CpCACHEsi
	ADD	R5, R0			/* next way */
	RET


	/* set cache size select */
TEXT setcachelvl(SB), $-4
	MTCP	CpSC, CpIDcssel, R0, C(CpID), C(CpIDidct), CpIDid
	DSB
	ISB
	RET

	/* return cache sizes */
TEXT getwayssets(SB), $-4
	MFCP	CpSC, CpIDcsize, R0, C(CpID), C(CpIDidct), CpIDid
	RET

/*
 * l1 cache operations.  not needed often since CpACsmpcoher on triggers
 * hardware l1 cache coherency.
 * l1 and l2 ops are intended to be called from C, thus need save no
 * caller's regs, only those we need to preserve across calls.
 */

TEXT cachedwb(SB), $-4
	MOVW	$cachedwb_sw(SB), R0
_wholel1:
	MOVW.W	R14, -8(SP)
	MOVW	$1, R8
	BL	wholecache(SB)
	MOVW.P	8(SP), PC		/* return via saved LR */

TEXT cachedwbinv(SB), $-4
	MOVW	$cachedwbinv_sw(SB), R0
	B	_wholel1

TEXT cachedinv(SB), $-4
	MOVW	$cachedinv_sw(SB), R0
	B	_wholel1

TEXT cacheuwbinv(SB), $-4
	MOVM.DB.W [R14], (SP)	/* save lr on stack */
	MOVW	CPSR, R1
	CPSID			/* splhi */

	MOVM.DB.W [R1], (SP)	/* save R1 on stack */

	BL	cachedwbinv(SB)
	BL	cacheiinv(SB)

	MOVM.IA.W (SP), [R1]	/* restore R1 (saved CPSR) */
	MOVW	R1, CPSR
	BARRIERS
	MOVM.IA.W (SP), [R14]	/* restore lr */
	RET

#ifdef NOT_TRIMSLICE
/*
 * architectural l2 cache operations
 * unused on trimslice.
 */

TEXT _l2cacheuwb(SB), $-4
	MOVW	$cachedwb_sw(SB), R0
_wholel2:
	MOVW.W	R14, -8(SP)
	MOVW	$2, R8
	BL	wholecache(SB)
	MOVW.P	8(SP), PC	/* return via saved LR */

TEXT _l2cacheuinv(SB), $-4
	MOVW	$cachedinv_sw(SB), R0
	B	_wholel2

TEXT _l2cacheuwbinv(SB), $-4
	MOVW.W	R14, -8(SP)
	MOVW	CPSR, R1
	CPSID			/* splhi */

	MOVM.DB.W [R1], (SP)	/* save R1 on stack */

	MOVW	$cachedwbinv_sw(SB), R0
	MOVW	$2, R8
	BL	wholecache(SB)

	BL	_l2cacheuinv(SB)

	MOVM.IA.W (SP), [R1]	/* restore R1 (saved CPSR) */
	MOVW	R1, CPSR
	BARRIERS
	MOVW.P	8(SP), PC	/* return via saved LR */
#endif

/*
 * perform some operation on an entire cache.
 *
 * callers are assumed to be the above l1 and l2 ops.
 * R0 is the function to call as the innermost (unrolled) loop.
 * R8 is the cache level (1-origin: 1 or 2).
 *
 * R0	func to call, at entry
 * R1	func to call, after entry
 * R2	CPSR at entry
 * R3	way shift (computed from R8)
 * R4	set shift (computed from R8)
 * R5	way increment for set/way register
 * R6	(maximum) set number
 * R7	way scratch
 * R8	cache level, 0-origin
 * R9	extern reg up
 * R10	extern reg m
 *
 * initial translation to assembler by 5c -S, then massaged by hand.
 */
TEXT wholecache(SB), $-4
	MOVW	CPSR, R2
	MOVM.DB.W [R14], (SP)	/* save regs on stack */

	MOVW	R0, R1		/* save argument for inner loop in R1 */
	SUB	$1, R8		/* convert cache level to zero origin */

	/* we might not have the MMU on yet, so map R1 (func) to R14's space */
	MOVW	R14, R0		/* get R14's segment ... */
	AND	$KSEGM, R0
	BIC	$KSEGM,	R1	/* strip segment from func address */
	ORR	R0, R1		/* combine them */

	/* get cache sizes */
	SLL	$1, R8, R0	/* R0 = (cache - 1) << 1 */
	/* select cache */
	MTCP	CpSC, CpIDcssel, R0, C(CpID), C(CpIDidct), CpIDid
	ISB
	MFCP	CpSC, CpIDcsize, R0, C(CpID), C(CpIDidct), CpIDid

	/* compute # of ways and sets for this cache level */
#ifdef NOT_CORTEX_A9
	SRA	$3, R0, R5	/* R5 (ways) = R0 >> 3 */
	AND	$((1<<10)-1), R5 /* R5 = (R0 >> 3) & MASK(10) */
	ADD	$1, R5		/* R5 (ways) = ((R0 >> 3) & MASK(10)) + 1 */
#endif

	SRA	$13, R0, R6	/* R6 = R0 >> 13 */
	AND	$((1<<15)-1), R6 /* R6 (max set) = (R0 >> 13) & MASK(15) */

	/* precompute set/way shifts for inner loop */
	MOVW	$(CACHECONF+0), R3	/* +0 = l1waysh */
	MOVW	$(CACHECONF+4), R4	/* +4 = l1setsh */
	CMP	$0, R8		/* cache == 1? */
	ADD.NE	$(4*2), R3	/* no, assume l2: +4*2 = l2waysh */
	ADD.NE	$(4*2), R4	/* +4*3 = l2setsh */

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

	/* precompute set/way register contents.  way is always 0 on a9. */
	MOVW	R8<<1, R7	/* R7 = (way=0) << L?WAYSH | (cache - 1) << 1 */

	MOVW	$1, R5		/* make R5 `increment by 1 way in set/way' */
	SLL	R3, R5	 	/* R5 = 1 << R3 (L?WAYSH) */

	CPSID			/* splhi to make entire op atomic */

	/* iterate over sets */
nextset:
	/*
	 * flush/invalidate all ways of this set (R6).
	 * optimisation: cortex-a9 L1 caches are fixed at 4 ways,
	 * so func at R1 operates on 4 consecutive ways at a time.
	 * call set/way operation with R0 arg. and R5 set/way increment.
	 * R0 will be incremented by the number of operations performed (4).
	 *
	 * also, the number of sets and ways will always be a power of two,
	 * so we can unroll the outer loop a bit.
	 */

	/* compute set/way register contents */
	ORR	R6<<R4, R7, R0 	/* R0 = way<<L?WAYSH | (cache-1)<<1 | set<<R4 */
	BL	(R1)
	SUB	$1, R6		/* set-- */

	ORR	R6<<R4, R7, R0 	/* R0 = way<<L?WAYSH | (cache-1)<<1 | set<<R4 */
	BL	(R1)
	SUB	$1, R6		/* set-- */

	ORR	R6<<R4, R7, R0 	/* R0 = way<<L?WAYSH | (cache-1)<<1 | set<<R4 */
	BL	(R1)
	SUB	$1, R6		/* set-- */

	ORR	R6<<R4, R7, R0 	/* R0 = way<<L?WAYSH | (cache-1)<<1 | set<<R4 */
	BL	(R1)
	SUB.S	$1, R6		/* set-- */

	BGE	nextset		/* set >= 0? more */

	MOVW	R2, CPSR	/* splx */
	BARRIERS

	MOVM.IA.W (SP), [R14]	/* restore regs */
	RET

wbuggery:
	PUTC('?'); PUTC('c'); PUTC('w')
	B	topanic
sbuggery:
	PUTC('?'); PUTC('c'); PUTC('s')
topanic:
	MOVW	$.string<>+0(SB), R0
	BIC	$KSEGM,	R0	/* strip segment from address */
	MOVW	R14, R1		/* get R14's segment ... */
	AND	$KSEGM, R1
	ORR	R1, R0		/* combine them */
	SUB	$(3*BY2WD), SP	/* not that it matters, since we're panicking */
	MOVW	R14, 8(SP)
	BL	panic(SB)	/* panic("msg %#p", LR) */
bugloop:
	BARRIERS
	WFI
	B	bugloop

	DATA	.string<>+0(SB)/8,$"bad cach"
	DATA	.string<>+8(SB)/8,$"e params"
	DATA	.string<>+16(SB)/8,$"\073 pc %\043p"
	DATA	.string<>+24(SB)/1,$"\z"
	GLOBL	.string<>+0(SB),$25
