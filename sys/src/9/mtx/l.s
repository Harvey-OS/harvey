#include	"mem.h"

/* use of SPRG registers in save/restore */
#define	SAVER0	SPRG0
#define	SAVER1	SPRG1
#define	SAVELR	SPRG2
#define	SAVEXX	SPRG3

/* special instruction definitions */
#define	BDNZ	BC	16,0,
#define	BDNE	BC	0,2,

#define	TLBIA	WORD	$((31<<26)|(307<<1))
#define	TLBSYNC	WORD	$((31<<26)|(566<<1))

/* on some models mtmsr doesn't synchronise enough (eg, 603e) */
#define	MSRSYNC	SYNC; ISYNC

#define	UREGSPACE	(UREGSIZE+8)

	TEXT start(SB), $-4

	/*
	 * setup MSR
	 * turn off interrupts
	 * use 0x000 as exception prefix
	 * enable machine check
	 */
	MOVW	MSR, R3
	MOVW	$(MSR_EE|MSR_IP), R4
	ANDN	R4, R3
	OR		$(MSR_ME), R3
	ISYNC
	MOVW	R3, MSR
	MSRSYNC

	/* except during trap handling, R0 is zero from now on */
	MOVW	$0, R0

	/* setup SB for pre mmu */
	MOVW	$setSB(SB), R2
	MOVW	$KZERO, R3
	ANDN	R3, R2

	BL	mmuinit0(SB)

	/* running with MMU on!! */

	/* set R2 to correct value */
	MOVW	$setSB(SB), R2

	/* debugger sets R1 to top of usable memory +1 */
	MOVW R1, memsize(SB)

	BL		kfpinit(SB)

	/* set up Mach */
	MOVW	$mach0(SB), R(MACH)
	ADD	$(MACHSIZE-8), R(MACH), R1	/* set stack */

	MOVW	R0, R(USER)
	MOVW	R0, 0(R(MACH))

	BL	main(SB)

	RETURN		/* not reached */

GLOBL	mach0(SB), $(MAXMACH*BY2PG)
GLOBL	memsize(SB), $4

/*
 * on return from this function we will be running in virtual mode.
 * We set up the Block Address Translation (BAT) registers thus:
 * 1) first 3 BATs are 256M blocks, starting from KZERO->0
 * 2) remaining BAT maps last 256M directly
 */
TEXT	mmuinit0(SB), $0
	/* reset all the tlbs */
	MOVW	$64, R3
	MOVW	R3, CTR
	MOVW	$0, R4
tlbloop:
	TLBIE	R4
	ADD		$BIT(19), R4
	BDNZ	tlbloop
	TLBSYNC

	/* KZERO -> 0 */
	MOVW	$(KZERO|(0x7ff<<2)|2), R3
	MOVW	$(PTEVALID|PTEWRITE), R4
	MOVW	R3, SPR(IBATU(0))
	MOVW	R4, SPR(IBATL(0))
	MOVW	R3, SPR(DBATU(0))
	MOVW	R4, SPR(DBATL(0))

	/* KZERO+256M -> 256M */
	ADD		$(1<<28), R3
	ADD		$(1<<28), R4
	MOVW	R3, SPR(IBATU(1))
	MOVW	R4, SPR(IBATL(1))
	MOVW	R3, SPR(DBATU(1))
	MOVW	R4, SPR(DBATL(1))

	/* KZERO+512M -> 512M */
	ADD		$(1<<28), R3
	ADD		$(1<<28), R4
	MOVW	R3, SPR(IBATU(2))
	MOVW	R4, SPR(IBATL(2))
	MOVW	R3, SPR(DBATU(2))
	MOVW	R4, SPR(DBATL(2))

	/* direct map last block, uncached, (?guarded) */
	MOVW	$((0xf<<28)|(0x7ff<<2)|2), R3
	MOVW	$((0xf<<28)|PTE1_I|PTE1_G|PTE1_RW), R4
	MOVW	R3, SPR(DBATU(3))
	MOVW	R4, SPR(DBATL(3))

	/* IBAT 3 unused */
	MOVW	R0, SPR(IBATU(3))
	MOVW	R0, SPR(IBATL(3))

	/* enable MMU */
	MOVW	LR, R3
	OR	$KZERO, R3
	MOVW	R3, SPR(SRR0)
	MOVW	MSR, R4
	OR	$(MSR_IR|MSR_DR), R4
	MOVW	R4, SPR(SRR1)
	RFI	/* resume in kernel mode in caller */

	RETURN

