#include "mem.h"

#define	INVAL		0xF400
#define	PUSH		0xF420
#define	DCH		(1<<6)
#define	ICH		(2<<6)
#define	PAGE		(2<<3)
#define	ALL		(3<<3)

#define	DBMAGIC		0xBADC0C0A

/*
 * Boot first processor
 */
TEXT	start(SB), $-4

	MOVW	$(SUPER|SPL(7)), SR
	MOVL	$a6base(SB), A6
	MOVL	$CDISABLE, R0
	MOVL	R0, CACR

	MOVL	$0x0401A020, R0
	MOVL	R0, ITT0		/* kernel text and data at 0x04000000 */
	MOVL	$0x00000000, R0
	MOVL	R0, ITT1		/* clear other register */
	MOVL	$0x0401A020, R0
	MOVL	R0, DTT0
	MOVL	$0x0BF4A000, R0
	MOVL	R0, DTT1		/* video at 0x0B000000 */

	/*
	 * Clear BSS
	 */
	MOVL	$edata(SB), A0
	MOVL	$end(SB), A1
clearbss:
	CLRL	(A0)+
	CMPL	A0, A1
	BLT	clearbss

	MOVL	$mach0(SB), A0
	MOVL	A0, m(SB)
	MOVL	$0, 0(A0)
	MOVL	A0, A7
	ADDL	$(MACHSIZE-4), A7	/* start stack under machine struct */
	MOVL	$0, u(SB)

	MOVL	$vectors(SB), A0
	MOVL	A0, VBR

	MOVL	$0x05, R0			/* supervisor data */
	MOVL	R0, DFC				/* DO NOT MODIFY! */

	BSR	main(SB)
	/* never returns */
dead:
	BRA	dead

/*
 * Take first processor into user mode
 */

TEXT	touser(SB), $0
	
	MOVL	usp+0(FP),A0
	MOVL	$(USERADDR+BY2PG-UREGVARSZ), A7
	MOVW	$0, -(A7)
	MOVL	$(UTZERO+32), -(A7)	/* header is in text */
	MOVW	$0, -(A7)
	MOVL	A0, USP
	MOVW	$(SUPER|SPL(0)), SR
	RTE

TEXT	spllo(SB), $0

	MOVL	$0, R0
	MOVW	SR, R0
	MOVW	$(SUPER|SPL(0)), SR
	RTS

TEXT	splhi(SB), $0

	MOVL	m(SB), A0
	MOVL	(A7), 4(A0)
	MOVL	$0, R0
	MOVW	SR, R0
	MOVW	$(SUPER|SPL(7)), SR
	RTS

TEXT	splx(SB), $0

	MOVL	sr+0(FP), R0
	MOVW	R0, SR
	RTS

TEXT	spldone(SB), $0

	RTS

TEXT	flushcpucache(SB), $0

	WORD	$(PUSH|ALL|ICH|DCH)		/* CPUSHA */
	MOVL	$CENABLE, R0
	MOVL	R0, CACR
	RTS

TEXT	flushatc(SB), $0

	WORD	$0xF518				/* PFLUSHA */
	RTS

TEXT	flushatcpage(SB), $0

	WORD	$0xF518				/* PFLUSHA */
	RTS
	MOVL	a+0(FP), A0
	WORD	$0xF508				/* PFLUSH (A0) */
	RTS

TEXT	mmusr(SB), $0

	MOVL	a+0(FP), A0
	MOVW	$0x05, R0			/* assume kernel data */
	TSTL	u+4(FP)
	BEQ	mmusr1
	MOVL	$0x01, R0			/* user data */
mmusr1:
	MOVL	R0, DFC
	WORD	$0xF568				/* PTEST (A0) */
	MOVL	$0x05, R0			/* supervisor data */
	MOVL	R0, DFC				/* DO NOT MODIFY! */
	MOVL	MMUSR, R0
	RTS

TEXT	flushpage(SB), $0

	MOVL	a+0(FP), A0
	WORD	$(PUSH|PAGE|DCH|ICH)		/* CPUSHP <IC+DC>, (A0) */
	RTS

