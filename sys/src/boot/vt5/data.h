enum {
	PRINTSIZE = 256,
};

#define		MS2NS(n) (((vlong)(n))*1000000LL)
#define		TK2MS(x) ((x)*(1000/HZ))

#define MB	(1024*1024)

/*
 * Where configuration info is left for the loaded programme.
 * This will turn into a structure as more is done by the boot loader
 * (e.g. why parse the .ini file twice?).
 * There are 3584 bytes available at CONFADDR on the PC.
 *
 * The low-level boot routines in l.s leave data for us at CONFADDR,
 * which we pick up before reading the plan9.ini file.
 */
#define BOOTLINELEN	64
#define BOOTARGS	((char*)(CONFADDR+BOOTLINELEN))
#define	BOOTARGSLEN	1024	/* size reduction */
#define	MAXCONF		32	/* from 100; size reduction */

/*
 * intc bits, as of 18 aug 2009.
 * specific to rae's virtex5 design
 */
enum {
	Bitllfifo,
	Bittemac,
	Bitdma,
	Bitdma2,
	Bituart,
	Bitmiiphy,
	Bitqtmmacfail,			/* qtm only */
	Bitqtmdraminit,			/* qtm only */

	Intllfifo=1<<Bitllfifo,		/* local-link FIFO */
	Inttemac= 1<<Bittemac,
	Intdma	= 1<<Bitdma,
	Intdma2	= 1<<Bitdma2,
	Intuart = 1<<Bituart,
	Intmiiphy = 1<<Bitmiiphy,
	Intqtmmacfail= 1<<Bitqtmmacfail,
	Intqtmdraminit= 1<<Bitqtmdraminit,
};

typedef struct Boot Boot;
typedef struct File File;
typedef struct Fs Fs;
typedef struct Lock Lock;
typedef union Mach Mach;
typedef struct Medium Medium;
typedef u32int Mreg;				/* Msr - bloody UART */
typedef struct Proc Proc;
typedef struct Timer Timer;
typedef struct Type Type;
typedef struct Ureg Ureg;

struct Lock
{
	uint	key;
	ulong	sr;
	ulong	pc;
//	Proc	*p;
	Mach	*m;
	ushort	isilock;
	long	lockcycles;
};


typedef union Mach {
	struct {
		int	machno;
		void*	up;		/* offset known in assember */

		/* ordering from here on irrelevant */

		ulong	ticks;		/* of the clock since boot time */
		long	oscclk;		/* oscillator frequency (MHz) */
		long	cpuhz;		/* general system clock (cycles) */
		long	clockgen;	/* clock generator frequency (cycles) */
		long	vcohz;
		long	pllhz;
		long	plbhz;
		long	opbhz;
		long	epbhz;
		long	pcihz;
		int	cputype;
		ulong	delayloop;
//		uvlong	cyclefreq;	/* frequency of user readable cycle clock */
		uvlong	fastclock;
		int	intr;
	};
//	uintptr	stack[MACHSIZE/sizeof(uintptr)];	/* now in dram */
} Mach;

extern Mach* machptr[MAXMACH];

#define	MACHP(n)	machptr[n]

extern register Mach* m;
extern register void* up;

/*
 * rest copied from /sys/src/boot/pc/dat.h:
 */

typedef struct Medium Medium;
typedef struct Boot Boot;

enum {					/* type */
	Tether		= 0,
	Tany		= -1,
};

enum {					/* name and flag */
	Fnone		= 0x00,

	Nfs		= 0x00,
	Ffs		= (1<<Nfs),
	Nboot		= 0x01,
	Fboot		= (1<<Nboot),
	Nbootp		= 0x02,
	Fbootp		= (1<<Nbootp),
	NName		= 3,

	Fany		= Fbootp|Fboot|Ffs,

	Fini		= 0x10,
	Fprobe		= 0x80,
};

typedef struct Type {
	ushort	type;
	ushort	flag;
	int	(*init)(void);
	void	(*initdev)(int, char*);
	void*	(*getfspart)(int, char*, int);	/* actually returns Dos* */
	void	(*addconf)(int);
	int	(*boot)(int, char*, Boot*);
	void	(*printdevs)(int);
	char**	parts;
	char**	inis;
	int	mask;
	Medium*	media;
} Type;

extern void (*etherdetach)(void);

#define	_MAGIC(f, b)	((f)|((((4*(b))+0)*(b))+7))
#define	Q_MAGIC		_MAGIC(0, 21)		/* powerpc */

typedef struct Exec Exec;
struct	Exec
{
	uchar	magic[4];		/* magic number */
	uchar	text[4];	 	/* size of text segment */
	uchar	data[4];	 	/* size of initialized data */
	uchar	bss[4];	  		/* size of uninitialized data */
	uchar	syms[4];	 	/* size of symbol table */
	uchar	entry[4];	 	/* entry point */
	uchar	spsz[4];		/* size of sp/pc offset table */
	uchar	pcsz[4];		/* size of pc/line number table */
};

typedef struct Block Block;
struct Block {
	Block*	next;
	uchar*	rp;			/* first unconsumed byte */
	uchar*	wp;			/* first empty byte */
	uchar*	lim;			/* 1 past the end of the buffer */
	uchar*	base;			/* start of the buffer */
	ulong	flag;
};
#define BLEN(s)	((s)->wp - (s)->rp)

enum {
	Eaddrlen	= 6,
	/* next two exclude 4-byte ether CRC */
	ETHERMINTU	= 60,		/* minimum transmit size */
	ETHERMAXTU	= 1514,		/* maximum transmit size */
	ETHERHDRSIZE	= 14,		/* size of an ethernet header */

	MaxEther	= 1,	/* from 6; size reduction */
};

typedef struct {
	uchar	d[Eaddrlen];
	uchar	s[Eaddrlen];
	uchar	type[2];
	uchar	data[1500];
	uchar	crc[4];
} Etherpkt;

enum {	/* returned by bootpass */
	MORE, ENOUGH, FAIL
};
enum {
	INITKERNEL,
	READEXEC,
	READ9TEXT,
	READ9DATA,
	READGZIP,
	READEHDR,
	READPHDR,
	READEPAD,
	READEDATA,
	TRYBOOT,
	INIT9LOAD,
	READ9LOAD,
	FAILED
};

struct Boot {
	int state;

	Exec exec;
	char *bp;	/* base ptr */
	char *wp;	/* write ptr */
	char *ep;	/* end ptr */
};

extern uvlong	clockintrs;
extern int	debug;
extern char	*defaultpartition;
extern int	iniread;
extern uvlong	myhz;
extern int	pxe;
extern int	securemem;
extern int	vga;
