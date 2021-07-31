/*
 * Memory and machine-specific definitions.  Used in C and assembler.
 */

/*
 * Sizes
 */
#define	BI2BY		8			/* bits per byte */
#define BI2WD		32			/* bits per word */
#define	BY2WD		4			/* bytes per word */
#define	BY2PG		4096			/* bytes per page */
#define	WD2PG		(BY2PG/BY2WD)		/* words per page */
#define	PGSHIFT		12			/* log(BY2PG) */
#define PGROUND(s)	(((s)+(BY2PG-1))&~(BY2PG-1))

#define	MAXMACH		1			/* max # cpus system can run */

/*
 * Time
 */
#define HZ		25			/* clock frequency */
#define	MS2HZ		(1000/HZ)		/* millisec per clock tick */
#define	TK2SEC(t)	((t)/HZ)		/* ticks to seconds */
#define	TK2MS(t)	((t)*MS2HZ)		/* ticks to milliseconds */
#define	MS2TK(t)	((t)/MS2HZ)		/* milliseconds to ticks */

/*
 * MMU
 */
#define	PTEVALID	0x00000001		/* valid */
#define	PTEUNCACHED	0
#define PTEWRITE	0x00000002		/* writeable */
#define	PTERONLY	0
#define PPN(x)		((x)&~(BY2PG-1))
#define PTEMAPMEM	(1024*1024)	
#define	PTEPERTAB	(PTEMAPMEM/BY2PG)
#define SEGMAPSIZE	16

/*
 * Address spaces
 */
#define KZERO		0xC0000000	/* base of kernel address space */
#define	TSTKTOP		0xFFC00000	/* end of new stack in sysexec */
#define TSTKSIZ 	10

#define UZERO		0x00000000	/* base of user address space */
#define	UTZERO		(UZERO+0x1000)	/* first address in user text */
#define	USTKTOP		0x40000000	/* byte just beyond user stack */
#define	USTKSIZE	(4*1024*1024)	/* size of user stack */

/*
 * Fundamental addresses
 */
#define SOFTADDR	(KZERO+0x0000)
#define VBADDR		(KZERO+0x0080)
#define KSTBADDR	(KZERO+0x1000)
#define MACHADDR	(KZERO+0x2000)
#define ISPOFFSET	(0x1000)
#define SPOFFSET	(0x1000-0x0100)
#define KTZERO		(KZERO+0x4000)	/* first address in kernel text */
#define PHYSADDR(v)	((v) & ~KZERO)
#define VIRTADDR(p)	(KZERO|(p))

#define	USERADDR	0xFFFFE000
#define UREGADDR	(USERADDR+ISPOFFSET-0x0020)
/*
 * Hobbit CPU definitions
 */
/*
 * registers
 */
#define MSP	      0x01	/* maximum stack pointer */
#define ISP	      0x02	/* interrupt stack pointer */
#define SP	      0x03	/* stack pointer */
#define CONFIG	      0x04	/* configuration */
#define PSW	      0x05	/* program status word */
#define SHAD	      0x06	/* shadow stack pointer */
#define VB	      0x07	/* vector base */
#define STB	      0x08	/* segment table base */
#define FAULT	      0x09	/* fault */
#define ID	      0x0A	/* identification */
#define TIMER1	      0x0B	/* timer one */
#define TIMER2	      0x0C	/* timer two */
#define FPSW	      0x10	/* floating point status word */

/*
 * config register
 */
#define KL	0x00010000	/* kernel little endian */
#define PX	0x00020000	/* PC Extension */
#define SE	0x00040000	/* stack cache enable */
#define IE	0x00080000	/* instruction cache enable */
#define PE	0x00100000	/* prefetch buffer enable */
#define PM	0x00200000	/* prefetch mode */
#define T1CI	0x00400000	/* TIMER1 counts instructions */
#define T1X	0x00800000	/* TIMER1 counts only in user mode */
#define T1IE	0x01000000	/* TIMER1 interrupt enable */
#define T2CI	0x02000000	/* TIMER2 counts instructions (5-bit field) */
#define T2OFF	0x3E000000	/* TIMER2 off */
#define T2X	0x40000000	/* TIMER2 counts only in user mode */
#define T2IE	0x80000000	/* TIMER2 interrupt enable */

/*
 * floating point status word register
 * (unimplemented)
 */
