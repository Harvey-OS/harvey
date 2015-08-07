/*
 * Interrupt/exception handling.
 */
#include "amd64.h"

.code64

	/* Interrupts.  * Let's just talk about hardware
	* interrupts. What has to happen is that we save enough state to
	* return to where we were, and that's all we do. Hardware needs a stack,
	* so it pushes SS and %rsp. Hardware has to
	* clear If, which means we have to save the flags. We might be
	* in user mode, so we need to change CS, so we need to save
	* CS. Finally, we have to know where we were, so we need to save
	* the RIP. And that's all get saved.
	* Further, if you look at idthandlers below, you see a call to intrp.
	* So on entry to intrp, the stack looks like this:
	* EFLAGS 24(%rsp)
	* CS	16(%rsp)
	* EIP of interrupted code 8(%rsp)
	* EIP from the call from the idthandlers. (%rsp)
	* We, finally, need to push the error code and type.,
	* then the registers.
	* Why the call from IDThandlers? So we can get a way to point
	* to the type. We don't need to save 0(%rsp), we just need
	* it to get the type. We can pop it and throw it away when
	* needed.
	* What does the C level function need?
	* It needs a pointer to the Uregs, as defined in ureg.h
	* The other problem: all the regs have to be saved, so
	* Plan 9 code can see them all.
	* Finally, if it's a nested interrupt, we need to know that
	* so we don't swapgs at the wrong time.
	* Finally, this is utterly different from how Plan 9 does it, because
	* all args are on the stack in Plan 9. We need %rdi for the pointer.
	* And, the final x86 mess: for some things, the error code pointer
	* is on the stack. For others, it's not. To make the stacks
	* look identical for the common code and exit we save %rax and line things up.
	*/

.globl ire
.globl irx
.globl irxe
// When we enter:
// registers are NOT saved. We need to save them all.
// return PC is on the stat8(%rsp). It should be left there.
// @ (%rsp) is the PC from the vector table.
// So indirecting on that will get us the interrupt #.
// We need to get what the return PC is pointing to into %rdi.
// We no longer need to make the stack look the same as in Plan 9
// because the arg is in %rdi
.globl _intrp
_intrp:
	pushq	%rax // bogus error code. Makes stack look like intre.
	// Except in this case, bogus error code is at 0(%rsp)
	// vno pointer is at 8(%rsp)
	MOVQ	8(%rsp), %rax
	// Now %rax points to the vector number.
	JMP	_intrcommon

// For intre, error is at top of stack on trap. But then we call here
// from the interrupt vectors so error is at 8(%rsp).
// I just realized I think intre means interrupt exception, e.g. page fault.
.globl _intre
_intre:
///jmp	_intre
	// The error code has been pushed. How I love the x86.
	// So the error code is ABOVE the pointer to the vector #.
	XCHGQ	%rax, (%rsp)
	// Now %rax points to the vector number.

// When we get here:
// %RAX points to our vector number, i.e. "return" pc from calls below.
// For intrp, (%rsp) is bogus code, 8(%rsp) is pointer to vno
// for intre, (%rsp) is pointer to vno, 8(%rsp) is error code.
// The rest of the stack is the same.
_intrcommon:
	// Get the vector number into %al
	MOVb	(%rax), %al
	ANDQ	$0xff, %rax
	// Put that at TOS (this is plan 9 argument style)
	XCHGQ	%rax, 0(%rsp)
	// 0(%rsp) now has the vno
	CMPW	$SSEL(SiCS, SsTIGDT|SsRPL0), 24(%rsp) 	/* old CS */
	JE	_intrnested

	SWAPGS

_intrnested:
	incq ire
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
	xorq	%rax, %rax
	pushq	%rax
	popfq				/* clear all flags. is there something else we should clear too? */
	CALL	_trap
.globl _intrr
_intrr:
	incq	irxe
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
	JE	_iretnested

	SWAPGS

_iretnested:
	// Throw away:
	// The %rax you pushed (error code)
	// EIP from the vector table.
	ADDQ	$16, %rsp
	incq irx
	iretq

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
