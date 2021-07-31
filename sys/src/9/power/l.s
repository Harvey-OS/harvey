#include "mem.h"

#define SP		R29

#define PROM		(KSEG1+0x1FC00000)
#define	NOOP		NOR R0,R0
#define	WAIT		NOOP; NOOP

/*
 * Boot first processor
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

	MOVW	R1, SP			/* user stack pointer */
	MOVW	M(STATUS), R1
	OR	$(KUP|IEP), R1
	MOVW	R1, M(STATUS)
	NOOP
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

TEXT	muxlock(SB),$0

	MOVW	R1, R2		/* sbsem */
	MOVW	4(FP), R3	/* &lk->val */

	MOVW	M(STATUS), R5	/* splhi */
	NOOP
	AND	$~IEC, R5, R4
	MOVW	R4, M(STATUS)
	NOOP

	MOVW	0(R2),R4	/* grab sbsem */
	AND	$1, R4
	BNE	R4, f1
	MOVW	0(R3),R4
	BNE	R4, f0

	MOVW	$1, R1
	MOVW	R1, 0(R3)	/* lk->val = 1 */
	MOVW	R0, 0(R2)	/* *sbsem = 0 */
	MOVW	R5, M(STATUS)	/* splx */
	RET

f0:	MOVW	R0, 0(R2)	/* *sbsem = 0 */
f1:	MOVW	R5, M(STATUS)	/* splx */
	MOVW	R0, R1		/* return 0 */
	RET

TEXT	spldone(SB), $0

	RET

TEXT	wbflush(SB), $-4

	MOVW	$WBFLUSH, R1
	MOVW	0(R1), R1
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

TEXT	getcallerpc(SB), $0

	MOVW	0(SP), R1
	RET

TEXT	gotopc(SB), $8

	MOVW	R1, 0(FP)
	MOVW	$(64*1024), R1
	MOVW	R1, 8(SP)
	MOVW	R0, R1
	JAL	icflush(SB)
	MOVW	0(FP), R1

	MOVW	_argc(SB), R4
	MOVW	_argv(SB), R5
	MOVW	_env(SB), R6
	MOVW	R0, 4(SP)
	JMP	(R1)

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
	MOVW	M(TLBVIRT), R2
	AND	$(~(BY2PG-1)), R1, R4	/* get the VPN */
	AND	$((NTLBPID-1)<<6), R2	/* get the pid */
	OR	R4, R2
	MOVW	R2, M(TLBVIRT)
	NOOP
	TLBP
	NOOP
	MOVW	M(INDEX), R2
	MOVW	R0, R1
	BLTZ	R2, bad
	MOVW	$1, R1
bad:
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

TEXT	vector80(SB), $-4

	MOVW	$exception(SB), R26
	JMP	(R26)

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
	SLL	$8, R27
	/* R27 = (((tlbvirt<<1)^(tlbvirt>>12)) & (STLBSIZE-1)) << 8 (8 to clear zero in TLBPHYS) */
	MOVW	R27, M(TLBPHYS)			/* scratch register, store */

	MOVW	$((MACHADDR+4) & 0xffff0000), R26	/* get m->stb BUG */	
	OR	$((MACHADDR+4) & 0xffff), R26
	MOVW	$MPID, R27				/* add BY2PG*machno */
	MOVB	3(R27), R27
	AND	$7, R27
	SLL	$PGSHIFT, R27
	ADDU	R27, R26
	
	MOVW	M(TLBPHYS), R27			/* scratch register, load */
	MOVW	(R26), R26
	SRL	$5, R27				/* R27 is now index * 8 */
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
	SUBU	$(CSYS<<2), R26

	JAL	saveregs(SB)

	MOVW	$setR30(SB), R30
	SUBU	$(UREGADDR-2*BY2WD-USERADDR), SP, R(USER)
	MOVW	$MPID, R1
	MOVB	3(R1), R1
	MOVW	$MACHADDR, R(MACH)		/* locn of &mach[0] */
	AND	$7, R1
	SLL	$PGSHIFT, R1
	ADDU	R1, R(MACH)			/* add offset for mach # */
	MOVW	4(SP), R1			/* first arg for syscall, trap */

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
	MOVW	4(SP), R1		/* first arg for trap */
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
	MOVW	FCR31, R1		/* Read it to stall the fpu */
	WAIT
	MOVW	R0, FCR31
	WAIT
	AND	$~CU1, R3
	MOVW	R3, M(STATUS)
	RET

TEXT	getstatus(SB), $0
	MOVW	M(STATUS), R1
	RET

TEXT	savefpregs(SB), $0
	MOVW	M(STATUS), R3
	MOVW	FCR31, R2		/* Read stalls the fpu until inst. complete. */
	WAIT

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
	WAIT

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
	MOVW	M(STATUS), R3
	NOOP
	AND	$~CU1, R3
	MOVW	R3, M(STATUS)
	RET

#define NOP	WORD	$0x0

