/*
 * Start-up request IPI handler.
 *
 * This code is executed on an application processor in response to receiving
 * a Start-up IPI (SIPI) from another processor.
 * This must be placed on a 4KiB boundary
 * somewhere in the 1st MB of conventional memory. However,
 * due to some shortcuts below it's restricted further to within the 1st 64KiB.
 * The AP starts in real-mode, with
 *   CS selector set to the startup memory address/16;
 *   CS base set to startup memory address;
 *   CS limit set to 64KiB;
 *   CPL and IP set to 0.
 * Parameters are passed to this code via a vector in low memory
 * indexed by the APIC number of the processor. The layout, size,
 * and location have to be kept in sync with the setup in sipi.c.
 */

#include "mem.h"
#include "amd64l.h"

/*
 * Some machine instructions not handled well by [68][al].
 * This is a messy piece of code, requiring instructions in real mode,
 * protected mode (+long mode on amd64). The MODE pseudo-op of 6[al] handles
 * the latter two OK, but 'MODE $16' is incomplete, e.g. it does
 * not truncate operands appropriately, hence the ugly 'rMOVAX' macro.
 * Fortunately, the only other instruction executed in real mode that
 * could cause a problem (ORL) is encoded such that it will work OK.
 */
#define	DELAY		BYTE $0xeb;		/* JMP .+2 */		\
			BYTE $0x00
/*
 * use this (data) definition instead of the built-in instruction
 * to avoid 8l deleting NOPs after a jump.  this matters to alignment.
 */
#define NOP		BYTE $0x90

#define pFARJMP32(s, o)	BYTE $0xea;		/* far jmp ptr32:16 */	\
			LONG $o; WORD $s

#define rFARJMP16(s, o)	BYTE $0xea;		/* far jump ptr16:16 */	\
			WORD $o; WORD $s;
#define rFARJMP32(s, o)	BYTE $0x66;		/* far jump ptr32:16 */	\
			pFARJMP32(s, o)
#define rLGDT(gdtptr)	BYTE $0x0f;		/* LGDT */		\
			BYTE $0x01; BYTE $0x16;				\
			WORD $gdtptr
#define rMOVAX(i)	BYTE $0xb8;		/* i -> AX */		\
			WORD $i;

/*
 * Real mode. Welcome to 1978.
 * Load a basic GDT, turn on protected mode and make
 * inter-segment jump to the protected mode code.
 */
MODE $16

TEXT _real<>(SB), 1, $-4
	rFARJMP16(0, _endofheader<>-KZERO(SB))	/*  */

_startofheader:
	NOP; NOP; NOP
	QUAD	$0xa5a5a5a5a5a5a5a5

TEXT _endofheader<>(SB), 1, $-4
	CLI
	MOVW	CS, AX
	MOVW	AX, DS				/* initialise DS */

	rLGDT(_gdtptr32p<>-KZERO(SB))		/* load a basic gdt */

	MOVL	CR0, AX
	ORL	$Pe, AX
/**/	MOVL	AX, CR0				/* turn on protected mode */
	DELAY					/* JMP .+2 */

	rMOVAX	(SSEL(SiDS, SsTIGDT|SsRPL0))	/*  */
	MOVW	AX, DS
	MOVW	AX, ES
	MOVW	AX, FS
	MOVW	AX, GS
	MOVW	AX, SS

	rFARJMP32(SSEL(SiCS, SsTIGDT|SsRPL0), _protected<>-KZERO(SB))

/*
 * Protected mode. Welcome to 1982.
 * Get the local APIC ID from the memory mapped APIC
 * and use it to locate the index to the parameter vector;
 * load the PDB with the page table address from the
 * information vector;
 * make an identity map for the inter-segment jump below,
 * using the stack space to hold a temporary PDP and PD;
 * enable and activate long mode;
 * make an inter-segment jump to the long mode code.
 */
MODE $32

