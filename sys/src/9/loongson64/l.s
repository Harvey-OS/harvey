#include "mem.h"
#include "spim64.s"

/*
 *	entrypoint. set SB, pass arguments to main().
 *	PMON's calling convention:
 *		argc	R4
 *		argv	R5
 *		envp	R6
 *		callvec	R7
 */

TEXT	start(SB), $-8
	MOVV	$setR30(SB), R30

	PUTC('9', R1, R2)

	/* don't enable any interrupts, out of EXL mode */
	MOVW	$(CU1|KX|UX), R1
	MOVW	R1, M(STATUS)
	EHB
	MOVW	R0, M(CAUSE)
	EHB
	MOVW	R0, M(COMPARE)
	EHB
	MOVW	R0, M(PERFCTL)
	EHB

	MOVV	R4, _argc(SB)
	MOVV	R5, _argv(SB)
	MOVV	R6, _env(SB)
	MOVV	R7, pmon_callvec(SB)

	MOVW    $(FPOVFL|FPZDIV|FPINVAL|FPFLUSH), R1
	MOVW    R1, FCR31		// permit only inexact and underflow
	NOP
	MOVD	$0.5, F26
	SUBD	F26, F26, F24
	ADDD	F26, F26, F28
	ADDD	F28, F28, F30

	MOVD	F24, F0
	MOVD	F24, F2
	MOVD	F24, F4
	MOVD	F24, F6
	MOVD	F24, F8
	MOVD	F24, F10
	MOVD	F24, F12
	MOVD	F24, F14
	MOVD	F24, F16
	MOVD	F24, F18
	MOVD	F24, F20
	MOVD	F24, F22

	MOVW	$TLBROFF, R1
	MOVW	R1, M(WIRED)
	EHB
	MOVV	R0, M(CONTEXT)
	EHB
	MOVV	R0, M(XCONTEXT)
	EHB

	/* set KSEG0 cachability before trying LL/SC in lock code */
	TOKSEG1(11)
	MOVW	M(CONFIG), R1
	AND		$(~CFG_K0), R1
	/* make kseg0 cachable */
	OR		$(PTECACHABILITY>>3), R1
	MOVW	R1, M(CONFIG)
	EHB
	TOKSEG0(11)

	MOVV	$MACHADDR, R(MACH)
	ADDVU	$(MACHSIZE-BY2WD), R(MACH), R29	/* set up stack */

	JAL		main(SB)

	/* main() returned */;
	PUTC('\r', R1, R2);	DELAY(R2);	PUTC('\n', R1, R2);	DELAY(R2);
	PUTC('m', R1, R2);	DELAY(R2);	PUTC('a', R1, R2);	DELAY(R2);
	PUTC('i', R1, R2);	DELAY(R2);	PUTC('n', R1, R2);	DELAY(R2);
	PUTC('(', R1, R2);	DELAY(R2);	PUTC(')', R1, R2);	DELAY(R2);
	PUTC(' ', R1, R2);	DELAY(R2);	PUTC('r', R1, R2);	DELAY(R2);
	PUTC('e', R1, R2);	DELAY(R2);	PUTC('t', R1, R2);	DELAY(R2);
	PUTC('u', R1, R2);	DELAY(R2);	PUTC('r', R1, R2);	DELAY(R2);
	PUTC('n', R1, R2);	DELAY(R2);	PUTC('e', R1, R2);	DELAY(R2);
	PUTC('d', R1, R2);	DELAY(R2);	PUTC('\r', R1, R2);	DELAY(R2);
	PUTC('\n', R1, R2);
	JMP		0(PC)		/* loop */

/* target for JALR in TOKSEG0/1 */
TEXT	ret0(SB), $-8
	AND		$~KSEG1, R22
	OR		$KSEG0, R22
	JMP		(R22)

TEXT	ret1(SB), $-8
	OR		$KSEG1, R22
	JMP		(R22)

/* print R1 in hex; clobbers R3—8 */
TEXT	printhex(SB), $-8
	MOVW	$64, R5
	MOVW	$9, R7
prtop:
	SUB		$4, R5
	MOVV	R1, R6
	SRLV	R5, R6
	AND		$0xf, R6
	SGTU	R6, R7, R8
	BEQ		R8, prdec		/* branch if R6 <= 9 */
	ADD		$('a'-10), R6
	JMP		prchar
prdec:
	ADD		$'0', R6
prchar:
	PUTC(R6, R3, R4)
	DELAY(R4)
	BNE		R5, prtop
	RET

/*
 * Take first processor into user mode
 * 	- argument is stack pointer to user
 */
