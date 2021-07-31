#include "mem.h"

#define SP		R29

#define PROM		(KSEG1+0x1FC00000)
#define	NOOP		NOR R0,R0
#define	WAIT		NOOP; NOOP
#define WBFLUSH		WORD $0x4100FFFF

/*
 * Boot first processor
 *   - why is the processor number loaded from R0 ?????
 */
TEXT	start(SB), $-4

	MOVW	$setR30(SB), R30
	MOVW	$(CU1|INTR5|INTR3|INTR2|INTR1|INTR0|SW1|SW0), R1
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

TEXT	boot(SB), $0

	MOVW	_argc(SB), R4
	MOVW	_argv(SB), R5
	MOVW	_env(SB), R6
	JMP	(R1)

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

TEXT	splx(SB), $0

	MOVW	M(STATUS), R2
	AND	$IEC, R1
	AND	$~IEC, R2
	OR	R2, R1
	MOVW	R1, M(STATUS)
	NOOP
	RET

TEXT	spllo(SB), $0

	MOVW	M(STATUS), R1
	OR	$IEC, R1, R2
	MOVW	R2, M(STATUS)
	NOOP
	RET

TEXT	wbflush(SB), $-4

	NOOP
	NOOP
	NOOP
	NOOP
	WBFLUSH
	NOOP
	RET

TEXT	puttlbx(SB), $0

	MOVW	4(FP), R2
	MOVW	8(FP), R3
	SLL	$8, R1
	MOVW	R2, M(TLBVIRT)
	MOVW	R3, M(TLBPHYS)
	MOVW	R1, M(INDEX)
	NOOP
	TLBWI
	NOOP
	RET

TEXT	vector80(SB), $-4

	MOVW	$exception(SB), R26
	JMP	(R26)

TEXT	exception(SB), $-4

	MOVW	$1, R26			/* not sys call */
	MOVW	SP, -0x90(SP)		/* drop this if possible */
	SUB	$0xA0, SP
	MOVW	R31, 0x28(SP)
	JAL	saveregs(SB)
	MOVW	4(SP), R1			/* first arg to syscall, trap */
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

/*
 *  we avoid using R4, R5, R6, and R7 so gotopc can call us without saving them
 */
TEXT	icflush(SB), $-4			/* icflush(physaddr, count) */

	MOVW	M(STATUS), R10
	MOVW	R1, R8
	MOVW	4(FP), R9
	MOVW	$KSEG0, R3
	OR	R3, R8
	MOVW	$0, M(STATUS)
	WBFLUSH
	NOOP
	MOVW	$KSEG1, R3
	MOVW	$icflush0(SB), R2
	MOVW	$(SWC|ISC), R1
	OR	R3, R2
	JMP	(R2)
TEXT icflush0(SB), $-4
	MOVW	R1, M(STATUS)
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
	MOVW	$icflush2(SB), R2
	OR	R3, R2
	JMP	(R2)
TEXT icflush2(SB), $-4
	MOVW	$0, M(STATUS)
	NOOP				/* +++ */
	MOVW	R10, M(STATUS)
	RET

TEXT	dcflush(SB), $-4			/* dcflush(physaddr, count) */

	MOVW	M(STATUS), R10
	MOVW	R1, R8
	MOVW	4(FP), R9
	MOVW	$KSEG0, R3
	OR	R3, R8
	MOVW	$0, M(STATUS)
	WBFLUSH
	NOOP
	MOVW	$ISC, R1
	MOVW	R1, M(STATUS)
	NOOP
_dcflush0:
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
	BGTZ	R9, _dcflush0
	MOVW	$0, M(STATUS)
	NOOP				/* +++ */
	MOVW	R10, M(STATUS)
	RET
