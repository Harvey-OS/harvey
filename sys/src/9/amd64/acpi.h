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

enum {
	Sdthdrsz = 36, /* size of SDT header */

	/* ACPI regions. Gas ids */
	Rsysmem = 0,
	Rsysio,
	Rpcicfg,
	Rembed,
	Rsmbus,
	Rcmos,
	Rpcibar,
	Ripmi,
	Rfixedhw = 0x7f,

	/* ACPI PM1 control */
	Pm1SciEn = 0x1, /* Generate SCI and not SMI */

	/* ACPI tbdf as encoded in acpi region base addresses */
	Rpciregshift = 0,
	Rpciregmask = 0xFFFF,
	Rpcifunshift = 16,
	Rpcifunmask = 0xFFFF,
	Rpcidevshift = 32,
	Rpcidevmask = 0xFFFF,
	Rpcibusshift = 48,
	Rpcibusmask = 0xFFFF,

	/* Apic structure types */
	ASlapic = 0, /* processor local apic */
	ASioapic,    /* I/O apic */
	ASintovr,    /* Interrupt source override */
	ASnmi,	     /* NMI source */
	ASlnmi,	     /* local apic nmi */
	ASladdr,     /* local apic address override */
	ASiosapic,   /* I/O sapic */
	ASlsapic,    /* local sapic */
	ASintsrc,    /* platform interrupt sources */
	ASlx2apic,   /* local x2 apic */
	ASlx2nmi,    /* local x2 apic NMI */

	/* Apic flags */
	AFbus = 0,	  /* polarity/trigger like in ISA */
	AFhigh = 1,	  /* active high */
	AFlow = 3,	  /* active low */
	AFpmask = 3,	  /* polarity bits */
	AFedge = 1 << 2,  /* edge triggered */
	AFlevel = 3 << 2, /* level triggered */
	AFtmask = 3 << 2, /* trigger bits */

	/* SRAT types */
	SRlapic = 0, /* Local apic/sapic affinity */
	SRmem,	     /* Memory affinity */
	SRlx2apic,   /* x2 apic affinity */

	/* Arg for _PIC */
	Ppic = 0, /* PIC interrupt model */
	Papic,	  /* APIC interrupt model */
	Psapic,	  /* SAPIC interrupt model */

	CMregion = 0, /* regio name spc base len accsz*/
	CMgpe,	      /* gpe name id */

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
	NACPITBLS, /* Number of ACPI tables */

	/* Atable constants */
	SIGSZ = 4 + 1,	    /* Size of the signature (including NUL) */
	OEMIDSZ = 6 + 1,    /* Size of the OEM ID (including NUL) */
	OEMTBLIDSZ = 8 + 1, /* Size of the OEM Table ID (including NUL) */

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
	Qid qid;       /* QID corresponding to this table. */
	Qid rqid;      /* This table's 'raw' QID. */
	Qid pqid;      /* This table's 'pretty' QID. */
	Qid tqid;      /* This table's 'table' QID. */
	int type;      /* This table's type */
	void *tbl;     /* pointer to the converted table, e.g. madt. */
	char name[16]; /* name of this table */

	Atable *parent;	   /* Parent pointer */
	Atable **children; /* children of this node (an array). */
	Dirtab *cdirs;	   /* child directory entries of this node. */
	usize nchildren;  /* count of this node's children */
	Atable *next;	   /* Pointer to the next sibling. */

	usize rawsize; /* Total size of raw table */
	u8 *raw;	/* Raw data. */
};

struct Gpe {
	usize stsio; /* port used for status */
	int stsbit;	 /* bit number */
	usize enio;	 /* port used for enable */
	int enbit;	 /* bit number */
	int nb;		 /* event number */
	char *obj;	 /* handler object  */
	int id;		 /* id as supplied by user */
};

struct Parse {
	char *sig;
	Atable *(*f)(unsigned char *, int); /* return nil to keep vmap */
};

