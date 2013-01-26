#include	"mem.h"

/* use of SPRG registers in save/restore */
#define	SAVER0	SPRG0
#define	SAVER1	SPRG1
#define	SAVELR	SPRG2
#define	SAVEXX	SPRG3

#ifdef ucuconf
/* These only exist on the PPC 755: */
#define	SAVER4	SPRG4
#define	SAVER5	SPRG5
#define	SAVER6	SPRG6
#define	SAVER7	SPRG7
#endif /* ucuconf */

/* special instruction definitions */
#define	BDNZ		BC	16, 0,
#define	BDNE		BC	0, 2,
#define	MTCRF(r, crm)	WORD	$((31<<26)|((r)<<21)|(crm<<12)|(144<<1))

/* #define	TLBIA	WORD	$((31<<26)|(370<<1)) Not implemented on the 603e */
#define	TLBSYNC		WORD	$((31<<26)|(566<<1))
#define	TLBLI(n)	WORD	$((31<<26)|((n)<<11)|(1010<<1))
#define	TLBLD(n)	WORD	$((31<<26)|((n)<<11)|(978<<1))

/* on some models mtmsr doesn't synchronise enough (eg, 603e) */
#define	MSRSYNC	SYNC

#define	UREGSPACE	(UREGSIZE+8)

TEXT start(SB), $-4

	/*
	 * setup MSR
	 * turn off interrupts
	 * use 0x000 as exception prefix
	 * enable machine check
	 */
	MOVW	MSR, R3
	MOVW	$(MSR_ME|MSR_EE|MSR_IP), R4
	ANDN	R4, R3
	SYNC
	MOVW	R3, MSR
	MSRSYNC

	/* except during trap handling, R0 is zero from now on */
	MOVW	$0, R0

	/* setup SB for pre mmu */
	MOVW	$setSB(SB), R2
	MOVW	$KZERO, R3
	ANDN	R3, R2

	/* before this we're not running above KZERO */
	BL	mmuinit0(SB)
	/* after this we are */

#ifdef ucuconf
	MOVW	$0x2000000, R4		/* size */
	MOVW	$0, R3			/* base address */
	RLWNM	$0, R3, $~(CACHELINESZ-1), R5
	CMP	R4, $0
	BLE	_dcf1
	SUB	R5, R3
	ADD	R3, R4
	ADD	$(CACHELINESZ-1), R4
	SRAW	$CACHELINELOG, R4
	MOVW	R4, CTR
_dcf0:	DCBF	(R5)
	ADD	$CACHELINESZ, R5
	BDNZ	_dcf0
_dcf1:
	SYNC

	/* BAT0, 3 unused, copy of BAT2 */
	MOVW	SPR(IBATL(2)), R3
	MOVW	R3, SPR(IBATL(0))
	MOVW	SPR(IBATU(2)), R3
	MOVW	R3, SPR(IBATU(0))
	MOVW	SPR(DBATL(2)), R3
	MOVW	R3, SPR(DBATL(0))
	MOVW	SPR(DBATU(2)), R3
	MOVW	R3, SPR(DBATU(0))

	MOVW	SPR(IBATL(2)), R3
	MOVW	R3, SPR(IBATL(3))
	MOVW	SPR(IBATU(2)), R3
	MOVW	R3, SPR(IBATU(3))
	MOVW	SPR(DBATL(2)), R3
	MOVW	R3, SPR(DBATL(3))
	MOVW	SPR(DBATU(2)), R3
	MOVW	R3, SPR(DBATU(3))
#endif /* ucuconf */

	/* running with MMU on!! */

	/* set R2 to correct value */
	MOVW	$setSB(SB), R2

	/* set up Mach */
	MOVW	$MACHADDR, R(MACH)
	ADD	$(MACHSIZE-8), R(MACH), R1	/* set stack */

	MOVW	R0, R(USER)		/* up-> set to zero */
	MOVW	R0, 0(R(MACH))		/* machno set to zero */

	BL	main(SB)

	RETURN				/* not reached */

/*
 * on return from this function we will be running in virtual mode.
 * We set up the Block Address Translation (BAT) registers thus:
 * 1) first 3 BATs are 256M blocks, starting from KZERO->0
 * 2) remaining BAT maps last 256M directly
 */