TEXT	kfpinit(SB), $0
	MOVFL	$0,FPSCR(7)
	MOVFL	$0xD,FPSCR(6)	/* VE, OE, ZE */
	MOVFL	$0, FPSCR(5)
	MOVFL	$0, FPSCR(3)
	MOVFL	$0, FPSCR(2)
	MOVFL	$0, FPSCR(1)
	MOVFL	$0, FPSCR(0)

	FMOVD	$4503601774854144.0, F27
	FMOVD	$0.5, F29
	FSUB		F29, F29, F28
	FADD	F29, F29, F30
	FADD	F30, F30, F31
	FMOVD	F28, F0
	FMOVD	F28, F1
	FMOVD	F28, F2
	FMOVD	F28, F3
	FMOVD	F28, F4
	FMOVD	F28, F5
	FMOVD	F28, F6
	FMOVD	F28, F7
	FMOVD	F28, F8
	FMOVD	F28, F9
	FMOVD	F28, F10
	FMOVD	F28, F11
	FMOVD	F28, F12
	FMOVD	F28, F13
	FMOVD	F28, F14
	FMOVD	F28, F15
	FMOVD	F28, F16
	FMOVD	F28, F17
	FMOVD	F28, F18
	FMOVD	F28, F19
	FMOVD	F28, F20
	FMOVD	F28, F21
	FMOVD	F28, F22
	FMOVD	F28, F23
	FMOVD	F28, F24
	FMOVD	F28, F25
	FMOVD	F28, F26
	RETURN

TEXT	splhi(SB), $0
	MOVW	LR, R31
	MOVW	R31, 4(R(MACH))	/* save PC in m->splpc */
	MOVW	MSR, R3
	RLWNM	$0, R3, $~MSR_EE, R4
	SYNC
	MOVW	R4, MSR
	MSRSYNC
	RETURN

TEXT	splx(SB), $0
	/* fall though */

TEXT	splxpc(SB), $0
	MOVW	LR, R31
	MOVW	R31, 4(R(MACH))	/* save PC in m->splpc */
	MOVW	MSR, R4
	RLWMI	$0, R3, $MSR_EE, R4
	SYNC
	MOVW	R4, MSR
	MSRSYNC
	RETURN

TEXT	spllo(SB), $0
	MOVW	MSR, R3
	OR	$MSR_EE, R3, R4
	SYNC
	MOVW	R4, MSR
	MSRSYNC
	RETURN

TEXT	spldone(SB), $0
	RETURN

TEXT	islo(SB), $0
	MOVW	MSR, R3
	RLWNM	$0, R3, $MSR_EE, R3
	RETURN

TEXT	setlabel(SB), $-4
	MOVW	LR, R31
	MOVW	R1, 0(R3)
	MOVW	R31, 4(R3)
	MOVW	$0, R3
	RETURN

TEXT	gotolabel(SB), $-4
	MOVW	4(R3), R31
	MOVW	R31, LR
	MOVW	0(R3), R1
	MOVW	$1, R3
	RETURN

TEXT	touser(SB), $-4
	MOVW	$(UTZERO+32), R5	/* header appears in text */
	MOVW	$(MSR_EE|MSR_PR|MSR_ME|MSR_IR|MSR_DR|MSR_RI), R4
	MOVW	R4, SPR(SRR1)
	MOVW	R3, R1
	MOVW	R5, SPR(SRR0)
	RFI

TEXT	icflush(SB), $-4	/* icflush(virtaddr, count) */
	MOVW	n+4(FP), R4
	RLWNM	$0, R3, $~(CACHELINESZ-1), R5
	SUB	R5, R3
	ADD	R3, R4
	ADD		$(CACHELINESZ-1), R4
	SRAW	$CACHELINELOG, R4
	MOVW	R4, CTR
icf0:	ICBI	(R5)
	ADD	$CACHELINESZ, R5
	BDNZ	icf0
	ISYNC
	RETURN

