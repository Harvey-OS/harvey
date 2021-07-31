#include "mem.h"
#include "osf1pal.h"

#define SP	R30

#define	HI_IPL	6			/* use 7 to disable mchecks */

TEXT	_main(SB), $-8
	MOVQ	$setSB(SB), R29
	MOVQ	R29, R16
	CALL_PAL $PALwrkgp
	MOVQ	$mach0(SB), R(MACH)
	MOVQ	$(BY2PG-8)(R(MACH)), R30
	MOVQ	R31, R(USER)
	MOVQ	R31, 0(R(MACH))

	MOVQ	$edata(SB), R1
	MOVQ	$end(SB), R2
clrbss:
	MOVQ	R31, (R1)
	ADDQ	$8, R1
	CMPUGT	R1, R2, R3
	BEQ	R3, clrbss

	MOVL	R0, bootconf(SB)	/* passed in from boot loader */

_fpinit:
	MOVQ	$1, R16
	CALL_PAL $PALwrfen

	MOVQ	initfpcr(SB), R1	/* MOVQ	$0x2800800000000000, R1 */
	MOVQ	R1, (R30)
	MOVT	(R30), F1
	MOVT	F1, FPCR

	MOVT	$0.5, F28
	ADDT	F28, F28, F29
	ADDT	F29, F29, F30

	MOVT	F31, F1
	MOVT	F31, F2
	MOVT	F31, F3
	MOVT	F31, F4
	MOVT	F31, F5
	MOVT	F31, F6
	MOVT	F31, F7
	MOVT	F31, F8
	MOVT	F31, F9
	MOVT	F31, F10
	MOVT	F31, F11
	MOVT	F31, F12
	MOVT	F31, F13
	MOVT	F31, F14
	MOVT	F31, F15
	MOVT	F31, F16
	MOVT	F31, F17
	MOVT	F31, F18
	MOVT	F31, F19
	MOVT	F31, F20
	MOVT	F31, F21
	MOVT	F31, F22
	MOVT	F31, F23
	MOVT	F31, F24
	MOVT	F31, F25
	MOVT	F31, F26
	MOVT	F31, F27

	JSR	main(SB)
	MOVQ	$_divq(SB), R31		/* touch _divq etc.; doesn't need to execute */
	MOVQ	$_divl(SB), R31		/* touch _divl etc.; doesn't need to execute */
	RET

TEXT	setpcb(SB), $-8
	MOVQ	R30, (R0)
	AND	$0x7FFFFFFF, R0, R16	/* make address physical */
	CALL_PAL $PALswpctx
	RET

GLOBL	mach0(SB), $(MAXMACH*BY2PG)
GLOBL	init_ptbr(SB), $8

TEXT	firmware(SB), $-8
	CALL_PAL $PALhalt

TEXT	xxfirmware(SB), $-8
	CALL_PAL $PALhalt

TEXT	splhi(SB), $0

	MOVL	R26, 4(R(MACH))		/* save PC in m->splpc */
	MOVQ	$HI_IPL, R16
	CALL_PAL $PALswpipl
	RET

TEXT	spllo(SB), $0
	MOVQ	R31, R16
	CALL_PAL $PALswpipl
	RET

TEXT	splx(SB), $0
	MOVL	R26, 4(R(MACH))		/* save PC in m->splpc */

TEXT splxpc(SB), $0			/* for iunlock */
	MOVQ	R0, R16
	CALL_PAL $PALswpipl
	RET

TEXT	spldone(SB), $0
	RET

TEXT	islo(SB), $0
	CALL_PAL $PALrdps
	AND	$IPL, R0
	XOR	$HI_IPL, R0
	RET

TEXT	mb(SB), $-8
	MB
	RET

TEXT	icflush(SB), $-8
	CALL_PAL $PALimb
	RET

TEXT	tlbflush(SB), $-8
	MOVQ	R0, R16
	MOVL	4(FP), R17
	CALL_PAL $PALtbi
	RET

TEXT	swpctx(SB), $-8
	MOVQ	R0, R16
	AND	$0x7FFFFFFF, R16	/* make address physical */
	CALL_PAL $PALswpctx
	RET

TEXT	wrent(SB), $-8
	MOVQ	R0, R17
	MOVL	4(FP), R16
	CALL_PAL $PALwrent
	RET

TEXT	wrvptptr(SB), $-8
	MOVQ	R0, R16
	CALL_PAL $PALwrvptptr
	RET

TEXT	cserve(SB), $-8
	MOVQ	R0, R16
	MOVL	4(FP), R17
	CALL_PAL $PALcserve
	RET

