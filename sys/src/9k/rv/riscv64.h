/*
 * risc-v definitions
 */
#define SYSTEM		0x73			/* op code */
#define ECALLINST	SYSTEM
#define ISCOMPRESSED(inst) (((inst) & MASK(2)) != 3)

/* registers */
#define LINK	R1
#define USER	6			/* up-> */
#define MACH	7			/* m-> */
#define ARG	8

#define UREGSIZE (XLEN*(32+6))	/* room for pc, regs, csrs, mode, scratch */

/*
 * byte offsets into Mach, m, (mainly for m->regsave), for assembler,
 * where XLEN is defined.
 */
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

/*
 * byte offsets into Sys->Reboot.
 */
#define RBTRAMP	0
#define RBENTRY XLEN
#define RBCODE	(2*XLEN)
#define RBSIZE	(3*XLEN)
#define RBSTALL	(4*XLEN)
#define RBSATP	(5*XLEN)

/* magic numbers for flags */
#define RBFLAGSET (0xecce|1)		/* ensure low bit set for begin.c */

enum {					/* CSRs */
	FFLAGS	= 1,
	FRM	= 2,
	FCSR	= 3,

	SUPERBASE = 0x100,
	SSTATUS	= 0x100,
	SEDELEG	= 0x102,
	SIDELEG	= 0x103,
	SIE	= 0x104,
	STVEC	= 0x105,
	SCOUNTEREN = 0x306,
	SSCRATCH = 0x140,
	SEPC	= 0x141,
	SCAUSE	= 0x142,
	STVAL	= 0x143,
	SIP	= 0x144,
	SATP	= 0x180,		/* page table root, paging mode */

	MCHBASE	= 0x300,
	MSTATUS	= 0x300,
	MISA	= 0x301,
	MEDELEG	= 0x302,
	MIDELEG	= 0x303,
	MIE	= 0x304,
	MTVEC	= 0x305,
	MCOUNTEREN = 0x306,
	MSTATUSH = 0x310,		/* RV32 only */
	MSCRATCH = 0x340,
	MEPC	= 0x341,
	MCAUSE	= 0x342,
	MTVAL	= 0x343,
	MIP	= 0x344,
	/* undocumented `chicken bits': disable features */
	MCHICKEN = 0x7c1,

	CYCLO	= 0xc00,		/* read/write, says lying doc. */
	TIMELO	= 0xc01,
	CYCHI	= 0xc80,		/* RV32 only */
	TIMEHI	= 0xc81,		/* RV32 only */

	MVENDORID = 0xf11,
	MARCHID	= 0xf12,
	MIMPLID	= 0xf13,
	MHARTID	= 0xf14,
};

enum {				/* xs & fs values in ?status */
	Off	= 0ull,		/* fp: disabled, to detect uses */
				/* will cause ill instr traps on fp use */
	Initial	= 1ull,		/* fp: on but initialised */
	Clean	= 2ull,		/* fp: loaded (from fpusave) but not changed */
	Dirty	= 3ull,		/* fp: fregs or fcsr differ from fpusave */
	Fsmask	= 3ull,
};

enum {				/* [ms]status bits for RV32 & RV64 */
	Sie	= (1<<1),	/* supervisor-mode intr enable */
	Mie	= (1<<3),	/* machine-mode intr enable */
	Spie	= (1<<5),	/* previous Sie */
	Ube	= (1<<6),	/* user big endian */
	Mpie	= (1<<7),	/* previous Mie (machine only) */
	Spp	= (1<<8),	/* previous priv. mode (super) */
	Mpp	= (3<<11),	/* previous privilege mode (machine only) */
	Mppuser	= (0<<11),
	Mppsuper = (1<<11),
	Mpphype	= (2<<11),
	Mppmach	= (3<<11),
	Fsshft	= 13,
	Fsst	= (Fsmask<<Fsshft),
	Xsshft	= 15,
	Xs	= (3<<Xsshft),
	Mprv	= (1<<17),	/* use Mpp priv.s for memory access (machine) */
	Sum	= (1<<18),	/* super user memory access (super) */
	Mxr	= (1<<19),	/* make executable readable (super) */
	Tvm	= (1<<20),	/* trap vm instrs. (machine) */
	Tw	= (1<<21),	/* timeout wfi (machine) */
	Tsr	= (1<<22),	/* trap SRET in super mode */
	Sd32	= (1<<31),	/* dirty state */
	Uxlshft	= 32,
	Uxl	= (3ull<<Uxlshft), /* user xlen */
	Sxlshft	= 34,
	Sxl	= (3ull<<Sxlshft), /* super xlen */
	Sbe64	= (1ull<<36),	/* super big endian */
	Mbe64	= (1ull<<37),	/* machine big endian */
	Sd64	= (1ull<<63),	/* dirty state */

