/*
 * Far call, absolute indirect.
 * The argument is the offset.
 * We use a global structure for the jump params,
 * so this is *not* reentrant or thread safe.
 */

#include "mem.h"

#define SSOVERRIDE	BYTE $0x36
#define CSOVERRIDE	BYTE $0x2E
#define RETF		BYTE $0xCB

GLOBL	apmjumpstruct+0(SB), $8

TEXT fortytwo(SB), $0
	MOVL	$42, AX
	RETF

TEXT getcs(SB), $0
	PUSHL	CS
	POPL	AX
	RET

TEXT apmfarcall(SB), $0
	/*
	 * We call push and pop ourselves.
	 * As soon as we do the first push or pop,
	 * we can't use FP anymore.
	 */
	MOVL	off+4(FP), BX
	MOVL	seg+0(FP), CX
	MOVL	BX, apmjumpstruct+0(SB)
	MOVL	CX, apmjumpstruct+4(SB)

	/* load necessary registers from Ureg */
	MOVL	ureg+8(FP), DI
	MOVL	28(DI), AX
	MOVL	16(DI), BX
	MOVL	24(DI), CX
	MOVL	20(DI), DX

	/* save registers, segments */
	PUSHL	DS
	PUSHL	ES
	PUSHL	FS
	PUSHL	GS
	PUSHL	BP
	PUSHL	DI

	/*
	 * paranoia: zero the segments, since it's the
	 * BIOS's responsibility to initialize them.
	 * (trick picked up from Linux driver).
	PUSHL	DX
	XORL	DX, DX
	PUSHL	DX
	POPL	DS
	PUSHL	DX
	POPL	ES
	PUSHL	DX
	POPL	FS
	PUSHL	DX
	POPL	GS
	POPL	DX
	 */

	PUSHL	$APMDSEG
	POPL	DS

	/*
	 * The actual call.
	 */
	CSOVERRIDE; BYTE $0xFF; BYTE $0x1D
	LONG $apmjumpstruct+0(SB)

	/* restore segments, registers */
	POPL	DI
	POPL	BP
	POPL	GS
	POPL	FS
	POPL	ES
	POPL	DS

	PUSHFL
	POPL	64(DI)

	/* store interesting registers back in Ureg */
	MOVL	AX, 28(DI)
	MOVL	BX, 16(DI)
	MOVL	CX, 24(DI)
	MOVL	DX, 20(DI)
	MOVL	SI, 4(DI)

	PUSHFL
	POPL	AX
	ANDL	$1, AX	/* carry flag */
	RET
