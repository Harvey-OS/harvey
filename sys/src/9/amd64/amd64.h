/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* Cr0 */
#define Pe 0x00000001 /* Protected Mode Enable */
#define Mp 0x00000002 /* Monitor Coprocessor */
#define Em 0x00000004 /* Emulate Coprocessor */
#define Ts 0x00000008 /* Task Switched */
#define Et 0x00000010 /* Extension Type */
#define Ne 0x00000020 /* Numeric Error  */
#define Wp 0x00010000 /* Write Protect */
#define Am 0x00040000 /* Alignment Mask */
#define Nw 0x20000000 /* Not Writethrough */
#define Cd 0x40000000 /* Cache Disable */
#define Pg 0x80000000 /* Paging Enable */

/* Cr3 */
#define Pwt 0x00000008 /* Page-Level Writethrough */
#define Pcd 0x00000010 /* Page-Level Cache Disable */

/* Cr4 */
#define Vme 0x00000001	      /* Virtual-8086 Mode Extensions */
#define Pvi 0x00000002	      /* Protected Mode Virtual Interrupts */
#define Tsd 0x00000004	      /* Time-Stamp Disable */
#define De 0x00000008	      /* Debugging Extensions */
#define Pse 0x00000010	      /* Page-Size Extensions */
#define Pae 0x00000020	      /* Physical Address Extension */
#define Mce 0x00000040	      /* Machine Check Enable */
#define Pge 0x00000080	      /* Page-Global Enable */
#define Pce 0x00000100	      /* Performance Monitoring Counter Enable */
#define Osfxsr 0x00000200     /* FXSAVE/FXRSTOR Support */
#define Osxmmexcpt 0x00000400 /* Unmasked Exception Support */

/* Rflags */
#define Cf 0x00000001	 /* Carry Flag */
#define Pf 0x00000004	 /* Parity Flag */
#define Af 0x00000010	 /* Auxiliary Flag */
#define Zf 0x00000040	 /* Zero Flag */
#define Sf 0x00000080	 /* Sign Flag */
#define Tf 0x00000100	 /* Trap Flag */
#define If 0x00000200	 /* Interrupt Flag */
#define Df 0x00000400	 /* Direction Flag */
#define Of 0x00000800	 /* Overflow Flag */
#define Iopl0 0x00000000 /* I/O Privilege Level */
#define Iopl1 0x00001000
#define Iopl2 0x00002000
#define Iopl3 0x00003000
#define Nt 0x00004000  /* Nested Task */
#define Rf 0x00010000  /* Resume Flag */
#define Vm 0x00020000  /* Virtual-8086 Mode */
#define Ac 0x00040000  /* Alignment Check */
#define Vif 0x00080000 /* Virtual Interrupt Flag */
#define Vip 0x00100000 /* Virtual Interrupt Pending */
#define Id 0x00200000  /* ID Flag */

/* MSRs */
#define PerfEvtbase 0xc0010000 /* Performance Event Select */
#define PerfCtrbase 0xc0010004 /* Performance Counters */

#define Efer 0xc0000080		/* Extended Feature Enable */
#define Star 0xc0000081		/* Legacy Target IP and [CS]S */
#define Lstar 0xc0000082	/* Long Mode Target IP */
#define Cstar 0xc0000083	/* Compatibility Target IP */
#define Sfmask 0xc0000084	/* SYSCALL Flags Mask */
#define FSbase 0xc0000100	/* 64-bit FS Base Address */
#define GSbase 0xc0000101	/* 64-bit GS Base Address */
#define KernelGSbase 0xc0000102 /* SWAPGS instruction */

/* Efer */
#define Sce 0x00000001	 /* System Call Extension */
#define Lme 0x00000100	 /* Long Mode Enable */
#define Lma 0x00000400	 /* Long Mode Active */
#define Nxe 0x00000800	 /* No-Execute Enable */
#define Svme 0x00001000	 /* SVM Extension Enable */
#define Ffxsr 0x00004000 /* Fast FXSAVE/FXRSTOR */

/* PML4E/PDPE/PDE/PTE */
#define PteP 0x0000000000000001	    /* Present */
#define PteRW 0x0000000000000002    /* Read/Write */
#define PteU 0x0000000000000004	    /* User/Supervisor */
#define PtePWT 0x0000000000000008   /* Page-Level Write Through */
#define PtePCD 0x0000000000000010   /* Page Level Cache Disable */
#define PteA 0x0000000000000020	    /* Accessed */
#define PteD 0x0000000000000040	    /* Dirty */
#define PtePS 0x0000000000000080    /* Page Size */
#define Pte4KPAT PtePS		    /* PTE PAT */
#define PteG 0x0000000000000100	    /* Global */
#define Pte2MPAT 0x0000000000001000 /* PDE PAT */
#define Pte1GPAT Pte2MPAT	    /* PDPE PAT */
#define PteNX 0x8000000000000000ULL /* No Execute */

