/*
 * cortex arm arch v7 cache flushing and invalidation
 * shared by l.s and rebootcode.s
 */

TEXT cacheiinv(SB), 1, $-4			/* I invalidate */
	MOVW	$0, R0
	MCR	CpSC, 0, R0, C(CpCACHE), C(CpCACHEinvi), CpCACHEall /* ok on cortex */
	BARRIERS
	RET

/*
 * set/way operators
 */

TEXT cachedwb_sw(SB), 1, $-4
	MCR	CpSC, 0, R0, C(CpCACHE), C(CpCACHEwb), CpCACHEsi
	RET

TEXT cachedwbinv_sw(SB), 1, $-4
	MCR	CpSC, 0, R0, C(CpCACHE), C(CpCACHEwbi), CpCACHEsi
	RET

TEXT cachedinv_sw(SB), 1, $-4
	MCR	CpSC, 0, R0, C(CpCACHE), C(CpCACHEinvd), CpCACHEsi
	RET

	/* set cache size select */
TEXT setcachelvl(SB), 1, $-4
	MCR	CpSC, CpIDcssel, R0, C(CpID), C(CpIDid), 0
	BARRIERS
	RET

	/* return cache sizes */
TEXT getwayssets(SB), 1, $-4
	MRC	CpSC, CpIDcsize, R0, C(CpID), C(CpIDid), 0
	RET

/*
 * l1 cache operations
 */

TEXT cachedwb(SB), 1, $-4
	MOVW.W	R14, -8(R13)
	MOVW	$cachedwb_sw(SB), R0
	MOVW	$1, R8
	BL	cacheall(SB)
	MOVW.P	8(R13), R15

TEXT cachedwbinv(SB), 1, $-4
	MOVW.W	R14, -8(R13)
	MOVW	$cachedwbinv_sw(SB), R0
	MOVW	$1, R8
	BL	cacheall(SB)
	MOVW.P	8(R13), R15

TEXT cachedinv(SB), 1, $-4
	MOVW.W	R14, -8(R13)
	MOVW	$cachedinv_sw(SB), R0
	MOVW	$1, R8
	BL	cacheall(SB)
	MOVW.P	8(R13), R15

TEXT cacheuwbinv(SB), 1, $-4
	MOVM.DB.W [R14], (R13)	/* save lr on stack */
	MOVW	CPSR, R1
	ORR	$(PsrDirq|PsrDfiq), R1, R0
	MOVW	R0, CPSR	/* splhi */
	BARRIERS

	BL	cachedwbinv(SB)
	BL	cacheiinv(SB)

	MOVW	R1, CPSR
	BARRIERS
	MOVM.IA.W (R13), [R14]	/* restore lr */
	RET

/*
 * l2 cache operations
 */

TEXT l2cacheuwb(SB), 1, $-4
	MOVW.W	R14, -8(R13)
	MOVW	$cachedwb_sw(SB), R0
	MOVW	$2, R8
	BL	cacheall(SB)
	MOVW.P	8(R13), R15

TEXT l2cacheuwbinv(SB), 1, $-4
	MOVW.W	R14, -8(R13)
	MOVW	CPSR, R1
	ORR	$(PsrDirq|PsrDfiq), R1, R0
	MOVW	R0, CPSR	/* splhi */
	BARRIERS

	MOVW	$cachedwbinv_sw(SB), R0
	MOVW	$2, R8
	BL	cacheall(SB)
	BL	l2cacheuinv(SB)

	MOVW	R1, CPSR
	BARRIERS
	MOVW.P	8(R13), R15

TEXT l2cacheuinv(SB), 1, $-4
	MOVW.W	R14, -8(R13)
	MOVW	$cachedinv_sw(SB), R0
	MOVW	$2, R8
	BL	cacheall(SB)
	MOVW.P	8(R13), R15

/*
 * initial translation by 5c, then massaged by hand.
 */

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

#define VARSTACK (8*4)		/* generous stack space for local variables */

