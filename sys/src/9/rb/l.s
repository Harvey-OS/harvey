/*
 * mips 24k machine assist for routerboard rb450g
 */
#include "mem.h"
#include "mips.s"

#define SANITY 0x12345678

	NOSCHED

/*
 * Boot only processor
 */
TEXT	start(SB), $-4
	MOVW	$setR30(SB), R30

PUTC('9', R1, R2)
	DI(0)

	MOVW	sanity(SB), R1
	CONST(SANITY, R2)
	SUBU	R1, R2, R2
	BNE	R2, insane
	NOP

	MOVW	R0, M(COMPARE)
	EHB

	/* don't enable any interrupts nor FP, but leave BEV on. */
	MOVW	$BEV,R1
	MOVW	R1, M(STATUS)
	UBARRIERS(7, R7, stshb)		/* returns to kseg1 space */
	MOVW	R0, M(CAUSE)
	EHB

	/* silence the atheros watchdog */
	MOVW	$(KSEG1|0x18060008), R1
	MOVW	R0, (R1)			/* set no action */
	SYNC

	MOVW	$PE, R1
	MOVW	R1, M(CACHEECC)		/* aka ErrCtl */
	EHB
	JAL	cleancache(SB)
	NOP

	MOVW	$TLBROFF, R1
	MOVW	R1, M(WIRED)

	MOVW	R0, M(CONTEXT)
	EHB

	/* set KSEG0 cachability before trying LL/SC in lock code */
	MOVW	M(CONFIG), R1
	AND	$~CFG_K0, R1
	/* make kseg0 cachable, enable write-through merging */
	OR	$((PTECACHABILITY>>3)|CFG_MM), R1
	MOVW	R1, M(CONFIG)
	BARRIERS(7, R7, cfghb)			/* back to kseg0 space */

	MOVW	$setR30(SB), R30		/* again */

	/* initialize Mach, including stack */
	MOVW	$MACHADDR, R(MACH)
	ADDU	$(MACHSIZE-BY2V), R(MACH), SP
	MOVW	R(MACH), R1
clrmach:
	MOVW	R0, (R1)
	ADDU	$BY2WD, R1
	BNE	R1, SP, clrmach
	NOP
	MOVW	R0, 0(R(MACH))			/* m->machno = 0 */
	MOVW	R0, R(USER)			/* up = nil */

	/* zero bss, byte-by-byte */
	MOVW	$edata(SB), R1
	MOVW	$end(SB), R2
clrbss:
	MOVB	R0, (R1)
	ADDU	$1, R1
	BNE	R1, R2, clrbss
	NOP

	MOVW	$0x16, R16
	MOVW	$0x17, R17
	MOVW	$0x18, R18
	MOVW	$0x19, R19
	MOVW	$0x20, R20
	MOVW	$0x21, R21
	MOVW	$0x22, R22
	MOVW	$0x23, R23

	MOVW	R0, HI
	MOVW	R0, LO

PUTC('\r', R1, R2)
PUTC('\n', R1, R2)
	JAL	main(SB)
	NOP
	CONST(ROM, R1)
	JMP	(R1)			/* back to the rom */

#define PUT(c) PUTC(c, R1, R2)
#define DELAY(lab) \
	CONST(34000000, R3); \
lab:	SUBU	$1, R3; \
	BNE	R3, lab; \
	NOP

insane:
	/*
	 * data segment is misaligned; kernel needs vl -R4096 or -R16384,
	 * as appropriate, for reboot.
	 */
	PUT('?'); PUT('d'); PUT('a'); PUT('t'); PUT('a'); PUT(' '); DELAY(dl1)
	PUT('s'); PUT('e'); PUT('g'); PUT('m'); PUT('e'); PUT('n'); DELAY(dl2)
	PUT('t'); PUT(' '); PUT('m'); PUT('i'); PUT('s'); PUT('a'); DELAY(dl3)
	PUT('l'); PUT('i'); PUT('g'); PUT('n'); PUT('e'); PUT('d'); DELAY(dl4)
	PUT('\r'); PUT('\n'); DELAY(dl5)
	CONST(ROM, R1)
	JMP	(R1)			/* back to the rom */
	NOP

/* target for JALRHB in BARRIERS */
TEXT ret(SB), $-4
	JMP	(R22)
	NOP

/* print R1 in hex; clobbers R3—8 */
TEXT printhex(SB), $-4
	MOVW	$32, R5
	MOVW	$9, R7
