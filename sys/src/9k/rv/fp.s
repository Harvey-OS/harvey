/*
 * floating point machine assist for riscv
 */
#include "mem.h"
#include "riscvl.h"

#define SAVE(n)	MOVD F(n), ((n)*8)(R9)
#define REST(n)	MOVD ((n)*8)(R9), F(n)

	GLOBL	fpzero<>(SB), $8
	DATA	fpzero<>+0(SB)/8, $0.0e0
	GLOBL	fphalf<>(SB), $8
	DATA	fphalf<>+0(SB)/8, $0.5
	GLOBL	fpone<>(SB), $8
	DATA	fpone<>+0(SB)/8, $1.0
	GLOBL	fptwo<>(SB), $8
	DATA	fptwo<>+0(SB)/8, $2.0

/*
 * enable fpu, set rounding, set last fp regs, leave fp in Initial state.
 */
TEXT _fpuinit(SB), $-4
	/* Off would cause ill instr traps on fp use */
	MOV	CSR(SSTATUS), R10
	MOV	$~Fsst, R9
	AND	R9, R10
	MOV	$(Initial<<Fsshft), R9
	OR	R9, R10
	MOV	R10, CSR(SSTATUS)	/* enable FPU */
	FENCE

	MOV	R0, R9
	MOV	R9, CSR(FCSR)		/* mostly to set default rounding */
	/* fall through */

TEXT fpconstset(SB), $-4
	/* set initial values to those assumed by the compiler */
	MOVD	fpzero<>+0(SB), F28
	MOVD	fphalf<>+0(SB), F29
	MOVD	fpone<>+0(SB), F30
	MOVD	fptwo<>+0(SB), F31
	FENCE
	RET

TEXT getfcsr(SB), $-4
	MOV	CSR(FCSR), R(ARG)
	RET

TEXT setfcsr(SB), $-4
	MOV	R(ARG), CSR(FCSR)
	RET

TEXT fpusave(SB), $-4
	MOV	CSR(SSTATUS), R9
	MOV	$Fsst, R11
	AND	R11, R9
	MOV	$(Off<<Fsshft), R10
	BEQ	R9, R10, noop		/* off, so nothing to do */
	MOV	$(Dirty<<Fsshft), R10
	BNE	R9, R10, clean		/* on but clean, just turn off */
dirty:
	/* store fp registers into argument double array before turning off */
	MOV	R(ARG), R9
	SAVE(0); SAVE(1); SAVE(2); SAVE(3); SAVE(4); SAVE(5); SAVE(6); SAVE(7)
	SAVE(8); SAVE(9); SAVE(10); SAVE(11); SAVE(12); SAVE(13); SAVE(14)
	SAVE(15); SAVE(16); SAVE(17); SAVE(18); SAVE(19); SAVE(20); SAVE(21)
	SAVE(22); SAVE(23); SAVE(24); SAVE(25); SAVE(26); SAVE(27); SAVE(28)
	SAVE(29); SAVE(30); SAVE(31)
clean:
	/* fall through */

	/* fp state is now clean (registers match argument array), turn off */
TEXT fpoff(SB), 1, $-4
	FENCE				/* sifive u74 erratum cip-930 */
	/* Off will cause ill instr traps on fp use */
	MOV	CSR(SSTATUS), R10
	MOV	$~Fsst, R9
	AND	R9, R10
	MOV	$(Off<<Fsshft), R9	/* regs will be restored on next use */
	OR	R9, R10
	MOV	R10, CSR(SSTATUS)	/* disable FPU */
	FENCE
noop:
	RET

TEXT fpon(SB), 1, $-4
	MOV	CSR(SSTATUS), R9
	MOV	$Fsst, R11
	AND	R9, R11
	MOV	$(Off<<Fsshft), R12
	BNE	R11, R12, ondone	/* on, so nothing to do */

	MOV	$~Fsst, R11
	AND	R9, R11
	MOV	$(Dirty<<Fsshft), R9	/* don't know, Dirty is safe */
	OR	R9, R11
	MOV	R11, CSR(SSTATUS)	/* enable FPU */
	FENCE
ondone:
	RET

/* fpu is assumed at entry to be off, thus clean */
TEXT fpurestore(SB), $-4
	MOV	CSR(SSTATUS), R10
	MOV	$~Fsst, R9
	AND	R9, R10
	MOV	$(Clean<<Fsshft), R9
	OR	R9, R10
	MOV	R10, CSR(SSTATUS)	/* enable FPU */
	FENCE

	/* load fp registers from argument array */
	MOV	R(ARG), R9
	REST(0); REST(1); REST(2); REST(3); REST(4); REST(5); REST(6); REST(7)
	REST(8); REST(9); REST(10); REST(11); REST(12); REST(13); REST(14)
	REST(15); REST(16); REST(17); REST(18); REST(19); REST(20); REST(21)
	REST(22); REST(23); REST(24); REST(25); REST(26); REST(27); REST(28)
	REST(29); REST(30); REST(31)

	/* set status to Clean again, after loading fregs */
	FENCE
	MOV	R10, CSR(SSTATUS)
	FENCE
	RET
