/*
 * Interrupt/exception handling.
 */
#include "amd64l.h"

.code64

// _intrp and _intre make the stack look the same to the C code and intrcommon. 
.globl _intrp
_intrp:
	PUSHQ	%rax				/* save AX */
	MOVQ	8(%rsp), %rax			/* idthandlers(SB) PC */
	JMP	_intrcommon

.globl _intre
_intre:
	XCHGQ	%rax, (%rsp)
_intrcommon:
	//MOVBQZX	(%rax), %rAX
	MOVL	%eax, %eax
	XCHGQ	%rax, (%rsp)

	SUBQ	$24, %rsp				/* R1[45], [DEFG]S */
	CMPW	$SSEL(SiCS, SsTIGDT|SsRPL0), 48(%rsp) 	/* old CS */
	JE	_intrnested

#if 0 // skip this stuff -- not needed?
	MOVQ	RUSER, 0(SP)
	MOVQ	RMACH, 8(SP)
	MOVW	DS, 16(SP)
	MOVW	ES, 18(SP)
	MOVW	FS, 20(SP)
	MOVW	GS, 22(SP)
#endif

	SWAPGS
#if 0
	// no longer used.
	BYTE $0x65; MOVQ 0, RMACH		/* m-> (MOVQ GS:0x0, R15) */
	MOVQ	16(RMACH), RUSER		/* up */
#endif

_intrnested:
	PUSHQ	%r13
	PUSHQ	%r12
	PUSHQ	%r11
	PUSHQ	%r10
	PUSHQ	%r9
	PUSHQ	%r8
	PUSHQ	%rBP
	PUSHQ	%rDI
	PUSHQ	%rSI
	PUSHQ	%rDX
	PUSHQ	%rCX
	PUSHQ	%rbx
	PUSHQ	%rax

	MOVQ	%rsp, %rdi
	PUSHQ	%rSP
	CALL	_trap

.globl _intrr
_intrr:
	POPQ	%rax

	POPQ	%rax
	POPQ	%rbx
	POPQ	%rCX
	POPQ	%rDX
	POPQ	%rSI
	POPQ	%rDI
	POPQ	%rBP
	POPQ	%r8
	POPQ	%r9
	POPQ	%r10
	POPQ	%r11
	POPQ	%r12
	POPQ	%r13

	CMPW	$SSEL(SiCS, SsTIGDT|SsRPL0), 48(%rsp) 	/* old CS */
	JE	_iretnested

	SWAPGS
	/*	per the architecture manual, moving 16 bits to FS can zero it. Bad ... 
	 *	not restoring it gives back the bad segment selector bug
	MOVW	22(SP), GS
	MOVW	20(SP), FS
	MOVW	18(SP), ES
	MOVW	16(SP), DS
	MOVQ	8(SP), RMACH
	MOVQ	0(SP), RUSER
	 */

_iretnested:
	ADDQ	$40, %rsp
	retq

