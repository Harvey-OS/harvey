/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* -----------------------------------------------------------------------------
 * ACPI is a table of tables. The tables define a hierarchy.
 *
 * From the hardware's perspective:
 * Each table that we care about has a header, and the header has a
 * length that includes the the length of all its subtables. So, even
 * if you can't completely parse a table, you can find the next table.
 *
 * The process of parsing is to find the RSDP, and then for each subtable
 * see what type it is and parse it. The process is recursive except for
 * a few issues: The RSDP signature and header differs from the header of
 * its subtables; their headers differ from the signatures of the tables
 * they contain. As you walk down the tree, you need different parsers.
 *
 * The parser is recursive descent. Each parsing function takes a pointer
 * to the parent of the node it is parsing and will attach itself to the parent
 * via that pointer. Parsing functions are responsible for building the data
 * structures that represent their node and recursive invocations of the parser
 * for subtables.
 *
 * So, in this case, it's something like this:
 *
 * RSDP is the root. It has a standard header and size. You map that
 * memory.  You find the first header, get its type and size, and
 * parse as much of it as you can. Parsing will involve either a
 * function or case statement for each element type. DMARs are complex
 * and need functions; APICs are simple and we can get by with case
 * statements.
 *
 * Each node in the tree is represented as a 'struct Atable'. This has a
 * pointer to the actual node data, a type tag, a name, pointers to this
 * node's children (if any) and a parent pointer. It also has a QID so that
 * the entire structure can be exposed as a filesystem. The Atable doesn't
 * contain any table data per se; it's metadata. The table pointer contains
 * the table data as well as a pointer back to it's corresponding Atable.
 *
 * In the end we present a directory tree for #apic that looks, in this example:
 * #acpi/DMAR/DRHD/0/{pretty,raw}
 *
 * 'cat pretty' will return JSON-encoded data described the element.
 * 'cat raw' gets you the raw bytes.
 */

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
typedef struct Dmar Dmar;
typedef struct Drhd Drhd;
typedef struct DevScope DevScope;
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

	/* Table types. */
	RSDP = 0,
	SDTH,
	RSDT,
	FADT,
	FACS,
	DSDT,
	SSDT,
	MADT,
	SBST,
	XSDT,
	ECDT,
	SLIT,
	SRAT,
	CPEP,
	MSCT,
	RASF,
	MPST,
	PMTT,
	BGRT,
	FPDT,
	GTDT,
	HPET,
	APIC,
	DMAR,
	/* DMAR types */
	DRHD,
	RMRR,
	ATSR,
	RHSA,
	ANDD,
	NACPITBLS,			/* Number of ACPI tables */

	/* Atable constants */
	SIGSZ		= 4+1,	/* Size of the signature (including NUL) */
	OEMIDSZ		= 6+1,	/* Size of the OEM ID (including NUL) */
	OEMTBLIDSZ	= 8+1,	/* Size of the OEM Table ID (including NUL) */

};

/*
 * ACPI table (sw)
 *
 * This Atable struct corresponds to an interpretation of the standard header
 * for all table types we support. It has a pointer to the converted data, i.e.
 * the structs created by functions like acpimadt and so on. Note: althouh the
 * various things in this are a superset of many ACPI table names (DRHD, DRHD
 * scopes, etc). The raw data follows this header.
 *
 * Child entries in the table are kept in an array of pointers. Each entry has
 * a pointer to it's logically "next" sibling, thus forming a linked list. But
 * these lists are purely for convenience and all point to nodes within the
 * same array.
 */
struct Atable {
	Qid qid;             /* QID corresponding to this table. */
	Qid rqid;			/* This table's 'raw' QID. */
	Qid pqid;			/* This table's 'pretty' QID. */
	Qid tqid;			/* This table's 'table' QID. */
	int type;					/* This table's type */
	void *tbl;					/* pointer to the converted table, e.g. madt. */
	char name[16];				/* name of this table */

	Atable *parent;		/* Parent pointer */
	Atable **children;	/* children of this node (an array). */
	Dirtab *cdirs;		/* child directory entries of this node. */
	size_t nchildren;			/* count of this node's children */
	Atable *next;		/* Pointer to the next sibling. */

	size_t rawsize;				/* Total size of raw table */
	uint8_t *raw;				/* Raw data. */
};

struct Gpe
{
	uintptr_t	stsio;		/* port used for status */
	int	stsbit;		/* bit number */
	uintptr_t	enio;		/* port used for enable */
	int	enbit;		/* bit number */
	int	nb;		/* event number */
	char*	obj;		/* handler object  */
	int	id;		/* id as supplied by user */
};

struct Parse
{
	char*	sig;
	Atable*	(*f)(unsigned char*, int);	/* return nil to keep vmap */
};

struct Regio{
	void	*arg;
	uint8_t	(*get8)(uintptr_t, void*);
	void	(*set8)(uintptr_t, uint8_t, void*);
	uint16_t	(*get16)(uintptr_t, void*);
	void	(*set16)(uintptr_t, uint16_t, void*);
	uint32_t	(*get32)(uintptr_t, void*);
	void	(*set32)(uintptr_t, uint32_t, void*);
	uint64_t	(*get64)(uintptr_t, void*);
	void	(*set64)(uintptr_t, uint64_t, void*);
};

struct Reg
{
	char*	name;
	int	spc;		/* io space */
	uint64_t	base;		/* address, physical */
	unsigned char*	p;		/* address, kmapped */
	uint64_t	len;
	int	tbdf;
	int	accsz;		/* access size */
};

