/* 
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

enum {			  /* MSRs */
       Efer = 0xC0000080, /* Extended Feature Enable */
};

enum {			/* Cr0 */
       Pe = 0x00000001, /* Protected Mode Enable */
       Mp = 0x00000002, /* Monitor Coprocessor */
       Em = 0x00000004, /* Emulate Coprocessor */
       Ts = 0x00000008, /* Task Switched */
       Et = 0x00000010, /* Extension Type */
       Ne = 0x00000020, /* Numeric Error  */
       Wp = 0x00010000, /* Write Protect */
       Am = 0x00040000, /* Alignment Mask */
       Nw = 0x20000000, /* Not Writethrough */
       Cd = 0x40000000, /* Cache Disable */
       Pg = 0x80000000, /* Paging Enable */
};

enum {			 /* Cr3 */
       Pwt = 0x00000008, /* Page-Level Writethrough */
       Pcd = 0x00000010, /* Page-Level Cache Disable */
};

enum {			 /* Cr4 */
       Vme = 0x00000001, /* Virtual-8086 Mode Extensions */
       Pvi = 0x00000002, /* Protected Mode Virtual Interrupts */
       //Tsd		= 0x00000004,		/* Time-Stamp Disable */
       De = 0x00000008,		/* Debugging Extensions */
       Pse = 0x00000010,	/* Page-Size Extensions */
       Pae = 0x00000020,	/* Physical Address Extension */
       Mce = 0x00000040,	/* Machine Check Enable */
       Pge = 0x00000080,	/* Page-Global Enable */
       Pce = 0x00000100,	/* Performance Monitoring Counter Enable */
       Osfxsr = 0x00000200,     /* FXSAVE/FXRSTOR Support */
       Osxmmexcpt = 0x00000400, /* Unmasked Exception Support */
};

enum {			   /* Efer */
       Sce = 0x00000001,   /* System Call Extension */
       Lme = 0x00000100,   /* Long Mode Enable */
       Lma = 0x00000400,   /* Long Mode Active */
       Nxe = 0x00000800,   /* No-Execute Enable */
       Ffxsr = 0x00004000, /* Fast FXSAVE/FXRSTOR */
};

enum {					/* Pte */
       PteP = 0x0000000000000001ULL,    /* Present */
       PteRW = 0x0000000000000002ULL,   /* Read/Write */
       PteUS = 0x0000000000000004ULL,   /* User/Supervisor */
       PtePWT = 0x0000000000000008ULL,  /* Page-Level Write Through */
       PtePCD = 0x0000000000000010ULL,  /* Page Level Cache Disable */
       PteA = 0x0000000000000020ULL,    /* Accessed */
       PteD = 0x0000000000000040ULL,    /* Dirty */
       PtePS = 0x0000000000000080ULL,   /* Page Size */
       PtePAT0 = PtePS,			/* Level 0 PAT */
       PteG = 0x0000000000000100ULL,    /* Global */
       PtePAT1 = 0x0000000000000800ULL, /* Level 1 PAT */
       PteNX = 0x8000000000000000ULL,   /* No Execute */
};

typedef u64int PTE;

#define KZERO64 0xFFFFFFFF80000000ULL

#define PGSIZE64 4096
#define PGSHIFT64 12 /* log(PGSIZE) */
#define PTESIZE64 512
#define PTESHIFT64 9 /* log(PTESIZE) */

#define PTEX64(pi, l) (((pi) >> ((((l)-1) * 9) + 12)) & ((1 << PTESHIFT64) - 1))
#define PPN64(pi) ((pi) & ~((1 << PTESHIFT64) - 1))
