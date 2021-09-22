#define SYSTEM		0x73			/* op code */
#define ECALLINST	SYSTEM
#define ISCOMPRESSED(inst) (((inst) & MASK(2)) != 3)

#define LINK	R1
#define USER	6			/* up-> */
#define MACH	7			/* m-> */
#define ARG	8

#define UREGSIZE (XLEN*(32+6))	/* room for pc, regs, csrs, mode, scratch */

#define MMACHNO		0
#define SPLPC		XLEN
#define MACHPROC	(2*XLEN)
#define SAVESR2		(3*XLEN)
#define SAVESR3		(4*XLEN)
#define SAVESR4		(5*XLEN)
#define SAVESR6		(6*XLEN)
#define SAVESR9		(7*XLEN)
#define SAVEMR2		(8*XLEN)
#define SAVEMR3		(9*XLEN)
#define SAVEMR4		(10*XLEN)
#define SAVEMR6		(11*XLEN)
#define SAVEMR9		(12*XLEN)
#define ONLINE		(13*XLEN)
#define MACHHARTID	(ONLINE+4)
#define MMACHMODE	(MACHHARTID+4)

#define RBTRAMP	0
#define RBENTRY XLEN
#define RBCODE	(2*XLEN)
#define RBSIZE	(3*XLEN)
#define RBSTALL	(4*XLEN)
#define RBSATP	(5*XLEN)

#define RBFLAGSET (0xecce|1)		/* ensure low bit set for begin.c */

#define FFLAGS		1
#define FRM		2
#define FCSR		3

#define SUPERBASE	0x100
#define SSTATUS		0x100
#define SEDELEG		0x102
#define SIDELEG		0x103
#define SIE		0x104
#define STVEC		0x105
#define SCOUNTEREN	0x306
#define SSCRATCH	0x140
#define SEPC		0x141
#define SCAUSE		0x142
#define STVAL		0x143
#define SIP		0x144
#define SATP		0x180			/* page table root, paging mode */

#define MCHBASE		0x300
#define MSTATUS		0x300
#define MISA		0x301
#define MEDELEG		0x302
#define MIDELEG		0x303
#define MIE		0x304
#define MTVEC		0x305
#define MCOUNTEREN	0x306
#define MSTATUSH	0x310			/* RV32 only */
#define MSCRATCH	0x340
#define MEPC		0x341
#define MCAUSE		0x342
#define MTVAL		0x343
#define MIP		0x344
#define MCHICKEN	0x7c1

#define CYCLO		0xc00			/* read/write, says lying doc. */
#define TIMELO		0xc01
#define CYCHI		0xc80			/* RV32 only */
#define TIMEHI		0xc81			/* RV32 only */

#define MVENDORID	0xf11
#define MARCHID		0xf12
#define MIMPLID		0xf13
#define MHARTID		0xf14

#define Off		0ull			/* fp: disabled, to detect uses */
#define Initial		1ull			/* fp: on but initialised */
#define Clean		2ull			/* fp: loaded (from fpusave) but not changed */
#define Dirty		3ull			/* fp: fregs or fcsr differ from fpusave */
#define Fsmask		3ull

#define Sie		(1<<1)			/* supervisor-mode intr enable */
#define Mie		(1<<3)			/* machine-mode intr enable */
#define Spie		(1<<5)			/* previous Sie */
#define Ube		(1<<6)			/* user big endian */
#define Mpie		(1<<7)			/* previous Mie (machine only) */
#define Spp		(1<<8)			/* previous priv. mode (super) */
#define Mpp		(3<<11)			/* previous privilege mode (machine only) */
#define Mppuser		(0<<11)
#define Mppsuper	(1<<11)
#define Mpphype		(2<<11)
#define Mppmach		(3<<11)
#define Fsshft		13
#define Fsst		(Fsmask<<Fsshft)
#define Xsshft		15
#define Xs		(3<<Xsshft)
#define Mprv		(1<<17)			/* use Mpp priv.s for memory access (machine) */
#define Sum		(1<<18)			/* super user memory access (super) */
#define Mxr		(1<<19)			/* make executable readable (super) */
#define Tvm		(1<<20)			/* trap vm instrs. (machine) */
#define Tw		(1<<21)			/* timeout wfi (machine) */
#define Tsr		(1<<22)			/* trap SRET in super mode */
#define Sd32		(1<<31)			/* dirty state */
#define Uxlshft		32
#define Uxl		(3ull<<Uxlshft)		/* user xlen */
#define Sxlshft		34
#define Sxl		(3ull<<Sxlshft)		/* super xlen */
#define Sbe64		(1ull<<36)		/* super big endian */
#define Mbe64		(1ull<<37)		/* machine big endian */
#define Sd64		(1ull<<63)		/* dirty state */

#define Sbe		(1<<4)			/* super big endian */
#define Mbe		(1<<5)			/* machine big endian */

#define Mxlen32		1ll
#define Mxlen64		2ll
#define Mxlenmask	3ll