TEXT _protected<>(SB), 1, $-4
	MOVL	$Lapicphys, BP			/* apicbase */
	MOVL	0x20(BP), BP			/* Id (apic.c) */
	SHRL	$24, BP				/* becomes RARG later */

	MOVL	BP, AX				/* apicno */
	IMULL	$SIPISIZE, AX			/* [apicno] */
	MOVL	$_real<>-KZERO(SB), BX		/* sipi handler address */
	ADDL	$PGSZ, BX			/* Sipi array base */
	ADDL	AX, BX				/* sipi[apicno] */

	MOVL	SIPIPML4(BX), SI		/* sipi[apicno].pml4 */
	MOVL	SI, AX
/**/	MOVL	AX, CR3				/* load my mmu */

#ifdef unused
	/*
	 * if machno is 0, we're doing a reboot, jump to protected-mode
	 * trampoline.
	 */
	MOVL	SIPIMACH(BX), CX		/* sipi->mach (in low 4G) */
	MOVL	0(CX), CX			/* m->machno */
	CMPL	CX, $0
	JNE	not0

	/* main() will eventually restore nvramwrite(Cmosreset, Rstpwron); */
	MOVL	SIPISTK(BX), SP			/* sipi[apicno].stack */
	MOVL	SIPIARGS(BX), CX		/* sipi->args */
	MOVL	RBTRAMP(CX), AX
	JMP*	AX				/* into reboot trampoline */
not0:
#endif
	/* we're starting an AP */
	MOVL	AX, DX			/* my pml4; see l32p.s, dat.h */
	SUBL	$MACHSTKSZ, DX		/* my sys; PDP for identity map */
	MOVL	DX, DI
	ADDL	$(PteRW|PteP), DX
	MOVL	DX, PML4O(0)(AX)	/* PML4E for identity map 0->0 */

	MOVL	DI, DX			/* my sys; PDP for identity map */
	ADDL	$(LOWPDP|PteRW|PteP), DX
	MOVL	DX, PDPO(0)(AX)		/* PDPE for identity map: 0->0 */

	MOVL	$(PtePS|PteRW|PteP), DX
	MOVL	DI, AX
	ADDL	$LOWPDP, AX		/* PD for identity map */
	/* match l32p.s mappings in _warp64 */
	MOVL	DX, PDO(0)(AX)		/* PDE for identity 0-2MB */
	ADDL	$PGLSZ(1), DX
	MOVL	DX, PDO(PGLSZ(1))(AX)	/* PDE for identity 2-4MB */
	ADDL	$PGLSZ(1), DX
	MOVL	DX, PDO(2*PGLSZ(1))(AX)	/* PDE for identity 4-6MB */
	ADDL	$PGLSZ(1), DX
	MOVL	DX, PDO(3*PGLSZ(1))(AX)	/* PDE for identity 6-8MB */

#include "l64lme.s"
	/*
	 * now in long mode and MODE $64 and KZERO space.
	 * AX is virt PML4, BX is unchanged (still Sipi*), DX is 0,
	 * SI is sipi[apicno].pml4.
	 */

/*
 * zap the identity map;
 * initialise the stack, and call the C startup code for cpus other than 0.
 */
	MOVQ	DX, PML4O(0)(AX)		/* zap identity map */
	MOVQ	SI, CR3				/* flush TLB */

	MOVQ	$KZERO, CX
	ADDQ	CX, BX				/* &sipi[apicno] */

	MOVQ	SIPISTK(BX), SP			/* estab. sipi[apicno].stack */

	PUSHQ	DX				/* clear flags, including If */
	POPFQ
	MOVLQZX	RARG, RARG			/* APIC ID */
	PUSHQ	RARG				/* apicno */

	MOVQ	SIPIPC(BX), AX			/* sipi[apicno].pc */
	CALL*	AX				/* (*sipi[apicno].pc)(apicno) */

_ndnr:
	STI
	HLT
	JMP	_ndnr				/* do not resuscitate */
