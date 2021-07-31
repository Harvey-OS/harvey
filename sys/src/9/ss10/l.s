#include "mem.h"
#include "sunmmu.h"


#define	SYSPSR		(PSREF|PSRET|PSRSUPER|SPL(15))
#define	NOOP		ORN R0, R0; ORN R0, R0; ORN R0, R0
#define	MXCCASI		0x2		/* Viking/E$ ASI */
#define	TLBASI		0x3		/* MMU flush/probe */
#define	MMUASI		0x4		/* Module control */
#define	PHYSASI		0x20	/* MMU bypass */
#define	SBUSASI		0x2E		/* MMU bypass to SBUS space*/
#define	IOASI		0x2F		/* MMU bypass to on-board IO space*/
#define	IFLUSHASI 	0x36
#define	DFLUSHASI 	0x37
#define	ACTIONASI	0x4C	/* Superscalar control */


/*
 * Initialization code before main(). Specially linked in mkfile. 
 */

TEXT	start(SB), $-4

	/* get virtual, fast */
	/* copy ROM's L1 page table entries, mapping VA:0 and VA:KZERO to PA:0 */
	MOVW	$setSB(SB), R2

	MOVW	$CTPR, R7
	MOVW	(R7, MMUASI), R5
	SLL		$4, R5, R5
	MOVW	(R5, PHYSASI), R5	/* context table entry 0 */
	SRL		$4, R5, R5
	SLL		$8, R5, R5		/* to physical pointer */

	MOVW	$(((KZERO>>24)&0xFF)*4), R4	/* KZERO base in level 1 ptab */
	MOVW	(R5, PHYSASI), R6	/* L1 region 0 set by boot */
	ADD		R5, R4, R3		/* workaround of assembler syntax */
	MOVW	R6, (R3, PHYSASI)	/* map KZERO */

	ADD		$4, R5
	MOVW	(R5, PHYSASI), R6
	ADD		R5, R4, R3
	MOVW	R6, (R3, PHYSASI)	/* map KZERO+16Mbytes */

	/* Set up0 the static base register */
	MOVW	$setSB(SB), R2

	MOVW	$startvirt(SB), R7
	JMPL		(R7)
	MOVW	$_mul(SB), R0	/* touch _mul etc.; doesn't need to execute */
	RETURN			/* can't get here */

TEXT	startvirt(SB), $-4

	MOVW	$rom(SB), R7
	MOVW	R8, (R7)	/* romvec passed in %i0==R8 */

	/* Turn off the cache but ensure table-walks cacheable. 
	 * Note that the E$ is set up by the boot ROM.  We leave it alone. */
	MOVW	(R0, MMUASI), R7
	ANDN	$(ENABCACHE|ENABSB), R7 
	MOVW	R7, (R0, MMUASI)
	FLUSH; NOOP

/* Cliff:  disable Sbus DMA was here*/

	MOVW	$BOOTSTACK, R1

	MOVW	$(SPL(0xF)|PSREF|PSRSUPER|PSRET), R7
	MOVW	R7, PSR

	MOVW	$(0x35<<22), R7		/* NVM OFM DZM NS */
	MOVW	R7, fsr+0(SB)
	MOVW	$fsr+0(SB), R8
	MOVW	(R8), FSR
	FMOVD	$0.5, F26		/* 0.5 -> F26 */
	FSUBD	F26, F26, F24		/* 0.0 -> F24 */
	FADDD	F26, F26, F28		/* 1.0 -> F28 */
	FADDD	F28, F28, F30		/* 2.0 -> F30 */

	FMOVD	F24, F0
	FMOVD	F24, F2
	FMOVD	F24, F4
	FMOVD	F24, F6
	FMOVD	F24, F8
	FMOVD	F24, F10
	FMOVD	F24, F12
	FMOVD	F24, F14
	FMOVD	F24, F16
	FMOVD	F24, F18
	FMOVD	F24, F20
	FMOVD	F24, F22

	MOVW	$mach0(SB), R(MACH)