TEXT mmuinit0(SB), $0
	/* reset all the tlbs */
	MOVW	$64, R3
	MOVW	R3, CTR
	MOVW	$0, R4

tlbloop:
	TLBIE	R4
	SYNC
	ADD	$BIT(19), R4
	BDNZ	tlbloop
	TLBSYNC

#ifndef ucuconf
	/* BATs 0 and 1 cover memory from 0x00000000 to 0x20000000 */

	/* KZERO -> 0, IBAT and DBAT, 256 MB */
	MOVW	$(KZERO|(0x7ff<<2)|2), R3
	MOVW	$(PTEVALID|PTEWRITE), R4	/* PTEVALID => Cache coherency on */
	MOVW	R3, SPR(IBATU(0))
	MOVW	R4, SPR(IBATL(0))
	MOVW	R3, SPR(DBATU(0))
	MOVW	R4, SPR(DBATL(0))

	/* KZERO+256M -> 256M, IBAT and DBAT, 256 MB */
	ADD	$(1<<28), R3
	ADD	$(1<<28), R4
	MOVW	R3, SPR(IBATU(1))
	MOVW	R4, SPR(IBATL(1))
	MOVW	R3, SPR(DBATU(1))
	MOVW	R4, SPR(DBATL(1))

	/* FPGABASE -> FPGABASE, DBAT, 16 MB */
	MOVW	$(FPGABASE|(0x7f<<2)|2), R3
	MOVW	$(FPGABASE|PTEWRITE|PTEUNCACHED), R4	/* FPGA memory, don't cache */
	MOVW	R3, SPR(DBATU(2))
	MOVW	R4, SPR(DBATL(2))

	/* IBAT 2 unused */
	MOVW	R0, SPR(IBATU(2))
	MOVW	R0, SPR(IBATL(2))

	/* direct map last block, uncached, (not guarded, doesn't work for BAT), DBAT only */
	MOVW	$(INTMEM|(0x7ff<<2)|2), R3
	MOVW	$(INTMEM|PTEWRITE|PTEUNCACHED), R4	/* Don't set PTEVALID here */
	MOVW	R3, SPR(DBATU(3))
	MOVW	R4, SPR(DBATL(3))

	/* IBAT 3 unused */
	MOVW	R0, SPR(IBATU(3))
	MOVW	R0, SPR(IBATL(3))
#else /* ucuconf */
	/* BAT 2 covers memory from 0x00000000 to 0x10000000 */

	/* KZERO -> 0, IBAT2 and DBAT2, 256 MB */
	MOVW	$(KZERO|(0x7ff<<2)|2), R3
	MOVW	$(PTEVALID|PTEWRITE), R4	/* PTEVALID => Cache coherency on */
	MOVW	R3, SPR(DBATU(2))
	MOVW	R4, SPR(DBATL(2))
	MOVW	R3, SPR(IBATU(2))
	MOVW	R4, SPR(IBATL(2))
#endif /* ucuconf */

	/* enable MMU */
	MOVW	LR, R3
	OR	$KZERO, R3
	MOVW	R3, SPR(SRR0)		/* Stored PC for RFI instruction */
	MOVW	MSR, R4
	OR	$(MSR_IR|MSR_DR|MSR_RI|MSR_FP), R4
	MOVW	R4, SPR(SRR1)
	RFI				/* resume in kernel mode in caller */

	RETURN

TEXT kfpinit(SB), $0
	MOVFL	$0, FPSCR(7)
	MOVFL	$0xD, FPSCR(6)		/* VE, OE, ZE */
	MOVFL	$0, FPSCR(5)
	MOVFL	$0, FPSCR(3)
	MOVFL	$0, FPSCR(2)
	MOVFL	$0, FPSCR(1)
	MOVFL	$0, FPSCR(0)

	FMOVD	$4503601774854144.0, F27
	FMOVD	$0.5, F29
	FSUB	F29, F29, F28
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

