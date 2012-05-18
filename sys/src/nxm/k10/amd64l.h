#define Pe		0x00000001		/* Protected Mode Enable */
#define Mp		0x00000002		/* Monitor Coprocessor */
#define Em		0x00000004		/* Emulate Coprocessor */
#define Ts		0x00000008		/* Task Switched */
#define Et		0x00000010		/* Extension Type */
#define Ne		0x00000020		/* Numeric Error  */
#define Wp		0x00010000		/* Write Protect */
#define Am		0x00040000		/* Alignment Mask */
#define Nw		0x20000000		/* Not Writethrough */
#define Cd		0x40000000		/* Cache Disable */
#define Pg		0x80000000		/* Paging Enable */

#define Pwt		0x00000008		/* Page-Level Writethrough */
#define Pcd		0x00000010		/* Page-Level Cache Disable */

#define Vme		0x00000001		/* Virtual-8086 Mode Extensions */
#define Pvi		0x00000002		/* Protected Mode Virtual Interrupts */
#define Tsd		0x00000004		/* Time-Stamp Disable */
#define De		0x00000008		/* Debugging Extensions */
#define Pse		0x00000010		/* Page-Size Extensions */
#define Pae		0x00000020		/* Physical Address Extension */
#define Mce		0x00000040		/* Machine Check Enable */
#define Pge		0x00000080		/* Page-Global Enable */
#define Pce		0x00000100		/* Performance Monitoring Counter Enable */
#define Osfxsr		0x00000200		/* FXSAVE/FXRSTOR Support */
#define Osxmmexcpt	0x00000400		/* Unmasked Exception Support */

#define Cf		0x00000001		/* Carry Flag */
#define Pf		0x00000004		/* Parity Flag */
#define Af		0x00000010		/* Auxiliary Flag */
#define Zf		0x00000040		/* Zero Flag */
#define Sf		0x00000080		/* Sign Flag */
#define Tf		0x00000100		/* Trap Flag */
#define If		0x00000200		/* Interrupt Flag */
#define Df		0x00000400		/* Direction Flag */
#define Of		0x00000800		/* Overflow Flag */
#define Iopl0		0x00000000		/* I/O Privilege Level */
#define Iopl1		0x00001000
#define Iopl2		0x00002000
#define Iopl3		0x00003000
#define Nt		0x00004000		/* Nested Task */
#define Rf		0x00010000		/* Resume Flag */
#define Vm		0x00020000		/* Virtual-8086 Mode */
#define Ac		0x00040000		/* Alignment Check */
#define Vif		0x00080000		/* Virtual Interrupt Flag */
#define Vip		0x00100000		/* Virtual Interrupt Pending */
#define Id		0x00200000		/* ID Flag */

#define PerfEvtbase	0xc0010000		/* Performance Event Select */
#define PerfCtrbase	0xc0010004		/* Performance Counters */

#define Efer		0xc0000080		/* Extended Feature Enable */
#define Star		0xc0000081		/* Legacy Target IP and [CS]S */
#define Lstar		0xc0000082		/* Long Mode Target IP */
#define Cstar		0xc0000083		/* Compatibility Target IP */
#define Sfmask		0xc0000084		/* SYSCALL Flags Mask */
#define FSbase		0xc0000100		/* 64-bit FS Base Address */
#define GSbase		0xc0000101		/* 64-bit GS Base Address */
#define KernelGSbase	0xc0000102		/* SWAPGS instruction */

#define Sce		0x00000001		/* System Call Extension */
#define Lme		0x00000100		/* Long Mode Enable */
#define Lma		0x00000400		/* Long Mode Active */
#define Nxe		0x00000800		/* No-Execute Enable */
#define Svme		0x00001000		/* SVM Extension Enable */
#define Ffxsr		0x00004000		/* Fast FXSAVE/FXRSTOR */

#define PteP		0x0000000000000001ull	/* Present */
#define PteRW		0x0000000000000002ull	/* Read/Write */
#define PteU		0x0000000000000004ull	/* User/Supervisor */
#define PtePWT		0x0000000000000008ull	/* Page-Level Write Through */
#define PtePCD		0x0000000000000010ull	/* Page Level Cache Disable */
#define PteA		0x0000000000000020ull	/* Accessed */
#define PteD		0x0000000000000040ull	/* Dirty */
#define PtePS		0x0000000000000080ull	/* Page Size */
#define Pte4KPAT	PtePS			/* PTE PAT */
#define PteG		0x0000000000000100ull	/* Global */
#define Pte2MPAT	0x0000000000001000ull	/* PDE PAT */
#define Pte1GPAT	Pte2MPAT		/* PDPE PAT */
#define PteNX		0x8000000000000000ull	/* No Execute */