TEXT	flushvirtpage(SB), $0

	MOVL	a+0(FP), A0
	WORD	$0xF568				/* PTEST (A0) */
	MOVL	MMUSR, A0
	WORD	$(PUSH|PAGE|DCH|ICH)		/* CPUSHP <IC+DC>, (A0) */
	RTS

TEXT	pushpage(SB), $0

	MOVL	a+0(FP), A0
	WORD	$(PUSH|PAGE|DCH)		/* CPUSHP <DC>, (A0) */
	RTS

TEXT	invalpage(SB), $0

	MOVL	a+0(FP), A0
	WORD	$(INVAL|PAGE|DCH)		/* CINVP <DC>, (A0) */
	RTS

TEXT	cacrtrap(SB), $0	/* user entry point to control cache, e.g. flush */
	WORD	$(PUSH|ALL|ICH|DCH)		/* CPUSHA */
	RTE

TEXT	initmmureg(SB), $0

	MOVL	a+0(FP), R0
	MOVL	R0, URP
	MOVL	R0, SRP
	MOVL	$0xC000, R0		/* enable mmu, 8K pages */
	MOVL	R0, TC
	RTS

TEXT	setlabel(SB), $0

	MOVL	sr+0(FP), A0
	MOVL	A7, (A0)+		/* stack pointer */
	MOVL	(A7), (A0)+		/* pc of caller */
	MOVW	SR, (A0)+		/* status register */
	CLRL	R0			/* ret 0 => not returning */
	RTS

TEXT	gotolabel(SB), $0

	MOVL	p+0(FP), A0
	MOVW	$(SUPER|SPL(7)), SR
	MOVL	(A0)+, A7		/* stack pointer */
	MOVL	(A0)+, (A7)		/* pc; stuff into stack frame */
	MOVW	(A0)+, R0		/* status register */
	MOVW	R0, SR
	MOVL	$1, R0			/* ret 1 => returning */
	RTS

/*
 * Test and set, as a subroutine
 */

TEXT	tas(SB), $0

	MOVL	$0, R0
	MOVL	a+0(FP), A0
	TAS	(A0)
	BEQ	tas_1
	MOVL	$1, R0
tas_1:
	RTS

/*
 * Floating point
 */

TEXT	fpsave(SB), $0

	FSAVE	(fp+0(FP))
	RTS

TEXT	fprestore(SB), $0

	FRESTORE	(fp+0(FP))
	RTS

TEXT	fpregsave(SB), $0

	FMOVEM	$0xFF, (3*4)(fr+0(FP))
	FMOVEMC	$0x7, (fr+0(FP))
	RTS

TEXT	fpregrestore(SB), $0

	FMOVEMC	(fr+0(FP)), $0x7
	FMOVEM	(3*4)(fr+0(FP)), $0xFF
	RTS

TEXT	fpcr(SB), $0

	MOVL	new+0(FP), R1
	MOVL	FPCR, R0
	MOVL	R1, FPCR
	RTS


TEXT	rfnote(SB), $0

	MOVL	uregp+0(FP), A7
	MOVL	((8+8)*BY2WD)(A7), A0
	MOVL	A0, USP
	MOVEM	(A7), $0x7FFF
	ADDL	$((8+8+1+1)*BY2WD), A7
	RTE

TEXT	illegal(SB), $0

	MOVL	$DBMAGIC, -(A7)
	SUBL	$((8+8+1)*BY2WD), A7
	MOVEM	$0x7FFF, (A7)
	MOVL	$a6base(SB), A6
	MOVL	USP, A0
	MOVL	A0, ((8+8)*BY2WD)(A7)
	MOVL	A7, -(A7)
	BSR	trap(SB)
	ADDL	$4, A7
	MOVL	((8+8)*BY2WD)(A7), A0
	MOVL	A0, USP
	MOVEM	(A7), $0x7FFF
	ADDL	$((8+8+1)*BY2WD), A7
	MOVL	$0, (A7)+		/* clear magic */
	RTE

TEXT	auto3trap(SB), $0

	MOVL	$DBMAGIC, -(A7)
	SUBL	$((8+8+1)*BY2WD), A7
	MOVEM	$0x7FFF, (A7)
	MOVL	$a6base(SB), A6
	MOVL	USP, A0
	MOVL	A0, ((8+8)*BY2WD)(A7)
	MOVL	A7, -(A7)
	BSR	auto3(SB)
	ADDL	$4, A7
	MOVL	((8+8)*BY2WD)(A7), A0
	MOVL	A0, USP
	MOVEM	(A7), $0x7FFF
	ADDL	$((8+8+1)*BY2WD), A7
	MOVL	$0, (A7)+		/* clear magic */
	RTE

