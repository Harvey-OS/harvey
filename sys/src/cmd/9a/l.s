/*
 * Memory and machine-specific definitions.  Used in C and assembler.
 */

/*
 * Sizes
 */

#define	BI2BY		8			/* bits per byte */
#define BI2WD		32			/* bits per word */
#define	BY2WD		4			/* bytes per word */
#define	BY2PG		4096			/* bytes per page */
#define	WD2PG		(BY2PG/BY2WD)		/* words per page */
#define	PGSHIFT		12			/* log(BY2PG) */

#define	MAXMACH		4			/* max # cpus system can run */

/*
 * Time
 */
#define	MS2HZ		50			/* millisec per clock tick */
#define	TK2SEC(t)	((t)/20)		/* ticks to seconds */
#define	TK2MS(t)	((t)*MS2HZ)		/* ticks to milliseconds */
#define	MS2TK(t)	((t)/MS2HZ)		/* milliseconds to ticks */

/*
 * CP0 registers
 */

#define INDEX		0
#define RANDOM		1
#define TLBPHYS		2
#define CONTEXT		4
#define BADVADDR	8
#define TLBVIRT		10
#define STATUS		12
#define CAUSE		13
#define EPC		14
#define	PRID		15

/*
 * M(STATUS) bits
 */
#define IEC		0x00000001
#define KUC		0x00000002
#define IEP		0x00000004
#define KUP		0x00000008
#define INTMASK		0x0000ff00
#define SW0		0x00000100
#define SW1		0x00000200
#define INTR0		0x00000400
#define INTR1		0x00000800
#define INTR2		0x00001000
#define INTR3		0x00002000
#define INTR4		0x00004000
#define INTR5		0x00008000
#define ISC		0x00010000
#define SWC		0x00020000
#define CU1		0x20000000

/*
 * Traps
 */

#define	UTLBMISS	(KSEG0+0x00)
#define	EXCEPTION	(KSEG0+0x80)

/*
 * Magic registers
 */

#define	MACH		25		/* R25 is m-> */
#define	USER		24		/* R24 is u-> */
#define	MPID		0xBF000000	/* long; low 3 bits identify mp bus slot */
#define WBFLUSH		0xBC000000	/* D-CACHE data; used for write buffer flush */

/*
 * Fundamental addresses
 */

#define	MACHADDR	0x80014000
#define	USERADDR	0xC0000000
#define	UREGADDR	(USERADDR+BY2PG-4-0xA0)
/*
 * MMU
 */

#define	KUSEG	0x00000000
#define KSEG0	0x80000000
#define KSEG1	0xA0000000
#define	KSEG2	0xC0000000
#define	KSEGM	0xE0000000	/* mask to check which seg */

#define	PTEGLOBL	(1<<8)
#define	PTEVALID	(1<<9)
#define	PTEWRITE	(1<<10)
#define	PTEPID(n)	((n)<<6)

#define	NTLBPID	64	/* number of pids */
#define	NTLB	64	/* number of entries */
#define	TLBROFF	8	/* offset of first randomly indexed entry */

/*
 * Address spaces
 */

#define	UZERO	KUSEG			/* base of user address space */
#define	UTZERO	(UZERO+BY2PG)		/* first address in user text */
#define	USTKTOP	KZERO			/* byte just beyond user stack */
#define	TSTKTOP	(USERADDR+100*BY2PG)	/* top of temporary stack */
#define	KZERO	KSEG0			/* base of kernel address space */
#define	KTZERO	(KSEG0+0x20000)		/* first address in kernel text */
#define	USTACKSIZE	(4*1024*1024)	/* size of user stack */
/*
 * Exception codes
 */
#define	CINT	 0		/* external interrupt */
#define	CTLBM	 1		/* TLB modification */
#define	CTLBL	 2		/* TLB miss (load or fetch) */
#define	CTLBS	 3		/* TLB miss (store) */
#define	CADREL	 4		/* address error (load or fetch) */
#define	CADRES	 5		/* address error (store) */
#define	CBUSI	 6		/* bus error (fetch) */
#define	CBUSD	 7		/* bus error (data load or store) */
#define	CSYS	 8		/* system call */
#define	CBRK	 9		/* breakpoint */
#define	CRES	10		/* reserved instruction */
#define	CCPU	11		/* coprocessor unusable */
#define	COVF	12		/* arithmetic overflow */
#define	CUNK13	13		/* undefined 13 */
#define	CUNK14	14		/* undefined 14 */
#define	CUNK15	15		/* undefined 15 */

#define	NSEG	5