TEXT	setlabel(SB), $-8
	MOVL	R30, 0(R0)
	MOVL	R26, 4(R0)
	MOVQ	$0, R0
	RET

TEXT	gotolabel(SB), $-8
	MOVL	0(R0), R30
	MOVL	4(R0), R26
	MOVQ	$1, R0
	RET

TEXT	tas(SB), $-8
	MOVQ	R0, R1			/* l */
tas1:
	MOVLL	(R1), R0		/* l->key */
	BNE	R0, tas2
	MOVQ	$1, R2
	MOVLC	R2, (R1)		/* l->key = 1 */
	BEQ	R2, tas1		/* write failed, try again? */
tas2:
	RET

TEXT	_xdec(SB), $-8
	MOVQ	R0, R1			/* p */
dec1:
	MOVLL	(R1), R0		/* *p */
	SUBL	$1, R0
	MOVQ	R0, R2
	MOVLC	R2, (R1)		/* --(*p) */
	BEQ	R2, dec1		/* write failed, retry */
	RET

TEXT	_xinc(SB), $-8
	MOVQ	R0, R1			/* p */
inc1:
	MOVLL	(R1), R0		/* *p */
	ADDL	$1, R0
	MOVLC	R0, (R1)		/* (*p)++ */
	BEQ	R0, inc1		/* write failed, retry */
	RET

TEXT	cmpswap(SB), $-8
	MOVQ	R0, R1	/* p */
	MOVL	old+4(FP), R2
	MOVL	new+8(FP), R3
	MOVLL	(R1), R0
	CMPEQ	R0, R2, R4
	BEQ	R4, fail	/* if R0 != [sic] R2, goto fail */
	MOVQ	R3, R0
	MOVLC	R0, (R1)
	RET
fail:
	MOVL	$0, R0
	RET
	
TEXT	fpenab(SB), $-8
	MOVQ	R0, R16
	CALL_PAL $PALwrfen
	RET

TEXT rpcc(SB), $0
	MOVL	R0, R1
	MOVL	$0, R0
	WORD	$0x6000C000		/* RPCC R0 */
	BEQ	R1, _ret
	MOVQ	R0, (R1)
_ret:
	RET

/*
 *	Exception handlers.  The stack frame looks like this:
 *
 *	R30+0:	(unused) link reg storage (R26) (32 bits)
 *	R30+4:	padding for alignment (32 bits)
 *	R30+8:	trap()'s first arg storage (R0) (32 bits -- type Ureg*)
 *	R30+12:	padding for alignment (32 bits)
 *	R30+16:	first 31 fields of Ureg, saved here (31*64 bits)
 *	R30+264:	other 6 fields of Ureg, saved by PALcode (6*64 bits)
 *	R30+312:	previous value of KSP before trap
 */

TEXT	arith(SB), $-8
	SUBQ	$(4*BY2WD+31*BY2V), R30
	MOVQ	R0, (4*BY2WD+4*BY2V)(R30)
	MOVQ	$1, R0
	JMP	trapcommon

TEXT	illegal0(SB), $-8
	SUBQ	$(4*BY2WD+31*BY2V), R30
	MOVQ	R0, (4*BY2WD+4*BY2V)(R30)
	MOVQ	$2, R0
	JMP	trapcommon

TEXT	fault0(SB), $-8
	SUBQ	$(4*BY2WD+31*BY2V), R30
	MOVQ	R0, (4*BY2WD+4*BY2V)(R30)
	MOVQ	$4, R0
	JMP	trapcommon

TEXT	unaligned(SB), $-8
	SUBQ	$(4*BY2WD+31*BY2V), R30
	MOVQ	R0, (4*BY2WD+4*BY2V)(R30)
	MOVQ	$6, R0
	JMP	trapcommon

TEXT	intr0(SB), $-8
	SUBQ	$(4*BY2WD+31*BY2V), R30
	MOVQ	R0, (4*BY2WD+4*BY2V)(R30)
	MOVQ	$3, R0

