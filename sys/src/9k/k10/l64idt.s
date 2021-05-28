/*
 * Interrupt/exception handling.
 */
#include "amd64l.h"

MODE $64

TEXT _intrp<>(SB), 1, $-4			/* no error code pushed */
	PUSHQ	AX				/* save AX */
	MOVQ	8(SP), AX			/* idthandlers(SB) PC */
	JMP	_intrcommon

TEXT _intre<>(SB), 1, $-4			/* error code pushed */
	XCHGQ	AX, (SP)
_intrcommon:
	MOVBQZX	(AX), AX
	XCHGQ	AX, (SP)

	SUBQ	$24, SP				/* R1[45], [DEFG]S */
	CMPW	48(SP), $SSEL(SiCS, SsTIGDT|SsRPL0)	/* old CS */
	JEQ	_intrnested

	MOVQ	RUSER, 0(SP)
	MOVQ	RMACH, 8(SP)
	MOVW	DS, 16(SP)
	MOVW	ES, 18(SP)
	MOVW	FS, 20(SP)
	MOVW	GS, 22(SP)

	SWAPGS
	BYTE $0x65; MOVQ 0, RMACH		/* m-> (MOVQ GS:0x0, R15) */
	MOVQ	16(RMACH), RUSER		/* up */

_intrnested:
	PUSHQ	R13
	PUSHQ	R12
	PUSHQ	R11
	PUSHQ	R10
	PUSHQ	R9
	PUSHQ	R8
	PUSHQ	BP
	PUSHQ	DI
	PUSHQ	SI
	PUSHQ	DX
	PUSHQ	CX
	PUSHQ	BX
	PUSHQ	AX

	MOVQ	SP, RARG
	PUSHQ	SP
	CALL	trap(SB)

TEXT _intrr<>(SB), 1, $-4			/* so ktrace can pop frame */
	POPQ	AX

	POPQ	AX
	POPQ	BX
	POPQ	CX
	POPQ	DX
	POPQ	SI
	POPQ	DI
	POPQ	BP
	POPQ	R8
	POPQ	R9
	POPQ	R10
	POPQ	R11
	POPQ	R12
	POPQ	R13

	CMPQ	48(SP), $SSEL(SiCS, SsTIGDT|SsRPL0)
	JEQ	_iretnested

	SWAPGS
	MOVW	22(SP), GS
	MOVW	20(SP), FS
	MOVW	18(SP), ES
	MOVW	16(SP), DS
	MOVQ	8(SP), RMACH
	MOVQ	0(SP), RUSER

_iretnested:
	ADDQ	$40, SP
	IRETQ