#define FP_RQ	0x000000F8	/* remainder quotient */
#define FP_XE	0x00001F00	/* excluded exceptions (5-bit field) */
#define FP_XL	0x0003E000	/* exceptions last operation (5-bit field) */
#define FP_XH	0x007C0000	/* exception halt enables (5-bit field) */
#define FP_XA	0x0F800000	/* accumulated exceptions (5-bit field) */
#define FP_RP	0x30000000	/* rounding precision (2-bit field) */
#define FP_RD	0x30000000	/* rounding direction (2-bit field) */

#define FP_V	0x00000001	/* exception fields 	- inexact */
#define FP_U	0x00000002	/*			- underflow */
#define FP_O	0x00000004	/*			- overflow */
#define FP_Z	0x00000008	/*			- division by 0 */
#define FP_I	0x00000010	/*			- inexact */

#define RP_E	0x00000000	/* rounding precision	- to extended */
#define RP_D	0x00000001	/*			- to double */
#define RP_S	0x00000002	/*			- to single */

#define RD_N	0x00000000	/* rounding direction	- to nearest */
#define RD_PI	0x00000001	/*			- toward +infinity */
#define RD_NI	0x00000002	/*			- toward -infinity */
#define RD_Z	0x00000003	/*			- toward 0 */

/*
 * program status word
 */
#define PSW_F	0x00000010	/* flag */
#define PSW_C	0x00000020	/* carry */
#define PSW_V	0x00000040	/* overflow */
#define PSW_TI	0x00000080	/* trace instruction */
#define PSW_TB	0x00000100	/* trace basic block */
#define PSW_S	0x00000200	/* current stack pointer (1 == SP) */
#define PSW_X	0x00000400	/* execution level (1 == user) */
#define PSW_E	0x00000800	/* ENTER guard */
#define PSW_IPL	0x00007000	/* interrupt priority level (3 bit field) */
#define PSW_UL	0x00008000	/* user little endian */
#define PSW_VP	0x00010000	/* virtual/physical (1 == virtual) */

/*
 * memory management
 */
/*
 * mmu: segment table base
 */
#define STB_C		0x00000800	/* cacheable */

/*
 * mmu: segment table entries
 */
#define STE_V		0x00000001	/* valid */
#define STE_W		0x00000002	/* writeable (non-paged segment only) */
#define STE_U		0x00000004	/* user (non-paged segment only) */
#define STE_S		0x00000008	/* segment (1 == non-paged segment) */
#define STE_C		0x00000800	/* cacheable */
#define NPSTE(pa, n, b)	((pa)|(((n)-1)<<12)|((b)|STE_S))
#define STE(pa, b)	((pa)|((b) & ~STE_S))

/*
 * mmu: page table entries
 */
#define PTE_V		0x00000001	/* valid */
#define PTE_W		0x00000002	/* writeable */
#define PTE_U		0x00000004	/* user */
#define PTE_R		0x00000008	/* referenced */
#define PTE_M		0x00000010	/* modified */
#define PTE_C		0x00000800	/* cacheable */
#define PTE(pa, b)	((pa)|(b))

/*
 * mmu: miscellaneous
 */
#define SEGSIZE		0x400000
#define SEGSHIFT	22
#define SEGNUM(va)	(((va)>>SEGSHIFT) & 0x03FF)
#define SEGOFFSET	0x3FFFFF
#define PGNUM(va)	(((va)>>PGSHIFT) & 0x03FF)
#define PGOFFSET	0x0FFF

/*
 * exceptions
 */
#define EXC_ZDIV	0x01
#define EXC_TRACE	0x02
#define EXC_ILLINS	0x03
#define EXC_ALIGN	0x04
#define EXC_PRIV	0x05
#define EXC_REG		0x06
#define EXC_FETCH	0x07
#define EXC_READ	0x08
#define EXC_WRITE	0x09
#define EXC_TEXTBERR	0x0A
#define EXC_DATABERR	0x0B

#define SCSIZE		0x0100		/* stack-cache size */
#define VBSIZE		0x0038		/* vector-base size */

#define RAMBASE		0x00000000	/* base of physical memory */
#define ROMBASE		0x40000000
#define IOBASE		0x80000000
#define IOBASEADDR	0x80000000
#define	IO(t, x)	((t *)(IOBASEADDR+(x)))

#define SCSIREG		IO(SCSIdev, 0x0200)
#define PDMAREG		IO(unsigned char, 0x0300)
#define TIMERREG	IO(void, 0x0400)
#define TIMERHZ		7372800

#define NVRAM		IO(Nvram, 0x2000)
#define NVLEN		2040
#define RTCREG		IO(RTCdev, 0x3FE0)

