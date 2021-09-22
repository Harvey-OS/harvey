/*
 * assembly-language machine assist for riscv rv64
 */
#include "mem.h"
#include "riscv64l.h"

/* registers */
TMP=15

/*
 * Read/write various system registers.
 */

TEXT rdtsc(SB), 1, $-4				/* Time Stamp Counter */
	FENCE
	MOV	CSR(CYCLO), R(ARG)
	RET
//TEXT wrtsc(SB), 1, $-4			/* set Time Stamp Counter */
//	MOV	R(ARG), CSR(CYCLO)
//	FENCE
//	RET

/* machine-mode registers */
TEXT putmcounteren(SB), 1, $-4
	MOV	R(ARG), CSR(MCOUNTEREN)
	RET
TEXT getmedeleg(SB), 1, $-4
	MOV	CSR(MEDELEG), R(ARG)
	RET
TEXT putmedeleg(SB), 1, $-4
	MOV	R(ARG), CSR(MEDELEG)
	RET
TEXT getmideleg(SB), 1, $-4
	MOV	CSR(MIDELEG), R(ARG)
	RET
TEXT putmideleg(SB), 1, $-4
	MOV	R(ARG), CSR(MIDELEG)
	RET
TEXT getmie(SB), 1, $-4
	MOV	CSR(MIE), R(ARG)
	RET
TEXT putmie(SB), 1, $-4
	MOV	R(ARG), CSR(MIE)
	RET
TEXT getmip(SB), 1, $-4
	MOV	CSR(MIP), R(ARG)
	RET
TEXT putmip(SB), 1, $-4
	MOV	R(ARG), CSR(MIP)
	RET
TEXT getmisa(SB), 1, $-4
	MOVW	CSR(MISA), R(ARG)
	RET
TEXT putmscratch(SB), 1,  $-4
	MOV	R(ARG), CSR(MSCRATCH)
	RET
TEXT getmsts(SB), 1, $-4
	MOV	CSR(MSTATUS), R(ARG)
	RET
TEXT putmsts(SB), 1, $-4
	MOV	R(ARG), CSR(MSTATUS)
	RET
TEXT putmtvec(SB), 1, $-4
	MOV	R(ARG), CSR(MTVEC)
	RET

TEXT getsatp(SB), 1, $-4
	MOVW	CSR(SATP), R(ARG)
	RET
TEXT putsatp(SB), 1, $-4
	FENCE
	FENCE_I
	SFENCE_VMA(0, 0)			/* flush TLB */
	MOV	R(ARG), CSR(SATP)		/* switch to the new map */
	SFENCE_VMA(0, 0)			/* flush TLB */
	FENCE
	FENCE_I
	RET
TEXT getsb(SB), 1, $-4
	MOV	R3, R(ARG)
	RET
TEXT putsb(SB), 1, $-4
	MOV	R(ARG), R3
	RET
TEXT setsb(SB), 1, $-4
	MOV	$setSB(SB), R3
	RET
TEXT getsie(SB), 1, $-4
	MOV	CSR(SIE), R(ARG)
	RET
TEXT putsie(SB), 1, $-4
	MOV	R(ARG), CSR(SIE)
	RET
TEXT getsip(SB), 1, $-4
	MOV	CSR(SIP), R(ARG)
	RET
TEXT putsip(SB), 1, $-4
	MOV	R(ARG), CSR(SIP)
	RET
TEXT getsp(SB), 1, $-4
	MOV	R2, R(ARG)
	RET
TEXT putsp(SB), 1,  $-4
	MOV	R(ARG), R2
TEXT putspalign(SB), 1, $-4
	RET
TEXT getsts(SB), 1, $-4
	MOV	CSR(SSTATUS), R(ARG)
	RET
TEXT putsts(SB), 1, $-4
	MOV	R(ARG), CSR(SSTATUS)
	RET
TEXT putsscratch(SB), 1,  $-4
	MOV	R(ARG), CSR(SSCRATCH)
	RET
TEXT putstvec(SB), 1, $-4
	MOV	R(ARG), CSR(STVEC)
	RET

