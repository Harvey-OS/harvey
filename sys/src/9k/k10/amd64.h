enum {						/* Cr0 */
	Pe		= 0x00000001,		/* Protected Mode Enable */
	Mp		= 0x00000002,		/* Monitor Coprocessor */
	Em		= 0x00000004,		/* Emulate Coprocessor */
	Ts		= 0x00000008,		/* Task Switched */
	Et		= 0x00000010,		/* Extension Type */
	Ne		= 0x00000020,		/* Numeric Error  */
	Wp		= 0x00010000,		/* Write Protect */
	Am		= 0x00040000,		/* Alignment Mask */
	Nw		= 0x20000000,		/* Not Writethrough */
	Cd		= 0x40000000,		/* Cache Disable */
	Pg		= 0x80000000,		/* Paging Enable */
};

enum {						/* Cr3 */
	Pwt		= 0x00000008,		/* Page-Level Writethrough */
	Pcd		= 0x00000010,		/* Page-Level Cache Disable */
};

enum {						/* Cr4 */
	Vme		= 0x00000001,		/* Virtual-8086 Mode Extensions */
	Pvi		= 0x00000002,		/* Protected Mode Virtual Interrupts */
	Tsd		= 0x00000004,		/* Time-Stamp Disable */
	De		= 0x00000008,		/* Debugging Extensions */
	Pse		= 0x00000010,		/* Page-Size Extensions */
	Pae		= 0x00000020,		/* Physical Address Extension */
	Mce		= 0x00000040,		/* Machine Check Enable */
	Pge		= 0x00000080,		/* Page-Global Enable */
	Pce		= 0x00000100,		/* Performance Monitoring Counter Enable */
	Osfxsr		= 0x00000200,		/* FXSAVE/FXRSTOR Support */
	Osxmmexcpt	= 0x00000400,		/* Unmasked Exception Support */
};

enum {						/* Rflags */
	Cf		= 0x00000001,		/* Carry Flag */
	Pf		= 0x00000004,		/* Parity Flag */
	Af		= 0x00000010,		/* Auxiliary Flag */
	Zf		= 0x00000040,		/* Zero Flag */
	Sf		= 0x00000080,		/* Sign Flag */
	Tf		= 0x00000100,		/* Trap Flag */
	If		= 0x00000200,		/* Interrupt Flag */
	Df		= 0x00000400,		/* Direction Flag */
	Of		= 0x00000800,		/* Overflow Flag */
	Iopl0		= 0x00000000,		/* I/O Privilege Level */
	Iopl1		= 0x00001000,
	Iopl2		= 0x00002000,
	Iopl3		= 0x00003000,
	Nt		= 0x00004000,		/* Nested Task */
	Rf		= 0x00010000,		/* Resume Flag */
	Vm		= 0x00020000,		/* Virtual-8086 Mode */
	Ac		= 0x00040000,		/* Alignment Check */
	Vif		= 0x00080000,		/* Virtual Interrupt Flag */
	Vip		= 0x00100000,		/* Virtual Interrupt Pending */
	Id		= 0x00200000,		/* ID Flag */
};

enum {						/* MSRs */
	Efer		= 0xc0000080,		/* Extended Feature Enable */
	Star		= 0xc0000081,		/* Legacy Target IP and [CS]S */
	Lstar		= 0xc0000082,		/* Long Mode Target IP */
	Cstar		= 0xc0000083,		/* Compatibility Target IP */
	Sfmask		= 0xc0000084,		/* SYSCALL Flags Mask */
	FSbase		= 0xc0000100,		/* 64-bit FS Base Address */
	GSbase		= 0xc0000101,		/* 64-bit GS Base Address */
	KernelGSbase	= 0xc0000102,		/* SWAPGS instruction */
};

enum {						/* Efer */
	Sce		= 0x00000001,		/* System Call Extension */
	Lme		= 0x00000100,		/* Long Mode Enable */
	Lma		= 0x00000400,		/* Long Mode Active */
	Nxe		= 0x00000800,		/* No-Execute Enable */
	Svme		= 0x00001000,		/* SVM Extension Enable */
	Ffxsr		= 0x00004000,		/* Fast FXSAVE/FXRSTOR */
};