TEXT splhi(SB), $0
	MOVW	LR, R31
	MOVW	R31, 4(R(MACH))		/* save PC in m->splpc */
	MOVW	MSR, R3
	RLWNM	$0, R3, $~MSR_EE, R4
	SYNC
	MOVW	R4, MSR
	MSRSYNC
	RETURN

TEXT splx(SB), $0
	/* fall though */

TEXT splxpc(SB), $0
	MOVW	LR, R31
	MOVW	R31, 4(R(MACH))		/* save PC in m->splpc */
	MOVW	MSR, R4
	RLWMI	$0, R3, $MSR_EE, R4
	SYNC
	MOVW	R4, MSR
	MSRSYNC
	RETURN

TEXT spllo(SB), $0
	MOVW	MSR, R3
	OR	$MSR_EE, R3, R4
	SYNC
	MOVW	R4, MSR
	MSRSYNC
	RETURN

TEXT spldone(SB), $0
	RETURN

TEXT islo(SB), $0
	MOVW	MSR, R3
	RLWNM	$0, R3, $MSR_EE, R3
	RETURN

TEXT setlabel(SB), $-4
	MOVW	LR, R31
	MOVW	R1, 0(R3)
	MOVW	R31, 4(R3)
	MOVW	$0, R3
	RETURN

TEXT gotolabel(SB), $-4
	MOVW	4(R3), R31
	MOVW	R31, LR
	MOVW	0(R3), R1
	MOVW	$1, R3
	RETURN

TEXT touser(SB), $-4
	MOVW	$(UTZERO+32), R5	/* header appears in text */
	MOVW	$(MSR_EE|MSR_PR|MSR_IR|MSR_DR|MSR_RI), R4
	MOVW	R4, SPR(SRR1)
	MOVW	R3, R1
	MOVW	R5, SPR(SRR0)
	RFI

TEXT dczap(SB), $-4			/* dczap(virtaddr, count) */
	MOVW	n+4(FP), R4
	RLWNM	$0, R3, $~(CACHELINESZ-1), R5
	CMP	R4, $0
	BLE	dcz1
	SUB	R5, R3
	ADD	R3, R4
	ADD	$(CACHELINESZ-1), R4
	SRAW	$CACHELINELOG, R4
	MOVW	R4, CTR
dcz0:
	DCBI	(R5)
	ADD	$CACHELINESZ, R5
	BDNZ	dcz0
dcz1:
	SYNC
	RETURN

TEXT dcflush(SB), $-4			/* dcflush(virtaddr, count) */
	MOVW	n+4(FP), R4
	RLWNM	$0, R3, $~(CACHELINESZ-1), R5
	CMP	R4, $0
	BLE	dcf1
	SUB	R5, R3
	ADD	R3, R4
	ADD	$(CACHELINESZ-1), R4
	SRAW	$CACHELINELOG, R4
	MOVW	R4, CTR
dcf0:	DCBST	(R5)
	ADD	$CACHELINESZ, R5
	BDNZ	dcf0
dcf1:
	SYNC
	RETURN

TEXT icflush(SB), $-4			/* icflush(virtaddr, count) */
	MOVW	n+4(FP), R4
	RLWNM	$0, R3, $~(CACHELINESZ-1), R5
	CMP	R4, $0
	BLE	icf1
	SUB	R5, R3
	ADD	R3, R4
	ADD	$(CACHELINESZ-1), R4
	SRAW	$CACHELINELOG, R4
	MOVW	R4, CTR
icf0:	ICBI	(R5)			/* invalidate the instruction cache */
	ADD	$CACHELINESZ, R5
	BDNZ	icf0
	ISYNC
icf1:
	RETURN

TEXT tas(SB), $0
	MOVW	R3, R4
	MOVW	$0xdead, R5
tas1:
	DCBF	(R4)			/* fix for 603x bug */
	SYNC
	LWAR	(R4), R3
	CMP	R3, $0
	BNE	tas0
	STWCCC	R5, (R4)
	BNE	tas1
	EIEIO
tas0:
	SYNC
	RETURN

TEXT _xinc(SB), $0			/* void _xinc(long *); */
	MOVW	R3, R4
