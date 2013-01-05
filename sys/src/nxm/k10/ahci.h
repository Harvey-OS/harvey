/*
 * advanced host controller interface (sata)
 * © 2007-9  coraid, inc
 */

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
	H64a	= 1<<31,	/* 64-bit addressing */
	Hncq	= 1<<30,	/* ncq */
	Hsntf	= 1<<29,	/* snotification reg. */
	Hmps	= 1<<28,	/* mech pres switch */
	Hss	= 1<<27,	/* staggered spinup */
	Halp	= 1<<26,	/* aggressive link pm */
	Hal	= 1<<25,	/* activity led */
	Hclo	= 1<<24,	/* command-list override */
	Hiss	= 1<<20,	/* for interface speed */
	Ham	= 1<<18,	/* ahci-mode only */
	Hpm	= 1<<17,	/* port multiplier */
	Hfbs	= 1<<16,	/* fis-based switching */
	Hpmb	= 1<<15,	/* multiple-block pio */
	Hssc	= 1<<14,	/* slumber state */
	Hpsc	= 1<<13,	/* partial-slumber state */
	Hncs	= 1<<8,		/* n command slots */
	Hcccs	= 1<<7,		/* coal */
	Hems	= 1<<6,		/* enclosure mgmt. */
	Hxs	= 1<<5,		/* external sata */
	Hnp	= 1<<0,		/* n ports */
};

/* ghc bits */
enum {
	Hae	= 1<<31,	/* enable ahci */
	Hie	= 1<<1,		/* " interrupts */
	Hhr	= 1<<0,		/* hba reset */
};

/* cap2 bits */
enum {
	Apts	= 1<<2,	/* automatic partial to slumber */
	Nvmp	= 1<<1,	/* nvmhci present; nvram */
	Boh	= 1<<0,	/* bios/os handoff supported */
};

/* emctl bits */
enum {
	Pm	= 1<<27,	/* port multiplier support */
	Alhd	= 1<<26,	/* activity led hardware driven */
	Xonly	= 1<<25,	/* rx messages not supported */
	Smb	= 1<<24,	/* single msg buffer; rx limited */
	Esgpio	= 1<<19,	/* sgpio messages supported */
	Eses2	= 1<<18,	/* ses-2 supported */
	Esafte	= 1<<17,	/* saf-te supported */
	Elmt	= 1<<16,	/* led msg types support */
	Emrst	= 1<<9,	/* reset all em logic */
	Tmsg	= 1<<8,	/* transmit message */
	Mr	= 1<<0,	/* message rx'd */
	Emtype	= Esgpio | Eses2 | Esafte | Elmt,
};

typedef struct {
	u32int	cap;
	u32int	ghc;
	u32int	isr;
	u32int	pi;		/* ports implemented */
	u32int	ver;
	u32int	ccc;		/* coaleasing control */
	u32int	cccports;
	u32int	emloc;
	u32int	emctl;
	u32int	cap2;
	u32int	bios;
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
	Apste	= 1<<23,	/* automatic slumber to partial cap */
	Afbsc	= 1<<22,	/* fis-based switching capable */
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
	Apwr	= Apod|Asud,
};

/* ctl register bits */
enum {
	Aipm	= 1<<8,		/* interface power mgmt. 3=off */
	Aspd	= 1<<4,
	Adet	= 1<<0,		/* device detection */
};

/* sstatus register bits */
enum{
	/* sstatus det */
	Smissing		= 0<<0,
	Spresent		= 1<<0,
	Sphylink		= 3<<0,
	Sbist		= 4<<0,
	Smask		= 7<<0,

	/* sstatus speed */
	Gmissing		= 0<<4,
	Gi		= 1<<4,
	Gii		= 2<<4,
	Giii		= 3<<4,
	Gmask		= 7<<4,

	/* sstatus ipm */
	Imissing		= 0<<8,
	Iactive		= 1<<8,
	Isleepy		= 2<<8,
	Islumber		= 6<<8,
	Imask		= 7<<8,

	SImask		= Smask | Imask,
	SSmask		= Smask | Isleepy,
};

#define	sstatus	scr0
#define	sctl	scr2
#define	serror	scr1
#define	sactive	scr3
#define	ntf	scr4

typedef struct {
	u32int	list;		/* PxCLB must be 1kb aligned */
	u32int	listhi;
	u32int	fis;		/* 256-byte aligned */
	u32int	fishi;
	u32int	isr;
	u32int	ie;		/* interrupt enable */
	u32int	cmd;
	u32int	res1;
	u32int	task;
	u32int	sig;
	u32int	scr0;
	u32int	scr2;
	u32int	scr1;
	u32int	scr3;
	u32int	ci;		/* command issue */
	u32int	scr4;
	u32int	fbs;
	u32int	res2[11];
	u32int	vendor[4];
} Aport;

/* in host's memory; not memory mapped */
typedef struct {
	uchar	*base;
	uchar	*d;
	uchar	*p;
	uchar	*r;
	uchar	*u;
	u32int	*devicebits;
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
	u32int	flags;
	u32int	len;
	u32int	ctab;
	u32int	ctabhi;
	uchar	reserved[16];
} Alist;

typedef struct {
	u32int	dba;
	u32int	dbahi;
	u32int	pad;
	u32int	count;
} Aprdt;

typedef struct {
	uchar	cfis[0x40];
	uchar	atapi[0x10];
	uchar	pad[0x30];
	Aprdt	prdt;
} Actab;

/* enclosure message header */
enum {
	Mled	= 0,
	Msafte	= 1,
	Mses2	= 2,
	Msgpio	= 3,
};

typedef struct {
	uchar	dummy;
	uchar	msize;
	uchar	dsize;
	uchar	type;
	uchar	hba;		/* bits 0:4 are the port */
	uchar	pm;
	uchar	led[2];
} Aledmsg;

enum {
	Aled	= 1<<0,
	Locled	= 1<<3,
	Errled	= 1<<6,

	Ledoff	= 0,
	Ledon	= 1,
};

typedef struct {
	uint	encsz;
	u32int	*enctx;
	u32int	*encrx;
} Aenc;

enum {
	Ferror	= 1,
	Fdone	= 2,
};

typedef struct {
	QLock;
	Rendez;
	uchar	flag;
	Sfisx;
	Afis	fis;
	Alist	*list;
	Actab	*ctab;
} Aportm;

typedef struct {
	Aport	*p;
	Aportm	*m;
} Aportc;