prtop:
	SUB	$4, R5
	MOVW	R1, R6
	SRL	R5, R6
	AND	$0xf, R6
	SGTU	R6, R7, R8
	BEQ	R8, prdec		/* branch if R6 <= 9 */
	NOP
	ADD	$('a'-10), R6
	JMP	prchar
	NOP
prdec:
	ADD	$'0', R6
prchar:
	PUTC(R6, R3, R4)
	BNE	R5, prtop
	NOP
	RETURN

/*
 * Take first processor into user mode
 * 	- argument is stack pointer to user
 */
TEXT	touser(SB), $-4
	MOVW	R1, SP
	MOVW	$(UTZERO+32), R2	/* header appears in text */
	MOVW	R2, M(EPC)
	EHB
	MOVW	M(STATUS), R4
	AND	$(~KMODEMASK), R4
	OR	$(KUSER|IE|EXL), R4	/* switch to user mode, intrs on, exc */
	MOVW	R4, M(STATUS)		/* " */
	ERET				/* clears EXL */

/*
 * manipulate interrupts
 */

/* enable an interrupt; bit is in R1 */
TEXT	intron(SB), $0
	MOVW	M(STATUS), R2
	OR	R1, R2
	MOVW	R2, M(STATUS)
	EHB
	RETURN

/* disable an interrupt; bit is in R1 */
TEXT	introff(SB), $0
	MOVW	M(STATUS), R2
	XOR	$-1, R1
	AND	R1, R2
	MOVW	R2, M(STATUS)
	EHB
	RETURN

/* on our 24k, wait instructions are not interruptible, alas. */
TEXT	idle(SB), $-4
	EI(1)				/* old M(STATUS) into R1 */
	EHB
	/* fall through */

TEXT	wait(SB), $-4
	WAIT
	NOP

	MOVW	R1, M(STATUS)		/* interrupts restored */
	EHB
	RETURN

TEXT	splhi(SB), $0
	EHB
	MOVW	R31, 12(R(MACH))	/* save PC in m->splpc */
	DI(1)				/* old M(STATUS) into R1 */
	EHB
	RETURN

TEXT	splx(SB), $0
	EHB
	MOVW	R31, 12(R(MACH))	/* save PC in m->splpc */
	MOVW	M(STATUS), R2
	AND	$IE, R1
	AND	$~IE, R2
	OR	R2, R1
	MOVW	R1, M(STATUS)
	EHB
	RETURN

TEXT	spllo(SB), $0
	EHB
	EI(1)				/* old M(STATUS) into R1 */
	EHB
	RETURN

TEXT	spldone(SB), $0
	RETURN

TEXT	islo(SB), $0
	MOVW	M(STATUS), R1
	AND	$IE, R1
	RETURN

TEXT	coherence(SB), $-4
	BARRIERS(7, R7, cohhb)
	SYNC
	EHB
	RETURN

/*
 * process switching
 */

TEXT	setlabel(SB), $-4
	MOVW	R29, 0(R1)
	MOVW	R31, 4(R1)
	MOVW	R0, R1
	RETURN

TEXT	gotolabel(SB), $-4
	MOVW	0(R1), R29
	MOVW	4(R1), R31
	MOVW	$1, R1
	RETURN

/*
 * the tlb routines need to be called at splhi.
 */

TEXT	puttlb(SB), $0			/* puttlb(virt, phys0, phys1) */
	EHB
	MOVW	R1, M(TLBVIRT)
	EHB
	MOVW	4(FP), R2		/* phys0 */
	MOVW	8(FP), R3		/* phys1 */
	MOVW	R2, M(TLBPHYS0)
	EHB
	MOVW	$PGSZ, R1
	MOVW	R3, M(TLBPHYS1)
	EHB
	MOVW	R1, M(PAGEMASK)
	OR	R2, R3, R4		/* MTC0 delay slot */
	AND	$PTEVALID, R4		/* MTC0 delay slot */
	EHB
	TLBP				/* tlb probe */
	EHB
	MOVW	M(INDEX), R1
	BGEZ	R1, index		/* if tlb entry found, use it */
	NOP
	BEQ	R4, dont		/* not valid? cf. kunmap */
	NOP
	MOVW	M(RANDOM), R1		/* write random tlb entry */
	MOVW	R1, M(INDEX)