/*	MOVW	$0x8, R7 /**/
	MOVW	R0, WIM
	JMPL	main(SB)
	MOVW	(R0), R0
	RETURN


/*
 * Special ASI access routines
 */

TEXT	getmxccd(SB), $0

	MOVD	(R7, MXCCASI), R8
	MOVW	4(FP), R10
	MOVD	R8, (R10)
	RETURN

TEXT	putmxccd(SB), $0

	MOVW	4(FP), R8
	MOVD	(R8), R8
	MOVD	R8, (R7, MXCCASI)
	RETURN


/* 
 * Synchronization
 */

TEXT	oldtas(SB), $0

	TAS	(R7), R7		/* LDSTUB, thank you ken */
	RETURN

TEXT	tas(SB), $0			/* it seems we must be splhi */

	MOVW	PSR, R8
	MOVW	$SYSPSR, R9
	MOVW	R9, PSR
	NOOP
	TAS	(R7), R7		/* LDSTUB, thank you ken */
	MOVW	R8, PSR
	NOOP
	RETURN

TEXT	softtas(SB), $0			/* all software; avoid LDSTUB */

	MOVW	PSR, R8
	MOVW	$SYSPSR, R9
	MOVW	R9, PSR
	NOOP
	MOVB	(R7), R10
	CMP	R10, R0
	BE	gotit
#ifdef asdf
	ANDN	$31, R7			/* flush cache line */
	MOVW	$0, (R7, 0xD)
#endif
	MOVW	$0xFF, R7
	MOVW	R8, PSR
	NOOP
	RETURN

gotit:
	MOVW	$0xFF, R10
	MOVB	R10, (R7)
#ifdef asdf
	ANDN	$31, R7			/* flush cache line */
	MOVW	$0, (R7, 0xD)
#endif
	MOVW	$0, R7
	MOVW	R8, PSR
	NOOP
	RETURN


/* 
 * Interrupt Priority Level
 */

TEXT	spllo(SB), $0

	MOVW	PSR, R7
	MOVW	R7, R10
	ANDN	$SPL(15), R10
	MOVW	R10, PSR
	NOOP
	RETURN

TEXT	splhi(SB), $0

	MOVW	R15, 4(R(MACH))	/* save PC in m->splpc */
	MOVW	PSR, R7
	MOVW	R7, R10
	OR	$SPL(15), R10
	MOVW	R10, PSR
	NOOP
	RETURN

TEXT	splx(SB), $0

	MOVW	R15, 4(R(MACH))	/* save PC in m->splpc */
	MOVW	R7, PSR		/* BUG: book says this is buggy */
	NOOP
	RETURN

TEXT	spldone(SB), $0

	RETURN


/*
 * Miscellany
 */

TEXT	touser(SB), $0
	MOVW	$(SYSPSR&~(PSREF|PSRET|SPL(15))), R8
	MOVW	R8, PSR
	NOOP

	MOVW	R7, R1
	SAVE	R0, R0			/* RETT is implicit RESTORE */
	MOVW	$(UTZERO+32), R7	/* PC; header appears in text */
	MOVW	$(UTZERO+32+4), R8	/* nPC */
	RETT	R7, R8

TEXT	rfnote(SB), $0

	MOVW	R7, R1			/* 1st arg is &uregpointer */
	ADD	$4, R1			/* point at ureg */
	JMP	restore

TEXT	_getcallerpc(SB), $0
	MOVW	0(R1), R7
	RETURN


/*
 * Trap handlers
 */

TEXT	traplink(SB), $-4

	/* R8 to R23 are free to play with */
	/* R17 contains PC, R18 contains nPC */
	/* R19 has PSR loaded from vector code */

	ANDCC	$PSRPSUPER, R19, R0
	BE	usertrap

