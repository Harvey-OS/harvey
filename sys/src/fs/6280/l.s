#include "mem.h"

#define SP		R29
#define HIPRI		(0)
#define LOPRI		(CU1|INTR1|INTR0|SW1|SW0)

#define FLUSH		MOVWR
#define INVALIDATE	MOVWL
#define CACHEREAD	MOVWR
#define CACHEWRITE	MOVWL
#define NOOP		WORD	$0x00000000
#define	WAIT		NOOP; NOOP

#define PROM		(KSEG1+0x1FC00000)
#define SBC0ADDR	(KSEG1+0x1E00F000)
#define TIMERIE		0x00000001

/*
 * Boot first processor
 */
TEXT	start(SB), $-4
	MOVW	$setR30(SB), R30
	MOVW	$SBC0ADDR, R1
	MOVW	$0, 8(R1)			/* clear interrupt mask */
	MOVW	$~0, R4
	MOVW	R4, 4(R1)			/* clear all pending interrupts */
	MOVW	$LOPRI, R1
	MOVW	$~(SW1|SW0), R2			/* clear software interrupts */
	MOVW	R1, M(STATUS)
	MOVW	$0x8000F, R1
	MOVW	R2, M(CAUSE)
	MOVW	R1, M(ERROR)

	MOVW	R0, FCR31
	MOVW	$MACHADDR, R(MACH)
	ADDU	$(BY2PG-4), R(MACH), SP
	MOVW	$0, R(USER)
	MOVW	R0, 0(R(MACH))

	MOVW	$end(SB), R2			/* clear bss */
	MOVW	$edata(SB), R1
	JMP	_clrbsse
_clrbssl:
	MOVW	$0, (R1)
	ADD	$4, R1
_clrbsse:
	BNE	R1, R2, _clrbssl

	JAL	main(SB)
	JMP	(R0)


TEXT	firmware(SB), $-4

	MOVW	$(PROM+0x18), R1 /**/
/*	MOVW	$(PROM+0x00), R1 /**/
	JMP	(R1)

TEXT	splhi(SB), $-4

	MOVW	M(STATUS), R1
	AND	$~IEC, R1, R2
	MOVW	R2, M(STATUS)
	RET

TEXT	spllo(SB), $-4

	MOVW	M(STATUS), R1
	OR	$IEC, R1, R2
	MOVW	R2, M(STATUS)
	RET

TEXT	splx(SB), $-4

	MOVW	M(STATUS), R2
	AND	$IEC, R1
	AND	$~IEC, R2
	OR	R2, R1
	MOVW	R1, M(STATUS)
	RET

TEXT intrclr(SB), $-4			/* R1 is bits to clear */
	MOVW	$SBC0ADDR, R4		/* chip address */
	MOVW	R1, 0x04(R4)		/* intvectclr */

	MOVW	M(STATUS), R8
	MOVW	R0, M(STATUS)		/* cpu interrupts off */
	NOOP
	NOOP

	MOVW	0x2C(R4), R3		/* timer counter */
	MOVW	0x28(R4), R2		/* timer compare */
	SUBU	R3, R2			/* compare - counter */
	MOVW	$((60*1000)*100), R3	/* (60*1000)*MS2HZ */
	SGTU	R2, R3, R2		/* clock interrupt soon? */
	BEQ	R2, _intrclr0

	MOVW	$TIMERIE, R2		/* set timer interrupt */
	MOVW	R2, 0x00(R4)		/* intvectset */

_intrclr0:
	MOVW	R8, M(STATUS)
	RET
	

TEXT wbflush(SB), $-4
	RET

TEXT	setlabel(SB), $-4

	MOVW	R31, 0(R1)
	MOVW	R29, 4(R1)
	MOVW	$0, R1
	RET

TEXT	gotolabel(SB), $-4

	MOVW	0(R1), R31
	MOVW	4(R1), R29
	MOVW	$1, R1
	RET

/*
 * lock test-and-set, tas(struct lock *l);
 * knows lock structure layout, will
 * initialise the pc element if successful
 */
TEXT tas(SB), $0
	MOVW	R1, R4			/* l */
	MOVW	M(STATUS), R5
	MOVW	$0, M(STATUS)
	WAIT;WAIT
	MOVW	(R4), R1		/* l->key */
	BNE	R1,R0, tas1
	MOVW	$0xdeadbeef, R2
	MOVW	R2, (R4)		/* l->key = 1 */
