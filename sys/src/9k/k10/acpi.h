typedef struct Atable Atable;
typedef struct Facs Facs;
typedef struct Fadt Fadt;
typedef struct Gas Gas;
typedef struct Gpe Gpe;
typedef struct Rsdp Rsdp;
typedef struct Sdthdr Sdthdr;
typedef struct Parse Parse;
typedef struct Xsdt Xsdt;
typedef struct Regio Regio;
typedef struct Reg Reg;
typedef struct Madt Madt;
typedef struct Msct Msct;
typedef struct Mdom Mdom;
typedef struct Apicst Apicst;
typedef struct Srat Srat;
typedef struct Slit Slit;
typedef struct SlEntry SlEntry;
typedef struct Hpet Hpet;

enum
{

	Sdthdrsz	= 36,	/* size of SDT header */

	/* ACPI regions. Gas ids */
	Rsysmem	= 0,
	Rsysio,
	Rpcicfg,
	Rembed,
	Rsmbus,
	Rcmos,
	Rpcibar,
	Ripmi,
	Rfixedhw	= 0x7f,

	/* ACPI PM1 control */
	Pm1SciEn		= 0x1,		/* Generate SCI and not SMI */

	/* ACPI tbdf as encoded in acpi region base addresses */
	Rpciregshift	= 0,
	Rpciregmask	= 0xFFFF,
	Rpcifunshift	= 16,
	Rpcifunmask	= 0xFFFF,
	Rpcidevshift	= 32,
	Rpcidevmask	= 0xFFFF,
	Rpcibusshift	= 48,
	Rpcibusmask	= 0xFFFF,

	/* Apic structure types */
	ASlapic = 0,	/* processor local apic */
	ASioapic,	/* I/O apic */
	ASintovr,	/* Interrupt source override */
	ASnmi,		/* NMI source */
	ASlnmi,		/* local apic nmi */
	ASladdr,	/* local apic address override */
	ASiosapic,	/* I/O sapic */
	ASlsapic,	/* local sapic */
	ASintsrc,	/* platform interrupt sources */
	ASlx2apic,	/* local x2 apic */
	ASlx2nmi,	/* local x2 apic NMI */

	/* Apic flags */
	AFbus	= 0,	/* polarity/trigger like in ISA */
	AFhigh	= 1,	/* active high */
	AFlow	= 3,	/* active low */
	AFpmask	= 3,	/* polarity bits */
	AFedge	= 1<<2,	/* edge triggered */
	AFlevel	= 3<<2,	/* level triggered */
	AFtmask	= 3<<2,	/* trigger bits */

	/* SRAT types */
	SRlapic = 0,	/* Local apic/sapic affinity */
	SRmem,		/* Memory affinity */
	SRlx2apic,	/* x2 apic affinity */

	/* Arg for _PIC */
	Ppic = 0,	/* PIC interrupt model */
	Papic,		/* APIC interrupt model */
	Psapic,		/* SAPIC interrupt model */


	CMregion = 0,			/* regio name spc base len accsz*/
	CMgpe,				/* gpe name id */

	Qdir = 0,
	Qctl,
	Qtbl,
	Qio,
};

/*
 * ACPI table (sw)
 */
struct Atable
{
	Atable*	next;		/* next table in list */
	int	is64;		/* uses 64bits */
	char	sig[5];		/* signature */
	char	oemid[7];	/* oem id str. */
	char	oemtblid[9];	/* oem tbl. id str. */
	uchar* tbl;		/* pointer to table in memory */
	long	dlen;		/* size of data in table, after Stdhdr */
};

struct Gpe
{
	int	stsio;		/* port used for status */
	int	stsbit;		/* bit number */
	int	enio;		/* port used for enable */
	int	enbit;		/* bit number */
	int	nb;		/* event number */
	char*	obj;		/* handler object  */
	int	id;		/* id as supplied by user */
};

struct Parse
{
	char*	sig;
	Atable*	(*f)(uchar*, int);	/* return nil to keep vmap */
};

