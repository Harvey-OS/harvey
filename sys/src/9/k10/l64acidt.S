/*
 * Interrupt/exception handling.
 */
#include "amd64.h"

.code64

.global _acintrp
_acintrp:
	pushq	%rax // bogus error code. Makes stack look like intre.
	// Except in this case, bogus error code is at 0(%rsp)
	// vno pointer is at 8(%rsp)
	MOVQ	8(%rsp), %rax
	// Now %rax points to the vector number.
	JMP	_acintrcommon

.globl _acintre
_acintre:
///jmp	_intre
	// The error code has been pushed. How I love the x86.
	// So the error code is ABOVE the pointer to the vector #.
	XCHGQ	%rax, (%rsp)

_acintrcommon:
		// Get the vector number into %al
	MOVb	(%rax), %al
	ANDQ	$0xff, %rax
	// Put that at TOS (this is plan 9 argument style)
	XCHGQ	%rax, 0(%rsp)
	// 0(%rsp) now has the vno
	CMPW	$SSEL(SiCS, SsTIGDT|SsRPL0), 24(%rsp) 	/* old CS */
	JE	_acintrnested

	SWAPGS

_acintrnested:
	PUSHQ	%r15
	PUSHQ	%r14
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

	MOVQ	%rsp, %rdi // it's ok, we saved %rdi.
	CALL	_trap
.globl _acintrr
_acintrr:
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
	POPQ	%r14
	POPQ	%r15

	CMPW	$SSEL(SiCS, SsTIGDT|SsRPL0), 24(%rsp) 	/* old CS */
	JE	_aciretnested

	SWAPGS

_aciretnested:
	// Throw away:
	// The %rax you pushed (error code)
	// EIP from the vector table.
	ADDQ	$16, %rsp
	iretq