TEXT clockenable(SB), 1, $-4
	MOV	$Stie, R9			/* super timer intr enable */
	CSRRS	CSR(SIE), R9, R0
	RET

TEXT clrstip(SB), 1, $-4
	MOV	$Stie, R(ARG)			/* super timer intr enable */
	CSRRC	CSR(SIE), R(ARG), R0
TEXT clrsipbit(SB), 1, $-4
	CSRRC	CSR(SIP), R(ARG), R0
	RET

TEXT mret(SB), 1, $-4
	MOV	$supermodemret(SB), R12
	MOV	R12, CSR(MEPC)
	MRET				/* leap to (MEPC) in super mode */
TEXT supermodemret(SB), 1, $-4
	RET

/*
 * jump to upper address range and adjust addresses in registers to match.
 * identity map and upper->lower map must already be in effect.
 */
TEXT jumphigh(SB), 1, $-4
	FENCE
	FENCE_I
	MOV	$KSEG0, R(TMP)		/* lowest upper address */
	OR	R(TMP), LINK
	OR	R(TMP), R2		/* sp */
	OR	R(TMP), R3		/* sb */
TEXT highalign(SB), 1, $-4
	OR	R(TMP), R(MACH)		/* m */
	BEQ	R(USER), jumpdone
	OR	R(TMP), R(USER)		/* up */
jumpdone:
	FENCE
	FENCE_I
	RET				/* jump into upper space */

/*
 * jump to lower (id) address range and adjust addresses in registers.
 * identity map must already be in effect.
 */
TEXT jumplow(SB), 1, $-4
	FENCE
	MOV	$~KSEG0, R(TMP)
	AND	R(TMP), LINK
	AND	R(TMP), R2		/* sp */
	AND	R(TMP), R3		/* sb */
TEXT lowalign(SB), 1, $-4
	AND	R(TMP), R(MACH)		/* m */
	AND	R(TMP), R(USER)		/* up */
	FENCE
	FENCE_I
	RET				/* jump into lower space */

/*
 * cache or tlb invalidation
 */
TEXT invlpg(SB), 1, $-4
	FENCE
	FENCE_I
	/* sifive u74 erratum cip-1200: can't do a non-global SFENCE */
//	SFENCE_VMA(0, ARG)	/* only fences pt leaf accesses for va in ARG */
	SFENCE_VMA(0, 0)
	RET

/* approximate intel's wbinvd instruction: i & d fences, flush mmu state */
TEXT wbinvd(SB), 1, $-4
	FENCE
	FENCE_I
	SFENCE_VMA(0, 0)
	RET

TEXT cacheflush(SB), 1, $-4
	FENCE
	FENCE_I
	RET

/*
 * Serialisation.
 */
TEXT coherence(SB), 1, $-4
	FENCE
	RET
TEXT pause(SB), 1, $-4
	FENCE
	PAUSE				/* can be a no-op */
	FENCE
	RET

TEXT splhi(SB), 1, $-4
_splhi:
	FENCE
	MOV	CSR(SSTATUS), R(ARG)
	AND	$Sie, R(ARG)		/* return old state */
	CSRRC	CSR(SSTATUS), $Sie, R0	/* disable intrs */
	MOV	LINK, SPLPC(R(MACH)) 	/* save PC in m->splpc */
_spldone:
	RET

TEXT spllo(SB), 1, $-4			/* marker for devkprof */
_spllo:
	MOV	CSR(SSTATUS), R(ARG)
	AND	$Sie, R(ARG)
	FENCE
	BNE	R(ARG), _splgolo	/* Sie=1, intrs were enabled? */
	/* they were disabled */
	MOV	R0, SPLPC(R(MACH))	/* clear m->splpc */
_splgolo:
	/* leave SIE bits alone */
	CSRRS	CSR(SSTATUS), $Sie, R0	/* enable intrs */
	RET

/* assumed between spllo and spldone by devkprof */
TEXT splx(SB), 1, $-4
	MOV	CSR(SSTATUS), R9
	AND	$Sie, R9
	AND	$Sie, R(ARG), R10
	BEQ	R9, R10, _spldone	/* already in desired state? */
	BEQ	R10, _splhi		/* want intrs disabled? */
	JMP	_spllo			/* want intrs enabled */