kerneltrap:
	/*
	 * Interrupt or fault from kernel
	 */
	ANDN	$7, R1, R20			/* dbl aligned */
	MOVW	R1, (0-(4*(32+6))+(4*1))(R20)	/* save R1=SP */
	/* really clumsy: store these in Ureg so can be restored below */
	MOVW	R2, (0-(4*(32+6))+(4*2))(R20)	/* SB */
	MOVW	R5, (0-(4*(32+6))+(4*5))(R20)	/* USER */
	MOVW	R6, (0-(4*(32+6))+(4*6))(R20)	/* MACH */
	SUB	$(4*(32+6)), R20, R1

trap1:
	MOVW	Y, R20
	MOVW	R20, (4*(32+0))(R1)		/* Y */
	MOVW	TBR, R20
	MOVW	R20, (4*(32+1))(R1)		/* TBR */
	AND	$~0x1F, R19			/* force CWP=0 */
	MOVW	R19, (4*(32+2))(R1)		/* PSR */
	MOVW	R18, (4*(32+3))(R1)		/* nPC */
	MOVW	R17, (4*(32+4))(R1)		/* PC */
	MOVW	R0, (4*0)(R1)
	MOVW	R3, (4*3)(R1)
	MOVW	R4, (4*4)(R1)
	MOVW	R7, (4*7)(R1)
	RESTORE	R0, R0
	/* now our registers R8-R31 are same as before trap */
	/* save registers two at a time */
	MOVD	R8, (4*8)(R1)
	MOVD	R10, (4*10)(R1)
	MOVD	R12, (4*12)(R1)
	MOVD	R14, (4*14)(R1)
	MOVD	R16, (4*16)(R1)
	MOVD	R18, (4*18)(R1)
	MOVD	R20, (4*20)(R1)
	MOVD	R22, (4*22)(R1)
	MOVD	R24, (4*24)(R1)
	MOVD	R26, (4*26)(R1)
	MOVD	R28, (4*28)(R1)
	MOVD	R30, (4*30)(R1)
	/* SP and SB and u and m are already set; away we go */
	MOVW	R1, R7		/* pointer to Ureg */
	SUB	$8, R1
	MOVW	$SYSPSR, R8
	MOVW	R8, PSR
	NOOP
	JMPL	trap(SB)

	ADD	$8, R1
restore:
	MOVW	(4*(32+2))(R1), R8		/* PSR */
	MOVW	R8, PSR
	NOOP

	MOVD	(4*30)(R1), R30
	MOVD	(4*28)(R1), R28
	MOVD	(4*26)(R1), R26
	MOVD	(4*24)(R1), R24
	MOVD	(4*22)(R1), R22
	MOVD	(4*20)(R1), R20
	MOVD	(4*18)(R1), R18
	MOVD	(4*16)(R1), R16
	MOVD	(4*14)(R1), R14
	MOVD	(4*12)(R1), R12
	MOVD	(4*10)(R1), R10
	MOVD	(4*8)(R1), R8
	SAVE	R0, R0
	MOVD	(4*6)(R1), R6
	MOVD	(4*4)(R1), R4
	MOVD	(4*2)(R1), R2
	MOVW	(4*(32+0))(R1), R20		/* Y */
	MOVW	R20, Y
	MOVW	(4*(32+4))(R1), R17		/* PC */
	MOVW	(4*(32+3))(R1), R18		/* nPC */
	MOVW	(4*1)(R1), R1	/* restore R1=SP */
	RETT	R17, R18
	
usertrap:
	/*
	 * Interrupt or fault from user
	 */
	MOVW	R1, R8
	MOVW	R2, R9
	MOVW	$setSB(SB), R2
	MOVW	$(USERADDR+BY2PG), R1
	MOVW	R8, (0-(4*(32+6))+(4*1))(R1)	/* save R1=SP */
	MOVW	R9, (0-(4*(32+6))+(4*2))(R1)	/* save R2=SB */
	MOVW	R5, (0-(4*(32+6))+(4*5))(R1)	/* save R5=USER */
	MOVW	R6, (0-(4*(32+6))+(4*6))(R1)	/* save R6=MACH */
	MOVW	$USERADDR, R(USER)
	MOVW	$mach0(SB), R(MACH)
	SUB	$(4*(32+6)), R1
	JMP	trap1