struct Regio{
	void	*arg;
	u8int	(*get8)(uintptr, void*);
	void	(*set8)(uintptr, u8int, void*);
	u16int	(*get16)(uintptr, void*);
	void	(*set16)(uintptr, u16int, void*);
	u32int	(*get32)(uintptr, void*);
	void	(*set32)(uintptr, u32int, void*);
	u64int	(*get64)(uintptr, void*);
	void	(*set64)(uintptr, u64int, void*);
};

struct Reg
{
	char*	name;
	int	spc;		/* io space */
	u64int	base;		/* address, physical */
	uchar*	p;		/* address, kmapped */
	u64int	len;
	int	tbdf;
	int	accsz;		/* access size */
};

/* Generic address structure.
 */
#pragma pack on
struct Gas
{
	u8int	spc;	/* address space id */
	u8int	len;	/* register size in bits */
	u8int	off;	/* bit offset */
	u8int	accsz;	/* 1: byte; 2: word; 3: dword; 4: qword */
	u64int	addr;	/* address (or acpi encoded tbdf + reg) */
};

/* Root system description table pointer.
 * Used to locate the root system description table RSDT
 * (or the extended system description table from version 2) XSDT.
 * The XDST contains (after the DST header) a list of pointers to tables:
 *	- FADT	fixed acpi description table.
 *		It points to the DSDT, AML code making the acpi namespace.
 *	- SSDTs	tables with AML code to add to the acpi namespace.
 *	- pointers to other tables for apics, etc.
 */

struct Rsdp
{
	u8int	signature[8];			/* "RSD PTR " */
	u8int	rchecksum;
	u8int	oemid[6];
	u8int	revision;
	u8int	raddr[4];			/* RSDT */
	u8int	length[4];
	u8int	xaddr[8];			/* XSDT */
	u8int	xchecksum;			/* XSDT */
	u8int	_33_[3];			/* reserved */
};

/* Header for ACPI description tables
 */
struct Sdthdr
{
	u8int	sig[4];			/* "FACP" or whatever */
	u8int	length[4];
	u8int	rev;
	u8int	csum;
	u8int	oemid[6];
	u8int	oemtblid[8];
	u8int	oemrev[4];
	u8int	creatorid[4];
	u8int	creatorrev[4];
};

/* Firmware control structure
 */
struct Facs
{
	u32int	hwsig;
	u32int	wakingv;
	u32int	glock;
	u32int	flags;
	u64int	xwakingv;
	u8int	vers;
	u32int	ospmflags;
};

#pragma pack off

/* Maximum System Characteristics table
 */
struct Msct
{
	int	ndoms;		/* number of domains */
	int	nclkdoms;	/* number of clock domains */
	u64int	maxpa;		/* max physical address */

	Mdom*	dom;		/* domain information list */
};

struct Mdom
{
	Mdom*	next;
	int	start;		/* start dom id */
	int	end;		/* end dom id */
	int	maxproc;	/* max processor capacity */
	u64int	maxmem;		/* max memory capacity */
};

/* Multiple APIC description table
 * Interrupts are virtualized by ACPI and each APIC has
 * a `virtual interrupt base' where its interrupts start.
 * Addresses are processor-relative physical addresses.
 * Only enabled devices are linked, others are filtered out.
 */
struct Madt
{
	uintmem	lapicpa;		/* local APIC addr */
	int	pcat;		/* the machine has PC/AT 8259s */
	Apicst*	st;		/* list of Apic related structures */
};

