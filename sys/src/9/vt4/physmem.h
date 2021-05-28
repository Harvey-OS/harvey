/*
 * Memory-mapped IO
 */

/*
 * virtex4 system loses top 1/9th of 128MB to ECC in the secure memory system.
 */
#define MEMTOP(phys)	((((((phys)/32)*8)/9) * 8*BY2WD) & -128)
#define MAXMEM		(128*MB)

/* memory map for rae's virtex4 design */
#define	PHYSDRAM	0
#define PHYSSRAM	0xfffe0000	/* 128K long, in top 128M */

#define	PHYSMMIO	Io

#define	Io		0xf0000000	/* ~512K of IO registers */
#define Uartlite	0xf0000000
#define Gpio		0xf0010000
#define Intctlr		0xf0020000
#define Temac		0xf0030000
#define Llfifo		0xf0040000
#define Dmactlr		0xf0050000
#define Dmactlr2	0xf0060000
/*
 * if these devices exist in a given hardware configuration,
 * they will be at these addresses.
 */
#define Qtm		0xf0070000	/* encrypted memory control */
#define Mpmc		0xf0080000	/* multi-port memory controller */
/* setting low bit interrupts cpu0; don't set Hie */
#define Intctlr2	0xf0090000	/* sw interrupt controller */