xincloop:
	DCBF	(R4)			/* fix for 603x bug */
	LWAR	(R4), R3
	ADD	$1, R3
	STWCCC	R3, (R4)
	BNE	xincloop
	RETURN

TEXT _xdec(SB), $0			/* long _xdec(long *); */
	MOVW	R3, R4
xdecloop:
	DCBF	(R4)			/* fix for 603x bug */
	LWAR	(R4), R3
	ADD	$-1, R3
	STWCCC	R3, (R4)
	BNE	xdecloop
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

TEXT tlbflushall(SB), $0
	MOVW	$TLBENTRIES, R3
	MOVW	R3, CTR
	MOVW	$0, R4
	ISYNC
tlbflushall0:
	TLBIE	R4
	SYNC
	ADD	$BIT(19), R4
	BDNZ	tlbflushall0
	TLBSYNC
	RETURN

TEXT tlbflush(SB), $0
	ISYNC
	TLBIE	R3
	SYNC
	TLBSYNC
	RETURN

TEXT gotopc(SB), $0
	MOVW	R3, CTR
	MOVW	LR, R31			/* for trace back */
	BR	(CTR)

/* On an imiss, we get here.  If we can resolve it, we do.
 * Otherwise take the real trap.  The code at the vector is
 *	MOVW	R0, SPR(SAVER0)	No point to this, of course
 *	MOVW	LR, R0
 *	MOVW	R0, SPR(SAVELR)
 *	BL	imiss(SB)		or dmiss, as the case may be
 *	BL	tlbvec(SB)
 */
TEXT imiss(SB), $-4
	/* Statistics */
	MOVW	$MACHPADDR, R1
	MOVW	0xc(R1), R3		/* count m->tlbfault */
	ADD	$1, R3
	MOVW	R3, 0xc(R1)
	MOVW	0x10(R1), R3		/* count m->imiss */
	ADD	$1, R3
	MOVW	R3, 0x10(R1)

	/* Real work */
	MOVW	SPR(HASH1), R1		/* (phys) pointer into the hash table */
	ADD	$BY2PTEG, R1, R2	/* end pointer */
	MOVW	SPR(iCMP), R3		/* pattern to look for */
imiss1:
	MOVW	(R1), R0
	CMP	R3, R0
	BEQ	imiss2			/* found the entry */
	ADD	$8, R1
	CMP	R1, R2			/* test end of loop */
	BNE	imiss1			/* Loop */
	/* Failed to find an entry; take the full trap */
	MOVW	SPR(SRR1), R1
	MTCRF(1, 0x80)			/* restore CR0 bits (they're auto saved in SRR1) */
	RETURN
imiss2:
	/* Found the entry */
	MOVW	4(R1), R2		/* Phys addr */
	MOVW	R2, SPR(RPA)
	MOVW	SPR(IMISS), R3
	TLBLI(3)

	/* Restore Registers */
	MOVW	SPR(SRR1), R1		/* Restore the CR0 field of the CR register from SRR1 */
	MTCRF(1, 0x80)
	MOVW	SPR(SAVELR), R0
	MOVW	R0, LR
	RFI

/* On a data load or store miss, we get here.  If we can resolve it, we do.
 * Otherwise take the real trap
 */
TEXT dmiss(SB), $-4
	/* Statistics */
	MOVW	$MACHPADDR, R1
	MOVW	0xc(R1), R3		/* count m->tlbfault */
	ADD	$1, R3
	MOVW	R3, 0xc(R1)
	MOVW	0x14(R1), R3		/* count m->dmiss */
	ADD	$1, R3
	MOVW	R3, 0x14(R1)
	/* Real work */
	MOVW	SPR(HASH1), R1		/* (phys) pointer into the hash table */
	ADD	$BY2PTEG, R1, R2	/* end pointer */
	MOVW	SPR(DCMP), R3		/* pattern to look for */
dmiss1:
	MOVW	(R1), R0
	CMP	R3, R0
	BEQ	dmiss2			/* found the entry */
	ADD	$8, R1
	CMP	R1, R2			/* test end of loop */
	BNE	dmiss1			/* Loop */
	/* Failed to find an entry; take the full trap */
	MOVW	SPR(SRR1), R1
	MTCRF(1, 0x80)			/* restore CR0 bits (they're auto saved in SRR1) */
	RETURN