/* first argument (in R0) is the function to call in the innermost loop */
/* second argument (in R8 when called from assembler) is cache level */
TEXT cacheall+0(SB), 1, $-4
//	MOVW	lvl+4(FP), R8	/* cache level */
	MOVM.DB.W [R14,R1-R8], (R13) /* save regs on stack */
	MOVW	R0, R1		/* save argument for inner loop in R1 */

	MOVW	CPSR, R0
	MOVM.DB.W [R0], (R13)	/* push CPSR */

	ORR	$(PsrDirq|PsrDfiq), R0
	MOVW	R0, CPSR	/* splhi */
	BARRIERS

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
	BARRIERS

	SUB	$1, R8, R0	/* R0 = cache - 1 */
	SLL	$1, R0		/* R0 = (cache - 1) << 1 */

	/* set cache size select */
	MCR	CpSC, CpIDcssel, R0, C(CpID), C(CpIDid), 0
	BARRIERS

	MRC	CpSC, CpIDcsize, R0, C(CpID), C(CpIDid), 0 /* get cache sizes */

	/* compute # of ways and sets for this cache level */
	SRA	$3, R0, R5	/* R5 (ways) = R0 >> 3 */
	AND	$1023, R5	/* R5 = (R0 >> 3) & MASK(10) */
	ADD	$1, R5		/* R5 = ((R0 >> 3) & MASK(10)) + 1 */
	MOVW	R5, ways-20(SP)	/* ways = ((R0 >> 3) & MASK(10)) + 1 */

	SRA	$13, R0, R3	/* R3 (sets) = R0 >> 13 */
	AND	$32767, R3	/* R3 = (R0 >> 13) & MASK(15) */
	ADD	$1, R3		/* R3 = ((R0 >> 13) & MASK(15)) + 1 */
	MOVW	R3, sets-12(SP)	/* sets = ((R0 >> 13) & MASK(15)) + 1 */

	/* force writes to stack out to dram */
	BARRIERS
	MOVW	R13, R0
	SUB	$VARSTACK, R0	/* move down past local variables (ways) */
	BIC	$(CACHELINESZ-1), R0
	MCR	CpSC, 0, R0, C(CpCACHE), C(CpCACHEwb), CpCACHEse
	ADD	$CACHELINESZ, R0
	MCR	CpSC, 0, R0, C(CpCACHE), C(CpCACHEwb), CpCACHEse
	ADD	$CACHELINESZ, R0
	MCR	CpSC, 0, R0, C(CpCACHE), C(CpCACHEwb), CpCACHEse
	ADD	$CACHELINESZ, R0
	MCR	CpSC, 0, R0, C(CpCACHE), C(CpCACHEwb), CpCACHEse
	BARRIERS
	MCR	CpSC, 0, R0, C(CpCACHE), C(CpCACHEwb), CpCACHEwait
	BARRIERS

	/* iterate over ways */
	MOVW	$0, R7		/* R7: way */
	B	midtest
middle:
	ADD	$1, R7		/* way++ */
midtest:
	MOVW	ways-20(SP), R2	/* R2: ways */
	CMP	R2, R7		/* way >= ways? */
	BGE	ret		/* then done */

	/* iterate over sets */
	MOVW	$0, R6		/* R6: set */
	B	intest
inner:
	ADD	$1, R6		/* set++ */
intest:
	MOVW	sets-12(SP), R5	/* R5: sets */
	CMP	R5, R6		/* set >= sets? */
	BGE	middle		/* then back to reinit of previous loop */

	/* compute set/way register contents */
	SUB	$1, R8, R4	/* R4 = cache - 1 */
	CMP	$1, R8		/* cache == 1? */
	SLL.EQ	$L1WAYSH, R7, R2 /* yes: R2 = way << L1WAYSH */
	SLL.NE	$L2WAYSH, R7, R2 /* no:  R2 = way << L2WAYSH */
	ORR	R4<<1, R2, R4	/* R4 = way << L?WAYSH | (cache - 1) << 1 */
	ORR.EQ	R6<<L1SETSH, R4, R0 /* R0=way<<L1WAYSH|(cache-1)<<1|set<<L1SETSH */
	ORR.NE	R6<<L2SETSH, R4, R0 /* R0=way<<L2WAYSH|(cache-1)<<1|set<<L2SETSH */

	SUB	$VARSTACK, R13	/* move sp down past local variables (ways) */
	/* must not use addresses relative to (SP) from here */
	BL	(R1)		/* call asm to do something with R0 */
	BARRIERS		/* make sure it has executed */
	ADD	$VARSTACK, R13	/* restore sp */
	/* may again use addresses relative to (SP) from here */

	B	inner

ret:
	/* drain write buffers */
	BARRIERS
	MCR	CpSC, 0, R0, C(CpCACHE), C(CpCACHEwb), CpCACHEwait
	BARRIERS

	MOVM.IA.W (R13), [R0]	/* pop CPSR */
	MOVM.IA.W (R13), [R14,R1-R8] /* restore regs */
	MOVW	R0, CPSR
	BARRIERS
	RET

buggery:
WAVE('?')
	MOVW	PC, R0
//	B	pczeroseg(SB)
	RET
