typedef struct Flash Flash;
typedef struct Flashchip Flashchip;
typedef struct Flashpart Flashpart;
typedef struct Flashregion Flashregion;

/*
 * logical partitions
 */
enum {
	Maxflashpart = 8
};

struct Flashpart {
	char*	name;
	ulong	start;
	ulong	end;
};

enum {
	Maxflashregion = 4
};

/*
 * physical erase block regions
 */
struct Flashregion {
	int	n;		/* number of blocks in region */
	ulong	start;		/* physical base address (allowing for banks) */
	ulong	end;
	ulong	erasesize;
	ulong	eraseshift;	/* log2(erasesize) */
	ulong	pagesize;	/* if non-0, size of pages within erase block */
	ulong	pageshift;	/* log2(pagesize) */
	ulong	spares;		/* spare bytes per page, for ecc, etc. */
};

/*
 * one of a set of chips in a given region
 */
struct Flashchip {
	int	nr;
	Flashregion regions[Maxflashregion];

	uchar	id;		/* flash manufacturer ID */
	ushort	devid;		/* flash device ID */
	int	width;		/* bytes per flash line */
	int	maxwb;		/* max write buffer size */
	ulong	devsize;	/* physical device size */
	int	alg;		/* programming algorithm (if CFI) */
	int	protect;	/* software protection */
};

/*
 * structure defining a contiguous region of flash memory
 */
struct Flash {
	QLock;			/* interlock on flash operations */
	Flash*	next;

	/* following are filled in before calling Flash.reset */
	char*	type;
	void*	addr;
	ulong	size;
	int	xip;		/* executing in place: don't query */
	int	(*reset)(Flash*);

	/* following are filled in by the reset routine */
	int	(*eraseall)(Flash*);
	int	(*erasezone)(Flash*, Flashregion*, ulong);
	/* (optional) reads of correct width and alignment */
	int	(*read)(Flash*, ulong, void*, long);
	/* writes of correct width and alignment */
	int	(*write)(Flash*, ulong, void*, long);
	int	(*suspend)(Flash*);
	int	(*resume)(Flash*);
	int	(*attach)(Flash*);

	/* following might be filled in by either archflashreset or reset routine */
	int	nr;
	Flashregion regions[Maxflashregion];

	uchar	id;		/* flash manufacturer ID */
	ushort	devid;		/* flash device ID */
	int	width;		/* bytes per flash line */
	int	interleave;	/* addresses are interleaved across set of chips */
	int	bshift;		/* byte addresses are shifted */
	ulong	cmask;		/* command mask for interleaving */
	int	maxwb;		/* max write buffer size */
	ulong	devsize;	/* physical device size */
	int	alg;		/* programming algorithm (if CFI) */
	void*	data;		/* flash type routines' private storage, or nil */
	Flashpart part[Maxflashpart];	/* logical partitions */
	int	protect;	/* software protection */
	char*	sort;		/* "nand", "nor", "serial", nil (unspecified) */
};

/*
 * called by link routine of driver for specific flash type: arguments are
 * conventional name for card type/model, and card driver's reset routine.
 */
void	addflashcard(char*, int (*)(Flash*));

/*
 * called by devflash.c:/^flashreset; if flash exists,
 * sets type, address, and size in bytes of flash
 * and returns 0; returns -1 if flash doesn't exist
 */
int	archflashreset(int, Flash*);

/*
 * enable/disable write protect
 */
void	archflashwp(Flash*, int);

/*
 * flash access taking width and interleave into account
 */
int	flashget(Flash*, ulong);
void	flashput(Flash*, ulong, int);

/*
 * Architecture specific routines for managing nand devices
 */

/*
 * do any device spcific initialisation
 */
void archnand_init(Flash*);

/*
 * if claim is 1, claim device exclusively, and enable it (power it up)
 * if claim is 0, release, and disable it (power it down)
 * claiming may be as simple as a qlock per device
 */
void archnand_claim(Flash*, int claim);

/*
 * set command latch enable (CLE) and address latch enable (ALE)
 * appropriately
 */
void archnand_setCLEandALE(Flash*, int cle, int ale);

/*
 * write a sequence of bytes to the device
 */
void archnand_write(Flash*, void *buf, int len);

/*
 * read a sequence of bytes from the device
 * if buf is 0, throw away the data
 */
void archnand_read(Flash*, void *buf, int len);