TEXT	touser(SB), $-8
	MOVV	R1, R29
	MOVW	$(UTZERO+32), R2	/* header appears in text */
	MOVW	R0, M(CAUSE)
	EHB

	MOVW	M(STATUS), R4
	AND		$(~KMODEMASK), R4
	OR		$(KUSER|IE|EXL), R4	/* switch to user mode, intrs on, exc */
	MOVW	R4, M(STATUS)

	MOVV	R2, M(EPC)
	ERET				/* clears EXL */

TEXT	_loop(SB), $-8
	MOVW	M(STATUS), R1
	JAL		printhex(SB)
	JMP		0(PC)

/*
 * manipulate interrupts
 */

/* enable an interrupt; bit is in R1 */
TEXT	intron(SB), $0
	MOVW	M(STATUS), R2
	OR		R1, R2
	MOVW	R2, M(STATUS)
	EHB
	RET

/* disable an interrupt; bit is in R1 */
TEXT	introff(SB), $0
	MOVW	M(STATUS), R2
	XOR		$-1, R1
	AND		R1, R2
	MOVW	R2, M(STATUS)
	EHB
	RET

TEXT	splhi(SB), $0
	MOVV	R31, 24(R(MACH))	/* save PC in m->splpc */
	MOVW	M(STATUS), R1
	EHB
	AND		$(~IE), R1, R2
	MOVW	R2, M(STATUS)
	EHB
	RET

TEXT	splx(SB), $0
	MOVW	R31, 24(R(MACH))	/* save PC in m->splpc */
	MOVW	M(STATUS), R2
	EHB
	AND		$IE, R1
	AND		$(~IE), R2
	OR		R2, R1
	MOVW	R1, M(STATUS)
	EHB
	RET

TEXT	spllo(SB), $0
	MOVW	M(STATUS), R1
	EHB
	OR		$IE, R1, R2
	MOVW	R2, M(STATUS)
	EHB
	RET

TEXT	spldone(SB), $0
	RET

TEXT	islo(SB), $0
	MOVW	M(STATUS), R1
	AND		$IE, R1
	RET

TEXT	coherence(SB), $-8
	SYNC
	NOP
	NOP
	RET

/*
 * process switching
 */

TEXT	setlabel(SB), $-8
	MOVV	R29, 0(R1)		/* sp */
	MOVV	R31, 8(R1)		/* pc */
	MOVV	R0, R1
	RET

TEXT	gotolabel(SB), $-8
	MOVV	0(R1), R29		/* sp */
	MOVV	8(R1), R31		/* pc */
	MOVW	$1, R1
	RET

TEXT	machstatus(SB), $0
	MOVW	M(STATUS), R1
	RET

TEXT	getstatus(SB), $0
	MOVW	M(STATUS), R1
	RET

TEXT	setstatus(SB), $0
	MOVW	R1, M(STATUS)
	EHB
	RET

TEXT	rdcount(SB), $0
	MOVW	M(COUNT), R1
	RET

TEXT	wrcount(SB), $0
	MOVW	R1, M(COUNT)
	EHB
	RET

TEXT	wrcompare(SB), $0
	MOVW	R1, M(COMPARE)
	EHB
	RET

TEXT	rdcompare(SB), $0
	MOVW	M(COMPARE), R1
	RET

TEXT	prid(SB), $0
	MOVW	M(PRID), R1
	RET

TEXT	getconfig(SB), $0
	MOVW	M(CONFIG), R1
	RET

TEXT	getcause(SB), $0
	MOVW	M(CAUSE), R1
	RET

/*
 * the tlb routines need to be called at splhi.
 */

TEXT	puttlb(SB), $0			/* puttlb(virt, phys0, phys1) */
	MOVV	R1, M(TLBVIRT)
	EHB
	MOVV	8(FP), R2		/* phys0 */
	MOVV	16(FP), R3		/* phys1 */
	MOVV	R2, M(TLBPHYS0)
	EHB
	MOVW	$PGSZ, R1
	MOVV	R3, M(TLBPHYS1)
	EHB
	MOVW	R1, M(PAGEMASK)
	EHB
	OR		R2, R3, R4
	AND		$PTEVALID, R4
	TLBP				/* tlb probe */
	EHB
	MOVW	M(INDEX), R1
	BGEZ	R1, index		/* if tlb entry found, use it */
	BEQ		R4, dont		/* not valid? cf. kunmap */
	MOVW	M(RANDOM), R1	/* write random tlb entry */
	MOVW	R1, M(INDEX)
	EHB
index:
	TLBWI				/* write indexed tlb entry */
	EHB
dont:
	RET

TEXT	getwired(SB),$0
	MOVW	M(WIRED), R1
	RET