/* Generic address structure.
 */
struct Gas
{
	uint8_t	spc;	/* address space id */
	uint8_t	len;	/* register size in bits */
	uint8_t	off;	/* bit offset */
	uint8_t	accsz;	/* 1: byte; 2: word; 3: dword; 4: qword */
	uint64_t	addr;	/* address (or acpi encoded tbdf + reg) */
} __attribute__ ((packed));

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
	uint8_t	signature[8];			/* "RSD PTR " */
	uint8_t	rchecksum;
	uint8_t	oemid[6];
	uint8_t	revision;
	uint8_t	raddr[4];			/* RSDT */
	uint8_t	length[4];
	uint8_t	xaddr[8];			/* XSDT */
	uint8_t	xchecksum;			/* XSDT */
	uint8_t	_33_[3];			/* reserved */
}  __attribute__ ((packed));

/* Header for ACPI description tables
 */
struct Sdthdr
{
	uint8_t	sig[4];			/* "FACP" or whatever */
	uint8_t	length[4];
	uint8_t	rev;
	uint8_t	csum;
	uint8_t	oemid[6];
	uint8_t	oemtblid[8];
	uint8_t	oemrev[4];
	uint8_t	creatorid[4];
	uint8_t	creatorrev[4];
}  __attribute__ ((packed));

/* Firmware control structure
 */
struct Facs
{
	uint8_t sig[4];
	uint8_t len[4];
	uint32_t	hwsig;
	uint32_t	wakingv;
	uint32_t	glock;
	uint32_t	flags;
	uint64_t	xwakingv;
	uint8_t	vers;
	uint32_t	ospmflags;
}  __attribute__ ((packed));


/* Maximum System Characteristics table
 */
struct Msct
{
	size_t ndoms;		/* number of discovered domains */
	int	nclkdoms;	/* number of clock domains */
	uint64_t	maxpa;	/* max physical address */
	Mdom*	dom;		/* domain information list */
};

struct Mdom
{
	Mdom*	next;
	int	start;		/* start dom id */
	int	end;		/* end dom id */
	int	maxproc;	/* max processor capacity */
	uint64_t maxmem;	/* max memory capacity */
};

/* Multiple APIC description table
 * Interrupts are virtualized by ACPI and each APIC has
 * a `virtual interrupt base' where its interrupts start.
 * Addresses are processor-relative physical addresses.
 * Only enabled devices are linked, others are filtered out.
 */
struct Madt
{
	uint64_t	lapicpa;		/* local APIC addr */
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
			uint32_t	ibase;	/* interrupt base addr. */
			uint64_t	addr;	/* base address */
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
			uint64_t	addr;	/* base address */
			uint64_t	len;
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
	uint64_t rowlen;
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
	uint32_t	facs;
	uint32_t	dsdt;
	/* 1 reserved */
	uint8_t	pmprofile;
	uint16_t	sciint;
	uint32_t	smicmd;
	uint8_t	acpienable;
	uint8_t	acpidisable;
	uint8_t	s4biosreq;
	uint8_t	pstatecnt;
	uint32_t	pm1aevtblk;
	uint32_t	pm1bevtblk;
	uint32_t	pm1acntblk;
	uint32_t	pm1bcntblk;
	uint32_t	pm2cntblk;
	uint32_t	pmtmrblk;
	uint32_t	gpe0blk;
	uint32_t	gpe1blk;
	uint8_t	pm1evtlen;
	uint8_t	pm1cntlen;
	uint8_t	pm2cntlen;
	uint8_t	pmtmrlen;
	uint8_t	gpe0blklen;
	uint8_t	gpe1blklen;
	uint8_t	gp1base;
	uint8_t	cstcnt;
	uint16_t	plvl2lat;
	uint16_t	plvl3lat;
	uint16_t	flushsz;
	uint16_t	flushstride;
	uint8_t	dutyoff;
	uint8_t	dutywidth;
	uint8_t	dayalrm;
	uint8_t	monalrm;
	uint8_t	century;
	uint16_t	iapcbootarch;
	/* 1 reserved */
	uint32_t	flags;
	Gas	resetreg;
	uint8_t	resetval;
	/* 3 reserved */
	uint64_t	xfacs;
	uint64_t	xdsdt;
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
	size_t len;
	size_t asize;
	uint8_t*	p;
};

/* DMAR.
 */
/*
 * Device scope.
 */
struct DevScope {
	int enumeration_id;
	int start_bus_number;
	int npath;
	int *paths;
};
/*
 * The device scope is basic tbdf as uint32_t. There is a special value
 * that means "everything" and if we see that we set "all" in the Drhd.
 */
struct Drhd {
	int flags;
	int segment;
	uintptr_t rba;
	uintptr_t all;	// This drhd scope is for everything.
	size_t nscope;
	struct DevScope *scopes;
};

struct Dmar {
	int haw;
	/*
	 * If your firmware disables x2apic mode, you should not be here.
	 * We ignore that bit.
	 */
	int intr_remap;
};

int acpiinit(void);
Atable *mkatable(Atable *parent,
                        int type, char *name, uint8_t *raw,
                        size_t rawsize, size_t addsize);
Atable *finatable(Atable *t, PSlice *slice);
Atable *finatable_nochildren(Atable *t);

extern Atable *apics;
extern Atable *dmar;
extern Atable *srat;

extern uintmem acpimblocksize(uintmem, int*);