dmiss2:
	/* Found the entry */
	MOVW	4(R1), R2		/* Phys addr */
	MOVW	R2, SPR(RPA)
	MOVW	SPR(DMISS), R3
	TLBLD(3)
	/* Restore Registers */
	MOVW	SPR(SRR1), R1		/* Restore the CR0 field of the CR register from SRR1 */
	MTCRF(1, 0x80)
	MOVW	SPR(SAVELR), R0
	MOVW	R0, LR
	RFI

/*
 * When a trap sets the TGPR bit (TLB miss traps do this),
 * registers get remapped: R0-R31 are temporarily inaccessible,
 * and Temporary Registers TR0-TR3 are mapped onto R0-R3.
 * While this bit is set, R4-R31 cannot be used.
 * The code at the vector has executed this code before
 * coming to tlbvec:
 *	MOVW	R0, SPR(SAVER0)	No point to this, of course
 *	MOVW	LR, R0
 *	MOVW	R0, SPR(SAVELR)
 *	BL	tlbvec(SB)
 * SAVER0 can be reused.  We're not interested in the value of TR0
 */
TEXT tlbvec(SB), $-4
 	MOVW	MSR, R1
	RLWNM	$0, R1, $~MSR_TGPR, R1	/* Clear the dreaded TGPR bit in the MSR */
	SYNC
	MOVW	R1, MSR
	MSRSYNC
	/* Now the GPRs are what they're supposed to be, save R0 again */
	MOVW	R0, SPR(SAVER0)
	/* Fall through to trapvec */

/*
 * traps force memory mapping off.
 * the following code has been executed at the exception
 * vector location
 *	MOVW	R0, SPR(SAVER0)
 *	MOVW	LR, R0
 *	MOVW	R0, SPR(SAVELR)
 *	bl	trapvec(SB)
 *
 */
TEXT trapvec(SB), $-4
	MOVW	LR, R0
	MOVW	R1, SPR(SAVER1)
	MOVW	R0, SPR(SAVEXX)		/* vector */

	/* did we come from user space */
	MOVW	SPR(SRR1), R0
	MOVW	CR, R1
	MOVW	R0, CR
	BC	4, 17, ktrap

	/* switch to kernel stack */
	MOVW	R1, CR
	MOVW	$MACHPADDR, R1		/* PADDR(m->) */
	MOVW	8(R1), R1		/* m->proc */
	RLWNM	$0, R1, $~KZERO, R1	/* PADDR(m->proc) */
	MOVW	8(R1), R1		/* m->proc->kstack */
	RLWNM	$0, R1, $~KZERO, R1	/* PADDR(m->proc->kstack) */
	ADD	$(KSTACK-UREGSIZE), R1	/* make room on stack */

	BL	saveureg(SB)
	BL	trap(SB)
	BR	restoreureg

ktrap:
	MOVW	R1, CR
	MOVW	SPR(SAVER1), R1
	RLWNM	$0, R1, $~KZERO, R1	/* set stack pointer */
	SUB	$UREGSPACE, R1

	BL	saveureg(SB)		/* addressed relative to PC */
	BL	trap(SB)
	BR	restoreureg

/*
 * enter with stack set and mapped.
 * on return, SB (R2) has been set, and R3 has the Ureg*,
 * the MMU has been re-enabled, kernel text and PC are in KSEG,
 * R(MACH) has been set, and R0 contains 0.
 *
 */