TEXT	setwired(SB),$0
	MOVW	R1, M(WIRED)
	EHB
	RET

TEXT	getrandom(SB),$0
	MOVW	M(RANDOM), R1
	RET

TEXT	getpagemask(SB),$0
	MOVW	M(PAGEMASK), R1
	RET

TEXT	setpagemask(SB),$0
	MOVW	R1, M(PAGEMASK)
	EHB
	MOVV	R0, R1			/* prevent accidents */
	RET

TEXT	puttlbx(SB), $0		/* puttlbx(index, virt, phys0, phys1, pagemask) */
	MOVV	8(FP), R2
	MOVV	16(FP), R3
	MOVV	24(FP), R4
	MOVW	32(FP), R5
	MOVV	R2, M(TLBVIRT)
	EHB
	MOVV	R3, M(TLBPHYS0)
	EHB
	MOVV	R4, M(TLBPHYS1)
	EHB
	MOVW	R5, M(PAGEMASK)
	EHB
	MOVW	R1, M(INDEX)
	EHB
	TLBWI
	EHB
	RET

TEXT	tlbvirt(SB), $0
	MOVV	M(TLBVIRT), R1
	RET

TEXT	gettlbvirt(SB), $0		/* gettlbvirt(index) */
	MOVV	M(TLBVIRT), R10		/* save our asid */
	MOVW	R1, M(INDEX)
	EHB
	TLBR				/* read indexed tlb entry */
	EHB
	MOVV	M(TLBVIRT), R1
	MOVV	R10, M(TLBVIRT)		/* restore our asid */
	EHB
	RET

TEXT	gettlbx(SB), $0			/* gettlbx(index, &entry) */
	MOVV	8(FP), R5
	MOVV	M(TLBVIRT), R10		/* save our asid */
	MOVW	R1, M(INDEX)
	EHB
	TLBR				/* read indexed tlb entry */
	EHB
	MOVV	M(TLBVIRT), R2
	MOVV	M(TLBPHYS0), R3
	MOVV	M(TLBPHYS1), R4
	MOVV	R2, 0(R5)
	MOVV	R3, 8(R5)
	MOVV	R4, 16(R5)
	MOVV	R10, M(TLBVIRT)		/* restore our asid */
	EHB
	RET

TEXT	gettlbp(SB), $0			/* gettlbp(tlbvirt, &entry) */
	MOVV	8(FP), R5
	MOVV	M(TLBVIRT), R10		/* save our asid */
	MOVV	R1, M(TLBVIRT)
	EHB
	TLBP				/* probe tlb */
	EHB
	MOVW	M(INDEX), R1
	BLTZ	R1, gettlbp1		/* if no tlb entry found, return */
	TLBR				/* read indexed tlb entry */
	EHB
	MOVV	M(TLBVIRT), R2
	MOVV	M(TLBPHYS0), R3
	MOVV	M(TLBPHYS1), R4
	MOVW	M(PAGEMASK), R6
	MOVV	R2, 0(R5)
	MOVV	R3, 8(R5)
	MOVV	R4, 16(R5)
	MOVW	R6, 24(R5)
gettlbp1:
	MOVV	R10, M(TLBVIRT)		/* restore our asid */
	EHB
	RET

/*
 * exceptions.
 * mips promises that there will be no current hazards upon entry
 * to exception handlers.
 */

/* vector at KSEG0+0x80, simple tlb refill */
TEXT	vector0(SB), $-8
	MOVV	$utlbmiss(SB), R26
	JMP		(R26)

/*
 * compute stlb hash index.
 * must match index calculation in mmu.c/putstlb()
 *
 * M(TLBVIRT) [page & asid] in arg, result in arg.
 * stir in swizzled asid; we get best results with asid in both high & low bits.
 *
 * page = tlbvirt >> (PGSHIFT+1);	// ignoring even/odd bit
 * R27 = ((tlbvirt<<(STLBLOG-8) ^ (uchar)tlbvirt ^ page ^
 *	((page & (MASK(HIPFNBITS) << STLBLOG)) >> HIPFNBITS)) &
 *	(STLBSIZE-1)) * 12;
 */
#define STLBHASH(arg, tmp, tmp2) \
	MOVV	arg, tmp2; \
	SRLV	$(PGSHIFT+1), arg;	/* move low page # bits to low bits */ \
	CONST	((MASK(HIPFNBITS) << STLBLOG), tmp); \
	AND	arg, tmp;		/* extract high page # bits */ \
	SRLV	$HIPFNBITS, tmp;	/* position them */ \
	XOR	tmp, arg;		/* include them */ \
	MOVV	tmp2, tmp;		/* asid in low byte */ \
	SLLV	$(STLBLOG-8), tmp;	/* move asid to high bits */ \
	XOR	tmp, arg;		/* include asid in high bits too */ \
	AND	$0xff, tmp2, tmp;	/* asid in low byte */ \
	XOR	tmp, arg;		/* include asid in low bits */ \
	CONST	(STLBSIZE-1, tmp); \
	AND	tmp, arg		/* chop to fit */

