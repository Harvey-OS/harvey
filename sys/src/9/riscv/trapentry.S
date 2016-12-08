#include "mem.h"
/* This file was provided by the RISCV project at UC Berkeley. */
#define PT_IP 0
#define PT_SP 8
#define PT_GP 16
#define PT_TP 24
#define PT_T0 32
#define PT_T1 40
#define PT_T2 48
#define PT_S0 56
#define PT_S1 64
#define PT_A0 72
#define PT_A1 80
#define PT_A2 88
#define PT_A3 96
#define PT_A4 104
#define PT_A5 112
#define PT_A6 120
#define PT_A7 128
#define PT_S2 136
#define PT_S3 144
#define PT_S4 152
#define PT_S5 160
#define PT_S6 168
#define PT_S7 176
#define PT_S8 184
#define PT_S9 192
#define PT_S10 200
#define PT_S11 208
#define PT_T3 216
#define PT_T4 224
#define PT_T5 232
#define PT_T6 240
#define PT_SSTATUS 248
#define PT_SEPC 256
#define PT_SBADADDR 264
#define PT_SCAUSE 272
#define PT_SINSNN 280
#define PT_SFTYPE 296
	
	.text
	.altmacro
	.macro SAVE_ALL
	LOCAL _restore_kernel_sp
	LOCAL _save_context

        addi sp, sp, -304
	sd x1,  PT_IP(sp)
	sd x3,  PT_GP(sp)
	sd x5,  PT_T0(sp)
	sd x6,  PT_T1(sp)
	sd x7,  PT_T2(sp)
	sd x8,  PT_S0(sp)
	sd x9,  PT_S1(sp)
	sd x10, PT_A0(sp)
	sd x11, PT_A1(sp)
	sd x12, PT_A2(sp)
	sd x13, PT_A3(sp)
	sd x14, PT_A4(sp)
	sd x15, PT_A5(sp)
	sd x16, PT_A6(sp)
	sd x17, PT_A7(sp)
	sd x18, PT_S2(sp)
	sd x19, PT_S3(sp)
	sd x20, PT_S4(sp)
	sd x21, PT_S5(sp)
	sd x22, PT_S6(sp)
	sd x23, PT_S7(sp)
	sd x24, PT_S8(sp)
	sd x25, PT_S9(sp)
	sd x26, PT_S10(sp)
	sd x27, PT_S11(sp)
	sd x28, PT_T3(sp)
	sd x29, PT_T4(sp)
	sd x30, PT_T5(sp)
	sd x31, PT_T6(sp)

	/* Disable FPU to detect illegal usage of
	   floating point in kernel space */
	//li t0, SR_FS

	csrr s0, sscratch
	csrrc s1, sstatus, t0
	csrr s2, sepc
	csrr s3, 0x143 /* and someday ... sbadaddr*/
	csrr s4, 0x142 //scause
	sd s0, PT_SP(sp)
	sd s1, PT_SSTATUS(sp)
	sd s2, PT_SEPC(sp)
	sd s3, PT_SBADADDR(sp)
	sd s4, PT_SCAUSE(sp)
	.endm

	.macro RESTORE_ALL
	ld a0, PT_SSTATUS(sp)
	ld a2, PT_SEPC(sp)
	csrw sstatus, a0
	csrw sepc, a2

	ld x1,  PT_IP(sp)
	ld x3,  PT_GP(sp)
	ld x5,  PT_T0(sp)
	ld x6,  PT_T1(sp)
	ld x7,  PT_T2(sp)
	ld x8,  PT_S0(sp)
	ld x9,  PT_S1(sp)
	ld x10, PT_A0(sp)
	ld x11, PT_A1(sp)
	ld x12, PT_A2(sp)
	ld x13, PT_A3(sp)
	ld x14, PT_A4(sp)
	ld x15, PT_A5(sp)
	ld x16, PT_A6(sp)
	ld x17, PT_A7(sp)
	ld x18, PT_S2(sp)
	ld x19, PT_S3(sp)
	ld x20, PT_S4(sp)
	ld x21, PT_S5(sp)
	ld x22, PT_S6(sp)
	ld x23, PT_S7(sp)
	ld x24, PT_S8(sp)
	ld x25, PT_S9(sp)
	ld x26, PT_S10(sp)
	ld x27, PT_S11(sp)
	ld x28, PT_T3(sp)
	ld x29, PT_T4(sp)
	ld x30, PT_T5(sp)
	ld x31, PT_T6(sp)

	.endm

	/* we just figure out early whether it's user or not, and branch to totally different code paths.
	 * it's just easier that way, even though there's duplication of code.
	 * At this level, duplication is not such a problem since, once written, this kind of code never changes.
	*/