#define INCONREG	IO(Device, 0x4000)
#define DUSCCREG	IO(Duscc, 0x4100)

#define I82380		(IOBASEADDR+0x02000000)
#define I82380REG	IO(Device, 0x02000000)

#define WAIT		NOP; NOP; NOP
#define FLUSH		FLUSHP; FLUSHI

TEXT _startup(SB), $0
	FLUSH
	MOV	$(/*PM|*/PE|IE|SE|PX), %CONFIG
	MOV	$0, %PSW
	MOV	$PHYSADDR(MACHADDR+ISPOFFSET), %ISP
	MOV	%ISP, %SHAD
	WAIT

	ENTER	R-16

	MOV	$_resetcode+(RAMBASE-KZERO)(SB), R0
	MOV	$PHYSADDR(SOFTADDR), R4

_softresetinit:
	CMPEQ	R0, $_resetcode+(RAMBASE-KZERO+0x4C)(SB)
	MOV	*R0.UH, *R4.UH
	ADD	$2, R0
	ADD	$2, R4
	JMPFY	_softresetinit

_stbinit:
	MOV	$PHYSADDR(KSTBADDR), R0
	MOV	R0, %STB			/* physical */

	SUB3	$16, %ISP			/* clear two pages */
l0:
	CMPEQ	R4, R0
	DQM	$0, *R4
	SUB	$16, R4
	JMPF	l0

	ADD3	$(SEGNUM(KZERO)<<2), R0		/* kernel segment-table entry */
	MOV	$NPSTE(RAMBASE, 1024, STE_C|STE_W|STE_V), *R4

	ADD3	$(SEGNUM(ROMBASE)<<2), R0	/* ROM segment-table entry */
	MOV	$NPSTE(ROMBASE, 32, STE_V), *R4

	ADD3	$(SEGNUM(IOBASE)<<2), R0	/* I/O segment-table entry */
	MOV	$NPSTE(IOBASE, 5, STE_W|STE_V), *R4

	ADD3	$(SEGNUM(I82380)<<2), R0	/* DMA segment-table entry */
	MOV	$NPSTE(I82380, 1, STE_W|STE_V), *R4

	MOV	$(MACHADDR+SPOFFSET), R0
	MOV	R0, R4
	MOV	$_virtual(SB), R8
	MOV	$(PSW_VP|PSW_S), R12
	FLUSH
	CRET

TEXT _virtual(SB), $16
	MOV	$(MACHADDR+ISPOFFSET), %ISP	/* virtual */

	MOV	$VBADDR, R4			/* initialise vector base */
	MOV	R4, %VB
	MOV	$vbase+(VBSIZE-4)(SB), R8	/* copy vector table */
	ADD	$(VBSIZE-4), R4
_vbinit:
	CMPEQ	R8, $vbase(SB)
	MOV	*R8, *R4
	SUB	$4, R8
	SUB	$4, R4
	JMPF	_vbinit

	MOV	$MACHADDR, m(SB)		/* initialise m */
	MOV	$MACHADDR, mach0(SB)

	MOV	$edata(SB), R4			/* clear BSS */
_clear:
	CMPHI	R4, $end(SB)
	JMPT	_done
	MOV	$0, *R4
	ADD	$4, R4
	JMP	_clear
_done:
	CALL	main(SB)
	RETURN					/* better not... */

TEXT splhi(SB), $0
	MOV	%PSW, R4
	AND	$~PSW_IPL, %PSW
	RETURN

TEXT spllo(SB), $0
	MOV	%PSW, R4
	OR	$PSW_IPL, %PSW
	RETURN

TEXT splx(SB), $0
	MOV	R4, %PSW
TEXT spldone(SB), $0
	RETURN

TEXT setlabel(SB), $0
	MOV	%ISP, *R4
	ADD	$4, R4
	MOV	%SP, *R4
	ADD	$4, R4
	MOV	%MSP, *R4
	ADD	$4, R4
	MOV	R0, *R4
	ADD	$4, R4
	MOV	%PSW, *R4
	MOV	$0, R4
	RETURN