#define SP		R29

#define PROM		(KSEG1+0x1FC00000)
#define	NOOP		NOR R0,R0
#define	WAIT		NOOP; NOOP

/*
 * Boot first processor
 *   - why is the processor number loaded from R0 ?????
 */
TEXT	start(SB), $-4

	MOVW	$setR30(SB), R30
	MOVW	$(CU1|INTR5|INTR4|INTR3|INTR2|INTR1|SW1|SW0), R1
	MOVW	R1, M(STATUS)
	WAIT

	MOVW	$(0x1C<<7), R1
	MOVW	R1, FCR31	/* permit only inexact and underflow */
	NOOP
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

	MOVW	$MACHADDR, R(MACH)
	ADDU	$(BY2PG-4), R(MACH), SP
	MOVW	$0, R(USER)
	MOVW	R0, 0(R(MACH))

	MOVW	$edata(SB), R1
	MOVW	$end(SB), R2

clrbss:
	MOVB	$0, (R1)
	ADDU	$1, R1
	BNE	R1, R2, clrbss

	MOVW	R4, _argc(SB)
	MOVW	R5, _argv(SB)
	MOVW	R6, _env(SB)
	JAL	main(SB)
	JMP	(R0)

/*
 * Take first processor into user mode
 * 	- argument is stack pointer to user
 */

TEXT	touser(SB), $-4

	MOVW	M(STATUS), R1
	OR	$(KUP|IEP), R1
	MOVW	R1, M(STATUS)
	NOOP
	MOVW	0(FP), SP
	MOVW	$(UTZERO+32), R26	/* header appears in text */
	RFE	(R26)

/*
 * Bring subsequent processors on line
 */
TEXT	newstart(SB), $0

	MOVW	$setR30(SB), R30
	MOVW	$(INTR5|INTR4|INTR3|INTR2|INTR1|SW1|SW0), R1
	MOVW	R1, M(STATUS)
	NOOP
	MOVW	$MACHADDR, R(MACH)
	MOVB	(MPID+3), R1
	AND	$7, R1
	SLL	$PGSHIFT, R1, R2
	ADDU	R2, R(MACH)
	ADDU	$(BY2PG-4), R(MACH), SP
	MOVW	$0, R(USER)
	MOVW	R1, 0(R(MACH))
	JAL	online(SB)
	JMP	(R0)

TEXT	firmware(SB), $0

	MOVW	$(PROM+0x18), R1 /**/
/*	MOVW	$(PROM+0x00), R1 /**/
	JMP	(R1)

TEXT	splhi(SB), $0

	MOVW	M(STATUS), R1
	AND	$~IEC, R1, R2
	MOVW	R2, M(STATUS)
	NOOP
	RET

TEXT	spllo(SB), $0

	MOVW	M(STATUS), R1
	OR	$IEC, R1, R2
	MOVW	R2, M(STATUS)
	NOOP
	RET

TEXT	splx(SB), $0

	MOVW	0(FP), R1
	MOVW	M(STATUS), R2
	AND	$IEC, R1
	AND	$~IEC, R2
	OR	R2, R1
	MOVW	R1, M(STATUS)
	NOOP
	RET

TEXT	wbflush(SB), $-4

	MOVW	$WBFLUSH, R1
	MOVW	0(R1), R1
	RET

TEXT	setlabel(SB), $0

	MOVW	0(FP), R2
	MOVW	$0, R1
	MOVW	R31, 0(R2)
	MOVW	R29, 4(R2)
	RET

TEXT	gotolabel(SB), $0

	MOVW	0(FP), R2
	MOVW	$1, R1
	MOVW	0(R2), R31
	MOVW	4(R2), R29
	RET

TEXT	gotopc(SB), $8

	MOVW	0(FP), R7		/* save arguments for later */
	MOVW	_argc(SB), R4
	MOVW	_argv(SB), R5
	MOVW	_env(SB), R6
	MOVW	R0, 4(SP)
	MOVW	$(64*1024), R1
	MOVW	R1, 8(SP)
	JAL	icflush(SB)
	JMP	(R7)

TEXT	puttlb(SB), $4

	JAL	splhi(SB)
	MOVW	0(FP), R2
	MOVW	4(FP), R3
	MOVW	R1, 4(SP)
	MOVW	R2, M(TLBVIRT)
	MOVW	R3, M(TLBPHYS)
	NOOP
	TLBP
	NOOP
	MOVW	M(INDEX), R4
	BGEZ	R4, index
	TLBWR
	NOOP
	JAL	splx(SB)
	RET
