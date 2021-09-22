/*
 * ACPI definitions in a distilled form
 * for device drivers and resources.
 */

typedef struct ACPIdev ACPIdev;
typedef struct ACPIres ACPIres;
typedef struct ACPIaddr ACPIaddr;
typedef struct ACPIlapic ACPIlapic;
typedef struct ACPIioapic ACPIioapic;
typedef struct ACPIint ACPIint;
typedef struct Reg Reg;

enum
{
	/* resource types known to us; keep this order */
	Rirq	= 0,
	Rdma,
	Rio,
	Rmem,
	Rmemas,			/* mem address space */
	Rioas,			/* io address space */
	Rbusas,			/* bus address space */
	Rpir,			/* pci interrupt routing info */
	Rbbn,			/* base bus number */

	/* ACPI dma resource flags */
	Dmachanmask	= 3<<5,
		Dmachancompat	= 0<<5,
		Dmachana	= 1<<5,
		Dmachanb	= 2<<5,
		Dmachanf	= 3<<5,
	Dmabm		= 1<<1,
	Dmaszmask	= 3,
		Dma8		= 0,
		Dma8and16	= 1,
		Dma16		= 2,

	/* address space resource flags */
	/* type specific flags are kept in the lo byte */
	Asmaxfixed	= 1<<(3+8),
	Asminfixed	= 1<<(2+8),
	Asconsume	= 1<<(0+8),	/* consumes or produces this as? */

	/* ACPI (extended) address space attrs */
	Muc		= 1,
	Mwc		= 2,
	Mwt		= 4,
	Mwb		= 8,
	Muce		= 0x10,		/* uncached exported, semaphores */
	Mnvr		= 0x8000,

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
};

/*
 * Information about a device.
 */
struct ACPIdev
{
	ACPIdev* next;		/* next device in flat device list */
	ACPIdev* child;		/* inner devices for buses */
	ACPIdev* cnext;		/* next sibling */

	char*	name;		/* device name */
	char*	acpiname;	/* path in the acpi ns */
	char*	pnpid;		/* pnp id */
	u64int	uid;		/* unique id valid within same pnpid */
	u64int	addr;		/* address, see below. */

	ACPIres* res;		/* current resource settings */
	ACPIres* other;		/* other possible res. settings */

};

/*
 * A resource related to a device.
 */
struct ACPIres
{
	int	type;
	ACPIres* next;		/* in device resoure list */
	union{
		struct{
			u32int	nb[16];
			uint	flags;	/* like in MPS */
		}irq;
		struct{
			uint	chans;
			uint	flags;
		}dma;
		struct{
			u64int	min;	/* address of the range */
			u64int	max;	/* address of the range */
			uint	align;	/* 1, 2, 4, 8 */
			int	isrw;	/* memory only: ro or rw? */
		}io, mem;
		struct{
			int	flags;	/* min, max are fixed? */
			u64int	mask;	/* which bits are decoded */
			u64int	min;	/* address on the other side */
			u64int	max;	/* idem. */
			u64int	len;	/* could be < max-min+1, if not fixed */
			long	off;	/* translation adds this */
			u64int	attr;	/* resource specific flags */
		}as;
		struct{
			int	dno;	/* pci device number. */
			int	pin;	/* interrupt pin 0-3 */
			int	link;	/* -1 or 0=LNKA, ... */
			int	airq;	/* acpi interrupt number */
			int	apic;	/* io apic id (if no link) */
			int	inti;	/* io apic input (if no link) */
		}pir;
		int	bbn;		/* base bus number */
	};
};

#pragma	varargck	type	"R"	ACPIres*

/*
 * ACPI region; used by the implementation.
 */
struct Reg
{
	char*	name;
	int	spc;		/* io space */
	u64int	base;		/* address, physical */
	uchar*	p;		/* address, kmapped */
	long	plen;		/* length kmapped */
	u64int	len;
	int	tbdf;
	int	accsz;		/* access size */
};

/*
 * ACPI addresses are encoded in u64int. Meaning depends
 * on the device type:
 * [E]ISA: slot nb.
 * Floppy: drive select for int13.
 * IDEctlr: 0:primary; 1:secondary
 * IDEchan: 0:master; 1:slave
 * PCI: 0xDDDDFFFF; device and function.
 * PCMCIA: slot nb.
 * SATA: 0xPPPPMMMM; port number and 0xFFFF or multipler port.
 * USB: port number
 */
#define ACPIpcidev(a)		((ulong)((a)>>16 & 0xFFFF))
#define	ACPIpcifn(a)		((ulong)((a) & 0xFFFF))
#define ACPIsataport(a)		((ulong)((a)>>16 & 0xFFFF))
#define ACPIsataportmult(a)	((ulong)((a)&0xFFFF))

/*
 * Primary interface for configuration:
 * Flat list of devices through ACPIdev.next.
 * Child device lists through ACPIdev.child and ACPIdev.cnext;
 */
extern ACPIdev *acpidevs;	/* List of devices */
extern int legacydevices;	/* Has ISA devices */
extern int keyboardpresent;	/* i8042 is present */
extern int novgapresent;	/* do not probe for vga memory */


/* Used by the acpi implementation */
extern char*	acpiregstr(int id);
extern long	acpiregio(Reg *r, void *p, ulong len, uintptr off, int iswr);
extern void	acpigpe(int i, char *obj);
extern void	acpiinit(void);
extern void	acpipwr(int i, int pm1a, int pm1b);
extern void	acpireset(void);
extern void	acpistart(void);
extern char*	smprinttables(void);