TEXT saveureg(SB), $-4
/*
 * save state
 */
	MOVMW	R2, 48(R1)		/* save r2 .. r31 in 48(R1) .. 164(R1) */
	MOVW	$MACHPADDR, R(MACH)	/* PADDR(m->) */
	MOVW	8(R(MACH)), R(USER)	/* up-> */
	MOVW	$MACHADDR, R(MACH)	/* m-> */
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
	MOVW	SPR(SAVELR), R6		/* LR */
	MOVW	R6, 24(R1)
	/* pad at 20(R1) */
	MOVW	SPR(SRR0), R0
	MOVW	R0, 16(R1)		/* old PC */
	MOVW	SPR(SRR1), R0
	MOVW	R0, 12(R1)		/* old status */
	MOVW	SPR(SAVEXX), R0
	MOVW	R0, 8(R1)		/* cause/vector */
	MOVW	SPR(DCMP), R0
	MOVW	R0, (160+8)(R1)
	MOVW	SPR(iCMP), R0
	MOVW	R0, (164+8)(R1)
	MOVW	SPR(DMISS), R0
	MOVW	R0, (168+8)(R1)
	MOVW	SPR(IMISS), R0
	MOVW	R0, (172+8)(R1)
	MOVW	SPR(HASH1), R0
	MOVW	R0, (176+8)(R1)
	MOVW	SPR(HASH2), R0
	MOVW	R0, (180+8)(R1)
	MOVW	SPR(DAR), R0
	MOVW	R0, (184+8)(R1)
	MOVW	SPR(DSISR), R0
	MOVW	R0, (188+8)(R1)
	ADD	$8, R1, R3		/* Ureg* */
	OR	$KZERO, R3		/* fix ureg */
	STWCCC	R3, (R1)		/* break any pending reservations */
	MOVW	$0, R0			/* compiler/linker expect R0 to be zero */
	MOVW	$setSB(SB), R2		/* SB register */

	MOVW	MSR, R5
	OR	$(MSR_IR|MSR_DR|MSR_FP|MSR_RI), R5	/* enable MMU */
	MOVW	R5, SPR(SRR1)
	MOVW	LR, R31
	OR	$KZERO, R31		/* return PC in KSEG0 */
	MOVW	R31, SPR(SRR0)
	OR	$KZERO, R1		/* fix stack pointer */
	RFI				/* returns to trap handler */

/*
 * restore state from Ureg and return from trap/interrupt
 */
TEXT forkret(SB), $0
	BR	restoreureg

restoreureg:
	MOVMW	48(R1), R2		/* restore r2 through r31 */
	/* defer R1 */
	MOVW	40(R1), R0
	MOVW	R0, SPR(SAVER0)		/* resave saved R0 */
	MOVW	36(R1), R0
	MOVW	R0, CTR
	MOVW	32(R1), R0
	MOVW	R0, XER
	MOVW	28(R1), R0
	MOVW	R0, CR			/* Condition register*/
	MOVW	24(R1), R0
	MOVW	R0, LR
	/* pad, skip */
	MOVW	16(R1), R0
	MOVW	R0, SPR(SRR0)		/* old PC */
	MOVW	12(R1), R0
	MOVW	R0, SPR(SRR1)		/* old MSR */
	/* cause, skip */
	MOVW	44(R1), R1		/* old SP */
	MOVW	SPR(SAVER0), R0
	RFI

TEXT getpvr(SB), $0
	MOVW	SPR(PVR), R3
	RETURN

TEXT getdec(SB), $0
	MOVW	SPR(DEC), R3
	RETURN

TEXT putdec(SB), $0
	MOVW	R3, SPR(DEC)
	RETURN

TEXT getdar(SB), $0
	MOVW	SPR(DAR), R3
	RETURN

TEXT getdsisr(SB), $0
	MOVW	SPR(DSISR), R3
	RETURN

TEXT getmsr(SB), $0
	MOVW	MSR, R3
	RETURN

TEXT putmsr(SB), $0
	MOVW	R3, MSR
	MSRSYNC
	RETURN

TEXT putsdr1(SB), $0
	SYNC
	MOVW	R3, SPR(SDR1)
	ISYNC
	RETURN

TEXT putsr(SB), $0
	MOVW	4(FP), R4
	SYNC
	MOVW	R4, SEG(R3)
	MSRSYNC
	RETURN

TEXT getsr(SB), $0
	MOVW	SEG(R3), R3
	RETURN

TEXT gethid0(SB), $0
	MOVW	SPR(HID0), R3
	RETURN

TEXT puthid0(SB), $0
	SYNC
	ISYNC
	MOVW	R3, SPR(HID0)
	SYNC
	RETURN

TEXT gethid1(SB), $0
	MOVW	SPR(HID1), R3
	RETURN