TEXT icflush(SB), $-4
	MOVW	R1, R4
	MOVW	4(FP), R5
	MOVW	$icflush0(SB), R2	/* Jump to uncache space */
	MOVW	$0xA0000000, R1
	OR	R1, R2
	JMP	(R2)
	NOP

TEXT icflush0(SB), $-4

	MOVW	$833, R12		/* cache_pass magic */
	MOVW	$0x10000, R9		/* icache size */

	MOVW	$0, R13
	MOVW	M(STATUS), R11

	BEQ	R9, R0, _icflush3
	MOVW	$0x30000, R2		/* swap and isolate */

	NOP
	NOP
	NOP
	NOP
	NOP
	MOVW	R2, M(STATUS)
	NOP
	NOP
	NOP
	NOP
	NOP
	
	MOVW	R4, R8
	MOVW	R5, R9
	AND	$0xFFFC, R8
	MOVW	$0x9F200000, R1
	OR	R1, R8

	MOVW	$icflush1(SB), R2
	JMP	(R2)
	NOP

TEXT icflush1(SB), $-4
_icflush1:
	SGT	R12, R13, R1
	BNE	R1, _icflush2

	MOVW	$0, R13
	MOVW	R11, M(STATUS)
	MOVW	$0x30000, R2		/* swap and isolate */

	NOP
	NOP
	NOP
	NOP
	NOP
	MOVW	R2, M(STATUS)
	NOP
	NOP
	NOP
	NOP
	NOP

_icflush2:
	MOVW	R0, 0(R8)
	MOVW	R0, 4(R8)
	MOVW	R0, 8(R8)
	MOVW	R0, 12(R8)
	MOVW	R0, 16(R8)
	MOVW	R0, 20(R8)
	MOVW	R0, 24(R8)
	MOVW	R0, 28(R8)
	ADDU	$-32, R9
	ADDU	$1, R13
	ADDU	$32, R8
	BGTZ	R9, _icflush1

	MOVW	$icflush3(SB), R2	/* Jump to uncache space */
	MOVW	$0xA0000000, R1
	OR	R1, R2
	JMP	(R2)
	NOP

TEXT icflush3(SB), $-4
_icflush3:
	NOP
	NOP
	NOP
	NOP
	NOP
	MOVW	R11, M(STATUS)
	NOP
	NOP
	NOP
	NOP
	NOP

	MOVW	$icflush4(SB), R2
	JMP	(R2)
	NOP

TEXT icflush4(SB), $-4
	RET

TEXT dcflush(SB), $-4
	MOVW	R1, R4
	MOVW	4(FP), R5
	MOVW	$dcflush0(SB), R2	/* Jump to uncache space */
	MOVW	$0xA0000000, R1
	OR	R1, R2
	JMP	(R2)
	NOP

TEXT dcflush0(SB), $-4

	MOVW	$833, R12		/* cache_pass magdc */
	MOVW	$0x10000, R9		/* dcache size */

	MOVW	$0, R13
	MOVW	M(STATUS), R11

	BEQ	R9, R0, _dcflush3
	MOVW	$0x10000, R2		/* isolate data cache */

	NOP
	NOP
	NOP
	NOP
	NOP
	MOVW	R2, M(STATUS)
	NOP
	NOP
	NOP
	NOP
	NOP
	
	MOVW	R4, R8
	MOVW	R5, R9
	AND	$0xFFFC, R8
	MOVW	$0x9F200000, R1
	OR	R1, R8

	MOVW	$dcflush1(SB), R2
	JMP	(R2)
	NOP

TEXT dcflush1(SB), $-4
_dcflush1:
	SGT	R12, R13, R1
	BNE	R1, _dcflush2

	MOVW	$0, R13
	MOVW	R11, M(STATUS)
	MOVW	$0x10000, R2		/* isolate data cache */

	NOP
	NOP
	NOP
	NOP
	NOP
	MOVW	R2, M(STATUS)
	NOP
	NOP
	NOP
	NOP
	NOP

_dcflush2:
	MOVW	R0, 0(R8)
	MOVW	R0, 4(R8)
	MOVW	R0, 8(R8)
	MOVW	R0, 12(R8)
	MOVW	R0, 16(R8)
	MOVW	R0, 20(R8)
	MOVW	R0, 24(R8)
	MOVW	R0, 28(R8)
	ADDU	$-32, R9
	ADDU	$1, R13
	ADDU	$32, R8
	BGTZ	R9, _dcflush1

	MOVW	$dcflush3(SB), R2	/* Jump to uncache space */
	MOVW	$0xA0000000, R1
	OR	R1, R2
	JMP	(R2)
	NOP

TEXT dcflush3(SB), $-4
_dcflush3:
	NOP
	NOP
	NOP
	NOP
	NOP
	MOVW	R11, M(STATUS)
	NOP
	NOP
	NOP
	NOP
	NOP

	MOVW	$dcflush4(SB), R2
	JMP	(R2)
	NOP

TEXT dcflush4(SB), $-4
	RET