TEXT	stlbhash(SB), $-8
	STLBHASH(R1, R2, R3)
	RET

TEXT	utlbmiss(SB), $-8
//	PUTC('u', R26, R27);
//	MOVV	M(BADVADDR), R1
//	EHB
//	JAL		printhex(SB)
//	JMP		0(PC)
//	NOP

	/*
	 * don't use R28 by using constants that span both word halves,
	 * it's unsaved so far.  avoid R24 (up in kernel) and R25 (m in kernel).
	 */
	/* update statistics */
	CONST	(MACHADDR, R26)		/* R26 = m-> */
	MOVW	32(R26), R27
	ADDU	$1, R27
	MOVW	R27, 32(R26)		/* m->tlbfault++ */

	MOVV	R23, M(ERROREPC)		/* save R23, M(ERROREPC) ok? */

#ifdef	KUTLBSTATS
	MOVW	M(STATUS), R23
	AND		$KUSER, R23
	BEQ		R23, kmiss

	MOVW	40(R26), R27
	ADDU	$1, R27
	MOVW	R27, 40(R26)		/* m->utlbfault++ */
	JMP	either
kmiss:
	MOVW	36(R26), R27
	ADDU	$1, R27
	MOVW	R27, 36(R26)		/* m->ktlbfault++ */
either:
#endif

	/* compute stlb index */
	EHB
	MOVV	M(TLBVIRT), R27		/* asid in low byte */
	STLBHASH(R27, R26, R23)
	MOVV	M(ERROREPC), R23		/* restore R23 */

	/* scale to a byte index (multiply by 24) */
	SLL		$1, R27, R26		/* × 2 */
	ADDU	R26, R27			/* × 3 */
	SLL		$3, R27				/* × 24 */

	CONST	(MACHADDR, R26)		/* R26 = m-> */
	MOVV	8(R26), R26		/* R26 = m->stb */
	ADDVU	R26, R27		/* R27 = &m->stb[hash] */

	MOVV	M(BADVADDR), R26
	AND		$BY2PG, R26
	BNE		R26, utlbodd		/* odd page? */

utlbeven:
	MOVV	8(R27), R26		/* R26 = m->stb[hash].phys0 */
	BEQ		R26, stlbm		/* nothing cached? do it the hard way */
	MOVV	R26, M(TLBPHYS0)
	EHB
	MOVV	16(R27), R26		/* R26 = m->stb[hash].phys1 */
	MOVV	R26, M(TLBPHYS1)
	EHB
	JMP		utlbcom

utlbodd:
	MOVV	16(R27), R26		/* R26 = m->stb[hash].phys1 */
	BEQ		R26, stlbm		/* nothing cached? do it the hard way */
	MOVV	R26, M(TLBPHYS1)
	EHB
	MOVV	8(R27), R26		/* R26 = m->stb[hash].phys0 */
	MOVV	R26, M(TLBPHYS0)
	EHB

utlbcom:
	MOVV	M(TLBVIRT), R26
	MOVV	(R27), R27		/* R27 = m->stb[hash].virt */
	BEQ		R27, stlbm		/* nothing cached? do it the hard way */

	/* is the stlb entry for the right virtual address? */
	BNE		R26, R27, stlbm		/* M(TLBVIRT) != m->stb[hash].virt? */

	/* if an entry exists, overwrite it, else write a random one */
	CONST	(PGSZ, R27)
	MOVW	R27, M(PAGEMASK)	/* select page size */
	EHB
	TLBP				/* probe tlb */
	EHB
	MOVW	M(INDEX), R26
	EHB
	BGEZ	R26, utlindex		/* if tlb entry found, rewrite it */
	TLBWR				/* else write random tlb entry */
	ERET
utlindex:
	TLBWI				/* write indexed tlb entry */
	ERET

/* not in the stlb either; make trap.c figure it out */
stlbm:
	MOVV	$exception(SB), R26
	JMP		(R26)

/* vector at KSEG1+0x100, cache error */
TEXT	vector100(SB), $-8
	MOVV	$exception(SB), R26
	JMP		(R26)

/* vector at KSEG0+0x180, others */
TEXT	vector180(SB), $-8
	MOVV	$exception(SB), R26
	JMP		(R26)