TEXT gotolabel(SB), $SCSIZE		/* gotolabel(Label *l) */
	MOV	l+0(FP), R0		/* l */
	MOV	$(PSW_VP|PSW_S), %PSW	/* no surprises */

	SUB3	$16, *R0		/* create frame for CRET */
	MOV	R4, %ISP
	ADD	$4, R0
	MOV	*R0, R16		/* SP */
	ADD	$4, R0
	MOV	*R0, R20		/* MSP */
	ADD	$4, R0
	MOV	*R0, R24		/* PC */
	ADD	$4, R0
	MOV	*R0, R28		/* PSW */
	DQM	R16, *R4		/* load CRET frame */

	ADD	$4, R16
	MOV	$1, *R16		/* return value */

	FLUSHI
	CRET

TEXT tas(SB), $0
	ORI	$1, *R4
	RETURN

TEXT mmuflushpte(SB), $0
	FLUSHPTE *R4
	RETURN

TEXT mmuflushtlb(SB), $0
	MOV	%STB, %STB
	RETURN

TEXT mmusetstb(SB), $0
	OR	$STB_C, R4
	MOV	R4, %STB
	RETURN

TEXT flushcpucache(SB), $0
	FLUSH
	RETURN

TEXT _vectorD(SB), $16			/* FP exception */
	MOV	$0x0D, R0
	JMP	_exception

TEXT _vectorC(SB), $16			/* timer 2 interrupt */
	MOV	$0x0C, R0
	JMP	_exception

TEXT _vectorB(SB), $16			/* timer 1 interrupt */
	MOV	$0x0B, R0
	JMP	_exception

TEXT _vectorA(SB), $16			/* interrupt 6 */
	MOV	$0x0A, R0
	JMP	_exception

TEXT _vector9(SB), $16			/* interrupt 5 */
	MOV	$0x09, R0
	JMP	_exception

TEXT _vector8(SB), $16			/* interrupt 4 */
	MOV	$0x08, R0
	JMP	_exception

TEXT _vector7(SB), $16			/* interrupt 3 */
	MOV	$0x07, R0
	JMP	_exception

TEXT _vector6(SB), $16			/* interrupt 2 */
	MOV	$0x06, R0
	JMP	_exception

TEXT _vector5(SB), $16			/* interrupt 1 */
	MOV	$0x05, R0
	JMP	_exception

TEXT _vector4(SB), $16			/* non-maskable interrupt */
	MOV	$0x04, R0
	JMP	_exception

TEXT _vector1(SB), $16			/* exception */
	MOV	$0x01, R0
	JMP	_exception		/* no return */

TEXT _vector0(SB), $16			/* syscall */
	MOV	$0x00, R0

_exception:
	MOV	%SP, R16
	SUB3	R16, %MSP
	CMPHI	R4, $SCSIZE
	AND3	$PSW_X, R28		/* was it user mode? */
	MOV	%FAULT, R12
	JMPFY	_mspok
	MOV	R16, %MSP
_mspok:
	CMPEQ	$0, R4
	MOV	R20, R8			/* id */
	MOV	%MSP, R20
	OR	$PSW_S, %PSW		/* switch to SP */
	MOV	%SP, %SHAD
	JMPF	_wasuser

_waskernel:
	ENTER	R-16
	MOV	%ISP, R4
	CALL	trap(SB)
	ADD	$16, %ISP
	ADD3	$4, %ISP
	MOV	*R4, R4			/* this doesn't quite cut it */
	CATCH	*R4
	POPN	R16
	KRET

_wasuser:
	ENTER	$(USERADDR+SPOFFSET-16)
	CATCH	R16
	MOV	%ISP, R4
	CALL	trap(SB)

TEXT touser(SB), $0
	ADD	$16, %ISP

	ADD3	$8, %ISP		/* Hobbit Mask3 bug */
	CMPEQ	*R4, $_cret(SB)
	JMPTY	__cret
	MOV	*R4, *$(USERADDR+SPOFFSET)

TEXT _cret(SB), $0			/* Hobbit Mask3 bug */
__cret:
	CRET

GLOBL vbase(SB), $VBSIZE
	DATA	vbase+0x00(SB).W, $_vector0(SB)	/* syscall */
	DATA	vbase+0x04(SB).W, $_vector1(SB)	/* exception */
	DATA	vbase+0x08(SB).W, $0xE0000020	/* niladic trap */
	DATA	vbase+0x0C(SB).W, $0xE0000020	/* unimplemented instruction */
	DATA	vbase+0x10(SB).W, $_vector4(SB)	/* non-maskable interrupt */
	DATA	vbase+0x14(SB).W, $_vector5(SB)	/* interrupt 1 */
	DATA	vbase+0x18(SB).W, $_vector6(SB)	/* interrupt 2 */
	DATA	vbase+0x1C(SB).W, $_vector7(SB)	/* interrupt 3 */
	DATA	vbase+0x20(SB).W, $_vector8(SB)	/* interrupt 4 */
	DATA	vbase+0x24(SB).W, $_vector9(SB)	/* interrupt 5 */
	DATA	vbase+0x28(SB).W, $_vectorA(SB)	/* interrupt 6 */
	DATA	vbase+0x2C(SB).W, $_vectorB(SB)	/* timer 1 interrupt */
	DATA	vbase+0x30(SB).W, $_vectorC(SB)	/* timer 2 interrupt */
	DATA	vbase+0x34(SB).W, $_vectorD(SB)	/* FP exception */