index:
	TLBWI
	NOOP
	JAL	splx(SB)
	RET

TEXT	puttlbx(SB), $0

	MOVW	0(FP), R4
	MOVW	4(FP), R2
	MOVW	8(FP), R3
	SLL	$8, R4
	MOVW	R2, M(TLBVIRT)
	MOVW	R3, M(TLBPHYS)
	MOVW	R4, M(INDEX)
	NOOP
	TLBWI
	NOOP
	RET

TEXT	tlbp(SB), $0
	TLBP
	NOOP
	MOVW	M(INDEX), R1
	RET
	
TEXT	tlbvirt(SB), $0
	TLBP
	NOOP
	MOVW	M(TLBVIRT), R1
	RET
	

TEXT	gettlb(SB), $0

	MOVW	0(FP), R3
	MOVW	4(FP), R4
	SLL	$8, R3
	MOVW	R3, M(INDEX)
	NOOP
	TLBR
	NOOP
	MOVW	M(TLBVIRT), R1
	MOVW	M(TLBPHYS), R2
	NOOP
	MOVW	R1, 0(R4)
	MOVW	R2, 4(R4)
	RET

TEXT	gettlbvirt(SB), $0

	MOVW	0(FP), R3
	SLL	$8, R3
	MOVW	R3, M(INDEX)
	NOOP
	TLBR
	NOOP
	MOVW	M(TLBVIRT), R1
	NOOP
	RET

TEXT	vector80(SB), $-4

	MOVW	$exception(SB), R26
	JMP	(R26)

TEXT	exception(SB), $-4

	MOVW	M(STATUS), R26
	AND	$KUP, R26
	BEQ	R26, waskernel

wasuser:
	MOVW	SP, R26
		/*
		 * set kernel sp: ureg - ureg* - pc
		 * done in 2 steps because R30 is not set
		 * and the loader will make a literal
		 */
	MOVW	$((UREGADDR-2*BY2WD) & 0xffff0000), SP
	OR	$((UREGADDR-2*BY2WD) & 0xffff), SP
	MOVW	R26, 0x10(SP)			/* user SP */
	MOVW	R31, 0x28(SP)
	MOVW	R30, 0x2C(SP)
	MOVW	M(CAUSE), R26
	MOVW	R(MACH), 0x3C(SP)
	MOVW	R(USER), 0x40(SP)
	AND	$(0xF<<2), R26
	SUB	$(CSYS<<2), R26

	JAL	saveregs(SB)

	MOVW	$setR30(SB), R30
	SUBU	$(UREGADDR-2*BY2WD-USERADDR), SP, R(USER)
	MOVW	$MPID, R1
	MOVB	3(R1), R1
	MOVW	$MACHADDR, R(MACH)		/* locn of mach 0 */
	AND	$7, R1
	SLL	$PGSHIFT, R1
	ADDU	R1, R(MACH)			/* add offset for mach # */

	BNE	R26, notsys

	JAL	syscall(SB)

	MOVW	0x28(SP), R31
	MOVW	0x08(SP), R26
	MOVW	0x2C(SP), R30
	MOVW	R26, M(STATUS)
	NOOP
	MOVW	0x0C(SP), R26		/* old pc */
	MOVW	0x10(SP), SP
	RFE	(R26)

notsys:
	JAL	trap(SB)

restore:
	JAL	restregs(SB)
	MOVW	0x28(SP), R31
	MOVW	0x2C(SP), R30
	MOVW	0x3C(SP), R(MACH)
	MOVW	0x40(SP), R(USER)
	MOVW	0x10(SP), SP
	RFE	(R26)

waskernel:
	MOVW	$1, R26			/* not sys call */
	MOVW	SP, -0x90(SP)		/* drop this if possible */
	SUB	$0xA0, SP
	MOVW	R31, 0x28(SP)
	JAL	saveregs(SB)
	JAL	trap(SB)
	JAL	restregs(SB)
	MOVW	0x28(SP), R31
	ADD	$0xA0, SP
	RFE	(R26)