index:
	EHB
	TLBWI				/* write indexed tlb entry */
	JRHB(31)			/* return and clear all hazards */
dont:
	RETURN

TEXT	getwired(SB),$0
	MOVW	M(WIRED), R1
	RETURN

TEXT	setwired(SB),$0
	MOVW	R1, M(WIRED)
	EHB
	RETURN

TEXT	getrandom(SB),$0
	MOVW	M(RANDOM), R1
	RETURN

TEXT	getpagemask(SB),$0
	MOVW	M(PAGEMASK), R1
	RETURN

TEXT	setpagemask(SB),$0
	EHB
	MOVW	R1, M(PAGEMASK)
	EHB
	MOVW	R0, R1			/* prevent accidents */
	RETURN

TEXT	puttlbx(SB), $0	/* puttlbx(index, virt, phys0, phys1, pagemask) */
	MOVW	4(FP), R2
	MOVW	8(FP), R3
	MOVW	12(FP), R4
	MOVW	16(FP), R5
	EHB
	MOVW	R2, M(TLBVIRT)
	EHB
	MOVW	R3, M(TLBPHYS0)
	MOVW	R4, M(TLBPHYS1)
	MOVW	R5, M(PAGEMASK)
	EHB
	MOVW	R1, M(INDEX)
	EHB
	TLBWI				/* write indexed tlb entry */
	JRHB(31)			/* return and clear all hazards */

TEXT	tlbvirt(SB), $0
	EHB
	MOVW	M(TLBVIRT), R1
	EHB
	RETURN

TEXT	gettlbx(SB), $0			/* gettlbx(index, &entry) */
	MOVW	4(FP), R5
	MOVW	M(TLBVIRT), R10		/* save our asid */
	EHB
	MOVW	R1, M(INDEX)
	EHB
	TLBR				/* read indexed tlb entry */
	EHB
	MOVW	M(TLBVIRT), R2
	MOVW	M(TLBPHYS0), R3
	MOVW	M(TLBPHYS1), R4
	MOVW	R2, 0(R5)
	MOVW	R3, 4(R5)
	MIPS24KNOP
	MOVW	R4, 8(R5)
	EHB
	MOVW	R10, M(TLBVIRT)		/* restore our asid */
	EHB
	RETURN

TEXT	gettlbp(SB), $0			/* gettlbp(tlbvirt, &entry) */
	MOVW	4(FP), R5
	MOVW	M(TLBVIRT), R10		/* save our asid */
	EHB
	MOVW	R1, M(TLBVIRT)
	EHB
	TLBP				/* probe tlb */
	EHB
	MOVW	M(INDEX), R1
	BLTZ	R1, gettlbp1		/* if no tlb entry found, return */
	NOP
	EHB
	TLBR				/* read indexed tlb entry */
	EHB
	MOVW	M(TLBVIRT), R2
	MOVW	M(TLBPHYS0), R3
	MOVW	M(TLBPHYS1), R4
	MOVW	M(PAGEMASK), R6
	MOVW	R2, 0(R5)
	MOVW	R3, 4(R5)
	MIPS24KNOP
	MOVW	R4, 8(R5)
	MOVW	R6, 12(R5)
gettlbp1:
	EHB
	MOVW	R10, M(TLBVIRT)		/* restore our asid */
	EHB
	RETURN

TEXT	gettlbvirt(SB), $0		/* gettlbvirt(index) */
	MOVW	M(TLBVIRT), R10		/* save our asid */
	EHB
	MOVW	R1, M(INDEX)
	EHB
	TLBR				/* read indexed tlb entry */
	EHB
	MOVW	M(TLBVIRT), R1
	EHB
	MOVW	R10, M(TLBVIRT)		/* restore our asid */
	EHB
	RETURN

/*
 * exceptions.
 * mips promises that there will be no current hazards upon entry
 * to exception handlers.
 */

TEXT	vector0(SB), $-4
	MOVW	$utlbmiss(SB), R26
	JMP	(R26)
	NOP

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
	MOVW	arg, tmp2; \
	SRL	$(PGSHIFT+1), arg;	/* move low page # bits to low bits */ \
	CONST	((MASK(HIPFNBITS) << STLBLOG), tmp); \
	AND	arg, tmp;		/* extract high page # bits */ \
	SRL	$HIPFNBITS, tmp;	/* position them */ \
	XOR	tmp, arg;		/* include them */ \
	MOVW	tmp2, tmp;		/* asid in low byte */ \
	SLL	$(STLBLOG-8), tmp;	/* move asid to high bits */ \
	XOR	tmp, arg;		/* include asid in high bits too */ \
	AND	$0xff, tmp2, tmp;	/* asid in low byte */ \
	XOR	tmp, arg;		/* include asid in low bits */ \
	CONST	(STLBSIZE-1, tmp); \
	AND	tmp, arg		/* chop to fit */