	/* mstatush (rv32 only) */
	Sbe	= (1<<4),	/* super big endian */
	Mbe	= (1<<5),	/* machine big endian */
};

enum {
	Mxlen32	= 1ll,
	Mxlen64 = 2ll,
	Mxlenmask = 3ll,
};

enum {
	Rv64intr	= (1ull<<63), /* interrupt, not exception */
};

enum {				/* cause exception codes for interrupts */
	Supswintr	= 1,
	Mchswintr	= 3,
	Suptmrintr	= 5,
	Mchtmrintr	= 7,
	Supextintr	= 9,		/* global intr */
	Mchextintr	= 11,		/* global intr */
	Local0intr	= 16,
	Luartintr	= Local0intr + 11,	/* this hart's uart */
	Nlintr		= (16+48),	/* # of local interrupts */
	Msdiff		= 2,		/* M* - S* */
};

/* local interrupt enables */
enum {			/* [ms]i[ep] bits for RV64.  only S* bits in si[ep] */
	Ssie	= (1<<Supswintr),	/* super sw */
	Msie	= (1<<Mchswintr),	/* machine sw */
	Stie	= (1<<Suptmrintr),	/* super timer */
	Mtie	= (1<<Mchtmrintr),	/* machine timer */
	Seie	= (1<<Supextintr),	/* super ext (sie); global (mie) */
	Meie	= (1<<Mchextintr),	/* machine external */
	Li0ie	= (1<<Local0intr),	/* local intr 0 enable */
	Luie	= (1<<Luartintr),	/* this hart's uart */

	Superie	= (Seie|Stie|Ssie),
	Machie	= (Meie|Mtie|Msie),
};

enum {				/* cause exception codes for traps */
	Instaddralign	= 0,
	Instaccess	= 1,
	Illinst		= 2,
	Breakpt		= 3,
	Loadaddralign	= 4,
	Loadaccess	= 5,
	Storeaddralign	= 6,
	Storeaccess	= 7,
	Envcalluser	= 8,		/* system call from user mode */
	Envcallsup	= 9,		/* environment call from super mode */
	Reserved10	= 10,
	Envcallmch	= 11,		/* environment call from machine mode */
	Instpage	= 12,		/* page faults */
	Loadpage	= 13,
	Reserved14	= 14,
	Storepage	= 15,
	Nexc		= 16,
};

enum {					/* CSR(SATP) */
	Svmask		= (VMASK(4)<<60), /* paging enable modes */
	Svnone		= (0ULL<<60),
	Sv39		= (8ULL<<60),
	Sv48		= (9ULL<<60),
	Sv57		= (10ULL<<60),	/* for future use */
	Sv64		= (11ULL<<60),
};

enum {					/* PTE; see mem.h PTE* */
	PteP		= 0x01,		/* Present/valid */
	PteR		= 0x02,		/* Read */
	PteW		= 0x04,		/* Write */
	PteX		= 0x08,		/* Execute */
	/* page accessible to U mode; only usable on leaves */
	PteU		= 0x10,
	PteG		= 0x20,		/* Global; used for upper addresses */
	PteA		= 0x40,		/* Accessed */
	PteD		= 0x80,		/* Dirty */

	PteRSW		= (MASK(3)<<8),	/* reserved for sw use */
	PtePS		= (1<<8),	/* big page, not 4K */
/*	PtePtr		= (1<<9),	/* ptr to next llvl pt (PteLeaf=0) */

	PteLeaf		= (PteR|PteW|PteX), /* if 0, next level; non-0 leaf */
	/* some implementations require A & D in leaf ptes to avoid faults */
	Pteleafmand	= (PteA|PteD),
	Pteleafvalid	= (Pteleafmand|PteP),
	PteAttrs	= (MASK(6)|PteRSW),
};

enum {
	/*
	 * there are 1024 possible interrupt sources, but we don't
	 * use them all.  icicle uses 187, beagle v uses fewer.
	 */
	Ngintr		= 187,

	// Gliirqdiff	= 13,	/* 1 zero + 4 l2 cache + 8 dma */
};

/* SBI stuff */
#define HSM	0x48534dll
