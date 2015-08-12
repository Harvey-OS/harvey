/* 
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

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

#define Pwt 0x00000008 /* Page-Level Writethrough */
#define Pcd 0x00000010 /* Page-Level Cache Disable */

#define Vme 0x00000001	/* Virtual-8086 Mode Extensions */
#define Pvi 0x00000002	/* Protected Mode Virtual Interrupts */
#define Tsd 0x00000004	/* Time-Stamp Disable */
#define De 0x00000008	 /* Debugging Extensions */
#define Pse 0x00000010	/* Page-Size Extensions */
#define Pae 0x00000020	/* Physical Address Extension */
#define Mce 0x00000040	/* Machine Check Enable */
#define Pge 0x00000080	/* Page-Global Enable */
#define Pce 0x00000100	/* Performance Monitoring Counter Enable */
#define Osfxsr 0x00000200     /* FXSAVE/FXRSTOR Support */
#define Osxmmexcpt 0x00000400 /* Unmasked Exception Support */

#define Cf 0x00000001    /* Carry Flag */
#define Pf 0x00000004    /* Parity Flag */
#define Af 0x00000010    /* Auxiliary Flag */
#define Zf 0x00000040    /* Zero Flag */
#define Sf 0x00000080    /* Sign Flag */
#define Tf 0x00000100    /* Trap Flag */
#define If 0x00000200    /* Interrupt Flag */
#define Df 0x00000400    /* Direction Flag */
#define Of 0x00000800    /* Overflow Flag */
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

#define PteP 0x00000001ul    /* Present */
#define PteRW 0x00000002ul   /* Read/Write */
#define PteU 0x00000004ul    /* User/Supervisor */
#define PtePWT 0x00000008ul  /* Page-Level Write Through */
#define PtePCD 0x00000010ul  /* Page Level Cache Disable */
#define PteA 0x00000020ul    /* Accessed */
#define PteD 0x00000040ul    /* Dirty */
#define PtePS 0x00000080ul   /* Page Size */
#define PtePAT0 PtePS	/* Level 0 PAT */
#define PteG 0x00000100ul    /* Global */
#define PtePAT1 0x00000800ul /* Level 1 PAT */

#define IdtDE 0x00  /* Divide-by-Zero Error */
#define IdtDB 0x01  /* Debug */
#define IdtNMI 0x02 /* Non-Maskable-Interrupt */
#define IdtBP 0x03  /* Breakpoint */
#define IdtOF 0x04  /* Overflow */
#define IdtBR 0x05  /* Bound-Range */
#define IdtUD 0x06  /* Invalid-Opcode */
#define IdtNM 0x07  /* Device-Not-Available */
#define IdtDF 0x08  /* Double-Fault */
#define Idt09 0x09  /* unsupported */
#define IdtTS 0x0a  /* Invalid-TSS */
#define IdtNP 0x0b  /* Segment-Not-Present */
#define IdtSS 0x0c  /* Stack */
#define IdtGP 0x0d  /* General-Protection */
#define IdtPF 0x0e  /* Page-Fault */
#define Idt0F 0x0f  /* reserved */
#define IdtMF 0x10  /* x87 FPE-Pending */
#define IdtAC 0x11  /* Alignment-Check */
#define IdtMC 0x12  /* Machine-Check */
#define IdtXF 0x13  /* SIMD Floating-Point */

#define SdISTM 0x0000000700000000ull /* Interrupt Stack Table Mask */
#define SdA 0x0000010000000000ull    /* Accessed */
#define SdR 0x0000020000000000ull    /* Readable (Code) */
#define SdW 0x0000020000000000ull    /* Writeable (Data) */
#define SdE 0x0000040000000000ull    /* Expand Down */
#define SdaTSS 0x0000090000000000ull /* Available TSS */
#define SdbTSS 0x00000b0000000000ull /* Busy TSS */
#define SdCG 0x00000c0000000000ull   /* Call Gate */
#define SdIG 0x00000e0000000000ull   /* Interrupt Gate */
#define SdTG 0x00000f0000000000ull   /* Trap Gate */
#define SdCODE 0x0000080000000000ull /* Code/Data */
#define SdS 0x0000100000000000ull    /* System/User */
#define SdDPL0 0x0000000000000000ull /* Descriptor Privilege Level */
#define SdDPL1 0x0000200000000000ull
#define SdDPL2 0x0000400000000000ull
#define SdDPL3 0x0000600000000000ull
#define SdP 0x0000800000000000ull  /* Present */
#define Sd4G 0x000f00000000ffffull /* 4G Limit */
#define SdL 0x0020000000000000ull  /* Long Attribute */
#define SdD 0x0040000000000000ull  /* Default Operand Size */
#define SdG 0x0080000000000000ull  /* Granularity */

#define SsRPL0 0x0000 /* Requestor Privilege Level */
#define SsRPL1 0x0001
#define SsRPL2 0x0002
#define SsRPL3 0x0003
#define SsTIGDT 0x0000 /* GDT Table Indicator  */
#define SsTILDT 0x0004 /* LDT Table Indicator */
#define SsSIM 0xfff8   /* Selector Index Mask */

#define SSEL(si, tirpl) (((si) << 3) | (tirpl)) /* Segment Selector */

#define SiNULL 0 /* NULL selector index */
#define SiCS 1   /* CS selector index */
#define SiDS 2   /* DS selector index */
#define SiLCS 3  /* Legacy CS selector index */
#define SiUDS 4  /* User DS selector index */
#define SiUCS 5  /* User CS selector index */
#define SiTSS 6  /* TSS selector index */
