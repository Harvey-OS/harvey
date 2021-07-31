/*
 * arm arch v5 l1 cache flushing and invalidation
 * shared by l.s and rebootcode.s
 */

/*
 * set/way operators
 */

TEXT cache1dwb_sw(SB), 1, $-4
	MCR	CpSC, 0, R0, C(CpCACHE), C(CpCACHEwb), CpCACHEsi
	RET

TEXT cache1dwbinv_sw(SB), 1, $-4
	MCR	CpSC, 0, R0, C(CpCACHE), C(CpCACHEwbi), CpCACHEsi
	RET

TEXT cache1dinv_sw(SB), 1, $-4
	MCR	CpSC, 0, R0, C(CpCACHE), C(CpCACHEinvd), CpCACHEsi
	RET

	/* return cache sizes */
//TEXT getwayssets(SB), 1, $-4
//	MRC	CpSC, CpIDcsize, R0, C(CpID), C(CpIDid), 0
//	RET

TEXT cache1dwb(SB), 1, $-4
	MOVW.W	R14, -8(R13)
	MOVW	$cache1dwb_sw(SB), R0
	BL	cache1all(SB)
	MOVW.P	8(R13), R15

TEXT cache1dwbinv(SB), 1, $-4
	MOVW.W	R14, -8(R13)
	MOVW	$cache1dwbinv_sw(SB), R0
	BL	cache1all(SB)
	MOVW.P	8(R13), R15

TEXT cache1dinv(SB), 1, $-4
	MOVW.W	R14, -8(R13)
	MOVW	$cache1dinv_sw(SB), R0
	BL	cache1all(SB)
	MOVW.P	8(R13), R15

TEXT cache1uwbinv(SB), 1, $-4
	MOVM.DB.W [R14], (R13)	/* save lr on stack */
	MOVW	CPSR, R1
	ORR	$(PsrDirq|PsrDfiq), R1, R0
	MOVW	R0, CPSR	/* splhi */
	BARRIERS

	BL	cache1dwbinv(SB)
	BL	cacheiinv(SB)

	MOVW	R1, CPSR
	BARRIERS
	MOVM.IA.W (R13), [R14]	/* restore lr */
	RET

/*
 * these shift values are for the arm926 L1 cache (A=2, L=5, S=7).
 * A = log2(# of ways), L = log2(bytes per cache line), S = log2(# sets).
 * see arm926ejs ref p. 104 (4-10).
 */
#define L1WAYSH 30		/* 32-A */
#define L1SETSH 5		/* L */

#define VARSTACK (8*4)		/* generous stack space for local variables */

/* argument in R0 is the function to call in the innermost loop */
TEXT cache1all+0(SB), 1, $-4
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
	ORR	R2, R1		/* combine them */

	/* drain write buffers */
	BARRIERS

	/* iterate over cache levels (just l1 in this case) */
	MOVW	$1, R8		/* R8: cache */
	B	outtest
outer:
	ADD	$1, R8		/* cache++ */
outtest:
	CMP	$1, R8		/* cache > 1? */
	BGT	ret		/* then done */

	SUB	$1, R8, R0	/* R0 = cache - 1 */
	SLL	$1, R0		/* R0 = (cache - 1) << 1 */

	/* sheeva l1: 128 (2^S) sets of 4 (2^A) ways */
	MOVW	$128, R5
	MOVW	R5, sets-12(SP)
	MOVW	$4, R2
	MOVW	R2, ways-20(SP)

	/* force writes to stack out to dram, in case op is pure inv. */
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

	/* iterate over ways */
	MOVW	$0, R7		/* R7: way */
	B	midtest
middle:
	ADD	$1, R7		/* way++ */
midtest:
	MOVW	ways-20(SP), R2	/* R2: ways */
	CMP	R2, R7		/* way >= ways? */
	BGE	outer		/* then back to outer loop reinit. */

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
	SUB	$1, R8, R4	/* R4 = cache - 1 (0) */
	SLL	$L1WAYSH, R7, R2 /* yes: R2 = way << L1WAYSH */
	ORR	R4<<1, R2, R4	/* R4 = way << L?WAYSH | (cache - 1) << 1 */
	ORR	R6<<L1SETSH, R4, R0 /* R0=way<<L1WAYSH|(cache-1)<<1|set<<L1SETSH */

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

	MOVM.IA.W (R13), [R0]	/* pop CPSR */
	MOVM.IA.W (R13), [R14,R1-R8] /* restore regs */
	MOVW	R0, CPSR
	BARRIERS
	RET
