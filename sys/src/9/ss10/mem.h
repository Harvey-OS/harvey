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
#define	MAXSIMM			8			/* Maximum number of SIMM slots */

/*
 * Time
 */
#define	HZ		50			/* clock frequency */
#define	MS2HZ		(1000/HZ)		/* millisec per clock tick */
#define	TK2SEC(t)	((t)/HZ)		/* ticks to seconds */
#define	TK2MS(t)	((((ulong)(t))*1000)/HZ)	/* ticks to milliseconds */
#define	MS2TK(t)	((((ulong)(t))*HZ)/1000)	/* milliseconds to ticks */

/*
 * PSR bits
 */
#define	PSRIMPL		0xF0000000
#define	PSRVER		0x0F000000
#define	PSRRESERVED	0x000FC000
#define	PSREC		0x00002000
#define	PSREF		0x00001000
#define	PSRPIL		0x00000F00
#define	PSRSUPER	0x00000080
#define	PSRPSUPER	0x00000040
#define	PSRET		0x00000020
#define	PSRCWP		0x0000001F
#define	SPL(n)		(n<<8)

/*
 * Magic registers
 */

#define	MACH		6		/* R6 is m-> */
#define	USER		5		/* R5 is u-> */

/*
 * Fundamental addresses
 */

#define	USERADDR	0xE0000000
#define	UREGADDR	(USERADDR+BY2PG-((32+6)*BY2WD))
#define	BOOTSTACK	(KTZERO-0*BY2PG)
#define	TRAPS		(KTZERO-2*BY2PG)

/*
 * Module Control Register
 */
							/* Description			Access	E$	NE	*/
#define	MCRIMPL		(0xF<<28)
#define	MCRVER		(0xF<<24)
#define	ENABPRE		(1<<18)	/* data prefetch enable 	rw			X	*/
#define	TWCACHE		(1<<16)	/* table-walk cacheable	rw		1	0	*/
#define	ALTCACHE	(1<<15)	/* alternate cacheable 	rw				*/
#define	ENABSNOOP	(1<<14)	/* snoop enable 		rw				*/
#define	BOOTMODE	(1<<13)	/* `must be cleared for normal operation' 	*/
#define	MEMPCHECK	(1<<12)	/* check parity			rw		*	0	*/
#define	MBUSMODE	(1<<11)	/* module type			ro		0	1	*/
#define	ENABSB		(1<<10)	/* store buffer 	enable	rw				*/
#define	ENABICACHE	(1<<9)	/* instr. cache enable	rw				*/
#define	ENABDCACHE	(1<<8)	/* data cache enable	rw				*/
#define	ENABCACHE	(ENABICACHE|ENABDCACHE)
#define	PSO			(1<<7)	/* "Always set to '0' in Sun-4M systems." 		*/
#define	NOFAULT		(1<<1)	/* no fault 			rw				*/
#define	ENABMMU	(1<<0)	/* MMU enable 		rw				*/

/*
 * MMU
 */

#define	VAMASK		0x3FFFFFFF
#define	BY2SEGM		(1<<18)
#define	PG2SEGM		(1<<6)
#define	CONTEXT		0x30000000	/* in ASI 2 */

/*
 * MMU regions
 */
#ifdef asdf
#define	INVALIDSEGM	0xFFFC0000	/* highest seg of VA reserved as invalid */
#define IOSEGSIZE	(MB + 2*MB)	/* 1 meg for screen plus overhead */	
#define IOSEGM		(INVALIDSEGM - IOSEGSIZE)
#endif asdf

/* 
 * special MMU regions
 *	DMA segment for SBus DMA mapping via I/O MMU (hardware fixes location)
 *	the frame buffer is mapped as one MMU region (16 Mbytes)
 *	IO segments for device register pages etc.
 */
#define	DMARANGE	0
#define	DMASEGSIZE	((16*MB)<<DMARANGE)
#define	DMASEGBASE	(0 - DMASEGSIZE)
#define	FBSEGSIZE	(1*(16*MB))	 /* multiples of 16*MB */
#define	FBSEGBASE	(DMASEGBASE - DMASEGSIZE)
#define	IOSEGSIZE	(2*MB)
#define	IOSEGBASE	(FBSEGBASE - IOSEGSIZE)