.globl supervisor_trap_entry
supervisor_trap_entry:
	/* If coming from userspace, preserve the user stack pointer and load
	   the kernel stack pointer.  If we came from the kernel, sscratch
	   will contain 0, and we should continue on the current stack. */
	csrrw sp, sscratch, sp
	/* if it is 0 we were in kernel mode. */
	beqz sp, kernel
	sd tp, 32(sp) // rathole.
	mv tp, sp
	ld sp, 16(sp) // Proc
	ld sp, 280(sp) // kstack
	addi sp, sp, 1024
	addi sp, sp, 1024
	addi sp, sp, 1024
	addi sp, sp, 1024
	addi sp, sp, 1024
	addi sp, sp, 1024
	addi sp, sp, 1024
	SAVE_ALL
	// tp is special, we don't want to touch it in kernel interrupts.
	ld x1,  32(tp)
	sd x1,  PT_TP(sp)

	/* Set sscratch register to 0, so that if a recursive exception
	   occurs, the exception vector knows it came from the kernel */
	csrw sscratch, x0

	la ra, user_ret_from_exception
	/* MSB of cause differentiates between
	   interrupts and exceptions */
	bge s4, zero, 1f

	/* Handle interrupts */
	move a0, sp /* pt_regs */
	tail _trap
1:
        /* Handle syscalls */
        li t0, 8
        beq s4, t0, user_handle_syscall
	move a0, sp /* pt_regs */
	tail _trap

user_handle_syscall:
	/* Advance SEPC to avoid executing the original
	   scall instruction on sret */
	addi s2, s2, 0x4
	sd s2, PT_SEPC(sp)
	/* System calls run with interrupts enabled */
	csrs sstatus, 1/*SR_IE*/
	move a1, sp /* pt_regs */
	ld a0, PT_A7(sp)
	call syscall
user_ret_from_syscall:

user_ret_from_exception:
#define sstatus 0x100
#define siebitno 1
	// splhi
	li	a0, siebitno
	csrrs	a0, sstatus, a0
	/* Put Mach pointer in sscratch in for user mode. */
	csrw sscratch, tp
user_restore_all:
	RESTORE_ALL
	ld x4,  PT_TP(sp)
	ld x2,  PT_SP(sp)
	/* FUUUUUUCCCCCKKKKK */
	/* someday this toolchain will be done. */
	.long 0x10200073
	sret

.globl _sysrforkret
_sysrforkret:
	addi	sp, sp, 32
	sd x0, PT_A0(sp)
	tail user_ret_from_syscall

kernel:
	csrrw sp, sscratch, sp
	//csrr sp, sscratch
	SAVE_ALL
	la ra, kernel_ret_from_exception
	/* MSB of cause differentiates between
	   interrupts and exceptions */
	bge s4, zero, 1f

	/* Handle interrupts */
	move a0, sp /* pt_regs */
	tail _trap
1:
        /* Handle syscalls */
        li t0, 8
        beq s4, t0, kernel_handle_syscall
	move a0, sp /* pt_regs */
	tail _trap

kernel_handle_syscall:
	// TODO: panic: syscall in kernel mode.
	j kernel_handle_syscall
kernel_ret_from_exception:
#define sstatus 0x100
#define siebitno 1
	// splhi
	li	a0, siebitno
	csrrc	a0, sstatus, a0

kernel_restore_all:
	RESTORE_ALL
	addi sp, sp, 304

	/* FUUUUUUCCCCCKKKKK */
	/* someday this toolchain will be done. */
	.long 0x10200073
	sret

#if 0
END(handle_exception)

ENTRY(ret_from_fork)
	la ra, ret_from_exception
	tail schedule_tail
ENDPROC(ret_from_fork)

/*
 * Integer register context switch
 * The callee-saved registers must be saved and restored.
 * 
 *   a0: previous task_struct (must be preserved across the switch)
 *   a1: next task_struct
 */
ENTRY(__switch_to)
	/* Save context into prev->thread */
	sd ra,  THREAD_RA(a0)
	sd sp,  THREAD_SP(a0)
	sd s0,  THREAD_S0(a0)
	sd s1,  THREAD_S1(a0)
	sd s2,  THREAD_S2(a0)
	sd s3,  THREAD_S3(a0)
	sd s4,  THREAD_S4(a0)
	sd s5,  THREAD_S5(a0)
	sd s6,  THREAD_S6(a0)
	sd s7,  THREAD_S7(a0)
	sd s8,  THREAD_S8(a0)
	sd s9,  THREAD_S9(a0)
	sd s10, THREAD_S10(a0)
	sd  s11, THREAD_S11(a0)
	/* Restore context from next->thread */
	ld ra,  THREAD_RA(a1)
	ld sp,  THREAD_SP(a1)
	ld s0,  THREAD_S0(a1)
	ld s1,  THREAD_S1(a1)
	ld s2,  THREAD_S2(a1)
	ld s3,  THREAD_S3(a1)
	ld s4,  THREAD_S4(a1)
	ld s5,  THREAD_S5(a1)
	ld s6,  THREAD_S6(a1)
	ld s7,  THREAD_S7(a1)
	ld s8,  THREAD_S8(a1)
	ld s9,  THREAD_S9(a1)
	ld s10, THREAD_S10(a1)
	ld s11, THREAD_S11(a1)
	mv tp, a1 /* Next current pointer */
	ret
