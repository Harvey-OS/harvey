
#define	FLASHMEM	(~0)
#define	MEM1BASE	0
#define	MEM1SIZE	0x02000000
#define	MEM2BASE	0
#define	MEM2SIZE	0
#define	PLAN9INI		(~0)

#define Saturn 0xf0000000

#define	TLBENTRIES	128

/*
 *  PTE bits for fault.c.  These belong to the second PTE word.  Validity is
 *  implied for putmmu(), and we always set PTE0_V.  PTEVALID is used
 *  here to set cache policy bits on a global basis.
 */
#define	PTEVALID		(PTE1_M|PTE1_W)	/* write through */
#define	PTEWRITE		(PTE1_RW|PTE1_C)
#define	PTERONLY	PTE1_RO
#define	PTEUNCACHED	PTE1_I