TEXT	utlbmiss(SB), $-4
	/*
	 * don't use R28 by using constants that span both word halves,
	 * it's unsaved so far.  avoid R24 (up in kernel) and R25 (m in kernel).
	 */
	/* update statistics */
	CONST	(MACHADDR, R26)		/* R26 = m-> */
	MOVW	16(R26), R27
	ADDU	$1, R27
	MOVW	R27, 16(R26)		/* m->tlbfault++ */

	MOVW	R23, M(DESAVE)		/* save R23 */

#ifdef	KUTLBSTATS
	MOVW	M(STATUS), R23
	AND	$KUSER, R23
	BEQ	R23, kmiss

	MOVW	24(R26), R27
	ADDU	$1, R27
	MOVW	R27, 24(R26)		/* m->utlbfault++ */
	JMP	either
kmiss:
	MOVW	20(R26), R27
	ADDU	$1, R27
	MOVW	R27, 20(R26)		/* m->ktlbfault++ */
either:
#endif

	/* compute stlb index */
	EHB
	MOVW	M(TLBVIRT), R27		/* asid in low byte */
	STLBHASH(R27, R26, R23)
	MOVW	M(DESAVE), R23		/* restore R23 */

	/* scale to a byte index (multiply by 12) */
	SLL	$1, R27, R26		/* × 2 */
	ADDU	R26, R27		/* × 3 */
	SLL	$2, R27			/* × 12 */

	CONST	(MACHADDR, R26)		/* R26 = m-> */
	MOVW	4(R26), R26		/* R26 = m->stb */
	ADDU	R26, R27		/* R27 = &m->stb[hash] */

	MOVW	M(BADVADDR), R26
	AND	$BY2PG, R26
	BNE	R26, utlbodd		/* odd page? */
	NOP

utlbeven:
	MOVW	4(R27), R26		/* R26 = m->stb[hash].phys0 */
	BEQ	R26, stlbm		/* nothing cached? do it the hard way */
	NOP
	MOVW	R26, M(TLBPHYS0)
	EHB
	MOVW	8(R27), R26		/* R26 = m->stb[hash].phys1 */
	JMP	utlbcom
	MOVW	R26, M(TLBPHYS1)	/* branch delay slot */

utlbodd:
	MOVW	8(R27), R26		/* R26 = m->stb[hash].phys1 */
	BEQ	R26, stlbm		/* nothing cached? do it the hard way */
	NOP
	MOVW	R26, M(TLBPHYS1)
	EHB
	MOVW	4(R27), R26		/* R26 = m->stb[hash].phys0 */
	MOVW	R26, M(TLBPHYS0)

utlbcom:
	EHB				/* MTC0/MFC0 hazard */
	MOVW	M(TLBVIRT), R26
	MOVW	(R27), R27		/* R27 = m->stb[hash].virt */
	BEQ	R27, stlbm		/* nothing cached? do it the hard way */
	NOP
	/* is the stlb entry for the right virtual address? */
	BNE	R26, R27, stlbm		/* M(TLBVIRT) != m->stb[hash].virt? */
	NOP

	/* if an entry exists, overwrite it, else write a random one */
	CONST	(PGSZ, R27)
	MOVW	R27, M(PAGEMASK)	/* select page size */
	EHB
	TLBP				/* probe tlb */
	EHB
	MOVW	M(INDEX), R26
	BGEZ	R26, utlindex		/* if tlb entry found, rewrite it */
	EHB				/* delay slot */
	TLBWR				/* else write random tlb entry */
	ERET
utlindex:
	TLBWI				/* write indexed tlb entry */
	ERET

/* not in the stlb either; make trap.c figure it out */
stlbm:
	MOVW	$exception(SB), R26
	JMP	(R26)
	NOP

TEXT	stlbhash(SB), $-4
	STLBHASH(R1, R2, R3)
	RETURN

TEXT	vector100(SB), $-4
	MOVW	$exception(SB), R26
	JMP	(R26)
	NOP