TEXT spldone(SB), 1, $-4			/* marker for devkprof */
	RET

TEXT islo(SB), 1, $-4
	MOV	CSR(SSTATUS), R(ARG)
	AND	$Sie, R(ARG)
	RET

/*
 * atomic operations
 */

TEXT aincnonneg(SB), 1, $XLEN			/* int aincnonneg(int*); */
	JAL	LINK, ainc(SB)
	BGT	R(ARG), _aincdone
	/* <= 0 after incr, so overflowed, so blow up */
	MOV	(R0), R0			/* over under sideways down */
_aincdone:
	RET

TEXT adecnonneg(SB), 1, $XLEN			/* int adecnonneg(int*); */
	JAL	LINK, adec(SB)
	BGE	R(ARG), _aincdone
	/* <0, so underflow.  use own copy of XORQ/MOVQ to disambiguate PC */
	MOV	(R0), R0			/* over under sideways down */
	RET

/* see libc/riscv64/atom.s for ainc & adec */
TEXT aadd(SB), 1, $-4			/* int aadd(int*, int addend); */
	MOVW	addend+XLEN(FP), R9
	AMOW(Amoadd, AQ|RL, 9, ARG, 10)
	ADDW	R9, R10, R(ARG)		/* old value + addend */
	SEXT_W(R(ARG))
	RET

TEXT amoswapw(SB), 0, $(2*XLEN)		/* ulong amoswapw(ulong *a, ulong nv) */
	MOVWU	nv+XLEN(FP), R9		/* new value */
	FENCE
	AMOW(Amoswap, AQ|RL, 9, ARG, 10) /* *R(ARG) += R9; old *R(ARG) -> R10 */
	FENCE
	MOVW	R10, R(ARG)		/* old value */
	RET

TEXT amoorw(SB), 0, $(2*XLEN)		/* ulong amoorw(ulong *a, ulong bits) */
	MOVWU	bits+XLEN(FP), R9
	FENCE
	AMOW(Amoor, AQ|RL, 9, ARG, 10) /* *R(ARG) |= R9; old *R(ARG) -> R10 */
	FENCE
	MOVW	R10, R(ARG)		/* old value */
	RET

/*
 * oddball instruction.
 */
TEXT ecall(SB), 0, $-4
	ECALL
	RET

/*
 * idle until an interrupt.  call it splhi().
 */
TEXT halt(SB), 1, $-4
	FENCE			/* make our changes visible to other harts */
	IDLE_WFI		/* WFI may be a no-op or hint */
	FENCE			/* make other harts' changes visible to us */
	RET

TEXT reset(SB), 1, $-4
bragain:
	EBREAK
	JMP	bragain

/*
 * Label consists of a stack pointer and a programme counter.
 */
TEXT gotolabel(SB), 1, $-4			/* gotolabel(Label *) */
	MOV	0(R(ARG)), R2			/* restore sp */
	MOV	XLEN(R(ARG)), LINK		/* restore return pc */
	MOV	$1, R(ARG)
	RET

TEXT setlabel(SB), 1, $-4			/* setlabel(Label *) */
	MOV	R2, 0(R(ARG))			/* store sp */
	MOV	LINK, XLEN(R(ARG))		/* store return pc */
	MOV	R0, R(ARG)
	RET

/*
 * mul64fract(uvlong *r, uvlong a, uvlong b)
 * Multiply two 64-bit numbers, return the middle 64 bits of the 128-bit result.
 *
 * use 64-bit multiply with 128-bit result
 */
TEXT mul64fract(SB), 1, $-4
	/* r, the address of the result, is in RARG */
	MOV	a+XLEN(FP), R9
	MOV	b+(2*XLEN)(FP), R11

	MULHU	R9, R11, R13			/* high unsigned(al*bl) */
	MUL	R9, R11, R14			/* low unsigned(al*bl) */

	SRL	$32, R14			/* lose lowest long */
	SLL	$32, R13  /* defeat sign extension & position low of hi as hi */
	OR	R13, R14

	MOV	R14, (R(ARG))
	RET
