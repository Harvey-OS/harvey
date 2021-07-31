#include "mem.h"

#define SP		R29

#define LIO_ADDR	0x1f000000	/* Local IO Segment */
#define CPU_CONFIG	(KSEG1+0x1FA00000)
#define PROM		(KSEG1+0x1FC00000)
#define	NOOP		WORD	$0x27
#define	WAIT		NOOP; NOOP

#define WBFLUSH	\
	MOVW	$CPU_CONFIG, R1 /* force a xfer through the WB */; \
	MOVW	0(R1), R1; \
	NOOP

/*
 * Boot first processor
 */
TEXT	start(SB), $-4

	MOVW	$setR30(SB), R30
	MOVW	$(CU1|INTR5|INTR4|INTR3|INTR2|INTR1|INTR0|SW1|SW0), R1
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

	MOVW	R(MACH), R1
	OR	$KSEG1, R1
	ADDU	$4, SP, R2
	OR	$KSEG1, R2
clrmach:
	MOVW	$0, (R1)
	ADDU	$4, R1
	BNE	R1, R2, clrmach

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

	MOVW	R1, SP
	MOVW	M(STATUS), R1
	OR	$(KUP|IEP), R1
	MOVW	R1, M(STATUS)
	NOOP
	MOVW	$(UTZERO+32), R26	/* header appears in text */
	RFE	(R26)

TEXT	firmware(SB), $0

	SLL	$3, R1
	ADDU	$PROM, R1
	JMP	(R1)

TEXT	splhi(SB), $0

	MOVW	R31, 8(R(MACH))	/* save PC in m->splpc */
	MOVW	M(STATUS), R1
	AND	$~IEC, R1, R2
	MOVW	R2, M(STATUS)
	NOOP
	RET

TEXT	splx(SB), $0

	MOVW	R31, 8(R(MACH))	/* save PC in m->splpc */
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

TEXT	spldone(SB), $0

	RET

TEXT	wbflush(SB), $-4

	NOOP
	NOOP
	NOOP
	NOOP
	WBFLUSH
	NOOP
	RET

TEXT	setlabel(SB), $-4

	MOVW	R29, 0(R1)
	MOVW	R31, 4(R1)
	MOVW	$0, R1
	RET

TEXT	gotolabel(SB), $-4

	MOVW	0(R1), R29
	MOVW	4(R1), R31
	MOVW	$1, R1
	RET

TEXT	gotopc(SB), $8

	MOVW	R1, 0(FP)		/* save arguments for later */
	MOVW	$(64*1024), R7
	MOVW	R7, 8(SP)
	JAL	icflush(SB)
	MOVW	0(FP), R7
	MOVW	_argc(SB), R4
	MOVW	_argv(SB), R5
	MOVW	_env(SB), R6
	MOVW	R0, 4(SP)
	JMP	(R7)

TEXT	puttlb(SB), $4

	MOVW	4(FP), R2
	MOVW	R1, M(TLBVIRT)
	MOVW	R2, M(TLBPHYS)
	NOOP
	TLBP
	NOOP
	MOVW	M(INDEX), R3
	BGEZ	R3, index
	TLBWR
	NOOP
	RET
index:
	TLBWI
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

	MOVW	4(FP), R4
	SLL	$8, R1
	MOVW	R1, M(INDEX)
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

	SLL	$8, R1
	MOVW	R1, M(INDEX)
	NOOP
	TLBR
	NOOP
	MOVW	M(TLBVIRT), R1
	NOOP
	RET

TEXT	vector0(SB), $-4
	MOVW	$((MACHADDR+12) & 0xffff0000), R26	/* get m->tlbfault BUG */	
	OR	$((MACHADDR+12) & 0xffff), R26
	MOVW	(R26), R27
	ADDU	$1, R27
	MOVW	R27, (R26)

	MOVW	$utlbmiss(SB), R26
	MOVW	M(TLBVIRT), R27
	JMP	(R26)