TEXT	vector180(SB), $-4
	MOVW	$exception(SB), R26
	JMP	(R26)
	NOP

TEXT	exception(SB), $-4
	MOVW	M(STATUS), R26
	AND	$KUSER, R26, R27
	BEQ	R27, waskernel
	MOVW	SP, R27			/* delay slot */

wasuser:
	CONST	(MACHADDR, SP)		/*  m-> */
	MOVW	8(SP), SP		/*  m->proc */
	MOVW	8(SP), SP		/*  m->proc->kstack */
	MOVW	M(STATUS), R26		/* redundant load */
	ADDU	$(KSTACK-UREGSIZE), SP
	MOVW	R31, Ureg_r31(SP)

	JAL	savereg1(SB)
	NOP

	MOVW	R30, Ureg_r30(SP)
	MOVW	R(MACH), Ureg_r25(SP)
	MIPS24KNOP
	MOVW	R(USER), Ureg_r24(SP)

	MOVW	$setR30(SB), R30
	CONST	(MACHADDR, R(MACH))		/* R(MACH) = m-> */
	MOVW	8(R(MACH)), R(USER)		/* up = m->proc */

	AND	$(EXCMASK<<2), R26, R1
	SUBU	$(CSYS<<2), R1
	BNE	R1, notsys
	NOP

	/* the carrera does this: */
//	ADDU	$8, SP, R1			/* first arg for syscall */

	MOVW	SP, R1				/* first arg for syscall */
	JAL	syscall(SB)
	SUBU	$Notuoffset, SP			/* delay slot */
sysrestore:
	JAL	restreg1(SB)
	ADDU	$Notuoffset, SP			/* delay slot */

	MOVW	Ureg_r31(SP), R31
	MOVW	Ureg_status(SP), R26
	MOVW	Ureg_r30(SP), R30
	MOVW	R26, M(STATUS)
	EHB
	MOVW	Ureg_pc(SP), R26		/* old pc */
	MOVW	Ureg_sp(SP), SP
	MOVW	R26, M(EPC)
	ERET

notsys:
	JAL	savereg2(SB)
	NOP

	/* the carrera does this: */
//	ADDU	$8, SP, R1			/* first arg for trap */

	MOVW	SP, R1				/* first arg for trap */
	JAL	trap(SB)
	SUBU	$Notuoffset, SP			/* delay slot */

	ADDU	$Notuoffset, SP

restore:
	JAL	restreg1(SB)
	NOP
	JAL	restreg2(SB)		/* restores R28, among others */
	NOP

	MOVW	Ureg_r30(SP), R30
	MOVW	Ureg_r31(SP), R31
	MOVW	Ureg_r25(SP), R(MACH)
	MOVW	Ureg_r24(SP), R(USER)
	MOVW	Ureg_sp(SP), SP
	MOVW	R26, M(EPC)
	ERET

waskernel:
	SUBU	$UREGSIZE, SP
	OR	$7, SP				/* conservative rounding */
	XOR	$7, SP
	MOVW	R31, Ureg_r31(SP)

	JAL	savereg1(SB)
	NOP
	JAL	savereg2(SB)
	NOP

	/* the carrera does this: */
//	ADDU	$8, SP, R1			/* first arg for trap */

	MOVW	SP, R1			/* first arg for trap */
	JAL	trap(SB)
	SUBU	$Notuoffset, SP			/* delay slot */

	ADDU	$Notuoffset, SP

	JAL	restreg1(SB)
	NOP

	/*
	 * if about to return to `wait', interrupt arrived just before
	 * executing wait, so move saved pc past it.
	 */
	MOVW	Ureg_pc(SP), R26
	MOVW	R26, R31
	MOVW	$wait(SB), R1
	SUBU	R1, R31
	BNE	R31, notwait
	NOP
	ADD	$BY2WD, R26		/* advance saved pc */
	MOVW	R26, Ureg_pc(SP)
notwait:
	JAL	restreg2(SB)		/* restores R28, among others */
	NOP

	MOVW	Ureg_r31(SP), R31
	MOVW	Ureg_sp(SP), SP
	MOVW	R26, M(EPC)
	ERET

TEXT	forkret(SB), $0
	JMP	sysrestore
	MOVW	R0, R1			/* delay slot; child returns 0 */

/*
 * save mandatory registers.
 * called with old M(STATUS) in R26.
 * called with old SP in R27
 * returns with M(CAUSE) in R26
 */