TEXT gethid2(SB), $0
	MOVW	SPR(HID2), R3
	RETURN

TEXT puthid2(SB), $0
	MOVW	R3, SPR(HID2)
	RETURN

TEXT eieio(SB), $0
	EIEIO
	RETURN

TEXT sync(SB), $0
	SYNC
	RETURN

/* Power PC 603e specials */
TEXT getimiss(SB), $0
	MOVW	SPR(IMISS), R3
	RETURN

TEXT geticmp(SB), $0
	MOVW	SPR(iCMP), R3
	RETURN

TEXT puticmp(SB), $0
	MOVW	R3, SPR(iCMP)
	RETURN

TEXT getdmiss(SB), $0
	MOVW	SPR(DMISS), R3
	RETURN

TEXT getdcmp(SB), $0
	MOVW	SPR(DCMP), R3
	RETURN

TEXT putdcmp(SB), $0
	MOVW	R3, SPR(DCMP)
	RETURN

TEXT getsdr1(SB), $0
	MOVW	SPR(SDR1), R3
	RETURN

TEXT gethash1(SB), $0
	MOVW	SPR(HASH1), R3
	RETURN

TEXT puthash1(SB), $0
	MOVW	R3, SPR(HASH1)
	RETURN

TEXT gethash2(SB), $0
	MOVW	SPR(HASH2), R3
	RETURN

TEXT puthash2(SB), $0
	MOVW	R3, SPR(HASH2)
	RETURN

TEXT getrpa(SB), $0
	MOVW	SPR(RPA), R3
	RETURN

TEXT putrpa(SB), $0
	MOVW	R3, SPR(RPA)
	RETURN

TEXT tlbli(SB), $0
	TLBLI(3)
	ISYNC
	RETURN

TEXT tlbld(SB), $0
	SYNC
	TLBLD(3)
	ISYNC
	RETURN

TEXT getsrr1(SB), $0
	MOVW	SPR(SRR1), R3
	RETURN

TEXT putsrr1(SB), $0
	MOVW	R3, SPR(SRR1)
	RETURN

TEXT fpsave(SB), $0
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

TEXT fprestore(SB), $0
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

TEXT dcacheenb(SB), $0
	SYNC
	MOVW	SPR(HID0), R4		/* Get HID0 and clear unwanted bits */
	RLWNM	$0, R4, $~(HID_DLOCK), R4
	MOVW	$(HID_DCFI|HID_DCE), R5
	OR	R4, R5
	MOVW	$HID_DCE, R3
	OR	R4, R3
	SYNC
//	MOVW	R5, SPR(HID0)		/* Cache enable and flash invalidate */
	MOVW	R3, SPR(HID0)		/* Cache enable */
	SYNC
	RETURN

TEXT icacheenb(SB), $0
	SYNC
	MOVW	SPR(HID0), R4		/* Get HID0 and clear unwanted bits */
	RLWNM	$0, R4, $~(HID_ILOCK), R4
	MOVW	$(HID_ICFI|HID_ICE), R5
	OR	R4, R5
	MOVW	$HID_ICE, R3
	OR	R4, R3
	SYNC
	MOVW	R5, SPR(HID0)		/* Cache enable and flash invalidate */
	MOVW	R3, SPR(HID0)		/* Cache enable */
	SYNC
	RETURN

#ifdef ucuconf
TEXT getpll(SB), $0
	MOVW	SPR(1009), R3
	ISYNC
	RETURN

TEXT getl2pm(SB), $0
	MOVW	SPR(1016), R3
	RETURN

TEXT getl2cr(SB), $0
	MOVW	SPR(1017), R3
	RETURN

TEXT putl2cr(SB), $0
	MOVW	R3, SPR(1017)
	RETURN

TEXT dcachedis(SB), $0
	SYNC
/*	MOVW	SPR(HID0), R4
	RLWNM	$0, R4, $~(HID_DCE), R4
	MOVW	R4, SPR(HID0)		/* L1 Cache disable */

	MOVW	SPR(1017), R4
	RLWNM	$0, R4, $~(0x80000000), R4
	MOVW	R4, SPR(1017)		/* L2 Cache disable */

	SYNC
	RETURN