tas1:
	MOVW	R5, M(STATUS)
	RET

TEXT	vector80(SB), $-4

	MOVW	$exception(SB), R26
	JMP	(R26)

TEXT	exception(SB), $-4

	MOVW	SP, -0x90(SP)		/* drop this if possible */
	SUB	$0xA0, SP
	MOVW	R31, 0x28(SP)
	JAL	saveregs(SB)
	MOVW	4(SP), R1
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

	MOVW	M(CAUSE), R1
	MOVW	M(BADVADDR), R2
	MOVW	R1, 0x14(SP)
	MOVW	M(ERROR), R1
	MOVW	R2, 0x18(SP)
	MOVW	R1, 0x1C(SP)
	MOVW	HI, R1
	MOVW	LO, R2
	MOVW	R1, 0x20(SP)
	MOVW	R2, 0x24(SP)
					/* LINK, SB, SP missing */
	MOVW	R28, 0x30(SP)

	MOVW	$SBC0ADDR, R1
	MOVW	0(R1), R2
	MOVW	R2, 0x34(SP)		/* R27 not saved - INTVECTSET */
	MOVW	8(R1), R2
	MOVW	R2, 0x38(SP)		/* R26 not saved - INTVECTMASK */
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
	MOVW	0x9C(SP), R1
	MOVW	0x0C(SP), R26		/* old pc */
	RET

/*
 *	float	famd(float a, int b, int c, int d)
 *		return ((a+b) * c) / d;
 */
	TEXT	famd(SB), $0

	MOVW	R1, a+0(FP)
	MOVW	M(STATUS), R6	/* splhi */
	AND	$~IEC, R6, R1
	MOVW	R1, M(STATUS)

	MOVW	FCR31, R7	/* save */
	MOVW	F0, R8
	MOVW	F2, R9

	MOVF	a+0(FP), F0	/* load */

	MOVF	b+4(FP), F2	/* convert-add */
	MOVWF	F2, F2
	ADDF	F2, F0

	MOVF	c+8(FP), F2	/* convert-mul */
	MOVWF	F2, F2
	MULF	F2, F0

	MOVF	d+12(FP), F2	/* convert-div */
	MOVWF	F2, F2
	DIVF	F2, F0

	MOVW	F0, R1		/* return */

	MOVW	R8, F0		/* restore */
	MOVW	R9, F2

	MOVW	R7, FCR31
	MOVW	R6, M(STATUS)	/* splx */
	RET

/*
 *	ulong	fdf(float a, int b)
 *		return a / b;
 */
	TEXT	fdf(SB), $0

	MOVW	R1, a+0(FP)
	MOVW	M(STATUS), R6	/* splhi */
	AND	$~IEC, R6, R1
	MOVW	R1, M(STATUS)

	MOVW	FCR31, R7	/* save */
	MOVW	F0, R8
	MOVW	F2, R9

	MOVF	a+0(FP), F0	/* load */

	MOVF	b+4(FP), F2	/* convert-div */
	MOVWF	F2, F2
	DIVF	F2, F0

	MOVFW	F0, F0		/* convert-return */
	MOVW	F0, R1

	MOVW	R8, F0		/* restore */
	MOVW	R9, F2

	MOVW	R7, FCR31
	MOVW	R6, M(STATUS)	/* splx */
	RET


TEXT abort(SB), $-4
	WORD	$0x0000000D
	RET

TEXT probe(SB), $-4			/* probe(addr, size) */
	MOVW	R1, 0(FP)
	MOVW	$probefault(SB), R1
	MOVW	R1, probepc(SB)
	MOVW	$1, R3
	MOVW	4(FP), R2		/* size */
	MOVW	0(FP), R4		/* source address */
_probebyte:
	BNE	R3, R2, _probeshort
	MOVBU	(R4), R1
	JMP	_probedone
_probeshort:
	SLL	$1, R3
	BNE	R3, R2, _probeword
	AND	$1, R4, R5
	BNE	R5, _probefault0
	MOVHU	(R4), R1
	JMP	_probedone
_probeword:
	SLL	$1, R3
	BNE	R3, R2, _probefault0
	AND	$3, R4, R5
	BNE	R5, _probefault0
	MOVW	(R4), R1
_probedone:
	MOVW	$0, probepc(SB)
	MOVW	$0, R1			/* return value */
	RET

TEXT probefault(SB), $-4
_probefault0:
	MOVW	$0, probepc(SB)
	MOVW	$-1, R1
	RET