TEXT	utlbmiss(SB), $-4

	MOVW	R27, R26
	SLL	$1, R26
	SRL	$12, R27
	XOR	R26, R27
	AND	$(STLBSIZE-1), R27
	SLL	$3, R27
	/* R27 = (((tlbvirt<<1)^(tlbvirt>>12)) & (STLBSIZE-1)) x 8 */

	MOVW	$((MACHADDR+4) & 0xffff0000), R26	/* get m->stb */	
	MOVW	((MACHADDR+4) & 0xffff)(R26), R26

	ADDU	R26, R27
	MOVW	4(R27), R26
	MOVW	R26, M(TLBPHYS)

	MOVW	M(TLBVIRT), R26
	MOVW	(R27), R27
	BNE	R26, R27, stlbm

	TLBWR
	MOVW	M(EPC), R27
	RFE	(R27)

stlbm:
/* Temporary */	
	MOVW	$((MACHADDR+16) & 0xffff0000), R26	/* get m->tlbpurge BUG */	
	OR	$((MACHADDR+16) & 0xffff), R26
	MOVW	(R26), R27
	ADDU	$1, R27
	MOVW	R27, (R26)

	MOVW	$exception(SB), R26
	JMP	(R26)

TEXT	vector80(SB), $-4

	MOVW	$exception(SB), R26
	JMP	(R26)

utas:
	/*
	 * test and set simulation for user-level processes
	 * cannot take a fault; therefore we probe the tlb by hand
	 */
	MOVW	M(EPC), R3
	ADDU	$4, R3			/* skip the cop3 instruction */
	BLTZ	R1, badtas

	MOVW	M(TLBVIRT), R2
	AND	$(~(BY2PG-1)), R1, R4	/* get the VPN */
	AND	$((NTLBPID-1)<<6), R2	/* get the pid */
	OR	R4, R2
	MOVW	R2, M(TLBVIRT)
	NOOP
	TLBP
	NOOP
	MOVW	M(INDEX), R2
	BLTZ	R2, badtas
	TLBR
	MOVW	R1, R2
	MOVW	M(TLBPHYS), R1
	MOVW	$1, R4
	AND	$PTEWRITE, R1
	BEQ	R1, badtas
	MOVBU	(R2), R1
	MOVBU	R4, (R2)
	RFE	(R3)
badtas:
	MOVW	$-1, R1
	RFE	(R3)

TEXT	exception(SB), $-4

	MOVW	M(STATUS), R26
	AND	$KUP, R26
	BEQ	R26, waskernel

wasuser:
	/*
	 * cop3 unusable == magnum test and set
	 */
	MOVW	M(CAUSE), R27
	MOVW	$(((EXCMASK<<2)|(3<<28)) & 0xffff0000), R26
	OR	$(((EXCMASK<<2)|(3<<28)) & 0xffff), R26
	AND	R26, R27
	MOVW	$(((CCPU<<2)|(3<<28)) & 0xffff0000), R26
	OR	$(((CCPU<<2)|(3<<28)) & 0xffff), R26
	BEQ	R26, R27, utas

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
	SUBU	$(CSYS<<2), R26

	JAL	saveregs(SB)

	MOVW	$setR30(SB), R30
	SUBU	$(UREGADDR-2*BY2WD-USERADDR), SP, R(USER)
	MOVW	$MACHADDR, R(MACH)		/* locn of mach 0 */
	MOVW	4(SP), R1			/* first arg to syscall, trap */

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
	SUBU	$0xA0, SP
	MOVW	R31, 0x28(SP)
	JAL	saveregs(SB)
	MOVW	4(SP), R1		/* first arg to syscall, trap */
	JAL	trap(SB)
	JAL	restregs(SB)
	MOVW	0x28(SP), R31
	ADDU	$0xA0, SP
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
	MOVW	R1, R26			/* 1st arg is &uregpointer */
	SUBU	$(BY2WD), R26, SP	/* pc hole */
	JMP	restore
	

TEXT	clrfpintr(SB), $0
	MOVW	M(STATUS), R3
	OR	$CU1, R3
	MOVW	R3, M(STATUS)
	WAIT

	MOVW	FCR31, R1
	MOVW	R1, R2
	AND	$~(0x3F<<12), R2
	MOVW	R2, FCR31

	AND	$~CU1, R3
	MOVW	R3, M(STATUS)
	RET

TEXT	getstatus(SB), $0
	MOVW	M(STATUS), R1
	RET