struct Apicst
{
	int	type;
	Apicst*	next;
	union{
		struct{
			int	pid;	/* processor id */
			int	id;	/* apic no */
		} lapic;
		struct{
			int	id;	/* io apic id */
			int	ibase;	/* interrupt base addr. */
			uintmem	addr;	/* base address */
		} ioapic, iosapic;
		struct{
			int	irq;	/* bus intr. source (ISA only) */
			int	intr;	/* system interrupt */
			int	flags;	/* apic flags */
		} intovr;
		struct{
			int	intr;	/* system interrupt */
			int	flags;	/* apic flags */
		} nmi;
		struct{
			int	pid;	/* processor id */
			int	flags;	/* lapic flags */
			int	lint;	/* lapic LINTn for nmi */
		} lnmi;
		struct{
			int	pid;	/* processor id */
			int	id;	/* apic id */
			int	eid;	/* apic eid */
			int	puid;	/* processor uid */
			char*	puids;	/* same thing */
		} lsapic;
		struct{
			int	pid;	/* processor id */
			int	peid;	/* processor eid */
			int	iosv;	/* io sapic vector */
			int	intr;	/* global sys intr. */
			int	type;	/* intr type */
			int	flags;	/* apic flags */
			int	any;	/* err sts at any proc */
		} intsrc;
		struct{
			int	id;	/* x2 apic id */
			int	puid;	/* processor uid */
		} lx2apic;
		struct{
			int	puid;
			int	flags;
			int	intr;
		} lx2nmi;
	};
};

/* System resource affinity table
 */
struct Srat
{
	int	type;
	Srat*	next;
	union{
		struct{
			int	dom;	/* proximity domain */
			int	apic;	/* apic id */
			int	sapic;	/* sapic id */
			int	clkdom;	/* clock domain */
		} lapic;
		struct{
			int	dom;	/* proximity domain */
			u64int	addr;	/* base address */
			u64int	len;
			int	hplug;	/* hot pluggable */
			int	nvram;	/* non volatile */
		} mem;
		struct{
			int	dom;	/* proximity domain */
			int	apic;	/* x2 apic id */
			int	clkdom;	/* clock domain */
		} lx2apic;
	};
};

/* System locality information table
 */
struct Slit {
	uvlong rowlen;
	SlEntry **e;
};

struct SlEntry {
	int dom;	/* proximity domain */
	uint dist;	/* distance to proximity domain */
};

/* Fixed ACPI description table.
 * Describes implementation and hardware registers.
 * PM* blocks are low level functions.
 * GPE* blocks refer to general purpose events.
 * P_* blocks are for processor features.
 * Has address for the DSDT.
 */
struct Fadt
{
	u32int	facs;
	u32int	dsdt;
	/* 1 reserved */
	u8int	pmprofile;
	u16int	sciint;
	u32int	smicmd;
	u8int	acpienable;
	u8int	acpidisable;
	u8int	s4biosreq;
	u8int	pstatecnt;
	u32int	pm1aevtblk;
	u32int	pm1bevtblk;
	u32int	pm1acntblk;
	u32int	pm1bcntblk;
	u32int	pm2cntblk;
	u32int	pmtmrblk;
	u32int	gpe0blk;
	u32int	gpe1blk;
	u8int	pm1evtlen;
	u8int	pm1cntlen;
	u8int	pm2cntlen;
	u8int	pmtmrlen;
	u8int	gpe0blklen;
	u8int	gpe1blklen;
	u8int	gp1base;
	u8int	cstcnt;
	u16int	plvl2lat;
	u16int	plvl3lat;
	u16int	flushsz;
	u16int	flushstride;
	u8int	dutyoff;
	u8int	dutywidth;
	u8int	dayalrm;
	u8int	monalrm;
	u8int	century;
	u16int	iapcbootarch;
	/* 1 reserved */
	u32int	flags;
	Gas	resetreg;
	u8int	resetval;
	/* 3 reserved */
	u64int	xfacs;
	u64int	xdsdt;
	Gas	xpm1aevtblk;
	Gas	xpm1bevtblk;
	Gas	xpm1acntblk;
	Gas	xpm1bcntblk;
	Gas	xpm2cntblk;
	Gas	xpmtmrblk;
	Gas	xgpe0blk;
	Gas	xgpe1blk;
};

/* XSDT/RSDT. 4/8 byte addresses starting at p.
 */
struct Xsdt
{
	int	len;
	int	asize;
	u8int*	p;
};

struct Hpet
{
	u32int	id;					/* Event Timer Bock ID */
	uintptr	addr;				/* ACPI Format Address (Gas) */
	u8int	seqno;				/* Sequence Number */
	u16int	minticks;			/* Minimum Clock Tick */
	u8int	attr;				/* Page Protection */
};

extern uintmem acpimblocksize(uintmem, int*);