trapcommon:
	MOVQ	R0, (4*BY2WD+0*BY2V)(R30)
	MOVQ	R16, (4*BY2WD+1*BY2V)(R30)
	MOVQ	R17, (4*BY2WD+2*BY2V)(R30)
	MOVQ	R18, (4*BY2WD+3*BY2V)(R30)

	/* R0 already saved, (4*BY2WD+4*BY2V)(R30) */
	MOVQ	R1, (4*BY2WD+5*BY2V)(R30)
	MOVQ	R2, (4*BY2WD+6*BY2V)(R30)
	MOVQ	R3, (4*BY2WD+7*BY2V)(R30)
	MOVQ	R4, (4*BY2WD+8*BY2V)(R30)
	MOVQ	R5, (4*BY2WD+9*BY2V)(R30)
	MOVQ	R6, (4*BY2WD+10*BY2V)(R30)
	MOVQ	R7, (4*BY2WD+11*BY2V)(R30)
	MOVQ	R8, (4*BY2WD+12*BY2V)(R30)
	MOVQ	R9, (4*BY2WD+13*BY2V)(R30)
	MOVQ	R10, (4*BY2WD+14*BY2V)(R30)
	MOVQ	R11, (4*BY2WD+15*BY2V)(R30)
	MOVQ	R12, (4*BY2WD+16*BY2V)(R30)
	MOVQ	R13, (4*BY2WD+17*BY2V)(R30)
	MOVQ	R14, (4*BY2WD+18*BY2V)(R30)
	MOVQ	R15, (4*BY2WD+19*BY2V)(R30)
	MOVQ	R19, (4*BY2WD+20*BY2V)(R30)
	MOVQ	R20, (4*BY2WD+21*BY2V)(R30)
	MOVQ	R21, (4*BY2WD+22*BY2V)(R30)
	MOVQ	R22, (4*BY2WD+23*BY2V)(R30)
	MOVQ	R23, (4*BY2WD+24*BY2V)(R30)
	MOVQ	R24, (4*BY2WD+25*BY2V)(R30)
	MOVQ	R25, (4*BY2WD+26*BY2V)(R30)
	MOVQ	R26, (4*BY2WD+27*BY2V)(R30)
	MOVQ	R27, (4*BY2WD+28*BY2V)(R30)
	MOVQ	R28, (4*BY2WD+29*BY2V)(R30)

	MOVQ	$HI_IPL, R16
	CALL_PAL $PALswpipl

	CALL_PAL $PALrdusp
	MOVQ	R0, (4*BY2WD+30*BY2V)(R30)	/* save USP */

	MOVQ	$mach0(SB), R(MACH)
	MOVQ	$(4*BY2WD)(R30), R0
	JSR	trap(SB)
trapret:
	MOVQ	(4*BY2WD+30*BY2V)(R30), R16	/* USP */
	CALL_PAL $PALwrusp			/* ... */
	MOVQ	(4*BY2WD+4*BY2V)(R30), R0
	MOVQ	(4*BY2WD+5*BY2V)(R30), R1
	MOVQ	(4*BY2WD+6*BY2V)(R30), R2
	MOVQ	(4*BY2WD+7*BY2V)(R30), R3
	MOVQ	(4*BY2WD+8*BY2V)(R30), R4
	MOVQ	(4*BY2WD+9*BY2V)(R30), R5
	MOVQ	(4*BY2WD+10*BY2V)(R30), R6
	MOVQ	(4*BY2WD+11*BY2V)(R30), R7
	MOVQ	(4*BY2WD+12*BY2V)(R30), R8
	MOVQ	(4*BY2WD+13*BY2V)(R30), R9
	MOVQ	(4*BY2WD+14*BY2V)(R30), R10
	MOVQ	(4*BY2WD+15*BY2V)(R30), R11
	MOVQ	(4*BY2WD+16*BY2V)(R30), R12
	MOVQ	(4*BY2WD+17*BY2V)(R30), R13
	MOVQ	(4*BY2WD+18*BY2V)(R30), R14
	MOVQ	(4*BY2WD+19*BY2V)(R30), R15
	MOVQ	(4*BY2WD+20*BY2V)(R30), R19
	MOVQ	(4*BY2WD+21*BY2V)(R30), R20
	MOVQ	(4*BY2WD+22*BY2V)(R30), R21
	MOVQ	(4*BY2WD+23*BY2V)(R30), R22
	MOVQ	(4*BY2WD+24*BY2V)(R30), R23
	MOVQ	(4*BY2WD+25*BY2V)(R30), R24
	MOVQ	(4*BY2WD+26*BY2V)(R30), R25
	MOVQ	(4*BY2WD+27*BY2V)(R30), R26
	MOVQ	(4*BY2WD+28*BY2V)(R30), R27
	MOVQ	(4*BY2WD+29*BY2V)(R30), R28
	/* USP already restored from (4*BY2WD+30*BY2V)(R30) */
	ADDQ	$(4*BY2WD+31*BY2V), R30
	CALL_PAL $PALrti

TEXT	forkret(SB), $0
	MOVQ	R31, R0				/* Fake out system call return */
	JMP	systrapret