enum {						/* PML4E/PDPE/PDE/PTE */
	PteP		= 0x0000000000000001ull,/* Present */
	PteRW		= 0x0000000000000002ull,/* Read/Write */
	PteU		= 0x0000000000000004ull,/* User/Supervisor */
	PtePWT		= 0x0000000000000008ull,/* Page-Level Write Through */
	PtePCD		= 0x0000000000000010ull,/* Page Level Cache Disable */
	PteA		= 0x0000000000000020ull,/* Accessed */
	PteD		= 0x0000000000000040ull,/* Dirty */
	PtePS		= 0x0000000000000080ull,/* Page Size */
	Pte4KPAT	= PtePS,		/* PTE PAT */
	PteG		= 0x0000000000000100ull,/* Global */
	Pte2MPAT	= 0x0000000000001000ull,/* PDE PAT */
	Pte1GPAT	= Pte2MPAT,		/* PDPE PAT */
	PteNX		= 0x8000000000000000ull,/* No Execute */
};

enum {						/* Exceptions */
	IdtDE		= 0,			/* Divide-by-Zero Error */
	IdtDB		= 1,			/* Debug */
	IdtNMI		= 2,			/* Non-Maskable-Interrupt */
	IdtBP		= 3,			/* Breakpoint */
	IdtOF		= 4,			/* Overflow */
	IdtBR		= 5,			/* Bound-Range */
	IdtUD		= 6,			/* Invalid-Opcode */
	IdtNM		= 7,			/* Device-Not-Available */
	IdtDF		= 8,			/* Double-Fault */
	Idt09		= 9,			/* unsupported */
	IdtTS		= 10,			/* Invalid-TSS */
	IdtNP		= 11,			/* Segment-Not-Present */
	IdtSS		= 12,			/* Stack */
	IdtGP		= 13,			/* General-Protection */
	IdtPF		= 14,			/* Page-Fault */
	Idt0F		= 15,			/* reserved */
	IdtMF		= 16,			/* x87 FPE-Pending */
	IdtAC		= 17,			/* Alignment-Check */
	IdtMC		= 18,			/* Machine-Check */
	IdtXF		= 19,			/* SIMD Floating-Point */
};

/*
 * Vestigial Segmented Virtual Memory.
 */
enum {						/* Segment Descriptor */
	SdISTM		= 0x0000000700000000ull,/* Interrupt Stack Table Mask */
	SdA		= 0x0000010000000000ull,/* Accessed */
	SdR		= 0x0000020000000000ull,/* Readable (Code) */
	SdW		= 0x0000020000000000ull,/* Writeable (Data) */
	SdE		= 0x0000040000000000ull,/* Expand Down */
	SdaTSS		= 0x0000090000000000ull,/* Available TSS */
	SdbTSS		= 0x00000b0000000000ull,/* Busy TSS */
	SdCG		= 0x00000c0000000000ull,/* Call Gate */
	SdIG		= 0x00000e0000000000ull,/* Interrupt Gate */
	SdTG		= 0x00000f0000000000ull,/* Trap Gate */
	SdCODE		= 0x0000080000000000ull,/* Code/Data */
	SdS		= 0x0000100000000000ull,/* System/User */
	SdDPL0		= 0x0000000000000000ull,/* Descriptor Privilege Level */
	SdDPL1		= 0x0000200000000000ull,
	SdDPL2		= 0x0000400000000000ull,
	SdDPL3		= 0x0000600000000000ull,
	SdP		= 0x0000800000000000ull,/* Present */
	Sd4G		= 0x000f00000000ffffull,/* 4G Limit */
	SdL		= 0x0020000000000000ull,/* Long Attribute */
	SdD		= 0x0040000000000000ull,/* Default Operand Size */
	SdG		= 0x0080000000000000ull,/* Granularity */
};

enum {						/* Segment Selector */
	SsRPL0		= 0x0000,		/* Requestor Privilege Level */
	SsRPL1		= 0x0001,
	SsRPL2		= 0x0002,
	SsRPL3		= 0x0003,
	SsTIGDT		= 0x0000,		/* GDT Table Indicator  */
	SsTILDT		= 0x0004,		/* LDT Table Indicator */
	SsSIM		= 0xfff8,		/* Selector Index Mask */
};

#define SSEL(si, tirpl)	(((si)<<3)|(tirpl))	/* Segment Selector */

enum {
	SiNULL		= 0,			/* NULL selector index */
	SiCS		= 1,			/* CS selector index */
	SiDS		= 2,			/* DS selector index */
	SiU32CS		= 3,			/* User CS selector index */
	SiUDS		= 4,			/* User DS selector index */
	SiUCS		= 5,			/* User CS selector index */
	SiFS		= 6,			/* FS selector index */
	SiGS		= 7,			/* GS selector index */
	SiTSS		= 8,			/* TSS selector index */
};

/*
 * Extern registers.
 */
#define RMACH		R15			/* m-> */
#define RUSER		R14			/* up-> */