/*
 * MMU entries
 */
#define	PTPVALID	1	/* page table pointer */
#define	PTEVALID	2	/* page table entry */
#define	PTERONLY	(2<<2)	/* read/execute */
#define	PTEWRITE	(3<<2)	/* read/write/execute */
#define	PTEKERNEL	(4<<2)
#define	PTENOCACHE	(0<<7)
#define	PTECACHE	(1<<7)
#define	PTEACCESS	(1<<5)
#define	PTEMODIFY	(1<<6)
#define	PTEMAINMEM	0
#define	PTEIO		0
#define PTEPROBEMEM	(PTEVALID|PTEKERNEL|PTENOCACHE|PTEWRITE|PTEMAINMEM)
#define PTEUNCACHED	PTEACCESS	/* use as software flag for putmmu */

#define	NTLBPID		64	/* limited by microsparc hardware contexts */

#define PTEMAPMEM	(1024*1024)	
#define	PTEPERTAB	(PTEMAPMEM/BY2PG)
#define SEGMAPSIZE	128

#define	INVALIDPTE	0
#define	PPN(pa)		(((pa)>>4)&0xFFFFFF0)
#define	PPT(pn)		((ulong*)KADDR((((ulong)(pn)&~0x3)<<4)))

/*
 * Address spaces
 */
#define	UZERO	0x00000000		/* base of user address space */
#define	UTZERO	(UZERO+BY2PG)		/* first address in user text */
#define	TSTKTOP	0x10000000		/* end of new stack in sysexec */
#define 	TSTKSIZ 32
#define	USTKTOP	(TSTKTOP-TSTKSIZ*BY2PG)	/* byte just beyond user stack */
#define	KZERO	0xE0000000		/* base of kernel address space */
#define	KTZERO	(KZERO+4*BY2PG)		/* first address in kernel text */
#define	USTKSIZE	(4*1024*1024)	/* size of user stack */

#define	MACHSIZE	4096

/*
 * Reference MMU registers (ASI 4)
 */
#define	MCR		0x000
#define	CTPR	0x100
#define	CXR		0x200
#define	SFSR		0x300
#define	SFAR		0x400
#define	AFSR		0x500	/* Not supported in Viking */
#define	AFAR	0x600	/* Not supported in Viking */
#define	SFARW	0x1400	/* Writable version of SFAR */

/*
 * MXCC registers and addresses (MXCC is Viking/E$ cache controller) ASI=0x2
 * From Sun-4M Architecture, Spec Number 950-1373-01, p. 116
 */
#define	MXCCSDATA	0x01C00000	/* Stream Data */
#define	MXCCSSRC	0x01C00100	/* Stream Source */
#define	MXCCSDEST	0x01C00200	/* Stream Destination */
#define	MXCCREFMISS	0x01C00300	/* Reference/Miss Count */
#define	MXCCBIST	0x01C00800	/* Built-in Selftest */
#define	MXCCCR		0x01C00A00	/* MXCC Control */
#define	MXCCSR		0x01C00B00	/* MXCC Status */
#define	MXCCRESET	0x01C00C00	/* Module Reset */
#define	MXCCERR		0x01C00E00	/* Error Registers */
#define	MXCCMPAR	0x01C00F00	/* MBus Port Address Register */

/* MXCC Control Register */
#define	ENABEPRE		(1<<5)		/* E$ prefetch enable */
#define	ENABMC		(1<<4)		/* Multiple Command Enable */
#define	ENABEPAR	(1<<3)		/* E$ parity enable */
#define	ENABECACHE	(1<<2)		/* E$ enable */

/* 
 * IO MMU page table entry
 */
#define	IOPTEVALID	(1<<1)
#define	IOPTEWRITE	(1<<2)
#define	IOPTECACHE	(1<<7)

#define	FLUSH	WORD	$0x81d80000	/* IFLUSH (R0) */
/*#define	FLUSH	IFLUSH 0(R0) */

