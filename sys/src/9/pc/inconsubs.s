/* Centronix parallel (printer) port */

/* base addresses */

#define	Lpt1	0x3bc
#define	Lpt2	0x378
#define	Lpt3	0x278

/* offsets, and bits in the registers */

#define	Qdlr	0x0	/* data latch register */

#define	Qpsr	0x1	/* printer status register */
#define	Fbusy		0x80
#define	Fintbar		0x40

#define	Qpcr	0x2	/* printer control register */
#define	Fie		0x10
#define	Fsi		0x08
#define	Finitbar	0x04
#define	Faf		0x02
#define	Fstrobe		0x01

/*
 *  read next data byte
 *
 *	outb(dev+Qpcr, Finitbar|Faf|Fsi|Fstrobe);
 *	data = (inb(dev+Qpsr)&0xf8)<<2;
 *	outb(dev+Qpcr, Finitbar|Faf|Fsi);
 *	data |= inb(dev+Qpsr)>>3;
 */
TEXT	Irdnext(SB),$0

	MOVL	dev+0(FP),DX	/* points to base address */
	MOVL	$(Finitbar|Faf|Fsi|Fstrobe),AX
	ADDL	$Qpcr,DX	/* points to control register */
	OUTB
	OUTB			/* have to wait, sorry */
	SUBL	$1,DX		/* points to status register */
	INB
	MOVL	AX,CX
	MOVL	$(Finitbar|Faf|Fsi),AX
	ADDL	$1,DX		/* points to control register */
	OUTB
	OUTB
	ANDL	$0xf8,CX
	SHLL	$2,CX
	SUBL	$1,DX		/* points to status register */
	INB
	SARL	$3,AX
	ORL	CX,AX
	RET