.globl idthandlers
idthandlers:
	call _intrp; .byte IdtDE		/* #DE Divide-by-Zero Error */
	call _intrp; .byte IdtDB		/* #DB Debug */
	call _intrp; .byte IdtNMI		/* #NMI Borked */
	call _intrp; .byte IdtBP		/* #BP Breakpoint */
	call _intrp; .byte IdtOF		/* #OF Overflow */
	call _intrp; .byte IdtBR		/* #BR Bound-Range */
	call _intrp; .byte IdtUD		/* #UD Invalid-Opcode */
	call _intrp; .byte IdtNM		/* #NM Device-Not-Available */
	call _intre; .byte IdtDF		/* #DF Double-Fault */
	call _intrp; .byte Idt09		/* reserved */
	call _intre; .byte IdtTS		/* #TS Invalid-TSS */
	call _intre; .byte IdtNP		/* #NP Segment-Not-Present */
	call _intre; .byte IdtSS		/* #SS Stack */
	call _intre; .byte IdtGP		/* #GP General-Protection */
	call _intre; .byte IdtPF		/* #PF Page-Fault */
	call _intrp; .byte Idt0F		/* reserved */
	call _intrp; .byte IdtMF		/* #MF x87 FPE-Pending */
	call _intre; .byte IdtAC		/* #AC Alignment-Check */
	call _intrp; .byte IdtMC		/* #MC Machine-Check */
	call _intrp; .byte IdtXF		/* #XF SIMD Floating-Point */
	call _intrp; .byte 0x14		/* reserved */
	call _intrp; .byte 0x15		/* reserved */
	call _intrp; .byte 0x16		/* reserved */
	call _intrp; .byte 0x17		/* reserved */
	call _intrp; .byte 0x18		/* reserved */
	call _intrp; .byte 0x19		/* reserved */
	call _intrp; .byte 0x1a		/* reserved */
	call _intrp; .byte 0x1b		/* reserved */
	call _intrp; .byte 0x1c		/* reserved */
	call _intrp; .byte 0x1d		/* reserved */
	call _intrp; .byte 0x1e		/* reserved */
	call _intrp; .byte 0x1f		/* reserved */
	call _intrp; .byte 0x20
	call _intrp; .byte 0x21
	call _intrp; .byte 0x22
	call _intrp; .byte 0x23
	call _intrp; .byte 0x24
	call _intrp; .byte 0x25
	call _intrp; .byte 0x26
	call _intrp; .byte 0x27
	call _intrp; .byte 0x28
	call _intrp; .byte 0x29
	call _intrp; .byte 0x2a
	call _intrp; .byte 0x2b
	call _intrp; .byte 0x2c
	call _intrp; .byte 0x2d
	call _intrp; .byte 0x2e
	call _intrp; .byte 0x2f
	call _intrp; .byte 0x30
	call _intrp; .byte 0x31
	call _intrp; .byte 0x32
	call _intrp; .byte 0x33
	call _intrp; .byte 0x34
	call _intrp; .byte 0x35
	call _intrp; .byte 0x36
	call _intrp; .byte 0x37
	call _intrp; .byte 0x38
	call _intrp; .byte 0x39
	call _intrp; .byte 0x3a
	call _intrp; .byte 0x3b
	call _intrp; .byte 0x3c
	call _intrp; .byte 0x3d
	call _intrp; .byte 0x3e
	call _intrp; .byte 0x3f
	call _intrp; .byte 0x40
	call _intrp; .byte 0x41
	call _intrp; .byte 0x42
	call _intrp; .byte 0x43
	call _intrp; .byte 0x44
	call _intrp; .byte 0x45
	call _intrp; .byte 0x46
	call _intrp; .byte 0x47
	call _intrp; .byte 0x48
	call _intrp; .byte 0x49
	call _intrp; .byte 0x4a
	call _intrp; .byte 0x4b
	call _intrp; .byte 0x4c
	call _intrp; .byte 0x4d
	call _intrp; .byte 0x4e
	call _intrp; .byte 0x4f
	call _intrp; .byte 0x50
	call _intrp; .byte 0x51
	call _intrp; .byte 0x52
	call _intrp; .byte 0x53
	call _intrp; .byte 0x54
	call _intrp; .byte 0x55
	call _intrp; .byte 0x56
	call _intrp; .byte 0x57
	call _intrp; .byte 0x58
	call _intrp; .byte 0x59
	call _intrp; .byte 0x5a
	call _intrp; .byte 0x5b
	call _intrp; .byte 0x5c
	call _intrp; .byte 0x5d
	call _intrp; .byte 0x5e
	call _intrp; .byte 0x5f
	call _intrp; .byte 0x60
	call _intrp; .byte 0x61
	call _intrp; .byte 0x62
	call _intrp; .byte 0x63
	call _intrp; .byte 0x64
	call _intrp; .byte 0x65
	call _intrp; .byte 0x66
	call _intrp; .byte 0x67
	call _intrp; .byte 0x68
	call _intrp; .byte 0x69
	call _intrp; .byte 0x6a
	call _intrp; .byte 0x6b
	call _intrp; .byte 0x6c
	call _intrp; .byte 0x6d
	call _intrp; .byte 0x6e
	call _intrp; .byte 0x6f
	call _intrp; .byte 0x70
	call _intrp; .byte 0x71
	call _intrp; .byte 0x72
	call _intrp; .byte 0x73
	call _intrp; .byte 0x74
	call _intrp; .byte 0x75
	call _intrp; .byte 0x76
	call _intrp; .byte 0x77
	call _intrp; .byte 0x78
	call _intrp; .byte 0x79
	call _intrp; .byte 0x7a
	call _intrp; .byte 0x7b
	call _intrp; .byte 0x7c
	call _intrp; .byte 0x7d
	call _intrp; .byte 0x7e
	call _intrp; .byte 0x7f
	call _intrp; .byte 0x80
	call _intrp; .byte 0x81
	call _intrp; .byte 0x82
	call _intrp; .byte 0x83
	call _intrp; .byte 0x84
	call _intrp; .byte 0x85
	call _intrp; .byte 0x86
	call _intrp; .byte 0x87
	call _intrp; .byte 0x88
	call _intrp; .byte 0x89
	call _intrp; .byte 0x8a
	call _intrp; .byte 0x8b
	call _intrp; .byte 0x8c
	call _intrp; .byte 0x8d
	call _intrp; .byte 0x8e
	call _intrp; .byte 0x8f
	call _intrp; .byte 0x90
	call _intrp; .byte 0x91
	call _intrp; .byte 0x92
	call _intrp; .byte 0x93
	call _intrp; .byte 0x94
	call _intrp; .byte 0x95
	call _intrp; .byte 0x96
	call _intrp; .byte 0x97
	call _intrp; .byte 0x98
	call _intrp; .byte 0x99
	call _intrp; .byte 0x9a
	call _intrp; .byte 0x9b
	call _intrp; .byte 0x9c
	call _intrp; .byte 0x9d
	call _intrp; .byte 0x9e
	call _intrp; .byte 0x9f
	call _intrp; .byte 0xa0
	call _intrp; .byte 0xa1
	call _intrp; .byte 0xa2
	call _intrp; .byte 0xa3
	call _intrp; .byte 0xa4
	call _intrp; .byte 0xa5
	call _intrp; .byte 0xa6
	call _intrp; .byte 0xa7
	call _intrp; .byte 0xa8
	call _intrp; .byte 0xa9
	call _intrp; .byte 0xaa
	call _intrp; .byte 0xab
	call _intrp; .byte 0xac
	call _intrp; .byte 0xad
	call _intrp; .byte 0xae
	call _intrp; .byte 0xaf
	call _intrp; .byte 0xb0
	call _intrp; .byte 0xb1
	call _intrp; .byte 0xb2
	call _intrp; .byte 0xb3
	call _intrp; .byte 0xb4
	call _intrp; .byte 0xb5
	call _intrp; .byte 0xb6
	call _intrp; .byte 0xb7
	call _intrp; .byte 0xb8
	call _intrp; .byte 0xb9
	call _intrp; .byte 0xba
	call _intrp; .byte 0xbb
	call _intrp; .byte 0xbc
	call _intrp; .byte 0xbd
	call _intrp; .byte 0xbe
	call _intrp; .byte 0xbf
	call _intrp; .byte 0xc0
	call _intrp; .byte 0xc1
	call _intrp; .byte 0xc2
	call _intrp; .byte 0xc3
	call _intrp; .byte 0xc4
	call _intrp; .byte 0xc5
	call _intrp; .byte 0xc6
	call _intrp; .byte 0xc7
	call _intrp; .byte 0xc8
	call _intrp; .byte 0xc9
	call _intrp; .byte 0xca
	call _intrp; .byte 0xcb
	call _intrp; .byte 0xcc
	call _intrp; .byte 0xce
	call _intrp; .byte 0xce
	call _intrp; .byte 0xcf
	call _intrp; .byte 0xd0
	call _intrp; .byte 0xd1
	call _intrp; .byte 0xd2
	call _intrp; .byte 0xd3
	call _intrp; .byte 0xd4
	call _intrp; .byte 0xd5
	call _intrp; .byte 0xd6
	call _intrp; .byte 0xd7
	call _intrp; .byte 0xd8
	call _intrp; .byte 0xd9
	call _intrp; .byte 0xda
	call _intrp; .byte 0xdb
	call _intrp; .byte 0xdc
	call _intrp; .byte 0xdd
	call _intrp; .byte 0xde
	call _intrp; .byte 0xdf
	call _intrp; .byte 0xe0
	call _intrp; .byte 0xe1
	call _intrp; .byte 0xe2
	call _intrp; .byte 0xe3
	call _intrp; .byte 0xe4
	call _intrp; .byte 0xe5
	call _intrp; .byte 0xe6
	call _intrp; .byte 0xe7
	call _intrp; .byte 0xe8
	call _intrp; .byte 0xe9
	call _intrp; .byte 0xea
	call _intrp; .byte 0xeb
	call _intrp; .byte 0xec
	call _intrp; .byte 0xed
	call _intrp; .byte 0xee
	call _intrp; .byte 0xef
	call _intrp; .byte 0xf0
	call _intrp; .byte 0xf1
	call _intrp; .byte 0xf2
	call _intrp; .byte 0xf3
	call _intrp; .byte 0xf4
	call _intrp; .byte 0xf5
	call _intrp; .byte 0xf6
	call _intrp; .byte 0xf7
	call _intrp; .byte 0xf8
	call _intrp; .byte 0xf9
	call _intrp; .byte 0xfa
	call _intrp; .byte 0xfb
	call _intrp; .byte 0xfc
	call _intrp; .byte 0xfd
	call _intrp; .byte 0xfe
	call _intrp; .byte 0xff