/* Exceptions */
#define IdtDE 0	 /* Divide-by-Zero Error */
#define IdtDB 1	 /* Debug */
#define IdtNMI 2 /* Non-Maskable-Interrupt */
#define IdtBP 3	 /* Breakpoint */
#define IdtOF 4	 /* Overflow */
#define IdtBR 5	 /* Bound-Range */
#define IdtUD 6	 /* Invalid-Opcode */
#define IdtNM 7	 /* Device-Not-Available */
#define IdtDF 8	 /* Double-Fault */
#define Idt09 9	 /* unsupported */
#define IdtTS 10 /* Invalid-TSS */
#define IdtNP 11 /* Segment-Not-Present */
#define IdtSS 12 /* Stack */
#define IdtGP 13 /* General-Protection */
#define IdtPF 14 /* Page-Fault */
#define Idt0F 15 /* reserved */
#define IdtMF 16 /* x87 FPE-Pending */
#define IdtAC 17 /* Alignment-Check */
#define IdtMC 18 /* Machine-Check */
#define IdtXM 19 /* SIMD Floating-Point */

/* Vestigial Segmented Virtual Memory */
#define SdISTM 0x0000000700000000 /* Interrupt Stack Table Mask */
#define SdA 0x0000010000000000	  /* Accessed */
#define SdR 0x0000020000000000	  /* Readable (Code) */
#define SdW 0x0000020000000000	  /* Writeable (Data) */
#define SdE 0x0000040000000000	  /* Expand Down */
#define SdaTSS 0x0000090000000000 /* Available TSS */
#define SdbTSS 0x00000b0000000000 /* Busy TSS */
#define SdCG 0x00000c0000000000	  /* Call Gate */
#define SdIG 0x00000e0000000000	  /* Interrupt Gate */
#define SdTG 0x00000f0000000000	  /* Trap Gate */
#define SdCODE 0x0000080000000000 /* Code/Data */
#define SdS 0x0000100000000000	  /* System/User */
#define SdDPL0 0x0000000000000000 /* Descriptor Privilege Level */
#define SdDPL1 0x0000200000000000
#define SdDPL2 0x0000400000000000
#define SdDPL3 0x0000600000000000
#define SdP 0x0000800000000000	/* Present */
#define Sd4G 0x000f00000000ffff /* 4G Limit */
#define SdL 0x0020000000000000	/* Long Attribute */
#define SdD 0x0040000000000000	/* Default Operand Size */
#define SdG 0x0080000000000000	/* Granularity */

/* Performance Counter Configuration */
#define PeHo 0x0000020000000000	    /* Host only */
#define PeGo 0x0000010000000000	    /* Guest only */
#define PeEvMskH 0x0000000f00000000 /* Event mask H */
#define PeCtMsk 0x00000000ff000000  /* Counter mask */
#define PeInMsk 0x0000000000800000  /* Invert mask */
#define PeCtEna 0x0000000000400000  /* Counter enable */
#define PeInEna 0x0000000000100000  /* Interrupt enable */
#define PePnCtl 0x0000000000080000  /* Pin control */
#define PeEdg 0x0000000000040000    /* Edge detect */
#define PeOS 0x0000000000020000	    /* OS mode */
#define PeUsr 0x0000000000010000    /* User mode */
#define PeUnMsk 0x000000000000ff00  /* Unit Mask */
#define PeEvMskL 0x00000000000000ff /* Event Mask L */

#define PeEvMsksh 32 /* Event mask shift */

/* Segment Selector */
#define SsRPL0 0x0000 /* Requestor Privilege Level */
#define SsRPL1 0x0001
#define SsRPL2 0x0002
#define SsRPL3 0x0003
#define SsTIGDT 0x0000 /* GDT Table Indicator  */
#define SsTILDT 0x0004 /* LDT Table Indicator */
#define SsSIM 0xfff8   /* Selector Index Mask */

#define SSEL(si, tirpl) (((si) << 3) | (tirpl)) /* Segment Selector */

#define SiNULL 0  /* NULL selector index */
#define SiCS 1	  /* CS selector index */
#define SiDS 2	  /* DS selector index */
#define SiU32CS 3 /* User CS selector index */
#define SiUDS 4	  /* User DS selector index */
#define SiUCS 5	  /* User CS selector index */
#define SiFS 6	  /* FS selector index */
#define SiGS 7	  /* GS selector index */
#define SiTSS 8	  /* TSS selector index */
