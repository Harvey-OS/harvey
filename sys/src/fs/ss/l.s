#include "mem.h"

#define	SYSPSR	(PSREF|PSRET|PSRSUPER|PSRPSUPER|SPL(15))
#define	NOOP	ORN R0, R0; ORN R0, R0; ORN R0, R0

TEXT	start(SB), $-4

	/* get virtual, fast */
	/* we are executing in segment 0, mapped to pmeg 0. stack is there too */
	/* get virtual by mapping segment(KZERO) to pmeg 0., and next to 1 */
	MOVW	$KZERO, R7
	MOVB	R0, (R7, 3)
	MOVW	$(KZERO+BY2SEGM), R7
	MOVW	$1, R9
	MOVB	R9, (R7, 3)
	/* now mapped correctly.  jmpl to where we want to be */
	MOVW	$setSB(SB), R2
	MOVW	$startvirt(SB), R7
	JMPL	(R7)
	MOVW	$_mul(SB), R0	/* touch _mul etc.; doesn't need to execute */
	RETURN			/* can't get here */

TEXT	startvirt(SB), $-4

	MOVW	$rom(SB), R7
	MOVW	R8, (R7)	/* romvec passed in %i0==R8 */

	MOVW	$BOOTSTACK, R1

	MOVW	$(SPL(0xF)|PSREF|PSRSUPER), R7
	MOVW	R7, PSR

	MOVW	$(0x35<<22), R7		/* NVM OFM DZM NS */
	MOVW	R7, fsr+0(SB)
	MOVW	fsr+0(SB), FSR
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

	MOVW	$edata(SB), R7		/* clear bss */
	MOVW	R0, 8(R1)
	MOVW	$end(SB), R8
	SUB	R7, R8
	MOVW	R8, 0xC(R1)
	JMPL	memset(SB)

	JMPL	main(SB)
	MOVW	(R0), R0
	RETURN

TEXT	tas(SB), $0

	TAS	(R7), R7		/* LDSTUB, thank you ken */
	RETURN

TEXT	swap1_should_work(SB), $0

	MOVW	R7, R8
	MOVW	$1, R7
	SWAP	(R8), R7
	RETURN

TEXT	swap1x(SB), $0

	MOVW	PSR, R9
	MOVW	R9, R10
	AND	$~PSRET, R10		/* BUG: book says this is buggy */
	MOVW	R10, PSR
	NOOP
	MOVW	(R7), R7
	CMP	R7, R0
	BNE	was1
	MOVW	$1, R10
	MOVW	R10, (R8)
was1:
	MOVW	R9, PSR
	RETURN

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

TEXT	rfnote(SB), $0

	MOVW	R7, R1			/* 1st arg is &uregpointer */
	ADD	$4, R1			/* point at ureg */
	JMP	restore

TEXT	traplink(SB), $-4

	/* R8 to R23 are free to play with */
	/* R17 contains PC, R18 contains nPC */
	/* R19 has PSR loaded from vector code */

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

TEXT	r1(SB), $0

	MOVW	R1, R7
	RETURN

TEXT	getwim(SB), $0

	MOVW	WIM, R7
	RETURN

TEXT	setlabel(SB), $0		/* beware, switched form cpu code */

	MOVW	R1, 4(R7)
	MOVW	R15, (R7)
	MOVW	$0, R7
	RETURN

TEXT	gotolabel(SB), $0		/* beware, switched form cpu code */

	MOVW	4(R7), R1
	MOVW	(R7), R15
	MOVW	$1, R7
	RETURN

TEXT	putcxsegm(SB), $0

	MOVW	R7, R8			/* context */
	MOVW	4(FP), R9		/* segment addr */
	MOVW	8(FP), R10		/* segment value */
	MOVW	$romputcxsegm(SB), R7
	MOVW	(R7), R7
	JMPL	(R7)
	RETURN

TEXT	putw4(SB), $0
	MOVW	4(FP), R8
	MOVW	R8, (R7, 4)
	RETURN

TEXT	getpsr(SB), $0

	MOVW	PSR, R7
	RETURN

TEXT	setpsr(SB), $0

	MOVW	R7, PSR
	NOOP
	RETURN

TEXT	putsegm(SB), $0

	MOVW	4(FP), R8
	MOVW	R8, (R7, 3)
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

/*
 *	float	famd(float a, int b, int c, int d)
 *		return ((a+b) * c) / d;
 */
	TEXT	famd(SB), $0

	MOVW	R7, a+0(FP)
	MOVW	PSR, R8
	MOVW	R8, R10
	OR	$SPL(15), R10
	MOVW	R10, PSR
	NOOP

	MOVW	FSR, fsr(SB)	/* save */
	MOVW	F0, f0(SB)
	MOVW	F2, f2(SB)

	FMOVF	a+0(FP), F0	/* load */
	FMOVF	b+4(FP), F2	/* convert-add */
	FMOVWF	F2, F2
	FADDF	F2, F0

	FMOVF	c+8(FP), F2	/* convert-mul */
	FMOVWF	F2, F2
	FMULF	F2, F0

	FMOVF	d+12(FP), F2	/* convert-div */
	FMOVWF	F2, F2
	FDIVF	F2, F0

	FMOVF	F0, rt(SB)
	MOVW	rt(SB), R7	/* return */

	FMOVF	f0(SB), F0	/* restore */
	FMOVF	f2(SB), F2

	MOVW	fsr(SB), FSR
	MOVW	R8, PSR		/* splx */
	NOOP
	RETURN
/*
 *	ulong	fdf(float a, int b)
 *		return a / b;
 */
	TEXT	fdf(SB), $0

	MOVW	R7, a+0(FP)
	MOVW	PSR, R8
	MOVW	R8, R10
	OR	$SPL(15), R10
	MOVW	R10, PSR
	NOOP

	MOVW	FSR, fsr(SB)	/* save */
	MOVW	F0, f0(SB)
	MOVW	F2, f2(SB)

	FMOVF	a+0(FP), F0	/* load */

	FMOVF	b+4(FP), F2	/* convert-div */
	FMOVWF	F2, F2
	FDIVF	F2, F0

	FMOVFW	F0, F0
	FMOVF	F0, rt(SB)
	MOVW	rt(SB), R7	/* return */

	FMOVF	f0(SB), F0	/* restore */
	FMOVF	f2(SB), F2

	MOVW	fsr(SB), FSR
	MOVW	R8, PSR		/* splx */
	NOOP
	RETURN

GLOBL	mach0+0(SB), $MACHSIZE
GLOBL	fsr+0(SB), $BY2WD
GLOBL	f0+0(SB), $BY2WD
GLOBL	f2+0(SB), $BY2WD
GLOBL	rt+0(SB), $BY2WD

/*
 * Interface to ROM.  Must save and restore state because
 * of different calling conventions.
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