TEXT softreset(SB), $0
	MOV	%PSW, *$(SOFTADDR+0x0124)
	MOV	%CONFIG, *$(SOFTADDR+0x0128)
	MOV	R0, *$(SOFTADDR+0x012C)
	MOV	$PSW_VP, %PSW
	MOV	%ISP, %SHAD
	ENTER	R-16
	MOV	$PHYSADDR(SOFTADDR), R8	/* PC */
	MOV	$0, R12			/* PSW */
	MOV	$0, %CONFIG
	FLUSH
	KRET

GLOBL _resetcode(SB), $0x50
	DATA	_resetcode+0x00(SB).UH, $0x2C00	/* MOV	%MSP,*$0x0100 */
	DATA	_resetcode+0x02(SB).UH, $0x867C
	DATA	_resetcode+0x04(SB).UH, $0x0001
	DATA	_resetcode+0x06(SB).UH, $0x0100
	DATA	_resetcode+0x08(SB).UH, $0x2C00	/* MOV	%ISP,*$0x0104 */
	DATA	_resetcode+0x0A(SB).UH, $0x867C
	DATA	_resetcode+0x0C(SB).UH, $0x0002
	DATA	_resetcode+0x0E(SB).UH, $0x0104
	DATA	_resetcode+0x10(SB).UH, $0x2C00	/* MOV	%SP,*$0x0108 */
	DATA	_resetcode+0x12(SB).UH, $0x867C
	DATA	_resetcode+0x14(SB).UH, $0x0003
	DATA	_resetcode+0x16(SB).UH, $0x0108
	DATA	_resetcode+0x18(SB).UH, $0x2C00	/* MOV	%PSW,*$0x010C */
	DATA	_resetcode+0x1A(SB).UH, $0x867C
	DATA	_resetcode+0x1C(SB).UH, $0x0005
	DATA	_resetcode+0x1E(SB).UH, $0x010C
	DATA	_resetcode+0x20(SB).UH, $0x2C00	/* MOV	%CONFIG,*$0x0110 */
	DATA	_resetcode+0x22(SB).UH, $0x867C
	DATA	_resetcode+0x24(SB).UH, $0x0004
	DATA	_resetcode+0x26(SB).UH, $0x0110
	DATA	_resetcode+0x28(SB).UH, $0x2C00	/* MOV	%SHAD,*$0x0114 */
	DATA	_resetcode+0x2A(SB).UH, $0x867C
	DATA	_resetcode+0x2C(SB).UH, $0x0006
	DATA	_resetcode+0x2E(SB).UH, $0x0114
	DATA	_resetcode+0x30(SB).UH, $0x2C00	/* MOV	%VB,*$0x0118 */
	DATA	_resetcode+0x32(SB).UH, $0x867C
	DATA	_resetcode+0x34(SB).UH, $0x0007
	DATA	_resetcode+0x36(SB).UH, $0x0118
	DATA	_resetcode+0x38(SB).UH, $0x2C00	/* MOV	%STB,*$0x011C */
	DATA	_resetcode+0x3A(SB).UH, $0x867C
	DATA	_resetcode+0x3C(SB).UH, $0x0008
	DATA	_resetcode+0x3E(SB).UH, $0x011C
	DATA	_resetcode+0x40(SB).UH, $0x2C00	/* MOV	%FAULT,*$0x0120 */
	DATA	_resetcode+0x42(SB).UH, $0x867C
	DATA	_resetcode+0x44(SB).UH, $0x0009
	DATA	_resetcode+0x46(SB).UH, $0x0120
	DATA	_resetcode+0x48(SB).UH, $0x80F3	/* JMP	*$#40000000 */
	DATA	_resetcode+0x4A(SB).UH, $0x4000
	DATA	_resetcode+0x4C(SB).UH, $0x0000