TEXT l2disable(SB), $0
	SYNC
	MOVW	SPR(1017), R4
	RLWNM	$0, R4, $~(0x80000000), R4
	MOVW	R4, SPR(1017)		/* L2 Cache disable */
	SYNC
	RETURN

TEXT getbats(SB), $0
	MOVW	SPR(DBATU(0)), R4
	MOVW	R4, 0(R3)
	MOVW	SPR(DBATL(0)), R4
	MOVW	R4, 4(R3)
	MOVW	SPR(IBATU(0)), R4
	MOVW	R4, 8(R3)
	MOVW	SPR(IBATL(0)), R4
	MOVW	R4, 12(R3)
	MOVW	SPR(DBATU(1)), R4
	MOVW	R4, 16(R3)
	MOVW	SPR(DBATL(1)), R4
	MOVW	R4, 20(R3)
	MOVW	SPR(IBATU(1)), R4
	MOVW	R4, 24(R3)
	MOVW	SPR(IBATL(1)), R4
	MOVW	R4, 28(R3)
	MOVW	SPR(DBATU(2)), R4
	MOVW	R4, 32(R3)
	MOVW	SPR(DBATL(2)), R4
	MOVW	R4, 36(R3)
	MOVW	SPR(IBATU(2)), R4
	MOVW	R4, 40(R3)
	MOVW	SPR(IBATL(2)), R4
	MOVW	R4, 44(R3)
	MOVW	SPR(DBATU(3)), R4
	MOVW	R4, 48(R3)
	MOVW	SPR(DBATL(3)), R4
	MOVW	R4, 52(R3)
	MOVW	SPR(IBATU(3)), R4
	MOVW	R4, 56(R3)
	MOVW	SPR(IBATL(3)), R4
	MOVW	R4, 60(R3)
	RETURN

TEXT setdbat0(SB), $0
	MOVW	0(R3), R4
	MOVW	R4, SPR(DBATU(0))
	MOVW	4(R3), R4
	MOVW	R4, SPR(DBATL(0))
	RETURN
#endif /* ucuconf */

TEXT mmudisable(SB), $0
	/* disable MMU */
	MOVW	LR, R4
	MOVW	$KZERO, R5
	ANDN	R5, R4
	MOVW	R4, SPR(SRR0)		/* Stored PC for RFI instruction */

	MOVW	MSR, R4
	MOVW	$(MSR_IR|MSR_DR|MSR_RI|MSR_FP), R5
	ANDN	R5, R4
	MOVW	R4, SPR(SRR1)

	MOVW	SPR(HID0), R4		/* Get HID0 and clear unwanted bits */
	MOVW	$(HID_ICE|HID_DCE), R5
	ANDN	R5, R4
	MOVW	R4, SPR(HID0)		/* Cache disable */
	RFI				/* resume caller with MMU off */
	RETURN

TEXT kreboot(SB), $0
	BL	mmudisable(SB)
	MOVW	R3, LR
	RETURN

TEXT mul64fract(SB), $0
	MOVW	a0+8(FP), R9
	MOVW	a1+4(FP), R10
	MOVW	b0+16(FP), R4
	MOVW	b1+12(FP), R5

	MULLW	R10, R5, R13		/* c2 = lo(a1*b1) */

	MULLW	R10, R4, R12		/* c1 = lo(a1*b0) */
	MULHWU	R10, R4, R7		/* hi(a1*b0) */
	ADD	R7, R13			/* c2 += hi(a1*b0) */

	MULLW	R9, R5, R6		/* lo(a0*b1) */
	MULHWU	R9, R5, R7		/* hi(a0*b1) */
	ADDC	R6, R12			/* c1 += lo(a0*b1) */
	ADDE	R7, R13			/* c2 += hi(a0*b1) + carry */

	MULHWU	R9, R4, R7		/* hi(a0*b0) */
	ADDC	R7, R12			/* c1 += hi(a0*b0) */
	ADDE	R0, R13			/* c2 += carry */

	MOVW	R12, 4(R3)
	MOVW	R13, 0(R3)
	RETURN