.globl acidthandlers
acidthandlers:	
	CALL _acintrp; .byte IdtDE		/* #DE Divide-by-Zero Error */
	CALL _acintrp; .byte IdtDB		/* #DB Debug */
	CALL _acintrp; .byte IdtNMI		/* #NMI Borked */
	CALL _acintrp; .byte IdtBP		/* #BP Breakpoint */
	CALL _acintrp; .byte IdtOF		/* #OF Overflow */
	CALL _acintrp; .byte IdtBR		/* #BR Bound-Range */
	CALL _acintrp; .byte IdtUD		/* #UD Invalid-Opcode */
	CALL _acintrp; .byte IdtNM		/* #NM Device-Not-Available */
	CALL _acintre; .byte IdtDF		/* #DF Double-Fault */
	CALL _acintrp; .byte Idt09		/* reserved */
	CALL _acintre; .byte IdtTS		/* #TS Invalid-TSS */
	CALL _acintre; .byte IdtNP		/* #NP Segment-Not-Present */
	CALL _acintre; .byte IdtSS		/* #SS Stack */
	CALL _acintre; .byte IdtGP		/* #GP General-Protection */
	CALL _acintre; .byte IdtPF		/* #PF Page-Fault */
	CALL _acintrp; .byte Idt0F		/* reserved */
	CALL _acintrp; .byte IdtMF		/* #MF x87 FPE-Pending */
	CALL _acintre; .byte IdtAC		/* #AC Alignment-Check */
	CALL _acintrp; .byte IdtMC		/* #MC Machine-Check */
	CALL _acintrp; .byte IdtXF		/* #XF SIMD Floating-Point */
	CALL _acintrp; .byte 0x14		/* reserved */
	CALL _acintrp; .byte 0x15		/* reserved */
	CALL _acintrp; .byte 0x16		/* reserved */
	CALL _acintrp; .byte 0x17		/* reserved */
	CALL _acintrp; .byte 0x18		/* reserved */
	CALL _acintrp; .byte 0x19		/* reserved */
	CALL _acintrp; .byte 0x1a		/* reserved */
	CALL _acintrp; .byte 0x1b		/* reserved */
	CALL _acintrp; .byte 0x1c		/* reserved */
	CALL _acintrp; .byte 0x1d		/* reserved */
	CALL _acintrp; .byte 0x1e		/* reserved */
	CALL _acintrp; .byte 0x1f		/* reserved */
	CALL _acintrp; .byte 0x20
	CALL _acintrp; .byte 0x21
	CALL _acintrp; .byte 0x22
	CALL _acintrp; .byte 0x23
	CALL _acintrp; .byte 0x24
	CALL _acintrp; .byte 0x25
	CALL _acintrp; .byte 0x26
	CALL _acintrp; .byte 0x27
	CALL _acintrp; .byte 0x28
	CALL _acintrp; .byte 0x29
	CALL _acintrp; .byte 0x2a
	CALL _acintrp; .byte 0x2b
	CALL _acintrp; .byte 0x2c
	CALL _acintrp; .byte 0x2d
	CALL _acintrp; .byte 0x2e
	CALL _acintrp; .byte 0x2f
	CALL _acintrp; .byte 0x30
	CALL _acintrp; .byte 0x31
	CALL _acintrp; .byte 0x32
	CALL _acintrp; .byte 0x33
	CALL _acintrp; .byte 0x34
	CALL _acintrp; .byte 0x35
	CALL _acintrp; .byte 0x36
	CALL _acintrp; .byte 0x37
	CALL _acintrp; .byte 0x38
	CALL _acintrp; .byte 0x39
	CALL _acintrp; .byte 0x3a
	CALL _acintrp; .byte 0x3b
	CALL _acintrp; .byte 0x3c
	CALL _acintrp; .byte 0x3d
	CALL _acintrp; .byte 0x3e
	CALL _acintrp; .byte 0x3f
	CALL _acintrp; .byte 0x40
	CALL _acintrp; .byte 0x41
	CALL _acintrp; .byte 0x42
	CALL _acintrp; .byte 0x43
	CALL _acintrp; .byte 0x44
	CALL _acintrp; .byte 0x45
	CALL _acintrp; .byte 0x46
	CALL _acintrp; .byte 0x47
	CALL _acintrp; .byte 0x48
	CALL _acintrp; .byte 0x49
	CALL _acintrp; .byte 0x4a
	CALL _acintrp; .byte 0x4b
	CALL _acintrp; .byte 0x4c
	CALL _acintrp; .byte 0x4d
	CALL _acintrp; .byte 0x4e
	CALL _acintrp; .byte 0x4f
	CALL _acintrp; .byte 0x50
	CALL _acintrp; .byte 0x51
	CALL _acintrp; .byte 0x52
	CALL _acintrp; .byte 0x53
	CALL _acintrp; .byte 0x54
	CALL _acintrp; .byte 0x55
	CALL _acintrp; .byte 0x56
	CALL _acintrp; .byte 0x57
	CALL _acintrp; .byte 0x58
	CALL _acintrp; .byte 0x59
	CALL _acintrp; .byte 0x5a
	CALL _acintrp; .byte 0x5b
	CALL _acintrp; .byte 0x5c
	CALL _acintrp; .byte 0x5d
	CALL _acintrp; .byte 0x5e
	CALL _acintrp; .byte 0x5f
	CALL _acintrp; .byte 0x60
	CALL _acintrp; .byte 0x61
	CALL _acintrp; .byte 0x62
	CALL _acintrp; .byte 0x63
	CALL _acintrp; .byte 0x64
	CALL _acintrp; .byte 0x65
	CALL _acintrp; .byte 0x66
	CALL _acintrp; .byte 0x67
	CALL _acintrp; .byte 0x68
	CALL _acintrp; .byte 0x69
	CALL _acintrp; .byte 0x6a
	CALL _acintrp; .byte 0x6b
	CALL _acintrp; .byte 0x6c
	CALL _acintrp; .byte 0x6d
	CALL _acintrp; .byte 0x6e
	CALL _acintrp; .byte 0x6f
	CALL _acintrp; .byte 0x70
	CALL _acintrp; .byte 0x71
	CALL _acintrp; .byte 0x72
	CALL _acintrp; .byte 0x73
	CALL _acintrp; .byte 0x74
	CALL _acintrp; .byte 0x75
	CALL _acintrp; .byte 0x76
	CALL _acintrp; .byte 0x77
	CALL _acintrp; .byte 0x78
	CALL _acintrp; .byte 0x79
	CALL _acintrp; .byte 0x7a
	CALL _acintrp; .byte 0x7b
	CALL _acintrp; .byte 0x7c
	CALL _acintrp; .byte 0x7d
	CALL _acintrp; .byte 0x7e
	CALL _acintrp; .byte 0x7f
	CALL _acintrp; .byte 0x80
	CALL _acintrp; .byte 0x81
	CALL _acintrp; .byte 0x82
	CALL _acintrp; .byte 0x83
	CALL _acintrp; .byte 0x84
	CALL _acintrp; .byte 0x85
	CALL _acintrp; .byte 0x86
	CALL _acintrp; .byte 0x87
	CALL _acintrp; .byte 0x88
	CALL _acintrp; .byte 0x89
	CALL _acintrp; .byte 0x8a
	CALL _acintrp; .byte 0x8b
	CALL _acintrp; .byte 0x8c
	CALL _acintrp; .byte 0x8d
	CALL _acintrp; .byte 0x8e
	CALL _acintrp; .byte 0x8f
	CALL _acintrp; .byte 0x90
	CALL _acintrp; .byte 0x91
	CALL _acintrp; .byte 0x92
	CALL _acintrp; .byte 0x93
	CALL _acintrp; .byte 0x94
	CALL _acintrp; .byte 0x95
	CALL _acintrp; .byte 0x96
	CALL _acintrp; .byte 0x97
	CALL _acintrp; .byte 0x98
	CALL _acintrp; .byte 0x99
	CALL _acintrp; .byte 0x9a
	CALL _acintrp; .byte 0x9b
	CALL _acintrp; .byte 0x9c
	CALL _acintrp; .byte 0x9d
	CALL _acintrp; .byte 0x9e
	CALL _acintrp; .byte 0x9f
	CALL _acintrp; .byte 0xa0
	CALL _acintrp; .byte 0xa1
	CALL _acintrp; .byte 0xa2
	CALL _acintrp; .byte 0xa3
	CALL _acintrp; .byte 0xa4
	CALL _acintrp; .byte 0xa5
	CALL _acintrp; .byte 0xa6
	CALL _acintrp; .byte 0xa7
	CALL _acintrp; .byte 0xa8
	CALL _acintrp; .byte 0xa9
	CALL _acintrp; .byte 0xaa
	CALL _acintrp; .byte 0xab
	CALL _acintrp; .byte 0xac
	CALL _acintrp; .byte 0xad
	CALL _acintrp; .byte 0xae
	CALL _acintrp; .byte 0xaf
	CALL _acintrp; .byte 0xb0
	CALL _acintrp; .byte 0xb1
	CALL _acintrp; .byte 0xb2
	CALL _acintrp; .byte 0xb3
	CALL _acintrp; .byte 0xb4
	CALL _acintrp; .byte 0xb5
	CALL _acintrp; .byte 0xb6
	CALL _acintrp; .byte 0xb7
	CALL _acintrp; .byte 0xb8
	CALL _acintrp; .byte 0xb9
	CALL _acintrp; .byte 0xba
	CALL _acintrp; .byte 0xbb
	CALL _acintrp; .byte 0xbc
	CALL _acintrp; .byte 0xbd
	CALL _acintrp; .byte 0xbe
	CALL _acintrp; .byte 0xbf
	CALL _acintrp; .byte 0xc0
	CALL _acintrp; .byte 0xc1
	CALL _acintrp; .byte 0xc2
	CALL _acintrp; .byte 0xc3
	CALL _acintrp; .byte 0xc4
	CALL _acintrp; .byte 0xc5
	CALL _acintrp; .byte 0xc6
	CALL _acintrp; .byte 0xc7
	CALL _acintrp; .byte 0xc8
	CALL _acintrp; .byte 0xc9
	CALL _acintrp; .byte 0xca
	CALL _acintrp; .byte 0xcb
	CALL _acintrp; .byte 0xcc
	CALL _acintrp; .byte 0xce
	CALL _acintrp; .byte 0xce
	CALL _acintrp; .byte 0xcf
	CALL _acintrp; .byte 0xd0
	CALL _acintrp; .byte 0xd1
	CALL _acintrp; .byte 0xd2
	CALL _acintrp; .byte 0xd3
	CALL _acintrp; .byte 0xd4
	CALL _acintrp; .byte 0xd5
	CALL _acintrp; .byte 0xd6
	CALL _acintrp; .byte 0xd7
	CALL _acintrp; .byte 0xd8
	CALL _acintrp; .byte 0xd9
	CALL _acintrp; .byte 0xda
	CALL _acintrp; .byte 0xdb
	CALL _acintrp; .byte 0xdc
	CALL _acintrp; .byte 0xdd
	CALL _acintrp; .byte 0xde
	CALL _acintrp; .byte 0xdf
	CALL _acintrp; .byte 0xe0
	CALL _acintrp; .byte 0xe1
	CALL _acintrp; .byte 0xe2
	CALL _acintrp; .byte 0xe3
	CALL _acintrp; .byte 0xe4
	CALL _acintrp; .byte 0xe5
	CALL _acintrp; .byte 0xe6
	CALL _acintrp; .byte 0xe7
	CALL _acintrp; .byte 0xe8
	CALL _acintrp; .byte 0xe9
	CALL _acintrp; .byte 0xea
	CALL _acintrp; .byte 0xeb
	CALL _acintrp; .byte 0xec
	CALL _acintrp; .byte 0xed
	CALL _acintrp; .byte 0xee
	CALL _acintrp; .byte 0xef
	CALL _acintrp; .byte 0xf0
	CALL _acintrp; .byte 0xf1
	CALL _acintrp; .byte 0xf2
	CALL _acintrp; .byte 0xf3
	CALL _acintrp; .byte 0xf4
	CALL _acintrp; .byte 0xf5
	CALL _acintrp; .byte 0xf6
	CALL _acintrp; .byte 0xf7
	CALL _acintrp; .byte 0xf8
	CALL _acintrp; .byte 0xf9
	CALL _acintrp; .byte 0xfa
	CALL _acintrp; .byte 0xfb
	CALL _acintrp; .byte 0xfc
	CALL _acintrp; .byte 0xfd
	CALL _acintrp; .byte 0xfe
	CALL _acintrp; .byte 0xff