TEXT idthandlers(SB), 1, $-4
	CALL _intrp<>(SB); BYTE $IdtDE		/* #DE Divide-by-Zero Error */
	CALL _intrp<>(SB); BYTE $IdtDB		/* #DB Debug */
	CALL _intrp<>(SB); BYTE $IdtNMI		/* #NMI Borked */
	CALL _intrp<>(SB); BYTE $IdtBP		/* #BP Breakpoint */
	CALL _intrp<>(SB); BYTE $IdtOF		/* #OF Overflow */
	CALL _intrp<>(SB); BYTE $IdtBR		/* #BR Bound-Range */
	CALL _intrp<>(SB); BYTE $IdtUD		/* #UD Invalid-Opcode */
	CALL _intrp<>(SB); BYTE $IdtNM		/* #NM Device-Not-Available */
	CALL _intre<>(SB); BYTE $IdtDF		/* #DF Double-Fault */
	CALL _intrp<>(SB); BYTE $Idt09		/* reserved */
	CALL _intre<>(SB); BYTE $IdtTS		/* #TS Invalid-TSS */
	CALL _intre<>(SB); BYTE $IdtNP		/* #NP Segment-Not-Present */
	CALL _intre<>(SB); BYTE $IdtSS		/* #SS Stack */
	CALL _intre<>(SB); BYTE $IdtGP		/* #GP General-Protection */
	CALL _intre<>(SB); BYTE $IdtPF		/* #PF Page-Fault */
	CALL _intrp<>(SB); BYTE $Idt0F		/* reserved */
	CALL _intrp<>(SB); BYTE $IdtMF		/* #MF x87 FPE-Pending */
	CALL _intre<>(SB); BYTE $IdtAC		/* #AC Alignment-Check */
	CALL _intrp<>(SB); BYTE $IdtMC		/* #MC Machine-Check */
	CALL _intrp<>(SB); BYTE $IdtXF		/* #XF SIMD Floating-Point */
	CALL _intrp<>(SB); BYTE $0x14		/* reserved */
	CALL _intrp<>(SB); BYTE $0x15		/* reserved */
	CALL _intrp<>(SB); BYTE $0x16		/* reserved */
	CALL _intrp<>(SB); BYTE $0x17		/* reserved */
	CALL _intrp<>(SB); BYTE $0x18		/* reserved */
	CALL _intrp<>(SB); BYTE $0x19		/* reserved */
	CALL _intrp<>(SB); BYTE $0x1a		/* reserved */
	CALL _intrp<>(SB); BYTE $0x1b		/* reserved */
	CALL _intrp<>(SB); BYTE $0x1c		/* reserved */
	CALL _intrp<>(SB); BYTE $0x1d		/* reserved */
	CALL _intrp<>(SB); BYTE $0x1e		/* reserved */
	CALL _intrp<>(SB); BYTE $0x1f		/* reserved */
	CALL _intrp<>(SB); BYTE $0x20
	CALL _intrp<>(SB); BYTE $0x21
	CALL _intrp<>(SB); BYTE $0x22
	CALL _intrp<>(SB); BYTE $0x23
	CALL _intrp<>(SB); BYTE $0x24
	CALL _intrp<>(SB); BYTE $0x25
	CALL _intrp<>(SB); BYTE $0x26
	CALL _intrp<>(SB); BYTE $0x27
	CALL _intrp<>(SB); BYTE $0x28
	CALL _intrp<>(SB); BYTE $0x29
	CALL _intrp<>(SB); BYTE $0x2a
	CALL _intrp<>(SB); BYTE $0x2b
	CALL _intrp<>(SB); BYTE $0x2c
	CALL _intrp<>(SB); BYTE $0x2d
	CALL _intrp<>(SB); BYTE $0x2e
	CALL _intrp<>(SB); BYTE $0x2f
	CALL _intrp<>(SB); BYTE $0x30
	CALL _intrp<>(SB); BYTE $0x31
	CALL _intrp<>(SB); BYTE $0x32
	CALL _intrp<>(SB); BYTE $0x33
	CALL _intrp<>(SB); BYTE $0x34
	CALL _intrp<>(SB); BYTE $0x35
	CALL _intrp<>(SB); BYTE $0x36
	CALL _intrp<>(SB); BYTE $0x37
	CALL _intrp<>(SB); BYTE $0x38
	CALL _intrp<>(SB); BYTE $0x39
	CALL _intrp<>(SB); BYTE $0x3a
	CALL _intrp<>(SB); BYTE $0x3b
	CALL _intrp<>(SB); BYTE $0x3c
	CALL _intrp<>(SB); BYTE $0x3d
	CALL _intrp<>(SB); BYTE $0x3e
	CALL _intrp<>(SB); BYTE $0x3f
	CALL _intrp<>(SB); BYTE $0x40
	CALL _intrp<>(SB); BYTE $0x41
	CALL _intrp<>(SB); BYTE $0x42
	CALL _intrp<>(SB); BYTE $0x43
	CALL _intrp<>(SB); BYTE $0x44
	CALL _intrp<>(SB); BYTE $0x45
	CALL _intrp<>(SB); BYTE $0x46
	CALL _intrp<>(SB); BYTE $0x47
	CALL _intrp<>(SB); BYTE $0x48
	CALL _intrp<>(SB); BYTE $0x49
	CALL _intrp<>(SB); BYTE $0x4a
	CALL _intrp<>(SB); BYTE $0x4b
	CALL _intrp<>(SB); BYTE $0x4c
	CALL _intrp<>(SB); BYTE $0x4d
	CALL _intrp<>(SB); BYTE $0x4e
	CALL _intrp<>(SB); BYTE $0x4f
	CALL _intrp<>(SB); BYTE $0x50
	CALL _intrp<>(SB); BYTE $0x51
	CALL _intrp<>(SB); BYTE $0x52
	CALL _intrp<>(SB); BYTE $0x53
	CALL _intrp<>(SB); BYTE $0x54
	CALL _intrp<>(SB); BYTE $0x55
	CALL _intrp<>(SB); BYTE $0x56
	CALL _intrp<>(SB); BYTE $0x57
	CALL _intrp<>(SB); BYTE $0x58
	CALL _intrp<>(SB); BYTE $0x59
	CALL _intrp<>(SB); BYTE $0x5a
	CALL _intrp<>(SB); BYTE $0x5b
	CALL _intrp<>(SB); BYTE $0x5c
	CALL _intrp<>(SB); BYTE $0x5d
	CALL _intrp<>(SB); BYTE $0x5e
	CALL _intrp<>(SB); BYTE $0x5f
	CALL _intrp<>(SB); BYTE $0x60
	CALL _intrp<>(SB); BYTE $0x61
	CALL _intrp<>(SB); BYTE $0x62
	CALL _intrp<>(SB); BYTE $0x63
	CALL _intrp<>(SB); BYTE $0x64
	CALL _intrp<>(SB); BYTE $0x65
	CALL _intrp<>(SB); BYTE $0x66
	CALL _intrp<>(SB); BYTE $0x67
	CALL _intrp<>(SB); BYTE $0x68
	CALL _intrp<>(SB); BYTE $0x69
	CALL _intrp<>(SB); BYTE $0x6a
	CALL _intrp<>(SB); BYTE $0x6b
	CALL _intrp<>(SB); BYTE $0x6c
	CALL _intrp<>(SB); BYTE $0x6d
	CALL _intrp<>(SB); BYTE $0x6e
	CALL _intrp<>(SB); BYTE $0x6f
	CALL _intrp<>(SB); BYTE $0x70
	CALL _intrp<>(SB); BYTE $0x71
	CALL _intrp<>(SB); BYTE $0x72
	CALL _intrp<>(SB); BYTE $0x73
	CALL _intrp<>(SB); BYTE $0x74
	CALL _intrp<>(SB); BYTE $0x75
	CALL _intrp<>(SB); BYTE $0x76
	CALL _intrp<>(SB); BYTE $0x77
	CALL _intrp<>(SB); BYTE $0x78
	CALL _intrp<>(SB); BYTE $0x79
	CALL _intrp<>(SB); BYTE $0x7a
	CALL _intrp<>(SB); BYTE $0x7b
	CALL _intrp<>(SB); BYTE $0x7c
	CALL _intrp<>(SB); BYTE $0x7d
	CALL _intrp<>(SB); BYTE $0x7e
	CALL _intrp<>(SB); BYTE $0x7f
	CALL _intrp<>(SB); BYTE $0x80
	CALL _intrp<>(SB); BYTE $0x81
	CALL _intrp<>(SB); BYTE $0x82
	CALL _intrp<>(SB); BYTE $0x83
	CALL _intrp<>(SB); BYTE $0x84
	CALL _intrp<>(SB); BYTE $0x85
	CALL _intrp<>(SB); BYTE $0x86
	CALL _intrp<>(SB); BYTE $0x87
	CALL _intrp<>(SB); BYTE $0x88
	CALL _intrp<>(SB); BYTE $0x89
	CALL _intrp<>(SB); BYTE $0x8a
	CALL _intrp<>(SB); BYTE $0x8b
	CALL _intrp<>(SB); BYTE $0x8c
	CALL _intrp<>(SB); BYTE $0x8d
	CALL _intrp<>(SB); BYTE $0x8e
	CALL _intrp<>(SB); BYTE $0x8f
	CALL _intrp<>(SB); BYTE $0x90
	CALL _intrp<>(SB); BYTE $0x91
	CALL _intrp<>(SB); BYTE $0x92
	CALL _intrp<>(SB); BYTE $0x93
	CALL _intrp<>(SB); BYTE $0x94
	CALL _intrp<>(SB); BYTE $0x95
	CALL _intrp<>(SB); BYTE $0x96
	CALL _intrp<>(SB); BYTE $0x97
	CALL _intrp<>(SB); BYTE $0x98
	CALL _intrp<>(SB); BYTE $0x99
	CALL _intrp<>(SB); BYTE $0x9a
	CALL _intrp<>(SB); BYTE $0x9b
	CALL _intrp<>(SB); BYTE $0x9c
	CALL _intrp<>(SB); BYTE $0x9d
	CALL _intrp<>(SB); BYTE $0x9e
	CALL _intrp<>(SB); BYTE $0x9f
	CALL _intrp<>(SB); BYTE $0xa0
	CALL _intrp<>(SB); BYTE $0xa1
	CALL _intrp<>(SB); BYTE $0xa2
	CALL _intrp<>(SB); BYTE $0xa3
	CALL _intrp<>(SB); BYTE $0xa4
	CALL _intrp<>(SB); BYTE $0xa5
	CALL _intrp<>(SB); BYTE $0xa6
	CALL _intrp<>(SB); BYTE $0xa7
	CALL _intrp<>(SB); BYTE $0xa8
	CALL _intrp<>(SB); BYTE $0xa9
	CALL _intrp<>(SB); BYTE $0xaa
	CALL _intrp<>(SB); BYTE $0xab
	CALL _intrp<>(SB); BYTE $0xac
	CALL _intrp<>(SB); BYTE $0xad
	CALL _intrp<>(SB); BYTE $0xae
	CALL _intrp<>(SB); BYTE $0xaf
	CALL _intrp<>(SB); BYTE $0xb0
	CALL _intrp<>(SB); BYTE $0xb1
	CALL _intrp<>(SB); BYTE $0xb2
	CALL _intrp<>(SB); BYTE $0xb3
	CALL _intrp<>(SB); BYTE $0xb4
	CALL _intrp<>(SB); BYTE $0xb5
	CALL _intrp<>(SB); BYTE $0xb6
	CALL _intrp<>(SB); BYTE $0xb7
	CALL _intrp<>(SB); BYTE $0xb8
	CALL _intrp<>(SB); BYTE $0xb9
	CALL _intrp<>(SB); BYTE $0xba
	CALL _intrp<>(SB); BYTE $0xbb
	CALL _intrp<>(SB); BYTE $0xbc
	CALL _intrp<>(SB); BYTE $0xbd
	CALL _intrp<>(SB); BYTE $0xbe
	CALL _intrp<>(SB); BYTE $0xbf
	CALL _intrp<>(SB); BYTE $0xc0
	CALL _intrp<>(SB); BYTE $0xc1
	CALL _intrp<>(SB); BYTE $0xc2
	CALL _intrp<>(SB); BYTE $0xc3
	CALL _intrp<>(SB); BYTE $0xc4
	CALL _intrp<>(SB); BYTE $0xc5
	CALL _intrp<>(SB); BYTE $0xc6
	CALL _intrp<>(SB); BYTE $0xc7
	CALL _intrp<>(SB); BYTE $0xc8
	CALL _intrp<>(SB); BYTE $0xc9
	CALL _intrp<>(SB); BYTE $0xca
	CALL _intrp<>(SB); BYTE $0xcb
	CALL _intrp<>(SB); BYTE $0xcc
	CALL _intrp<>(SB); BYTE $0xce
	CALL _intrp<>(SB); BYTE $0xce
	CALL _intrp<>(SB); BYTE $0xcf
	CALL _intrp<>(SB); BYTE $0xd0
	CALL _intrp<>(SB); BYTE $0xd1
	CALL _intrp<>(SB); BYTE $0xd2
	CALL _intrp<>(SB); BYTE $0xd3
	CALL _intrp<>(SB); BYTE $0xd4
	CALL _intrp<>(SB); BYTE $0xd5
	CALL _intrp<>(SB); BYTE $0xd6
	CALL _intrp<>(SB); BYTE $0xd7
	CALL _intrp<>(SB); BYTE $0xd8
	CALL _intrp<>(SB); BYTE $0xd9
	CALL _intrp<>(SB); BYTE $0xda
	CALL _intrp<>(SB); BYTE $0xdb
	CALL _intrp<>(SB); BYTE $0xdc
	CALL _intrp<>(SB); BYTE $0xdd
	CALL _intrp<>(SB); BYTE $0xde
	CALL _intrp<>(SB); BYTE $0xdf
	CALL _intrp<>(SB); BYTE $0xe0
	CALL _intrp<>(SB); BYTE $0xe1
	CALL _intrp<>(SB); BYTE $0xe2
	CALL _intrp<>(SB); BYTE $0xe3
	CALL _intrp<>(SB); BYTE $0xe4
	CALL _intrp<>(SB); BYTE $0xe5
	CALL _intrp<>(SB); BYTE $0xe6
	CALL _intrp<>(SB); BYTE $0xe7
	CALL _intrp<>(SB); BYTE $0xe8
	CALL _intrp<>(SB); BYTE $0xe9
	CALL _intrp<>(SB); BYTE $0xea
	CALL _intrp<>(SB); BYTE $0xeb
	CALL _intrp<>(SB); BYTE $0xec
	CALL _intrp<>(SB); BYTE $0xed
	CALL _intrp<>(SB); BYTE $0xee
	CALL _intrp<>(SB); BYTE $0xef
	CALL _intrp<>(SB); BYTE $0xf0
	CALL _intrp<>(SB); BYTE $0xf1
	CALL _intrp<>(SB); BYTE $0xf2
	CALL _intrp<>(SB); BYTE $0xf3
	CALL _intrp<>(SB); BYTE $0xf4
	CALL _intrp<>(SB); BYTE $0xf5
	CALL _intrp<>(SB); BYTE $0xf6
	CALL _intrp<>(SB); BYTE $0xf7
	CALL _intrp<>(SB); BYTE $0xf8
	CALL _intrp<>(SB); BYTE $0xf9
	CALL _intrp<>(SB); BYTE $0xfa
	CALL _intrp<>(SB); BYTE $0xfb
	CALL _intrp<>(SB); BYTE $0xfc
	CALL _intrp<>(SB); BYTE $0xfd
	CALL _intrp<>(SB); BYTE $0xfe
	CALL _intrp<>(SB); BYTE $0xff