TEXT	syscall0(SB), $-8
	SUBQ	$(4*BY2WD+31*BY2V), R30
	MOVQ	R0, (4*BY2WD+4*BY2V)(R30)	/* save scallnr in R0 */
	MOVQ	$HI_IPL, R16
	CALL_PAL $PALswpipl
	MOVQ	$mach0(SB), R(MACH)
	CALL_PAL $PALrdusp
	MOVQ	R0, (4*BY2WD+30*BY2V)(R30)	/* save USP */
	MOVQ	R26, (4*BY2WD+27*BY2V)(R30)	/* save last return address */
	MOVQ	$(4*BY2WD)(R30), R0		/* pass address of Ureg */
	JSR	syscall(SB)
systrapret:
	MOVQ	(4*BY2WD+30*BY2V)(R30), R16	/* USP */
	CALL_PAL $PALwrusp			/* consider doing this in execregs... */
	MOVQ	(4*BY2WD+27*BY2V)(R30), R26	/* restore last return address */
	ADDQ	$(4*BY2WD+31*BY2V), R30
	CALL_PAL $PALretsys

/*
 * Take first processor into user mode
 * 	- argument is stack pointer to user
 */

TEXT	touser(SB), $-8
	MOVQ	R0, R16
	CALL_PAL $PALwrusp			/* set USP to value passed */
	SUBQ	$(6*BY2V), R30			/* create frame for retsys */
	MOVQ	$(UTZERO+32), R26		/* header appears in text */
	MOVQ	R26, (1*BY2V)(R30)		/* PC -- only reg that matters */
	CALL_PAL $PALretsys

TEXT	rfnote(SB), $0
	SUBL	$(2*BY2WD), R0, SP
	JMP	trapret

TEXT	savefpregs(SB), $-8
	MOVT	F0, 0x00(R0)
	MOVT	F1, 0x08(R0)
	MOVT	F2, 0x10(R0)
	MOVT	F3, 0x18(R0)
	MOVT	F4, 0x20(R0)
	MOVT	F5, 0x28(R0)
	MOVT	F6, 0x30(R0)
	MOVT	F7, 0x38(R0)
	MOVT	F8, 0x40(R0)
	MOVT	F9, 0x48(R0)
	MOVT	F10, 0x50(R0)
	MOVT	F11, 0x58(R0)
	MOVT	F12, 0x60(R0)
	MOVT	F13, 0x68(R0)
	MOVT	F14, 0x70(R0)
	MOVT	F15, 0x78(R0)
	MOVT	F16, 0x80(R0)
	MOVT	F17, 0x88(R0)
	MOVT	F18, 0x90(R0)
	MOVT	F19, 0x98(R0)
	MOVT	F20, 0xA0(R0)
	MOVT	F21, 0xA8(R0)
	MOVT	F22, 0xB0(R0)
	MOVT	F23, 0xB8(R0)
	MOVT	F24, 0xC0(R0)
	MOVT	F25, 0xC8(R0)
	MOVT	F26, 0xD0(R0)
	MOVT	F27, 0xD8(R0)
	MOVT	F28, 0xE0(R0)
	MOVT	F29, 0xE8(R0)
	MOVT	F30, 0xF0(R0)
	MOVT	F31, 0xF8(R0)
	MOVT	FPCR, F0
	MOVT	F0, 0x100(R0)

	MOVQ	$0, R16
	CALL_PAL $PALwrfen		/* disable */
	RET

TEXT	restfpregs(SB), $-8
	MOVQ	$1, R16
	CALL_PAL $PALwrfen		/* enable */

	MOVT	0x100(R0), F0
	MOVT	F0, FPCR
	MOVT	0x00(R0), F0
	MOVT	0x08(R0), F1
	MOVT	0x10(R0), F2
	MOVT	0x18(R0), F3
	MOVT	0x20(R0), F4
	MOVT	0x28(R0), F5
	MOVT	0x30(R0), F6
	MOVT	0x38(R0), F7
	MOVT	0x40(R0), F8
	MOVT	0x48(R0), F9
	MOVT	0x50(R0), F10
	MOVT	0x58(R0), F11
	MOVT	0x60(R0), F12
	MOVT	0x68(R0), F13
	MOVT	0x70(R0), F14
	MOVT	0x78(R0), F15
	MOVT	0x80(R0), F16
	MOVT	0x88(R0), F17
	MOVT	0x90(R0), F18
	MOVT	0x98(R0), F19
	MOVT	0xA0(R0), F20
	MOVT	0xA8(R0), F21
	MOVT	0xB0(R0), F22
	MOVT	0xB8(R0), F23
	MOVT	0xC0(R0), F24
	MOVT	0xC8(R0), F25
	MOVT	0xD0(R0), F26
	MOVT	0xD8(R0), F27
	MOVT	0xE0(R0), F28
	MOVT	0xE8(R0), F29
	MOVT	0xF0(R0), F30
	MOVT	0xF8(R0), F31
	RET