struct Regio {
	void *arg;
	u8 (*get8)(usize, void *);
	void (*set8)(usize, u8, void *);
	u16 (*get16)(usize, void *);
	void (*set16)(usize, u16, void *);
	u32 (*get32)(usize, void *);
	void (*set32)(usize, u32, void *);
	u64 (*get64)(usize, void *);
	void (*set64)(usize, u64, void *);
};

struct Reg {
	char *name;
	int spc;	  /* io space */
	u64 base;	  /* address, physical */
	unsigned char *p; /* address, kmapped */
	u64 len;
	int tbdf;
	int accsz; /* access size */
};

/* Generic address structure.
 */
struct Gas {
	u8 spc;   /* address space id */
	u8 len;   /* register size in bits */
	u8 off;   /* bit offset */
	u8 accsz; /* 1: byte; 2: word; 3: dword; 4: qword */
	u64 addr; /* address (or acpi encoded tbdf + reg) */
} __attribute__((packed));

/* Root system description table pointer.
 * Used to locate the root system description table RSDT
 * (or the extended system description table from version 2) XSDT.
 * The XDST contains (after the DST header) a list of pointers to tables:
 *	- FADT	fixed acpi description table.
 *		It points to the DSDT, AML code making the acpi namespace.
 *	- SSDTs	tables with AML code to add to the acpi namespace.
 *	- pointers to other tables for apics, etc.
 */

#define RSDPTR "RSD PTR "

struct Rsdp {
	u8 signature[8]; /* "RSD PTR " */
	u8 rchecksum;
	u8 oemid[6];
	u8 revision;
	u8 raddr[4]; /* RSDT */
	u8 length[4];
	u8 xaddr[8];  /* XSDT */
	u8 xchecksum; /* XSDT */
	u8 _33_[3];   /* reserved */
} __attribute__((packed));

/* Header for ACPI description tables
 */
struct Sdthdr {
	u8 sig[4]; /* "FACP" or whatever */
	u8 length[4];
	u8 rev;
	u8 csum;
	u8 oemid[6];
	u8 oemtblid[8];
	u8 oemrev[4];
	u8 creatorid[4];
	u8 creatorrev[4];
} __attribute__((packed));

/* Firmware ACPI control structure
 */
struct Facs {
	u8 sig[4];
	u32 len;
	u32 hwsig;
	u32 wakingv;
	u32 glock;
	u32 flags;
	u64 xwakingv;
	u8 vers;
	u8 reserved1[3];
	u32 ospmflags;
	u8 reserved2[24];
} __attribute__((packed));

/* Maximum System Characteristics table
 */
struct Msct {
	usize ndoms;	/* number of discovered domains */
	int nclkdoms;	/* number of clock domains */
	u64 maxpa; /* max physical address */
	Mdom *dom;	/* domain information list */
};

struct Mdom {
	Mdom *next;
	int start;	 /* start dom id */
	int end;	 /* end dom id */
	int maxproc;	 /* max processor capacity */
	u64 maxmem; /* max memory capacity */
};

/* Multiple APIC description table
 * Interrupts are virtualized by ACPI and each APIC has
 * a `virtual interrupt base' where its interrupts start.
 * Addresses are processor-relative physical addresses.
 * Only enabled devices are linked, others are filtered out.
 */
struct Madt {
	u64 lapicpa; /* local APIC addr */
	int pcat;	  /* the machine has PC/AT 8259s */
	Apicst *st;	  /* list of Apic related structures */
};

struct Apicst {
	int type;
	Apicst *next;
	union {
		struct {
			int pid; /* processor id */
			int id;	 /* apic no */
		} lapic;
		struct {
			int id;		/* io apic id */
			u32 ibase; /* interrupt base addr. */
			u64 addr;	/* base address */
		} ioapic, iosapic;
		struct {
			int irq;   /* bus intr. source (ISA only) */
			int intr;  /* system interrupt */
			int flags; /* apic flags */
		} intovr;
		struct {
			int intr;  /* system interrupt */
			int flags; /* apic flags */
		} nmi;
		struct {
			int pid;   /* processor id */
			int flags; /* lapic flags */
			int lint;  /* lapic LINTn for nmi */
		} lnmi;
		struct {
			int pid;     /* processor id */
			int id;	     /* apic id */
			int eid;     /* apic eid */
			int puid;    /* processor uid */
			char *puids; /* same thing */
		} lsapic;
		struct {
			int pid;   /* processor id */
			int peid;  /* processor eid */
			int iosv;  /* io sapic vector */
			int intr;  /* global sys intr. */
			int type;  /* intr type */
			int flags; /* apic flags */
			int any;   /* err sts at any proc */
		} intsrc;
		struct {
			int id;	  /* x2 apic id */
			int puid; /* processor uid */
		} lx2apic;
		struct {
			int puid;
			int flags;
			int intr;
		} lx2nmi;
	};
};