TEXT	savereg1(SB), $-4
	MOVW	R1, Ureg_r1(SP)

	MOVW	$(~KMODEMASK),R1	/* don't use R28, it's unsaved so far */
	AND	R26, R1
	MOVW	R1, M(STATUS)
	EHB

	MOVW	R26, Ureg_status(SP)	/* status */
	MOVW	R27, Ureg_sp(SP)	/* user SP */

	MOVW	M(EPC), R1
	MOVW	M(CAUSE), R26

	MOVW	R23, Ureg_r23(SP)
	MOVW	R22, Ureg_r22(SP)
	MIPS24KNOP
	MOVW	R21, Ureg_r21(SP)
	MOVW	R20, Ureg_r20(SP)
	MIPS24KNOP
	MOVW	R19, Ureg_r19(SP)
	MOVW	R1, Ureg_pc(SP)
	RETURN

/*
 * all other registers.
 * called with M(CAUSE) in R26
 */
TEXT	savereg2(SB), $-4
	MOVW	R2, Ureg_r2(SP)

	MOVW	M(BADVADDR), R2
	MOVW	R26, Ureg_cause(SP)
	MOVW	M(TLBVIRT), R1
	MOVW	R2, Ureg_badvaddr(SP)
	MOVW	R1, Ureg_tlbvirt(SP)
	MOVW	HI, R1
	MOVW	LO, R2
	MOVW	R1, Ureg_hi(SP)
	MOVW	R2, Ureg_lo(SP)
	MIPS24KNOP
					/* LINK,SB,SP missing */
	MOVW	R28, Ureg_r28(SP)
					/* R27, R26 not saved */
					/* R25, R24 missing */
					/* R23- R19 saved in save1 */
	MOVW	R18, Ureg_r18(SP)
	MIPS24KNOP
	MOVW	R17, Ureg_r17(SP)
	MOVW	R16, Ureg_r16(SP)
	MIPS24KNOP
	MOVW	R15, Ureg_r15(SP)
	MOVW	R14, Ureg_r14(SP)
	MIPS24KNOP
	MOVW	R13, Ureg_r13(SP)
	MOVW	R12, Ureg_r12(SP)
	MIPS24KNOP
	MOVW	R11, Ureg_r11(SP)
	MOVW	R10, Ureg_r10(SP)
	MIPS24KNOP
	MOVW	R9, Ureg_r9(SP)
	MOVW	R8, Ureg_r8(SP)
	MIPS24KNOP
	MOVW	R7, Ureg_r7(SP)
	MOVW	R6, Ureg_r6(SP)
	MIPS24KNOP
	MOVW	R5, Ureg_r5(SP)
	MOVW	R4, Ureg_r4(SP)
	MIPS24KNOP
	MOVW	R3, Ureg_r3(SP)
	RETURN

TEXT	restreg1(SB), $-4
	MOVW	Ureg_r23(SP), R23
	MOVW	Ureg_r22(SP), R22
	MOVW	Ureg_r21(SP), R21
	MOVW	Ureg_r20(SP), R20
	MOVW	Ureg_r19(SP), R19
	RETURN

TEXT	restreg2(SB), $-4
					/* LINK,SB,SP missing */
	MOVW	Ureg_r28(SP), R28
					/* R27, R26 not saved */
					/* R25, R24 missing */
					/* R19- R23 restored in rest1 */
	MOVW	Ureg_r18(SP), R18
	MOVW	Ureg_r17(SP), R17
	MOVW	Ureg_r16(SP), R16
	MOVW	Ureg_r15(SP), R15
	MOVW	Ureg_r14(SP), R14
	MOVW	Ureg_r13(SP), R13
	MOVW	Ureg_r12(SP), R12
	MOVW	Ureg_r11(SP), R11
	MOVW	Ureg_r10(SP), R10
	MOVW	Ureg_r9(SP), R9
	MOVW	Ureg_r8(SP), R8
	MOVW	Ureg_r7(SP), R7
	MOVW	Ureg_r6(SP), R6
	MOVW	Ureg_r5(SP), R5
	MOVW	Ureg_r4(SP), R4
	MOVW	Ureg_r3(SP), R3
	MOVW	Ureg_lo(SP), R2
	MOVW	Ureg_hi(SP), R1
	MOVW	R2, LO
	MOVW	R1, HI

	MOVW	Ureg_status(SP), R1
	MOVW	Ureg_r2(SP), R2
	MOVW	R1, M(STATUS)		/* could change interruptibility */
	EHB
	MOVW	Ureg_r1(SP), R1	/* BOTCH */
	MOVW	Ureg_pc(SP), R26
	RETURN