TEXT	savefpregs(SB), $0
	MOVW	M(STATUS), R3
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
	OR	$CU1, R3
	MOVW	R3, M(STATUS)
	MOVW	fpstat+4(FP), R2
	NOOP

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

TEXT	fcr31(SB), $0

	MOVW	FCR31, R1
	RET

TEXT	prid(SB), $0

	MOVW	M(PRID), R1
	WAIT
	RET

/*
 * Emulate 68020 test and set
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

/*
 *  we avoid using R4, R5, R6, and R7 so gotopc can call us without saving them
 */
TEXT	icflush(SB), $-4			/* icflush(physaddr, count) */

	MOVW	M(STATUS), R10
	MOVW	R1, R8
	MOVW	4(FP), R9
	AND	$0xfffc, R8
	MOVW	$(KSEG0|LIO_ADDR), R3
	OR	R3, R8
	MOVW	$0, M(STATUS)
	WBFLUSH
	NOOP
	MOVW	$KSEG1, R3
	MOVW	$icflush0(SB), R2
	MOVW	$(SWC|ISC), R1
	OR	R3, R2
	JMP	(R2)
	/* Work in uncached space */
TEXT icflush0(SB), $-4
	MOVW	R1, M(STATUS)
	MOVW	$icflush1(SB), R2
	JMP	(R2)
TEXT icflush1(SB), $-4
_icflush1:
	MOVW	R0, 0x00(R8)
	MOVW	R0, 0x04(R8)
	MOVW	R0, 0x08(R8)
	MOVW	R0, 0x0C(R8)
	MOVW	R0, 0x10(R8)
	MOVW	R0, 0x14(R8)
	MOVW	R0, 0x18(R8)
	MOVW	R0, 0x1C(R8)
	MOVW	R0, 0x20(R8)
	MOVW	R0, 0x24(R8)
	MOVW	R0, 0x28(R8)
	MOVW	R0, 0x2C(R8)
	MOVW	R0, 0x30(R8)
	MOVW	R0, 0x34(R8)
	MOVW	R0, 0x38(R8)
	MOVW	R0, 0x3C(R8)
	SUBU	$0x40, R9
	ADDU	$0x40, R8
	BGTZ	R9, _icflush1
	MOVW	$icflush2(SB), R2
	OR	R3, R2
	JMP	(R2)
TEXT icflush2(SB), $-4
	MOVW	$0, M(STATUS)
	NOOP
	MOVW	R10, M(STATUS)
	RET

TEXT	dcflush(SB), $-4			/* dcflush(physaddr, count) */

	MOVW	M(STATUS), R10
	MOVW	R1, R8
	MOVW	4(FP), R9
	AND	$0xfffc, R8
	MOVW	$(KSEG0|LIO_ADDR), R3
	OR	R3, R8
	MOVW	$0, M(STATUS)
	WBFLUSH
	NOOP
	MOVW	$ISC, R1
	MOVW	R1, M(STATUS)
	NOOP
_dcflush0:
	MOVW	R0, 0x00(R8)
	MOVW	R0, 0x04(R8)
	MOVW	R0, 0x08(R8)
	MOVW	R0, 0x0C(R8)
	MOVW	R0, 0x10(R8)
	MOVW	R0, 0x14(R8)
	MOVW	R0, 0x18(R8)
	MOVW	R0, 0x1C(R8)
	MOVW	R0, 0x20(R8)
	MOVW	R0, 0x24(R8)
	MOVW	R0, 0x28(R8)
	MOVW	R0, 0x2C(R8)
	MOVW	R0, 0x30(R8)
	MOVW	R0, 0x34(R8)
	MOVW	R0, 0x38(R8)
	MOVW	R0, 0x3C(R8)
	SUBU	$0x40, R9
	ADDU	$0x40, R8
	BGTZ	R9, _dcflush0
	MOVW	$0, M(STATUS)
	NOOP				/* +++ */
	MOVW	R10, M(STATUS)
	RET

TEXT	getcallerpc(SB), $0

	MOVW	0(SP), R1
	RET

TEXT	busprobe(SB), $-4

	NOOP
	MOVW	(R1), R2
	MOVW	$0, R1
	NOOP
	RET

TEXT	rdcount(SB), $-4

	MOVW	$0, R1
	RET
