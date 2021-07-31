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