TEXT	syslink(SB), $-4

	/* R8 to R23 are free to play with */
	/* R17 contains PC, R18 contains nPC */
	/* R19 has PSR loaded from vector code */
	/* assume user did it; syscall checks */

	MOVW	R1, R8
	MOVW	R2, R9
	MOVW	$setSB(SB), R2
	MOVW	$(USERADDR+BY2PG), R1
	MOVW	R8, (0-(4*(32+6))+4)(R1)	/* save R1=SP */
	SUB	$(4*(32+6)), R1
	MOVW	R9, (4*2)(R1)			/* save R2=SB */
	MOVW	R3, (4*3)(R1)			/* global register */
	MOVD	R4, (4*4)(R1)			/* global register, R5=USER */
	MOVD	R6, (4*6)(R1)			/* save R6=MACH, R7=syscall# */
	MOVW	$USERADDR, R(USER)
	MOVW	$mach0(SB), R(MACH)
	MOVW	TBR, R20
	MOVW	R20, (4*(32+1))(R1)		/* TBR */
	AND	$~0x1F, R19
	MOVW	R19, (4*(32+2))(R1)		/* PSR */
	MOVW	R18, (4*(32+3))(R1)		/* nPC */
	MOVW	R17, (4*(32+4))(R1)		/* PC */
	RESTORE	R0, R0
	/* now our registers R8-R31 are same as before trap */
	MOVW	R15, (4*15)(R1)
	/* SP and SB and u and m are already set; away we go */
	MOVW	R1, R7			/* pointer to Ureg */
	SUB	$8, R1
	MOVW	$SYSPSR, R8
	MOVW	R8, PSR
	JMPL	syscall(SB)
	/* R7 contains return value from syscall */

	ADD	$8, R1
	MOVW	(4*(32+2))(R1), R8		/* PSR */
	MOVW	R8, PSR
	NOOP

	MOVW	(4*15)(R1), R15
	SAVE	R0, R0
	MOVW	(4*6)(R1), R6
	MOVD	(4*4)(R1), R4
	MOVD	(4*2)(R1), R2
	MOVW	(4*(32+4))(R1), R17		/* PC */
	MOVW	(4*(32+3))(R1), R18		/* nPC */
	MOVW	(4*1)(R1), R1	/* restore R1=SP */
	RETT	R17, R18

TEXT	puttbr(SB), $0

	MOVW	R7, TBR
	NOOP
	RETURN

TEXT	gettbr(SB), $0

	MOVW	TBR, R7
	RETURN


/*
 * Setjmp/Longjmp
 */

TEXT	setlabel(SB), $0

	MOVW	R1, (R7)
	MOVW	R15, 4(R7)
	MOVW	$0, R7
	RETURN

TEXT	gotolabel(SB), $0

	MOVW	(R7), R1
	MOVW	4(R7), R15
	MOVW	$1, R7
	RETURN

TEXT	getpsr(SB), $0

	MOVW	PSR, R7
	RETURN

TEXT	setpsr(SB), $0

	MOVW	R7, PSR
	NOOP
	RETURN


/* 
 * FP Routines
 */