TEXT	dcflush(SB), $-4	/* dcflush(virtaddr, count) */
	MOVW	n+4(FP), R4
	RLWNM	$0, R3, $~(CACHELINESZ-1), R5
	CMP	R4, $0
	BLE	dcf1
	SUB	R5, R3
	ADD	R3, R4
	ADD		$(CACHELINESZ-1), R4
	SRAW	$CACHELINELOG, R4
	MOVW	R4, CTR
dcf0:	DCBF	(R5)
	ADD	$CACHELINESZ, R5
	BDNZ	dcf0
dcf1:
	SYNC
	RETURN

TEXT	tas(SB), $0
	SYNC
	MOVW	R3, R4
	MOVW	$0xdead,R5
tas1:
	DCBF	(R4)	/* fix for 603x bug */
	LWAR	(R4), R3
	CMP	R3, $0
	BNE	tas0
	STWCCC	R5, (R4)
	BNE	tas1
tas0:
	SYNC
	ISYNC
	RETURN

TEXT	_xinc(SB),$0	/* void _xinc(long *); */
	MOVW	R3, R4
xincloop:
	DCBF	(R4)	/* fix for 603x bug */
	LWAR	(R4), R3
	ADD		$1, R3
	STWCCC	R3, (R4)
	BNE		xincloop
	RETURN

TEXT	_xdec(SB),$0	/* long _xdec(long *); */
	MOVW	R3, R4
xdecloop:
	DCBF	(R4)	/* fix for 603x bug */
	LWAR	(R4), R3
	ADD		$-1, R3
	STWCCC	R3, (R4)
	BNE		xdecloop
	RETURN

TEXT cmpswap(SB),$0	/* int cmpswap(long*, long, long) */
	MOVW	R3, R4	/* addr */
	MOVW	old+4(FP), R5
	MOVW	new+8(FP), R6
	DCBF	(R4)		/* fix for 603x bug? */
	LWAR	(R4), R3
	CMP	R3, R5
	BNE fail
	STWCCC	R6, (R4)
	BNE fail
	MOVW $1, R3
	RETURN
fail:
	MOVW $0, R3
	RETURN

TEXT	getpvr(SB), $0
	MOVW	SPR(PVR), R3
	RETURN

TEXT	getdec(SB), $0
	MOVW	SPR(DEC), R3
	RETURN

TEXT	putdec(SB), $0
	MOVW	R3, SPR(DEC)
	RETURN

TEXT	getdar(SB), $0
	MOVW	SPR(DAR), R3
	RETURN

TEXT	getdsisr(SB), $0
	MOVW	SPR(DSISR), R3
	RETURN

TEXT	getmsr(SB), $0
	MOVW	MSR, R3
	RETURN

TEXT	putmsr(SB), $0
	SYNC
	MOVW	R3, MSR
	MSRSYNC
	RETURN

TEXT	putsdr1(SB), $0
	MOVW	R3, SPR(SDR1)
	RETURN

TEXT	putsr(SB), $0
	MOVW	4(FP), R4
	MOVW	R4, SEG(R3)
	RETURN

TEXT	gethid0(SB), $0
	MOVW	SPR(HID0), R3
	RETURN

TEXT	gethid1(SB), $0
	MOVW	SPR(HID1), R3
	RETURN

TEXT	puthid0(SB), $0
	MOVW	R3, SPR(HID0)
	RETURN

TEXT	puthid1(SB), $0
	MOVW	R3, SPR(HID1)
	RETURN

TEXT	eieio(SB), $0
	EIEIO
	RETURN

TEXT	sync(SB), $0
	SYNC
	RETURN

TEXT	tlbflushall(SB), $0
	MOVW	$64, R3
	MOVW	R3, CTR
	MOVW	$0, R4
tlbflushall0:
	TLBIE	R4
	ADD		$BIT(19), R4
	BDNZ	tlbflushall0
	EIEIO
	TLBSYNC
	SYNC
	RETURN

TEXT	tlbflush(SB), $0
	TLBIE	R3
	RETURN

TEXT	gotopc(SB), $0
	MOVW	R3, CTR
	MOVW	LR, R31	/* for trace back */
	BR	(CTR)

