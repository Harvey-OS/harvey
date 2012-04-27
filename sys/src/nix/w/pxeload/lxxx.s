#include "386l.h"

#define KZERO		(0)

#define	DELAY		BYTE $0xeb;		/* JMP .+2 */		\
			BYTE $0x00
#define NOP		BYTE $0x90		/* NOP */

#define pFARJMP32(s, o)	BYTE $0xea;		/* far jmp ptr32:16 */	\
			LONG $o; WORD $s

#define rFARJMP16(s, o)	BYTE $0xea;		/* far jump ptr16:16 */	\
			WORD $o; WORD $s;
#define rFARJMP32(s, o)	BYTE $0x66;		/* far jump ptr32:16 */	\
			pFARJMP32(s, o)
#define rLGDT(gdtptr)	BYTE $0x0f;		/* LGDT */		\
			BYTE $0x01; BYTE $0x16;				\
			WORD $gdtptr

#define rAX		0			/* rX  */
#define rCX		1
#define rDX		2
#define rBX		3
#define rSP		4			/* SP */
#define rBP		5			/* BP */
#define rSI		6			/* SI */
#define rDI		7			/* DI */

#define rMOV16(i, rX)	BYTE $(0xB8+rX);	/* i -> rX */		\
			WORD $i;
#define rMOV32(i, rX)	BYTE $0x66;		/* i -> rX */		\
			BYTE $(0xB8+rX);				\
			LONG $i;

/*
 * Real mode. Welcome to 1978.
 */
TEXT _real<>(SB), 1, $-4
	rFARJMP16(0, _endofheader<>-KZERO(SB))	/*  */
	NOP; NOP; NOP				/* align */

_startofheader:
	WORD $0; WORD $0			/* flags */

TEXT _gdt32p<>(SB), 1, $-4
	LONG	$0x0000; LONG $0		/* NULL descriptor */
	LONG	$0xffff; LONG $0x00cf9a00	/* CS */
	LONG	$0xffff; LONG $0x00cf9200	/* DS */

TEXT _gdtptr32p<>(SB), 1, $-4
	WORD	$(3*8-1)
	LONG	$_gdt32p<>-KZERO(SB)

TEXT _hello<>(SB), $0
	BYTE $'H'; BYTE $'e'; BYTE $'l'; BYTE $'l';
	BYTE $'o'; BYTE $' '; BYTE $'f'; BYTE $'r';
	BYTE $'o'; BYTE $'m'; BYTE $' '; BYTE $'1';
	BYTE $'9'; BYTE $'7'; BYTE $'8'; BYTE $'.';
	BYTE $'\r';
	BYTE $'\n';
	BYTE $'\z';
	BYTE $'\z';

TEXT _endofheader<>(SB), 1, $-4
	CLI
	CLD

	XORL	AX, AX
	MOVW	AX, DS				/* initialise DS */
	MOVW	AX, ES				/* initialise ES */
	MOVW	AX, SS				/* initialise SS */

	rMOV16(_real<>-KZERO(SB), rSP)		/* stack */
	PUSHL	AX				/* CS segment */
	rMOV16(_setCS<>-KZERO(SB), rAX)		/* address within segment */
	PUSHL	AX
	BYTE	$0xcb				/* far return */

TEXT _setCS<>(SB), 1, $-4
	MOVB	$0x0f, AH			/* get current display mode */
	INT	$0x10

	SUBB	$3, AL				/* is it text mode 3? */
	JEQ	_cgaputs

	MOVB	$0x0f, AH			/* text mode 3 */
	MOVB	$0x03, AL
	INT	$0x10

_cgaputs:
	rMOV16(_hello<>-KZERO(SB), rAX)		/* a cheery wee message */
	MOVL	AX, SI

	XORL	BX, BX				/* page number, colour */
_cgaputsloop:
	LODSB					/* next character in string */
	ORB	AL, AL				/* \0? */
	JEQ	_cgaend

	MOVB	$0x0e, AH			/* teletype output */
	INT	$0x10
	JMP	_cgaputsloop
_cgaend:

	rMOV16(_real<>+(8-KZERO)(SB), rSI)	/* flag */
	LODSW
	ORL	AX, AX
	JNE	_e820

	INCL	AX
	rMOV16(_real<>+(8-KZERO)(SB), rDI)	/* flag */
	STOSW

	rMOV16(0x0467, rDI)			/* reset entry point */
	rMOV16(_endofheader<>-KZERO(SB), rAX)
	STOSW
	MOVW	CS, AX
	STOSW

	rMOV16(0x70, rDX)
	MOVB	$0x8f, AL
	OUTB
	DELAY
	INCL	DX
	MOVB	$0x0a, AL
	OUTB
	DELAY

rMOV16(0x1234, rBP)
rMOV16(0x5678, rDX)
	rFARJMP16(0xffff, 0)			/* reset */

_e820:
	rMOV16(_real<>-(KZERO+1024)(SB), rDI)	/* e820 save area */

//	XORL	AX, AX				/* nothing to see here (yet) */
//	STOSL
//	STOSL

	XORL	BX, BX				/* continuation */
_e820int:
	rMOV32(40, rCX)				/* buffer size */
	rMOV32(0x534d4150, rDX)			/* signature - ASCII "SMAP" */
	rMOV32(0xe820, rAX)			/* function code */

	INT	$0x15				/* query system address map */
	JCS	_e820end

	INCL	AX
_e820end:
	STOSL
	MOVL	CX, AX
	STOSL
//	MOVL	BX, AX
//	STOSL
	rMOV16(0xaabb, rAX)
	STOSL
	MOVW	CS, AX
	STOSL

	rLGDT(_gdtptr32p<>-KZERO(SB))		/* load a basic gdt */

	MOVL	CR0, AX
	ORL	$Pe, AX
	MOVL	AX, CR0				/* turn on protected mode */
	DELAY					/* JMP .+2 */

	rMOV16(SSEL(SiDS, SsTIGDT|SsRPL0), rAX)	/*  */
	MOVW	AX, DS
	MOVW	AX, ES
	MOVW	AX, FS
	MOVW	AX, GS
	MOVW	AX, SS

	rFARJMP32(SSEL(SiCS, SsTIGDT|SsRPL0), _start32p-KZERO(SB))

#ifdef standalone
/*
 * Protected mode. Welcome to 1982.
 */
TEXT _start32p(SB), 1, $-4
_ndnr:
	JMP	_ndnr
	RET
#endif /* standalone */