/* System resource affinity table
 */
struct Srat {
	int type;
	Srat *next;
	union {
		struct {
			int dom;    /* proximity domain */
			int apic;   /* apic id */
			int sapic;  /* sapic id */
			int clkdom; /* clock domain */
		} lapic;
		struct {
			int dom;       /* proximity domain */
			u64 addr; /* base address */
			u64 len;
			int hplug; /* hot pluggable */
			int nvram; /* non volatile */
		} mem;
		struct {
			int dom;    /* proximity domain */
			int apic;   /* x2 apic id */
			int clkdom; /* clock domain */
		} lx2apic;
	};
};

/* System locality information table
 */
struct Slit {
	u64 rowlen;
	SlEntry **e;
};

struct SlEntry {
	int dom;   /* proximity domain */
	u32 dist; /* distance to proximity domain */
};

/* Fixed ACPI description table.
 * Describes implementation and hardware registers.
 * PM* blocks are low level functions.
 * GPE* blocks refer to general purpose events.
 * P_* blocks are for processor features.
 * Has address for the DSDT.
 */
struct Fadt {
	u32 facs;
	u32 dsdt;
	/* 1 reserved */
	u8 pmprofile;
	u16 sciint;
	u32 smicmd;
	u8 acpienable;
	u8 acpidisable;
	u8 s4biosreq;
	u8 pstatecnt;
	u32 pm1aevtblk;
	u32 pm1bevtblk;
	u32 pm1acntblk;
	u32 pm1bcntblk;
	u32 pm2cntblk;
	u32 pmtmrblk;
	u32 gpe0blk;
	u32 gpe1blk;
	u8 pm1evtlen;
	u8 pm1cntlen;
	u8 pm2cntlen;
	u8 pmtmrlen;
	u8 gpe0blklen;
	u8 gpe1blklen;
	u8 gp1base;
	u8 cstcnt;
	u16 plvl2lat;
	u16 plvl3lat;
	u16 flushsz;
	u16 flushstride;
	u8 dutyoff;
	u8 dutywidth;
	u8 dayalrm;
	u8 monalrm;
	u8 century;
	u16 iapcbootarch;
	/* 1 reserved */
	u32 flags;
	Gas resetreg;
	u8 resetval;
	/* 3 reserved */
	u64 xfacs;
	u64 xdsdt;
	Gas xpm1aevtblk;
	Gas xpm1bevtblk;
	Gas xpm1acntblk;
	Gas xpm1bcntblk;
	Gas xpm2cntblk;
	Gas xpmtmrblk;
	Gas xgpe0blk;
	Gas xgpe1blk;
};

/* XSDT/RSDT. 4/8 byte addresses starting at p.
 */
struct Xsdt {
	usize len;
	usize asize;
	u8 *p;
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
 * The device scope is basic tbdf as u32. There is a special value
 * that means "everything" and if we see that we set "all" in the Drhd.
 */
struct Drhd {
	int flags;
	int segment;
	usize rba;
	usize all;	      // This drhd scope is for everything.
	usize nscope;
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
		 int type, char *name, u8 *raw,
		 usize rawsize, usize addsize);
Atable *finatable(Atable *t, PSlice *slice);
Atable *finatable_nochildren(Atable *t);

extern Atable *apics;
extern Atable *dmar;
extern Atable *srat;

extern u64 acpimblocksize(u64, int *);