TEXT	savefpregs(SB), $0

	MOVW	FSR, 0(R7)
	ADD	$4, R7

	MOVD	F0, (0*4)(R7)
	MOVD	F2, (2*4)(R7)
	MOVD	F4, (4*4)(R7)
	MOVD	F6, (6*4)(R7)
	MOVD	F8, (8*4)(R7)
	MOVD	F10, (10*4)(R7)
	MOVD	F12, (12*4)(R7)
	MOVD	F14, (14*4)(R7)
	MOVD	F16, (16*4)(R7)
	MOVD	F18, (18*4)(R7)
	MOVD	F20, (20*4)(R7)
	MOVD	F22, (22*4)(R7)
	MOVD	F24, (24*4)(R7)
	MOVD	F26, (26*4)(R7)
	MOVD	F28, (28*4)(R7)
	MOVD	F30, (30*4)(R7)

	MOVW	PSR, R8
	ANDN	$PSREF, R8
	MOVW	R8, PSR
	RETURN

TEXT	enabfp(SB), $0

	MOVW	PSR, R8
	OR	$PSREF, R8
	MOVW	R8, PSR
	RETURN

TEXT	disabfp(SB), $0

	MOVW	PSR, R8
	ANDN	$PSREF, R8
	MOVW	R8, PSR
	RETURN

TEXT	restfpregs(SB), $0

	MOVW	PSR, R8
	OR	$PSREF, R8
	MOVW	R8, PSR

	NOOP			/* wait for PSR to quiesce */
	MOVW	fsr+4(FP), FSR
	ADD	$4, R7

	MOVD	(0*4)(R7), F0
	MOVD	(2*4)(R7), F2
	MOVD	(4*4)(R7), F4
	MOVD	(6*4)(R7), F6
	MOVD	(8*4)(R7), F8
	MOVD	(10*4)(R7), F10
	MOVD	(12*4)(R7), F12
	MOVD	(14*4)(R7), F14
	MOVD	(16*4)(R7), F16
	MOVD	(18*4)(R7), F18
	MOVD	(20*4)(R7), F20
	MOVD	(22*4)(R7), F22
	MOVD	(24*4)(R7), F24
	MOVD	(26*4)(R7), F26
	MOVD	(28*4)(R7), F28
	MOVD	(30*4)(R7), F30

	ANDN	$PSREF, R8
	MOVW	R8, PSR
	RETURN

TEXT	getfpq(SB), $0

	MOVW	R7, R8	/* must be D aligned */
	MOVW	$fsr+0(SB), R9
	MOVW	$0, R7
getfpq1:
	MOVW	FSR, (R9)
	MOVW	(R9), R10
	ANDCC	$(1<<13), R10		/* queue not empty? */
	BE	getfpq2
	MOVW	(R8), R0		/* SS2 bug fix */
	MOVD	FQ, (R8)
	ADD	$1, R7
	ADD	$8, R8
	BA	getfpq1
getfpq2:
	RETURN

TEXT	getfsr(SB), $0
	MOVW	$fsr+0(SB), R7
	MOVW	FSR, (R7)
	MOVW	(R7), R7
	RETURN

TEXT	clearftt(SB), $0
	MOVW	R7, fsr+0(SB)
	MOVW	$fsr+0(SB), R7
	MOVW	(R7), FSR
	FMOVF	F0, F0
	RETURN


GLOBL	mach0+0(SB), $MACHSIZE
GLOBL	fsr+0(SB), $BY2WD

/*
 * Interface to OPEN BOOT ROM.  Must save and restore state because
 * of different calling conventions.  We don't use it, but it's here
 * for reference..
 */

TEXT	call(SB), $16
	MOVW	R1, R14		/* save my SP in their SP */
	MOVW	R2, sb-4(SP)
	MOVW	R(MACH), mach-8(SP)
	MOVW	R(USER), user-12(SP)
	MOVW	param1+4(FP), R8
	MOVW	param2+8(FP), R9
	MOVW	param3+12(FP), R10
	MOVW	param4+16(FP), R11
	JMPL	(R7)
	MOVW	R14, R1		/* restore my SP */
	MOVW	user-12(SP), R(USER)
	MOVW	mach-8(SP), R(MACH)
	MOVW	sb-4(SP), R2
	MOVW	R8, R7		/* move their return value into mine */
	RETURN