/*
 * traps force memory mapping off.
 * the following code has been executed at the exception
 * vector location
 *	MOVW R0, SPR(SAVER0)
 *	MOVW LR, R0
 *	MOVW R0, SPR(SAVELR) 
 *	bl	trapvec(SB)
 */
TEXT	trapvec(SB), $-4
	MOVW	LR, R0
	MOVW	R1, SPR(SAVER1)
	MOVW	R0, SPR(SAVEXX)	/* vector */

	/* did we come from user space */
	MOVW	SPR(SRR1), R0
	MOVW	CR, R1
	MOVW	R0, CR
	BC	4,17,ktrap
	
	/* switch to kernel stack */
	MOVW	R1, CR
	MOVW	R2, R0
	MOVW	$setSB(SB), R2
	RLWNM	$0, R2, $~KZERO, R2		/* PADDR(setSB) */
	MOVW	$mach0(SB), R1	/* m-> */
	RLWNM	$0, R1, $~KZERO, R1		/* PADDR(m->) */
	MOVW	8(R1), R1				/* m->proc  */
	RLWNM	$0, R1, $~KZERO, R1		/* PADDR(m->proc) */
	MOVW	8(R1), R1				/* m->proc->kstack */
	RLWNM	$0, R1, $~KZERO, R1		/* PADDR(m->proc->kstack) */
	ADD	$(KSTACK-UREGSIZE), R1
	MOVW	R0, R2
	BL	saveureg(SB)
	BL	trap(SB)
	BR	restoreureg
ktrap:
	MOVW	R1, CR
	MOVW	SPR(SAVER1), R1
	RLWNM	$0, R1, $~KZERO, R1		/* PADDR(R1) */
	SUB	$UREGSPACE, R1
	BL	saveureg(SB)
	BL	trap(SB)
	BR	restoreureg

/*
 * enter with stack set and mapped.
 * on return, SB (R2) has been set, and R3 has the Ureg*,
 * the MMU has been re-enabled, kernel text and PC are in KSEG,
 * R(MACH) has been set, and R0 contains 0.
 *
 */
TEXT	saveureg(SB), $-4
/*
 * save state
 */
	MOVMW	R2, 48(R1)	/* r2:r31 */
	MOVW	$setSB(SB), R2
	RLWNM	$0, R2, $~KZERO, R2		/* PADDR(setSB) */
	MOVW	$mach0(SB), R(MACH)
	RLWNM	$0, R(MACH), $~KZERO, R(MACH)		/* PADDR(m->) */
	MOVW	8(R(MACH)), R(USER)
	MOVW	$mach0(SB), R(MACH)
	MOVW	$setSB(SB), R2
	MOVW	SPR(SAVER1), R4
	MOVW	R4, 44(R1)
	MOVW	SPR(SAVER0), R5
	MOVW	R5, 40(R1)
	MOVW	CTR, R6
	MOVW	R6, 36(R1)
	MOVW	XER, R4
	MOVW	R4, 32(R1)
	MOVW	CR, R5
	MOVW	R5, 28(R1)
	MOVW	SPR(SAVELR), R6	/* LR */
	MOVW	R6, 24(R1)
	/* pad at 20(R1) */
	MOVW	SPR(SRR0), R0
	MOVW	R0, 16(R1)				/* old PC */
	MOVW	SPR(SRR1), R0
	MOVW	R0, 12(R1)				/* old status */
	MOVW	SPR(SAVEXX), R0
	MOVW	R0, 8(R1)	/* cause/vector */
	ADD	$8, R1, R3	/* Ureg* */
	OR	$KZERO, R3	/* fix ureg */
	STWCCC	R3, (R1)	/* break any pending reservations */
	MOVW	$0, R0	/* compiler/linker expect R0 to be zero */

	MOVW	MSR, R5
	OR	$(MSR_IR|MSR_DR|MSR_FP|MSR_RI), R5	/* enable MMU */
	MOVW	R5, SPR(SRR1)
	MOVW	LR, R31
	OR	$KZERO, R31	/* return PC in KSEG0 */
	MOVW	R31, SPR(SRR0)
	OR	$KZERO, R1	/* fix stack pointer */
	RFI	/* returns to trap handler */

/*
 * restore state from Ureg and return from trap/interrupt
 */
TEXT	forkret(SB), $0
	BR	restoreureg