TEXT	saveregs(SB), $-4
	MOVW	R1, 0x9C(SP)
	MOVW	R2, 0x98(SP)
	ADDU	$8, SP, R1
	MOVW	R1, 0x04(SP)		/* arg to base of regs */
	MOVW	M(STATUS), R1
	MOVW	M(EPC), R2
	MOVW	R1, 0x08(SP)
	MOVW	R2, 0x0C(SP)

	BEQ	R26, return		/* sys call, don't save */

	MOVW	M(CAUSE), R1
	MOVW	M(BADVADDR), R2
	MOVW	R1, 0x14(SP)
	MOVW	M(TLBVIRT), R1
	MOVW	R2, 0x18(SP)
	MOVW	R1, 0x1C(SP)
	MOVW	HI, R1
	MOVW	LO, R2
	MOVW	R1, 0x20(SP)
	MOVW	R2, 0x24(SP)
					/* LINK,SB,SP missing */
	MOVW	R28, 0x30(SP)
					/* R27, R26 not saved */
					/* R25, R24 missing */
	MOVW	R23, 0x44(SP)
	MOVW	R22, 0x48(SP)
	MOVW	R21, 0x4C(SP)
	MOVW	R20, 0x50(SP)
	MOVW	R19, 0x54(SP)
	MOVW	R18, 0x58(SP)
	MOVW	R17, 0x5C(SP)
	MOVW	R16, 0x60(SP)
	MOVW	R15, 0x64(SP)
	MOVW	R14, 0x68(SP)
	MOVW	R13, 0x6C(SP)
	MOVW	R12, 0x70(SP)
	MOVW	R11, 0x74(SP)
	MOVW	R10, 0x78(SP)
	MOVW	R9, 0x7C(SP)
	MOVW	R8, 0x80(SP)
	MOVW	R7, 0x84(SP)
	MOVW	R6, 0x88(SP)
	MOVW	R5, 0x8C(SP)
	MOVW	R4, 0x90(SP)
	MOVW	R3, 0x94(SP)
return:
	RET

TEXT	restregs(SB), $-4
					/* LINK,SB,SP missing */
	MOVW	0x30(SP), R28
					/* R27, R26 not saved */
					/* R25, R24 missing */
	MOVW	0x44(SP), R23
	MOVW	0x48(SP), R22
	MOVW	0x4C(SP), R21
	MOVW	0x50(SP), R20
	MOVW	0x54(SP), R19
	MOVW	0x58(SP), R18
	MOVW	0x5C(SP), R17
	MOVW	0x60(SP), R16
	MOVW	0x64(SP), R15
	MOVW	0x68(SP), R14
	MOVW	0x6C(SP), R13
	MOVW	0x70(SP), R12
	MOVW	0x74(SP), R11
	MOVW	0x78(SP), R10
	MOVW	0x7C(SP), R9
	MOVW	0x80(SP), R8
	MOVW	0x84(SP), R7
	MOVW	0x88(SP), R6
	MOVW	0x8C(SP), R5
	MOVW	0x90(SP), R4
	MOVW	0x94(SP), R3
	MOVW	0x24(SP), R2
	MOVW	0x20(SP), R1
	MOVW	R2, LO
	MOVW	R1, HI
	MOVW	0x08(SP), R1
	MOVW	0x98(SP), R2
	MOVW	R1, M(STATUS)
	NOOP
	MOVW	0x9C(SP), R1
	MOVW	0x0C(SP), R26		/* old pc */
	RET

TEXT	rfnote(SB), $0
	MOVW	0(FP), R26		/* 1st arg is &uregpointer */
	SUBU	$(BY2WD), R26, SP	/* pc hole */
	BNE	R26, restore
	

TEXT	clrfpintr(SB), $0
	MOVW	FCR31, R1
	MOVW	R1, R2
	AND	$~(0x3F<<12), R2
	MOVW	R2, FCR31
	RET

TEXT	savefpregs(SB), $0
	MOVW	M(STATUS), R3
	MOVW	0(FP), R1
	MOVW	FCR31, R2

	MOVD	F0, 0x00(R1)
	MOVD	F2, 0x08(R1)
	MOVD	F4, 0x10(R1)
	MOVD	F6, 0x18(R1)
	MOVD	F8, 0x20(R1)
	MOVD	F10, 0x28(R1)
	MOVD	F12, 0x30(R1)
	MOVD	F14, 0x38(R1)
	MOVD	F16, 0x40(R1)
	MOVD	F18, 0x48(R1)
	MOVD	F20, 0x50(R1)
	MOVD	F22, 0x58(R1)
	MOVD	F24, 0x60(R1)
	MOVD	F26, 0x68(R1)
	MOVD	F28, 0x70(R1)
	MOVD	F30, 0x78(R1)

	MOVW	R2, 0x80(R1)
	AND	$~CU1, R3
	MOVW	R3, M(STATUS)
	RET

