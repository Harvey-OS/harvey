
typedef struct Vic Vic;
struct Vic
{
	uchar	pad1[3], vicr;
	uchar	pad2[3], vicr7;
	uchar	pad3[3], vicr6;
	uchar	pad4[3], vicr5;
	uchar	pad5[3], vicr4;
	uchar	pad6[3], vicr3;
	uchar	pad7[3], vicr2;
	uchar	pad8[3], vicr1;
	uchar	pad9[35],icgs;
	uchar	pada[3], icms;
	uchar	padb[3], egicr;
	uchar	padc[3], icgsvbr;
	uchar	padd[3], icmsvbr;
	uchar	pade[7], egvbr;
	uchar	padf[3], icsw;
	uchar	padg[3], icr0;
	uchar	padh[3], icr1;
	uchar	padi[3], icr2;
	uchar	padj[3], icr3;
	uchar	padk[3], icr4;
	uchar	padl[3], icr5;
	uchar	padm[3], icr6;
	uchar	padn[3], icr7;
	uchar	pado[3], irsr;
	uchar	padp[3], irq1;
	uchar	padq[3], irq2;
	uchar	padr[3], irq3;
	uchar	pads[3], irq4;
	uchar	padt[3], irq5;
	uchar	padu[3], irq6;
	uchar	padv[3], irq7;
	uchar	padw[3], ttor;
	uchar	padx[3], lbtr;
	uchar	pae9[3], btdr;
	uchar	pady[3], iconf;
	uchar	padz[3], arcr;
	uchar	paez[3], amsr;
	uchar	pae1[3], besr;
	uchar	pae2[7], ss0cr0;
	uchar	pae3[3], ss0cr1;
	uchar	pae4[3], ss1cr0;
	uchar	pae5[3], ss1cr1;
	uchar	pae6[3], rcr;
	uchar	pae7[3], btcr;
	uchar	pae8[3], srr;
};


typedef struct Taxi Taxi;
struct Taxi
{
	ulong	rcvoffrd;	/* 0xc0000110 - read recv fifo offset flag */
	uchar	pad0[0x5c];
	ulong	rxfifo;		/* 0xc0000170 - recv fifo data */
	uchar	pad1[0xac];
	ulong	txoffwr;	/* 0xc0000220 - write tx fifo offset flag */
	uchar	pad2[0x4c];
	ulong	txfifo;		/* 0xc0000270 - tx fifo data */
	uchar	pad3[0x8c];
	ulong	mask;		/* 0xc0000300 - MASK port register */
	uchar	pad4[0xfc];
	ulong	fifocsr;	/* 0xc0000400 - Fifo status register */
	uchar	pad5[0xfc];
	uchar	csr;		/* 0xc0000500 - Misc status register */
	uchar	p1[3];
	uchar	pad6[0xffffafc];
	ulong	fiforst;	/* 0xd0000000 - fifo reset */
	uchar	pad7[0x10c];
	ulong	rcvoffwr;	/* 0xd0000110 - recv offset write */
	uchar	pad8[0xec];
	ulong	txoffrd;	/* 0xd0000200 - tx offset read */
	char	pad9[0x1fc];
	ulong	rst0;		/* 0xd0000400 - taxi reset = 0 */
	char	pada[0xfc];
	ulong	rst1;		/* 0xd0000500 - taxi reset = 0 */
	uchar	padb[0xfc];
	ulong	ctl;		/* 0xd0000600 - command control */
};

enum
{
	/* Fifo status */
	Xmit_flag		=	(1<<0),
	Rcv_flag		=	(1<<1),
	Rcv_ne			=	(1<<2),
};