restoreureg:
	MOVMW	48(R1), R2	/* r2:r31 */
	/* defer R1 */
	MOVW	40(R1), R0
	MOVW	R0, SPR(SAVER0)
	MOVW	36(R1), R0
	MOVW	R0, CTR
	MOVW	32(R1), R0
	MOVW	R0, XER
	MOVW	28(R1), R0
	MOVW	R0, CR	/* CR */
	MOVW	24(R1), R0
	MOVW	R0, LR
	/* pad, skip */
	MOVW	16(R1), R0
	MOVW	R0, SPR(SRR0)	/* old PC */
	MOVW	12(R1), R0
	MOVW	R0, SPR(SRR1)	/* old MSR */
	/* cause, skip */
	MOVW	44(R1), R1	/* old SP */
	MOVW	SPR(SAVER0), R0
	RFI

TEXT	fpsave(SB), $0
	FMOVD	F0, (0*8)(R3)
	FMOVD	F1, (1*8)(R3)
	FMOVD	F2, (2*8)(R3)
	FMOVD	F3, (3*8)(R3)
	FMOVD	F4, (4*8)(R3)
	FMOVD	F5, (5*8)(R3)
	FMOVD	F6, (6*8)(R3)
	FMOVD	F7, (7*8)(R3)
	FMOVD	F8, (8*8)(R3)
	FMOVD	F9, (9*8)(R3)
	FMOVD	F10, (10*8)(R3)
	FMOVD	F11, (11*8)(R3)
	FMOVD	F12, (12*8)(R3)
	FMOVD	F13, (13*8)(R3)
	FMOVD	F14, (14*8)(R3)
	FMOVD	F15, (15*8)(R3)
	FMOVD	F16, (16*8)(R3)
	FMOVD	F17, (17*8)(R3)
	FMOVD	F18, (18*8)(R3)
	FMOVD	F19, (19*8)(R3)
	FMOVD	F20, (20*8)(R3)
	FMOVD	F21, (21*8)(R3)
	FMOVD	F22, (22*8)(R3)
	FMOVD	F23, (23*8)(R3)
	FMOVD	F24, (24*8)(R3)
	FMOVD	F25, (25*8)(R3)
	FMOVD	F26, (26*8)(R3)
	FMOVD	F27, (27*8)(R3)
	FMOVD	F28, (28*8)(R3)
	FMOVD	F29, (29*8)(R3)
	FMOVD	F30, (30*8)(R3)
	FMOVD	F31, (31*8)(R3)
	MOVFL	FPSCR, F0
	FMOVD	F0, (32*8)(R3)
	RETURN

TEXT	fprestore(SB), $0
	FMOVD	(32*8)(R3), F0
	MOVFL	F0, FPSCR
	FMOVD	(0*8)(R3), F0
	FMOVD	(1*8)(R3), F1
	FMOVD	(2*8)(R3), F2
	FMOVD	(3*8)(R3), F3
	FMOVD	(4*8)(R3), F4
	FMOVD	(5*8)(R3), F5
	FMOVD	(6*8)(R3), F6
	FMOVD	(7*8)(R3), F7
	FMOVD	(8*8)(R3), F8
	FMOVD	(9*8)(R3), F9
	FMOVD	(10*8)(R3), F10
	FMOVD	(11*8)(R3), F11
	FMOVD	(12*8)(R3), F12
	FMOVD	(13*8)(R3), F13
	FMOVD	(14*8)(R3), F14
	FMOVD	(15*8)(R3), F15
	FMOVD	(16*8)(R3), F16
	FMOVD	(17*8)(R3), F17
	FMOVD	(18*8)(R3), F18
	FMOVD	(19*8)(R3), F19
	FMOVD	(20*8)(R3), F20
	FMOVD	(21*8)(R3), F21
	FMOVD	(22*8)(R3), F22
	FMOVD	(23*8)(R3), F23
	FMOVD	(24*8)(R3), F24
	FMOVD	(25*8)(R3), F25
	FMOVD	(26*8)(R3), F26
	FMOVD	(27*8)(R3), F27
	FMOVD	(28*8)(R3), F28
	FMOVD	(29*8)(R3), F29
	FMOVD	(30*8)(R3), F30
	FMOVD	(31*8)(R3), F31
	RETURN
