/*
 * PCMCIA support code.
 */

typedef struct PCMslot		PCMslot;
typedef struct PCMconftab	PCMconftab;

/*
 * Map between ISA memory space and PCMCIA card memory space.
 */
struct PCMmap {
	ulong	ca;			/* card address */
	ulong	cea;			/* card end address */
	ulong	isa;			/* ISA address */
	int	len;			/* length of the ISA area */
	int	attr;			/* attribute memory */
	int	ref;
};

/* configuration table entry */
struct PCMconftab
{
	int	index;
	ushort	irqs;		/* legal irqs */
	uchar	irqtype;
	uchar	bit16;		/* true for 16 bit access */
	struct {
		ulong	start;
		ulong	len;
	} io[16];
	int	nio;
	uchar	vpp1;
	uchar	vpp2;
	uchar	memwait;
	ulong	maxwait;
	ulong	readywait;
	ulong	otherwait;
};

/* a card slot */
struct PCMslot
{
	Lock;
	int	ref;

	void	*cp;		/* controller for this slot */
	long	memlen;		/* memory length */
	uchar	base;		/* index register base */
	uchar	slotno;		/* slot number */

	/* status */
	uchar	special;	/* in use for a special device */
	uchar	already;	/* already inited */
	uchar	occupied;
	uchar	battery;
	uchar	wrprot;
	uchar	powered;
	uchar	configed;
	uchar	enabled;
	uchar	busy;

	/* cis info */
	ulong	msec;		/* time of last slotinfo call */
	char	verstr[512];	/* version string */
	int	ncfg;		/* number of configurations */
	struct {
		ushort	cpresent;	/* config registers present */
		ulong	caddr;		/* relative address of config registers */
	} cfg[8];
	int	nctab;		/* number of config table entries */
	PCMconftab	ctab[8];
	PCMconftab	*def;	/* default conftab */

	/* memory maps */
	Lock	mlock;		/* lock down the maps */
	int	time;
	PCMmap	mmap[4];	/* maps, last is always for the kernel */
};