#define IdtDE		0			/* Divide-by-Zero Error */
#define IdtDB		1			/* Debug */
#define IdtNMI		2			/* Non-Maskable-Interrupt */
#define IdtBP		3			/* Breakpoint */
#define IdtOF		4			/* Overflow */
#define IdtBR		5			/* Bound-Range */
#define IdtUD		6			/* Invalid-Opcode */
#define IdtNM		7			/* Device-Not-Available */
#define IdtDF		8			/* Double-Fault */
#define Idt09		9			/* unsupported */
#define IdtTS		10			/* Invalid-TSS */
#define IdtNP		11			/* Segment-Not-Present */
#define IdtSS		12			/* Stack */
#define IdtGP		13			/* General-Protection */
#define IdtPF		14			/* Page-Fault */
#define Idt0F		15			/* reserved */
#define IdtMF		16			/* x87 FPE-Pending */
#define IdtAC		17			/* Alignment-Check */
#define IdtMC		18			/* Machine-Check */
#define IdtXF		19			/* SIMD Floating-Point */

#define SdISTM		0x0000000700000000ull	/* Interrupt Stack Table Mask */
#define SdA		0x0000010000000000ull	/* Accessed */
#define SdR		0x0000020000000000ull	/* Readable (Code) */
#define SdW		0x0000020000000000ull	/* Writeable (Data) */
#define SdE		0x0000040000000000ull	/* Expand Down */
#define SdaTSS		0x0000090000000000ull	/* Available TSS */
#define SdbTSS		0x00000b0000000000ull	/* Busy TSS */
#define SdCG		0x00000c0000000000ull	/* Call Gate */
#define SdIG		0x00000e0000000000ull	/* Interrupt Gate */
#define SdTG		0x00000f0000000000ull	/* Trap Gate */
#define SdCODE		0x0000080000000000ull	/* Code/Data */
#define SdS		0x0000100000000000ull	/* System/User */
#define SdDPL0		0x0000000000000000ull	/* Descriptor Privilege Level */
#define SdDPL1		0x0000200000000000ull
#define SdDPL2		0x0000400000000000ull
#define SdDPL3		0x0000600000000000ull
#define SdP		0x0000800000000000ull	/* Present */
#define Sd4G		0x000f00000000ffffull	/* 4G Limit */
#define SdL		0x0020000000000000ull	/* Long Attribute */
#define SdD		0x0040000000000000ull	/* Default Operand Size */
#define SdG		0x0080000000000000ull	/* Granularity */

#define PeHo		0x0000020000000000ull	/* Host only */
#define PeGo		0x0000010000000000ull	/* Guest only */
#define PeEvMskH	0x0000000f00000000ull	/* Event mask H */
#define PeCtMsk		0x00000000ff000000ull	/* Counter mask */
#define PeInMsk		0x0000000000800000ull	/* Invert mask */
#define PeCtEna		0x0000000000400000ull	/* Counter enable */
#define PeInEna		0x0000000000100000ull	/* Interrupt enable */
#define PePnCtl		0x0000000000080000ull	/* Pin control */
#define PeEdg		0x0000000000040000ull	/* Edge detect */
#define PeOS		0x0000000000020000ull	/* OS mode */
#define PeUsr		0x0000000000010000ull	/* User mode */
#define PeUnMsk		0x000000000000ff00ull	/* Unit Mask */
#define PeEvMskL	0x00000000000000ffull	/* Event Mask L */

#define PeEvMsksh	32ull			/* Event mask shift */

#define SsRPL0		0x0000			/* Requestor Privilege Level */
#define SsRPL1		0x0001
#define SsRPL2		0x0002
#define SsRPL3		0x0003
#define SsTIGDT		0x0000			/* GDT Table Indicator  */
#define SsTILDT		0x0004			/* LDT Table Indicator */
#define SsSIM		0xfff8			/* Selector Index Mask */

#define SSEL(si, tirpl)	(((si)<<3)|(tirpl))	/* Segment Selector */

#define SiNULL		0			/* NULL selector index */
#define SiCS		1			/* CS selector index */
#define SiDS		2			/* DS selector index */
#define SiU32CS		3			/* User CS selector index */
#define SiUDS		4			/* User DS selector index */
#define SiUCS		5			/* User CS selector index */
#define SiFS		6			/* FS selector index */
#define SiGS		7			/* GS selector index */
#define SiTSS		8			/* TSS selector index */

#define RMACH		R15			/* m-> */
#define RUSER		R14			/* up-> */
