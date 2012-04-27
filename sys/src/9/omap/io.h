/*
 * the ``general-purpose'' memory controller.
 * only works with flash memory.
 */

enum {
	/* syscfg bits */
	Idlemask	= MASK(2) << 3,
	Noidle		= 1 << 3,

	/* config bits */
	Postnandwrites	= 1<<0,	/* force nand reg. writes to be posted */

	/* indices of cscfg[].cfg[] */
	Csctl		= 1 - 1,		/* chip-select signal ctl */
	Csmap		= 7 - 1,		/* chip-select addr map cfg */

	/* Csctl bits */
	Muxadddata	= 1 << 9,
	Devtypemask	= MASK(2) << 10,
	Devtypenor	= 0 << 10,
	Devtypenand	= 2 << 10,
	Devsizemask	= 1 << 12,
	Devsize8	= 0 << 12,
	Devsize16	= 1 << 12,
	Writesync	= 1 << 27,
	Readsync	= 1 << 29,

	/* Csmap bits */
	Csvalid		= 1 << 6,
	MB16		= 017 << 8,		/* 16MB size */
	MB128		= 010 << 8,		/* 128MB size */
};

typedef struct Gpmc Gpmc;
typedef struct Gpmccs Gpmccs;

/*
 * configuration for non-dram (e.g., flash) memory
 */
struct Gpmc {				/* hw registers */
	uchar	_pad0[0x10];
	ulong	syscfg;
	ulong	syssts;
	ulong	irqsts;
	ulong	irqenable;
	uchar	_pad1[0x40 - 0x20];
	ulong	tmout_ctl;
	ulong	erraddr;
	ulong	errtype;
	ulong	_pad7;
	ulong	config;
	ulong	sts;
	uchar	_pad2[0x60 - 0x58];

	/* chip-select config */
	struct Gpmccs {
		ulong	cfg[7];
		ulong	nandcmd;
		ulong	nandaddr;
		ulong	nanddata;
		ulong	_pad6[2];
	} cscfg[8];

	/* prefetch */
	ulong	prefcfg[2];
	ulong	_pad8;
	ulong	prefctl;
	ulong	prefsts;

	/* ecc */
	ulong	ecccfg;
	ulong	eccctl;
	ulong	eccsize;
	ulong	eccres[9];
	uchar	_pad3[0x240 - 0x224];

	/* bch */
	ulong	bchres[8][4];
	uchar	_pad4[0x2d0 - 0x2c0];
	ulong	bchswdata;
};