TEXT	exception(SB), $-8
//	PUTC('t', R26, R27)
//	MOVV	R29, R1
//	JAL		printhex(SB)
//	JMP		0(PC)
//	NOP

	MOVW	M(STATUS), R26
	AND		$KUSER, R26, R27
	BEQ		R27, waskernel

wasuser:
	MOVV	R29, R27
	CONST	(MACHADDR, R29)		/*  m-> */
	MOVV	16(R29), R29		/*  m->proc */
	MOVV	16(R29), R29		/*  m->proc->kstack */
	MOVW	M(STATUS), R26		/* redundant load */
	ADDVU	$(KSTACK-UREGSIZE), R29
	MOVV	R31, Ureg_r31(R29)

	JAL		savereg1(SB)

	MOVV	R30, Ureg_r30(R29)
	MOVV	R(MACH), Ureg_r25(R29)
	MOVV	R(USER), Ureg_r24(R29)

	MOVV	$setR30(SB), R30
	CONST	(MACHADDR, R(MACH))		/* R(MACH) = m-> */
	MOVV	16(R(MACH)), R(USER)		/* up = m->proc */

	AND		$(EXCMASK<<2), R26, R1	/* R26 = M(CAUSE) from savereg1 */
	SUBU	$(CSYS<<2), R1
	BNE		R1, notsys

	MOVV	R29, R1				/* first arg for syscall */
	SUBVU	$Notuoffset, R29
	JAL		syscall(SB)

sysrestore:
	ADDVU	$Notuoffset, R29
	JAL		restreg1(SB)

	MOVV	Ureg_r31(R29), R31
	MOVW	Ureg_status(R29), R26
	MOVV	Ureg_r30(R29), R30
	MOVW	R26, M(STATUS)
	EHB
	MOVV	Ureg_pc(R29), R26		/* old pc */
	MOVV	Ureg_sp(R29), R29
	MOVV	R26, M(EPC)
	ERET

notsys:
	JAL		savereg2(SB)

	MOVV	R29, R1				/* first arg for trap */
	SUBVU	$Notuoffset, R29
	JAL		trap(SB)

	ADDVU	$Notuoffset, R29

restore:
	JAL		restreg1(SB)
	JAL		restreg2(SB)		/* restores R28, among others, R26 = old pc */

	MOVV	Ureg_r30(R29), R30
	MOVV	Ureg_r31(R29), R31
	MOVV	Ureg_r25(R29), R(MACH)
	MOVV	Ureg_r24(R29), R(USER)
	MOVV	Ureg_sp(R29), R29
	MOVV	R26, M(EPC)
	ERET

waskernel:
	MOVV	R29, R27
	SUBVU	$UREGSIZE, R29
	OR		$7, R29				/* conservative rounding */
	XOR		$7, R29
	MOVV	R31, Ureg_r31(R29)

	JAL		savereg1(SB)
	JAL		savereg2(SB)

	MOVV	R29, R1			/* first arg for trap */
	SUBVU	$Notuoffset, R29
	JAL		trap(SB)

	ADDVU	$Notuoffset, R29

	JAL		restreg1(SB)
	JAL		restreg2(SB)		/* restores R28, among others, R26 = old pc */

	MOVV	Ureg_r31(R29), R31
	MOVV	Ureg_sp(R29), R29
	MOVV	R26, M(EPC)
	ERET

TEXT	forkret(SB), $0
	MOVV	R0, R1
	JMP		sysrestore

/*
 * save mandatory registers.
 * called with old M(STATUS) in R26.
 * called with old SP in R27
 * returns with M(CAUSE) in R26
 */
TEXT	savereg1(SB), $-8
	MOVV	R1, Ureg_r1(R29)

	MOVW	$(~KMODEMASK), R1	/* don't use R28, it's unsaved so far */
	AND		R26, R1
	MOVW	R1, M(STATUS)		/* kernel mode, no interrupt */
	EHB

	MOVW	R26, Ureg_status(R29)	/* status */
	MOVV	R27, Ureg_sp(R29)	/* user SP */

	MOVV	M(EPC), R1
	MOVW	M(CAUSE), R26

	MOVV	R23, Ureg_r23(R29)
	MOVV	R22, Ureg_r22(R29)
	MOVV	R21, Ureg_r21(R29)
	MOVV	R20, Ureg_r20(R29)
	MOVV	R19, Ureg_r19(R29)
	MOVV	R1, Ureg_pc(R29)
	RET

/*
 * all other registers.
 * called with M(CAUSE) in R26
 */