TEXT	auto5trap(SB), $0

	MOVL	$DBMAGIC, -(A7)
	SUBL	$((8+8+1)*BY2WD), A7
	MOVEM	$0x7FFF, (A7)
	MOVL	$a6base(SB), A6
	MOVL	USP, A0
	MOVL	A0, ((8+8)*BY2WD)(A7)
	MOVL	A7, -(A7)
	BSR	auto5(SB)
	ADDL	$4, A7
	MOVL	((8+8)*BY2WD)(A7), A0
	MOVL	A0, USP
	MOVEM	(A7), $0x7FFF
	ADDL	$((8+8+1)*BY2WD), A7
	MOVL	$0, (A7)+		/* clear magic */
	RTE

TEXT	auto6trap(SB), $0

	MOVL	$DBMAGIC, -(A7)
	SUBL	$((8+8+1)*BY2WD), A7
	MOVEM	$0x7FFF, (A7)
	MOVL	$a6base(SB), A6
	MOVL	USP, A0
	MOVL	A0, ((8+8)*BY2WD)(A7)
	MOVL	A7, -(A7)
	BSR	auto6(SB)
	ADDL	$4, A7
	MOVL	((8+8)*BY2WD)(A7), A0
	MOVL	A0, USP
	MOVEM	(A7), $0x7FFF
	ADDL	$((8+8+1)*BY2WD), A7
	MOVL	$0, (A7)+		/* clear magic */
	RTE

TEXT	systrap(SB), $0

	MOVL	$DBMAGIC, -(A7)
	SUBL	$((8+8+1)*BY2WD), A7
	MOVL	A6, ((8+6)*BY2WD)(A7)
	MOVL	R0, (A7)
	MOVL	$a6base(SB), A6
	MOVL	USP, A0
	MOVL	A0, ((8+8)*BY2WD)(A7)
	MOVL	A7, -(A7)
	BSR	syscall(SB)
	MOVL	((1+8+8)*BY2WD)(A7), A0
	MOVL	A0, USP
	MOVL	((1+8+6)*BY2WD)(A7), A6
	ADDL	$((1+8+8+1)*BY2WD), A7
	MOVL	$0, (A7)+		/* clear magic */
	RTE

TEXT	buserror(SB), $0

	MOVL	$DBMAGIC, -(A7)
	SUBL	$((8+8+1)*BY2WD), A7
	MOVEM	$0x7FFF, (A7)
	MOVL	$a6base(SB), A6
	MOVL	USP, A0
	MOVL	A0, ((8+8)*BY2WD)(A7)
	PEA	((8+8+1+3)*BY2WD)(A7)
	PEA	4(A7)
	BSR	fault68040(SB)
	ADDL	$8, A7
	MOVL	((8+8)*BY2WD)(A7), A0
	MOVL	A0, USP
	MOVEM	(A7), $0x7FFF
	ADDL	$((8+8+1)*BY2WD), A7
	MOVL	$0, (A7)+		/* clear magic */
	RTE

GLOBL	mach0+0(SB), $MACHSIZE
GLOBL	splpc(SB), $4
GLOBL	u(SB), $4
GLOBL	m(SB), $4

/*
 * muldiv(a,b,c) returns (a*b)/c using 64-bit intermediate
 */
TEXT	muldiv(SB), $0
	MOVL	a+0(FP), R0
	MOVL	b+4(FP), R3
	MOVL	c+8(FP), R2
	WORD	$0x4C03
	WORD	$0x0C01			/* MULS	R3, R0 -> [l:R0,h:R1]) */
	WORD	$0x4C42
	WORD	$0x0C01			/* DIVS	R2, [R0,R3] -> [q:R0,r:R1] */
	RTS

TEXT	getcallerpc(SB), $0
	MOVL	(a+0(FP)), R0
	RTS

TEXT	getsr+0(SB), $0
	MOVL	$0, R0
	MOVW	SR, R0
	RTS