#define Rv64intr	(1ull<<63)		/* interrupt, not exception */

#define Supswintr	1
#define Mchswintr	3
#define Suptmrintr	5
#define Mchtmrintr	7
#define Supextintr	9			/* global intr */
#define Mchextintr	11			/* global intr */
#define Local0intr	16
#define Luartintr	Local0intr		/* this hart's uart */
#define Nlintr		(16+48)			/* # of local interrupts */
#define Msdiff		2			/* M* - S* */

#define Ssie		(1<<Supswintr)		/* super sw */
#define Msie		(1<<Mchswintr)		/* machine sw */
#define Stie		(1<<Suptmrintr)		/* super timer */
#define Mtie		(1<<Mchtmrintr)		/* machine timer */
#define Seie		(1<<Supextintr)		/* super ext (sie); global (mie) */
#define Meie		(1<<Mchextintr)		/* machine external */
#define Li0ie		(1<<Local0intr)		/* local intr 0 enable */
#define Luie		(1<<Luartintr)		/* this hart's uart */

#define Superie		(Seie|Stie|Ssie)
#define Machie		(Meie|Mtie|Msie)

#define Instaddralign	0
#define Instaccess	1
#define Illinst		2
#define Breakpt		3
#define Loadaddralign	4
#define Loadaccess	5
#define Storeaddralign	6
#define Storeaccess	7
#define Envcalluser	8			/* system call from user mode */
#define Envcallsup	9			/* environment call from super mode */
#define Reserved10	10
#define Envcallmch	11			/* environment call from machine mode */
#define Instpage	12			/* page faults */
#define Loadpage	13
#define Reserved14	14
#define Storepage	15
#define Nexc		16

#define Svmask		(VMASK(4)<<60)		/* paging enable modes */
#define Svnone		(0ULL<<60)
#define Sv39		(8ULL<<60)
#define Sv48		(9ULL<<60)
#define Sv57		(10ULL<<60)		/* for future use */
#define Sv64		(11ULL<<60)

#define PteP		0x01			/* Present/valid */
#define PteR		0x02			/* Read */
#define PteW		0x04			/* Write */
#define PteX		0x08			/* Execute */
#define PteU		0x10
#define PteG		0x20			/* Global; used for upper addresses */
#define PteA		0x40			/* Accessed */
#define PteD		0x80			/* Dirty */

#define PteRSW		(MASK(3)<<8)		/* reserved for sw use */
#define PtePS		(1<<8)			/* big page, not 4K */

#define PteLeaf		(PteR|PteW|PteX)	/* if 0, next level; non-0 leaf */
#define Pteleafmand	(PteA|PteD)
#define Pteleafvalid	(Pteleafmand|PteP)
#define PteAttrs	(MASK(6)|PteRSW)

#define Ngintr		187


#define HSM	0x48534dll
#include <atom.h>

/*
 * Macros for accessing page table entries; change the
 * C-style array-index macros into a page table byte offset
 * RV64 PTEs are vlongs, so shift by 3 bits.
 */
// #define PTROOTO(v)	((PTLX((v), 3))<<3)
#define PDPO(v)		((PTLX((v), 2))<<3)
#define PDO(v)		((PTLX((v), 1))<<3)
#define PTO(v)		((PTLX((v), 0))<<3)

#ifndef MASK
#define MASK(w)		((1u  <<(w)) - 1)
#define VMASK(w)	((1ull<<(w)) - 1)
#endif

/* instructions not yet known to ia and ja */
#define ECALL	SYS	$0
#define EBREAK	SYS	$1
#define URET	SYS	$2
#define SRET	SYS	$0x102
#define WFI	SYS	$0x105
#define MRET	SYS	$0x302

/*
 * strap and mtrap macros
 */
#define PUSH(n)		MOV R(n), ((n)*XLEN)(R2)
#define POP(n)		MOV ((n)*XLEN)(R2), R(n)

#define SAVER1_31 \
	PUSH(1); PUSH(2); PUSH(3); PUSH(4); PUSH(5); PUSH(6); PUSH(7); PUSH(8);\
	PUSH(9); PUSH(10); PUSH(11); PUSH(12); PUSH(13); PUSH(14); PUSH(15);\
	PUSH(16); PUSH(17); PUSH(18); PUSH(19); PUSH(20); PUSH(21); PUSH(22);\
	PUSH(23); PUSH(24); PUSH(25); PUSH(26); PUSH(27); PUSH(28); PUSH(29);\
	PUSH(30); PUSH(31)

#define POPR8_31 \
	POP(8); POP(9); POP(10); POP(11); POP(12); POP(13); POP(14); POP(15);\
	POP(16); POP(17); POP(18); POP(19); POP(20); POP(21); POP(22); POP(23);\
	POP(24); POP(25); POP(26); POP(27); POP(28); POP(29); POP(30); POP(31)

#undef	PTLX
#define PTLX(v, l) (((v) >> ((l)*PTSHFT+PGSHFT)) & ((1<<PTSHFT)-1))