TEXT	savereg2(SB), $-8
	MOVV	R2, Ureg_r2(R29)

	MOVV	M(BADVADDR), R2
	MOVV	R26, Ureg_cause(R29)
	MOVV	M(TLBVIRT), R1
	MOVV	R2, Ureg_badvaddr(R29)
	MOVV	R1, Ureg_tlbvirt(R29)
	MOVV	HI, R1
	MOVV	LO, R2
	MOVV	R1, Ureg_hi(R29)
	MOVV	R2, Ureg_lo(R29)
					/* LINK,SB,SP missing */
	MOVV	R28, Ureg_r28(R29)
					/* R27, R26 not saved */
					/* R25, R24 missing */
					/* R23- R19 saved in save1 */
	MOVV	R18, Ureg_r18(R29)
	MOVV	R17, Ureg_r17(R29)
	MOVV	R16, Ureg_r16(R29)
	MOVV	R15, Ureg_r15(R29)
	MOVV	R14, Ureg_r14(R29)
	MOVV	R13, Ureg_r13(R29)
	MOVV	R12, Ureg_r12(R29)
	MOVV	R11, Ureg_r11(R29)
	MOVV	R10, Ureg_r10(R29)
	MOVV	R9, Ureg_r9(R29)
	MOVV	R8, Ureg_r8(R29)
	MOVV	R7, Ureg_r7(R29)
	MOVV	R6, Ureg_r6(R29)
	MOVV	R5, Ureg_r5(R29)
	MOVV	R4, Ureg_r4(R29)
	MOVV	R3, Ureg_r3(R29)
	RET

/* restore R23-R19 */
TEXT	restreg1(SB), $-8
	MOVV	Ureg_r23(R29), R23
	MOVV	Ureg_r22(R29), R22
	MOVV	Ureg_r21(R29), R21
	MOVV	Ureg_r20(R29), R20
	MOVV	Ureg_r19(R29), R19
	RET

/*
 * all other registers.
 * returns with pc in R26
 */
TEXT	restreg2(SB), $-8
					/* LINK,SB,SP missing */
	MOVV	Ureg_r28(R29), R28
					/* R27, R26 not saved */
					/* R25, R24 missing */
					/* R19- R23 restored in rest1 */
	MOVV	Ureg_r18(R29), R18
	MOVV	Ureg_r17(R29), R17
	MOVV	Ureg_r16(R29), R16
	MOVV	Ureg_r15(R29), R15
	MOVV	Ureg_r14(R29), R14
	MOVV	Ureg_r13(R29), R13
	MOVV	Ureg_r12(R29), R12
	MOVV	Ureg_r11(R29), R11
	MOVV	Ureg_r10(R29), R10
	MOVV	Ureg_r9(R29), R9
	MOVV	Ureg_r8(R29), R8
	MOVV	Ureg_r7(R29), R7
	MOVV	Ureg_r6(R29), R6
	MOVV	Ureg_r5(R29), R5
	MOVV	Ureg_r4(R29), R4
	MOVV	Ureg_r3(R29), R3
	MOVV	Ureg_lo(R29), R2
	MOVV	Ureg_hi(R29), R1
	MOVV	R2, LO
	MOVV	R1, HI

	MOVW	Ureg_status(R29), R1
	MOVV	Ureg_r2(R29), R2
	MOVW	R1, M(STATUS)		/* could change interruptibility */
	EHB
	MOVV	Ureg_r1(R29), R1	/* BOTCH */
	MOVV	Ureg_pc(R29), R26
	RET

/*
 * floating point stuff
 */

TEXT	savefpregs(SB), $0
	MOVW	FCR31, R2
	MOVW	M(STATUS), R3
	AND		$~FPEXCMASK, R2, R4
	MOVW	R4, FCR31

	MOVD	F0, (0*8)(R1)
	MOVD	F2, (1*8)(R1)
	MOVD	F4, (2*8)(R1)
	MOVD	F6, (3*8)(R1)
	MOVD	F8, (4*8)(R1)
	MOVD	F10, (5*8)(R1)
	MOVD	F12, (6*8)(R1)
	MOVD	F14, (7*8)(R1)
	MOVD	F16, (8*8)(R1)
	MOVD	F18, (9*8)(R1)
	MOVD	F20, (10*8)(R1)
	MOVD	F22, (11*8)(R1)
	MOVD	F24, (12*8)(R1)
	MOVD	F26, (13*8)(R1)
	MOVD	F28, (14*8)(R1)
	MOVD	F30, (15*8)(R1)

	MOVW	R2, (16*8)(R1)	/* FCR31 */
	AND		$~CU1, R3
	MOVW	R3, M(STATUS)
	EHB
	RET