TEXT	restfpregs(SB), $0

	MOVW	M(STATUS), R3
	MOVW	0(FP), R1
	OR	$CU1, R3
	MOVW	R3, M(STATUS)
	MOVW	0x80(R1), R2

	MOVD	0x00(R1), F0
	MOVD	0x08(R1), F2
	MOVD	0x10(R1), F4
	MOVD	0x18(R1), F6
	MOVD	0x20(R1), F8
	MOVD	0x28(R1), F10
	MOVD	0x30(R1), F12
	MOVD	0x38(R1), F14
	MOVD	0x40(R1), F16
	MOVD	0x48(R1), F18
	MOVD	0x50(R1), F20
	MOVD	0x58(R1), F22
	MOVD	0x60(R1), F24
	MOVD	0x68(R1), F26
	MOVD	0x70(R1), F28
	MOVD	0x78(R1), F30

	MOVW	R2, FCR31
	AND	$~CU1, R3
	MOVW	R3, M(STATUS)
	RET

/*
 *  we avoid using R4, R5, R6, and R7 so gotopc can call us without saving them
 */
TEXT icflush(SB), $-4			/* icflush(physaddr, nbytes) */

	MOVW	M(STATUS), R10
	MOVW	0(FP), R8
	MOVW	4(FP), R9
	MOVW	$KSEG0, R3
	OR	R3, R8
	MOVW	$0, M(STATUS)
	MOVW	$WBFLUSH, R1		/* wbflush */
	MOVW	0(R1), R1
	NOOP
	MOVW	$KSEG1, R3
	MOVW	$icflush0(SB), R2	/* make sure PC is in uncached address space */
	MOVW	$(SWC|ISC), R1
	OR	R3, R2
	JMP	(R2)

TEXT icflush0(SB), $-4

	MOVW	R1, M(STATUS)		/* swap and isolate cache, splhi */
	MOVW	$icflush1(SB), R2
	JMP	(R2)

TEXT icflush1(SB), $-4

_icflush1:
	MOVBU	R0, 0x00(R8)
	MOVBU	R0, 0x04(R8)
	MOVBU	R0, 0x08(R8)
	MOVBU	R0, 0x0C(R8)
	MOVBU	R0, 0x10(R8)
	MOVBU	R0, 0x14(R8)
	MOVBU	R0, 0x18(R8)
	MOVBU	R0, 0x1C(R8)
	MOVBU	R0, 0x20(R8)
	MOVBU	R0, 0x24(R8)
	MOVBU	R0, 0x28(R8)
	MOVBU	R0, 0x2C(R8)
	MOVBU	R0, 0x30(R8)
	MOVBU	R0, 0x34(R8)
	MOVBU	R0, 0x38(R8)
	MOVBU	R0, 0x3C(R8)
	SUB	$0x40, R9
	ADD	$0x40, R8
	BGTZ	R9, _icflush1
	MOVW	$icflush2(SB), R2	/* make sure PC is in uncached address space */
	OR	R3, R2
	JMP	(R2)

TEXT icflush2(SB), $-4

	MOVW	$0, M(STATUS)		/* swap back caches, de-isolate them, and stay splhi */
	NOOP				/* +++ */
	MOVW	R10, M(STATUS)
	RET

TEXT dcflush(SB), $-4			/* dcflush(physaddr, nbytes) */

	MOVW	M(STATUS), R6
	MOVW	0(FP), R4
	MOVW	4(FP), R5
	MOVW	$KSEG0, R3
	OR	R3, R4
	MOVW	$0, M(STATUS)
	MOVW	$WBFLUSH, R1
	MOVW	0(R1), R1
	NOOP
	MOVW	$ISC, R1
	MOVW	R1, M(STATUS)
_dcflush0:
	MOVBU	R0, 0x00(R4)
	MOVBU	R0, 0x04(R4)
	MOVBU	R0, 0x08(R4)
	MOVBU	R0, 0x0C(R4)
	MOVBU	R0, 0x10(R4)
	MOVBU	R0, 0x14(R4)
	MOVBU	R0, 0x18(R4)
	MOVBU	R0, 0x1C(R4)
	MOVBU	R0, 0x20(R4)
	MOVBU	R0, 0x24(R4)
	MOVBU	R0, 0x28(R4)
	MOVBU	R0, 0x2C(R4)
	MOVBU	R0, 0x30(R4)
	MOVBU	R0, 0x34(R4)
	MOVBU	R0, 0x38(R4)
	MOVBU	R0, 0x3C(R4)
	SUB	$0x40, R5
	ADD	$0x40, R4
	BGTZ	R5, _dcflush0
	MOVW	$0, M(STATUS)
	NOOP				/* +++ */
	MOVW	R6, M(STATUS)
	RET