#ifdef OLD_MIPS_EXAMPLE
/* this appears to be a dreg from the distant past */
TEXT	rfnote(SB), $0
	MOVW	R1, R26			/* 1st arg is &uregpointer */
	JMP	restore
	SUBU	$(BY2WD), R26, SP	/* delay slot: pc hole */
#endif

/*
 * degenerate floating-point stuff
 */

TEXT	clrfpintr(SB), $0
	RETURN

TEXT	savefpregs(SB), $0
	RETURN

TEXT	restfpregs(SB), $0
	RETURN

TEXT	fcr31(SB), $0			/* fp csr */
	MOVW	R0, R1
	RETURN

/*
 * Emulate 68020 test and set: load linked / store conditional
 */

TEXT	tas(SB), $0
	MOVW	R1, R2		/* address of key */
tas1:
	MOVW	$1, R3
	LL(2, 1)
	NOP
	SC(2, 3)
	NOP
	BEQ	R3, tas1
	NOP
	RETURN

TEXT	_xinc(SB), $0
	MOVW	R1, R2		/* address of counter */
loop:
	MOVW	$1, R3
	LL(2, 1)
	NOP
	ADDU	R1, R3, R3
	SC(2, 3)
	NOP
	BEQ	R3, loop
	NOP
	RETURN

TEXT	_xdec(SB), $0
	SYNC
	MOVW	R1, R2		/* address of counter */
loop1:
	MOVW	$-1, R3
	LL(2, 1)
	NOP
	ADDU	R1, R3, R3
	MOVW	R3, R1
	SC(2, 3)
	NOP
	BEQ	R3, loop1
	NOP
	RETURN

TEXT cmpswap(SB), $0
	MOVW	R1, R2		/* address of key */
	MOVW	old+4(FP), R3	/* old value */
	MOVW	new+8(FP), R4	/* new value */
	LL(2, 1)		/* R1 = (R2) */
	NOP
	BNE	R1, R3, fail
	NOP
	MOVW	R4, R1
	SC(2, 1)	/* (R2) = R1 if (R2) hasn't changed; R1 = success */
	NOP
	RETURN
fail:
	MOVW	R0, R1
	RETURN

/*
 *  cache manipulation
 */

/*
 *  we avoided using R4, R5, R6, and R7 so gotopc can call us without saving
 *  them, but gotopc is now gone.
 */
TEXT	icflush(SB), $-4			/* icflush(virtaddr, count) */
	MOVW	4(FP), R9
	DI(10)				/* intrs off, old status -> R10 */
	UBARRIERS(7, R7, ichb);		/* return to kseg1 (uncached) */
	ADDU	R1, R9			/* R9 = last address */
	MOVW	$(~(CACHELINESZ-1)), R8
	AND	R1, R8			/* R8 = first address, rounded down */
	ADDU	$(CACHELINESZ-1), R9
	AND	$(~(CACHELINESZ-1)), R9	/* round last address up */
	SUBU	R8, R9			/* R9 = revised count */
icflush1:
//	CACHE	PD+HWB, (R8)		/* flush D to ram */
	CACHE	PI+HINV, (R8)		/* invalidate in I */
	SUBU	$CACHELINESZ, R9
	BGTZ	R9, icflush1
	ADDU	$CACHELINESZ, R8	/* delay slot */

	BARRIERS(7, R7, ic2hb);		/* return to kseg0 (cached) */
	MOVW	R10, M(STATUS)
	JRHB(31)			/* return and clear all hazards */

TEXT	dcflush(SB), $-4			/* dcflush(virtaddr, count) */
	MOVW	4(FP), R9
	DI(10)				/* intrs off, old status -> R10 */
	SYNC
	EHB
	ADDU	R1, R9			/* R9 = last address */
	MOVW	$(~(CACHELINESZ-1)), R8
	AND	R1, R8			/* R8 = first address, rounded down */
	ADDU	$(CACHELINESZ-1), R9
	AND	$(~(CACHELINESZ-1)), R9	/* round last address up */
	SUBU	R8, R9			/* R9 = revised count */