TEXT	restfpregs(SB), $0	/* restfpregs(fpsave, fpstatus) */
	MOVW	M(STATUS), R3
	OR		$CU1, R3
	MOVW	R3, M(STATUS)
	EHB
	MOVW	8(FP), R2
	MOVW	R2, FCR31
	NOP

	MOVD	(0*8)(R1), F0
	MOVD	(1*8)(R1), F2
	MOVD	(2*8)(R1), F4
	MOVD	(3*8)(R1), F6
	MOVD	(4*8)(R1), F8
	MOVD	(5*8)(R1), F10
	MOVD	(6*8)(R1), F12
	MOVD	(7*8)(R1), F14
	MOVD	(8*8)(R1), F16
	MOVD	(9*8)(R1), F18
	MOVD	(10*8)(R1), F20
	MOVD	(11*8)(R1), F22
	MOVD	(12*8)(R1), F24
	MOVD	(13*8)(R1), F26
	MOVD	(14*8)(R1), F28
	MOVD	(15*8)(R1), F30

	AND		$~CU1, R3
	MOVW	R3, M(STATUS)
	EHB
	RET

TEXT	fcr31(SB), $0
	MOVW	FCR31, R1
	MOVW	M(STATUS), R3
	AND		$~CU1, R3
	MOVW	R3, M(STATUS)
	EHB
	RET

TEXT	clrfpintr(SB), $0
	MOVW	M(STATUS), R3
	OR		$CU1, R3
	MOVW	R3, M(STATUS)
	EHB

	MOVW	FCR31, R1
	AND		$~FPEXCMASK, R1, R2
	MOVW	R2, FCR31

	AND		$~CU1, R3
	MOVW	R3, M(STATUS)
	EHB
	RET

/*
 * Emulate 68020 test and set: load linked / store conditional
 */

TEXT	tas(SB), $0
	MOVV	R1, R2		/* address of key */
tas1:
	MOVW	$1, R3
	LL(2, 1)
	NOP
	SC(2, 3)
	NOP
	BEQ		R3, tas1
	RET

TEXT	ainc(SB), $0
	MOVV	R1, R2		/* address of counter */
loop:
	MOVW	$1, R3
	LL(2, 1)
	NOP
	ADDU	R1, R3
	MOVV	R3, R1		/* return new value */
	SC(2, 3)
	NOP
	BEQ		R3, loop
	RET

TEXT	adec(SB), $0
	SYNC
	NOP
	NOP
	MOVV	R1, R2		/* address of counter */
loop1:
	MOVW	$-1, R3
	LL(2, 1)
	NOP
	ADDU	R1, R3
	MOVV	R3, R1		/* return new value */
	SC(2, 3)
	NOP
	BEQ		R3, loop1
	RET

TEXT cmpswap(SB), $0
	MOVV	R1, R2			/* address of key */
	MOVW	old+8(FP), R3	/* old value */
	MOVW	new+16(FP), R4	/* new value */
	LL(2, 1)				/* R1 = (R2) */
	NOP
	BNE		R1, R3, fail
	MOVV	R4, R1
	SC(2, 1)	/* (R2) = R1 if (R2) hasn't changed; R1 = success */
	NOP
	RET
fail:
	MOVV	R0, R1
	RET

/*
 *  cache manipulation
 */

/*
 *  we avoided using R4, R5, R6, and R7 so gotopc can call us without saving
 *  them, but gotopc is now gone.
 */
TEXT	icflush(SB), $-8			/* icflush(virtaddr, count) */
	MOVW	M(STATUS), R10			/* old status -> R10 */
	MOVW	8(FP), R9
	MOVW	R0, M(STATUS)			/* intrs off */
	EHB

	TOKSEG1(11)				/* return to kseg1 (uncached) */
	ADDVU	R1, R9			/* R9 = last address */
	MOVW	$(~(CACHELINESZ-1)), R8
	AND		R1, R8			/* R8 = first address, rounded down */
	ADDVU	$(CACHELINESZ-1), R9
	AND		$(~(CACHELINESZ-1)), R9	/* round last address up */
	SUBVU	R8, R9			/* R9 = revised count */
icflush1:
//	CACHE	PD+HWB, (R8)		/* flush D to ram */
//	CACHE	PI+HINV, (R8)		/* invalidate in I */
	CACHE	SD+HWBI, (R8)		/* flush & invalidate thru L2 */
	SUBVU	$CACHELINESZ, R9
	ADDVU	$CACHELINESZ, R8
	BGTZ	R9, icflush1

	TOKSEG0(11)				/* return to kseg0 (cached) */
	MOVW	R10, M(STATUS)
	EHB
	RET

