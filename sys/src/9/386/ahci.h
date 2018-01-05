/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * advanced host controller interface (sata)
 * © 2007  coraid, inc
 */

/* ata errors */
enum {
	Emed	= 1<<0,		/* media error */
	Enm	= 1<<1,		/* no media */
	Eabrt	= 1<<2,		/* abort */
	Emcr	= 1<<3,		/* media change request */
	Eidnf	= 1<<4,		/* no user-accessible address */
	Emc	= 1<<5,		/* media change */
	Eunc	= 1<<6,		/* data error */
	Ewp	= 1<<6,		/* write protect */
	Eicrc	= 1<<7,		/* interface crc error */

	Efatal	= Eidnf|Eicrc,	/* must sw reset */
};

/* ata status */
enum {
	ASerr	= 1<<0,		/* error */
	ASdrq	= 1<<3,		/* request */
	ASdf	= 1<<5,		/* fault */
	ASdrdy	= 1<<6,		/* ready */
	ASbsy	= 1<<7,		/* busy */

	ASobs	= 1<<1|1<<2|1<<4,
};

/* pci configuration */
enum {
	Abar	= 5,
};

/*
 * ahci memory configuration
 *
 * 0000-0023	generic host control
 * 0024-009f	reserved
 * 00a0-00ff	vendor specific.
 * 0100-017f	port 0
 * ...
 * 1080-1100	port 31
 */

/* cap bits: supported features */
enum {
	Hs64a	= 1<<31,	/* 64-bit addressing */
	Hsncq	= 1<<30,	/* ncq */
	Hssntf	= 1<<29,	/* snotification reg. */
	Hsmps	= 1<<28,	/* mech pres switch */
	Hsss	= 1<<27,	/* staggered spinup */
	Hsalp	= 1<<26,	/* aggressive link pm */
	Hsal	= 1<<25,	/* activity led */
	Hsclo	= 1<<24,	/* command-list override */
	Hiss	= 1<<20,	/* for interface speed */
//	Hsnzo	= 1<<19,
	Hsam	= 1<<18,	/* ahci-mode only */
	Hspm	= 1<<17,	/* port multiplier */
//	Hfbss	= 1<<16,
	Hpmb	= 1<<15,	/* multiple-block pio */
	Hssc	= 1<<14,	/* slumber state */
	Hpsc	= 1<<13,	/* partial-slumber state */
	Hncs	= 1<<8,		/* n command slots */
	Hcccs	= 1<<7,		/* coal */
	Hems	= 1<<6,		/* enclosure mgmt. */
	Hsxs	= 1<<5,		/* external sata */
	Hnp	= 1<<0,		/* n ports */
};

/* ghc bits */
enum {
	Hae	= 1<<31,	/* enable ahci */
	Hie	= 1<<1,		/* " interrupts */
	Hhr	= 1<<0,		/* hba reset */
};

typedef struct {
	uint32_t	cap;
	uint32_t	ghc;
	uint32_t	isr;
	uint32_t	pi;		/* ports implemented */
	uint32_t	ver;
	uint32_t	ccc;		/* coaleasing control */
	uint32_t	cccports;
	uint32_t	emloc;
	uint32_t	emctl;
} Ahba;

enum {
	Acpds	= 1<<31,	/* cold port detect status */
	Atfes	= 1<<30,	/* task file error status */
	Ahbfs	= 1<<29,	/* hba fatal */
	Ahbds	= 1<<28,	/* hba error (parity error) */
	Aifs	= 1<<27,	/* interface fatal  §6.1.2 */
	Ainfs	= 1<<26,	/* interface error (recovered) */
	Aofs	= 1<<24,	/* too many bytes from disk */
	Aipms	= 1<<23,	/* incorrect prt mul status */
	Aprcs	= 1<<22,	/* PhyRdy change status Pxserr.diag.n */
	Adpms	= 1<<7,		/* mechanical presence status */
	Apcs 	= 1<<6,		/* port connect  diag.x */
	Adps 	= 1<<5,		/* descriptor processed */
	Aufs 	= 1<<4,		/* unknown fis diag.f */
	Asdbs	= 1<<3,		/* set device bits fis received w/ i bit set */
	Adss	= 1<<2,		/* dma setup */
	Apio	= 1<<1,		/* pio setup fis */
	Adhrs	= 1<<0,		/* device to host register fis */

	IEM	= Acpds|Atfes|Ahbds|Ahbfs|Ahbds|Aifs|Ainfs|Aprcs|Apcs|Adps|
			Aufs|Asdbs|Adss|Adhrs,
	Ifatal	= Atfes|Ahbfs|Ahbds|Aifs,
};

/* serror bits */
enum {
	SerrX	= 1<<26,	/* exchanged */
	SerrF	= 1<<25,	/* unknown fis */
	SerrT	= 1<<24,	/* transition error */
	SerrS	= 1<<23,	/* link sequence */
	SerrH	= 1<<22,	/* handshake */
	SerrC	= 1<<21,	/* crc */
	SerrD	= 1<<20,	/* not used by ahci */
	SerrB	= 1<<19,	/* 10-tp-8 decode */
	SerrW	= 1<<18,	/* comm wake */
	SerrI	= 1<<17,	/* phy internal */
	SerrN	= 1<<16,	/* phyrdy change */