dcflush1:
//	CACHE	PI+HINV, (R8)		/* invalidate in I */
	CACHE	PD+HWBI, (R8)		/* flush & invalidate in D */
	SUBU	$CACHELINESZ, R9
	BGTZ	R9, dcflush1
	ADDU	$CACHELINESZ, R8	/* delay slot */
	SYNC
	EHB
	MOVW	R10, M(STATUS)
	JRHB(31)			/* return and clear all hazards */

/* the i and d caches may be different sizes, so clean them separately */
TEXT	cleancache(SB), $-4
	DI(10)				/* intrs off, old status -> R10 */

	UBARRIERS(7, R7, cchb);		/* return to kseg1 (uncached) */
	MOVW	R0, R1			/* index, not address */
	MOVW	$ICACHESIZE, R9
iccache:
	CACHE	PI+IWBI, (R1)		/* flush & invalidate I by index */
	SUBU	$CACHELINESZ, R9
	BGTZ	R9, iccache
	ADDU	$CACHELINESZ, R1	/* delay slot */

	BARRIERS(7, R7, cc2hb);		/* return to kseg0 (cached) */

	MOVW	R0, R1			/* index, not address */
	MOVW	$DCACHESIZE, R9
dccache:
	CACHE	PD+IWBI, (R1)		/* flush & invalidate D by index */
	SUBU	$CACHELINESZ, R9
	BGTZ	R9, dccache
	ADDU	$CACHELINESZ, R1	/* delay slot */

	SYNC
	MOVW	R10, M(STATUS)
	JRHB(31)			/* return and clear all hazards */

/*
 * access to CP0 registers
 */

TEXT	prid(SB), $0
	MOVW	M(PRID), R1
	RETURN

TEXT	rdcount(SB), $0
	MOVW	M(COUNT), R1
	RETURN

TEXT	wrcount(SB), $0
	MOVW	R1, M(COUNT)
	EHB
	RETURN

TEXT	wrcompare(SB), $0
	MOVW	R1, M(COMPARE)
	EHB
	RETURN

TEXT	rdcompare(SB), $0
	MOVW	M(COMPARE), R1
	RETURN

TEXT	getconfig(SB), $-4
	MOVW	M(CONFIG), R1
	RETURN

TEXT	getconfig1(SB), $-4
	MFC0(CONFIG, 1, 1)
	RETURN

TEXT	getconfig2(SB), $-4
	MFC0(CONFIG, 2, 1)
	RETURN

TEXT	getconfig3(SB), $-4
	MFC0(CONFIG, 3, 1)
	RETURN

TEXT	getconfig4(SB), $-4
	MFC0(CONFIG, 4, 1)
	RETURN

TEXT	getconfig7(SB), $-4
	MFC0(CONFIG, 7, 1)
	RETURN

TEXT	gethwreg3(SB), $-4
	RDHWR(3, 1)
	RETURN

TEXT	getcause(SB), $-4
	MOVW	M(CAUSE), R1
	RETURN

TEXT	C_fcr0(SB), $-4		/* fp implementation */
	MOVW	$0x500, R1	/* claim to be an r4k, thus have ll/sc */
	RETURN

TEXT	getstatus(SB), $0
	MOVW	M(STATUS), R1
	RETURN

TEXT	setstatus(SB), $0
	MOVW	R1, M(STATUS)
	EHB
	RETURN

TEXT	setwatchhi0(SB), $0
	MOVW	R1, M(WATCHHI)
	EHB
	RETURN

/*
 * beware that the register takes a double-word address, so it's not
 * precise to the individual instruction.
 */
TEXT	setwatchlo0(SB), $0
	MOVW	R1, M(WATCHLO)
	EHB
	RETURN

TEXT	setsp(SB), $-4
	MOVW	R1, SP
	RETURN

TEXT	getintctl(SB), $-4
	MFC0(STATUS, 1, 1)
	RETURN

TEXT	getsrsctl(SB), $-4
	MFC0(STATUS, 2, 1)
	RETURN

TEXT	getsrsmap(SB), $-4
	MFC0(STATUS, 3, 1)
	RETURN

TEXT	getperfctl0(SB), $-4
	MFC0(PERFCOUNT, 0, 1)
	RETURN

TEXT	getperfctl1(SB), $-4
	MFC0(PERFCOUNT, 2, 1)
	RETURN

	GLOBL	sanity(SB), $4
	DATA	sanity(SB)/4, $SANITY

	SCHED