TEXT	dcflush(SB), $-8			/* dcflush(virtaddr, count) */
	MOVW	M(STATUS), R10			/* old status -> R10 */
	MOVW	8(FP), R9
	MOVW	R0, M(STATUS)			/* intrs off */
	EHB
	SYNC

	TOKSEG1(11)				/* return to kseg1 (uncached) */
	ADDVU	R1, R9			/* R9 = last address */
	MOVW	$(~(CACHELINESZ-1)), R8
	AND		R1, R8			/* R8 = first address, rounded down */
	ADDVU	$(CACHELINESZ-1), R9
	AND		$(~(CACHELINESZ-1)), R9	/* round last address up */
	SUBVU	R8, R9			/* R9 = revised count */
dcflush1:
//	CACHE	PI+HINV, (R8)		/* invalidate in I */
//	CACHE	PD+HWBI, (R8)		/* flush & invalidate in D */
	CACHE	SD+HWBI, (R8)		/* flush & invalidate thru L2 */
	SUBVU	$CACHELINESZ, R9
	ADDVU	$CACHELINESZ, R8
	BGTZ	R9, dcflush1
	SYNC

	TOKSEG0(11)				/* return to kseg0 (cached) */
	MOVW	R10, M(STATUS)
	EHB
	RET

TEXT	dcinvalid(SB), $-8			/* dcinvalid(virtaddr, count) */
	MOVW	M(STATUS), R10			/* old status -> R10 */
	MOVW	8(FP), R9
	MOVW	R0, M(STATUS)			/* intrs off */
	EHB
	SYNC

	TOKSEG1(11)				/* return to kseg1 (uncached) */
	ADDVU	R1, R9			/* R9 = last address */
	MOVW	$(~(CACHELINESZ-1)), R8
	AND		R1, R8			/* R8 = first address, rounded down */
	ADDVU	$(CACHELINESZ-1), R9
	AND		$(~(CACHELINESZ-1)), R9	/* round last address up */
	SUBVU	R8, R9			/* R9 = revised count */
dcinvalid1:
//	CACHE	PD+HINV, (R8)		/* invalidate in D */
	CACHE	SD+HINV, (R8)		/* invalidate thru L2 */
	SUBVU	$CACHELINESZ, R9
	ADDVU	$CACHELINESZ, R8
	BGTZ	R9, dcinvalid1
	SYNC

	TOKSEG0(11)				/* return to kseg0 (cached) */
	MOVW	R10, M(STATUS)
	EHB
	RET

TEXT	cleancache(SB), $-8
	MOVW	M(STATUS), R10			/* old status -> R10 */
	MOVW	R0, M(STATUS)			/* intrs off */
	EHB

	TOKSEG1(11)				/* return to kseg1 (uncached) */
	MOVW	$KSEG0, R1			/* index, not address, kseg0 avoids tlb */
	MOVW	$(SCACHESIZE/4), R9		/* 4-way cache */
ccache:
	CACHE	SD+IWBI, 0(R1)		/* flush & invalidate thru L2 by index */
	CACHE	SD+IWBI, 1(R1)		/* ways are least significant bits */
	CACHE	SD+IWBI, 2(R1)
	CACHE	SD+IWBI, 3(R1)
	SUBU	$CACHELINESZ, R9
	ADDU	$CACHELINESZ, R1
	BGTZ	R9, ccache
	SYNC

	TOKSEG0(11)				/* return to kseg0 (cached) */
	MOVW	R10, M(STATUS)
	EHB
	RET

/*
 *	PMON routines, for early debugging.
 *	wrapper converts PMON's calling convention to 0c's.
 *	we need to leave CFSZ space to prevent stack corrupted
 */

#define CFSZ	48		// XXX

DATA	pmon_callvec(SB)/4, $0	/* pmon call vector */
GLOBL	pmon_callvec(SB), $4

#define pmon_wrap(name, index) \
TEXT	name(SB), $((CFSZ+12+7)&(~7)); \
	MOVV	R1, R4; \
	MOVW	arg2+4(FP), R5; \
	MOVW	arg3+8(FP), R6; \
	MOVW	arg4+12(FP), R7; \
	MOVW	R29, R2; \
	AND		$~7, R29; /* pmon needs R29 8-aligned */ \
	MOVW	R31, -4(SP); \
	MOVW	R30, -8(SP); \
	MOVW	R2, -12(SP); \
	MOVW	pmon_callvec(SB), R8; \
	MOVW	(4*(index))(R8), R8; \
	JAL		(R8); \
	MOVW	-8(SP), R30; \
	MOVW	-4(SP), R31; \
	MOVW	-12(SP), R29; \
	MOVV	R2, R1; \
	MOVW	R31, 0(R29); \
	RET

pmon_wrap(pmonprint, 5)