ENDPROC(__switch_to)

ENTRY(__fstate_save)
	li t1, SR_FS
	csrs sstatus, t1
	frcsr t0
	fsd f0,  THREAD_F0(a0)
	fsd f1,  THREAD_F1(a0)
	fsd f2,  THREAD_F2(a0)
	fsd f3,  THREAD_F3(a0)
	fsd f4,  THREAD_F4(a0)
	fsd f5,  THREAD_F5(a0)
	fsd f6,  THREAD_F6(a0)
	fsd f7,  THREAD_F7(a0)
	fsd f8,  THREAD_F8(a0)
	fsd f9,  THREAD_F9(a0)
	fsd f10, THREAD_F10(a0)
	fsd f11, THREAD_F11(a0)
	fsd f12, THREAD_F12(a0)
	fsd f13, THREAD_F13(a0)
	fsd f14, THREAD_F14(a0)
	fsd f15, THREAD_F15(a0)
	fsd f16, THREAD_F16(a0)
	fsd f17, THREAD_F17(a0)
	fsd f18, THREAD_F18(a0)
	fsd f19, THREAD_F19(a0)
	fsd f20, THREAD_F20(a0)
	fsd f21, THREAD_F21(a0)
	fsd f22, THREAD_F22(a0)
	fsd f23, THREAD_F23(a0)
	fsd f24, THREAD_F24(a0)
	fsd f25, THREAD_F25(a0)
	fsd f26, THREAD_F26(a0)
	fsd f27, THREAD_F27(a0)
	fsd f28, THREAD_F28(a0)
	fsd f29, THREAD_F29(a0)
	fsd f30, THREAD_F30(a0)
	fsd f31, THREAD_F31(a0)
	sw t0, THREAD_FCSR(a0)
	csrc sstatus, t1
	ret
ENDPROC(__fstate_save)

ENTRY(__fstate_restore)
	li t1, SR_FS
	lw t0, THREAD_FCSR(a0)
	csrs sstatus, t1
	fld f0,  THREAD_F0(a0)
	fld f1,  THREAD_F1(a0)
	fld f2,  THREAD_F2(a0)
	fld f3,  THREAD_F3(a0)
	fld f4,  THREAD_F4(a0)
	fld f5,  THREAD_F5(a0)
	fld f6,  THREAD_F6(a0)
	fld f7,  THREAD_F7(a0)
	fld f8,  THREAD_F8(a0)
	fld f9,  THREAD_F9(a0)
	fld f10, THREAD_F10(a0)
	fld f11, THREAD_F11(a0)
	fld f12, THREAD_F12(a0)
	fld f13, THREAD_F13(a0)
	fld f14, THREAD_F14(a0)
	fld f15, THREAD_F15(a0)
	fld f16, THREAD_F16(a0)
	fld f17, THREAD_F17(a0)
	fld f18, THREAD_F18(a0)
	fld f19, THREAD_F19(a0)
	fld f20, THREAD_F20(a0)
	fld f21, THREAD_F21(a0)
	fld f22, THREAD_F22(a0)
	fld f23, THREAD_F23(a0)
	fld f24, THREAD_F24(a0)
	fld f25, THREAD_F25(a0)
	fld f26, THREAD_F26(a0)
	fld f27, THREAD_F27(a0)
	fld f28, THREAD_F28(a0)
	fld f29, THREAD_F29(a0)
	fld f30, THREAD_F30(a0)
	fld f31, THREAD_F31(a0)
	fscsr t0
	csrc sstatus, t1
	ret
ENDPROC(__fstate_restore)


	.section ".rodata"
	/* Exception vector table */
ENTRY(excp_vect_table)
	PTR do_trap_insn_misaligned
	PTR do_page_fault
	PTR do_trap_insn_illegal
	PTR do_trap_unknown
	PTR do_trap_unknown
	PTR do_page_fault
	PTR do_trap_unknown
	PTR do_page_fault
	PTR 0 /* handle_syscall */
	PTR do_trap_break
excp_vect_table_end:
END(excp_vect_table)
	
#endif