	ErrE	= 1<<11,	/* internal */
	ErrP	= 1<<10,	/* ata protocol violation */
	ErrC	= 1<<9,		/* communication */
	ErrT	= 1<<8,		/* transient */
	ErrM	= 1<<1,		/* recoverd comm */
	ErrI	= 1<<0,		/* recovered data integrety */

	ErrAll	= ErrE|ErrP|ErrC|ErrT|ErrM|ErrI,
	SerrAll	= SerrX|SerrF|SerrT|SerrS|SerrH|SerrC|SerrD|SerrB|SerrW|
			SerrI|SerrN|ErrAll,
	SerrBad	= 0x7f<<19,
};

/* cmd register bits */
enum {
	Aicc	= 1<<28,	/* interface communcations control. 4 bits */
	Aasp	= 1<<27,	/* aggressive slumber & partial sleep */
	Aalpe 	= 1<<26,	/* aggressive link pm enable */
	Adlae	= 1<<25,	/* drive led on atapi */
	Aatapi	= 1<<24,	/* device is atapi */
	Aesp	= 1<<21,	/* external sata port */
	Acpd	= 1<<20,	/* cold presence detect */
	Ampsp	= 1<<19,	/* mechanical pres. */
	Ahpcp	= 1<<18,	/* hot plug capable */
	Apma	= 1<<17,	/* pm attached */
	Acps	= 1<<16,	/* cold presence state */
	Acr	= 1<<15,	/* cmdlist running */
	Afr	= 1<<14,	/* fis running */
	Ampss	= 1<<13,	/* mechanical presence switch state */
	Accs	= 1<<8,		/* current command slot 12:08 */
	Afre	= 1<<4,		/* fis enable receive */
	Aclo	= 1<<3,		/* command list override */
	Apod	= 1<<2,		/* power on dev (requires cold-pres. detect) */
	Asud	= 1<<1,		/* spin-up device;  requires ss capability */
	Ast	= 1<<0,		/* start */

	Arun	= Ast|Acr|Afre|Afr,
};

/* ctl register bits */
enum {
	Aipm	= 1<<8,		/* interface power mgmt. 3=off */
	Aspd	= 1<<4,
	Adet	= 1<<0,		/* device detection */
};

#define	sstatus	scr0
#define	sctl	scr2
#define	serror	scr1
#define	sactive	scr3

typedef struct {
	uint32_t	list;		/* PxCLB must be 1kb aligned. */
	uint32_t	listhi;
	uint32_t	fis;		/* 256-byte aligned */
	uint32_t	fishi;
	uint32_t	isr;
	uint32_t	ie;		/* interrupt enable */
	uint32_t	cmd;
	uint32_t	res1;
	uint32_t	task;
	uint32_t	sig;
	uint32_t	scr0;
	uint32_t	scr2;
	uint32_t	scr1;
	uint32_t	scr3;
	uint32_t	ci;		/* command issue */
	uint32_t	ntf;
	unsigned char	res2[8];
	uint32_t	vendor;
} Aport;

enum {
	/*
	 * Aport sstatus bits (actually states):
	 * 11-8 interface power management
	 *  7-4 current interface speed (generation #)
	 *  3-0 device detection
	 */
	Intslumber	= 0x600,
	Intpartpwr	= 0x200,
	Intactive	= 0x100,
	Intpm		= 0xf00,

	Devphyoffline	= 4,
	Devphycomm	= 2,		/* phy communication established */
	Devpresent	= 1,
	Devdet		= Devpresent | Devphycomm | Devphyoffline,
};

/* in host's memory; not memory mapped */
typedef struct {
	unsigned char	*base;
	unsigned char	*d;
	unsigned char	*p;
	unsigned char	*r;
	unsigned char	*u;
	uint32_t	*devicebits;
} Afis;

enum {
	Lprdtl	= 1<<16,	/* physical region descriptor table len */
	Lpmp	= 1<<12,	/* port multiplier port */
	Lclear	= 1<<10,	/* clear busy on R_OK */
	Lbist	= 1<<9,
	Lreset	= 1<<8,
	Lpref	= 1<<7,		/* prefetchable */
	Lwrite	= 1<<6,
	Latapi	= 1<<5,
	Lcfl	= 1<<0,		/* command fis length in double words */
};

/* in hosts memory; memory mapped */
typedef struct {
	uint32_t	flags;
	uint32_t	len;
	uint32_t	ctab;
	uint32_t	ctabhi;
	unsigned char	reserved[16];
} Alist;

typedef struct {
	uint32_t	dba;
	uint32_t	dbahi;
	uint32_t	pad;
	uint32_t	count;
} Aprdt;

typedef struct {
	unsigned char	cfis[0x40];
	unsigned char	atapi[0x10];
	unsigned char	pad[0x30];
	Aprdt	prdt;
} Actab;

enum {
	Ferror	= 1,
	Fdone	= 2,
};

enum {
	Dllba 	= 1,
	Dsmart	= 1<<1,
	Dpower	= 1<<2,
	Dnop	= 1<<3,
	Datapi	= 1<<4,
	Datapi16= 1<<5,
};

typedef struct {
	QLock ql;
	Rendez Rendez;
	unsigned char	flag;
	unsigned char	feat;
	unsigned char	smart;
	Afis	fis;
	Alist	*list;
	Actab	*ctab;
} Aportm;

typedef struct {
	Aport	*p;
	Aportm	*pm;
} Aportc;
