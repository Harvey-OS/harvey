typedef struct List {
	void	*next;
} List;

typedef struct Alarm Alarm;
typedef struct Alarm {
	List;
	int	busy;
	long	dt;
	void	(*f)(Alarm*);
	void	*arg;
} Alarm;

typedef struct Apminfo {
	int haveinfo;
	int ax;
	int cx;
	int dx;
	int di;
	int ebx;
	int esi;
} Apminfo;

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

typedef struct IOQ IOQ;
typedef struct IOQ {
	uchar	buf[4096];
	uchar	*in;
	uchar	*out;
	int	state;
	int	(*getc)(IOQ*);
	int	(*putc)(IOQ*, int);
	void	*ptr;
};

enum {
	Eaddrlen	= 6,
	/* next two exclude 4-byte ether CRC */
	ETHERMINTU	= 60,		/* minimum transmit size */
	ETHERMAXTU	= 1514,		/* maximum transmit size */
	ETHERHDRSIZE	= 14,		/* size of an ethernet header */

	MaxEther	= 6,
};

typedef struct {
	uchar	d[Eaddrlen];
	uchar	s[Eaddrlen];
	uchar	type[2];
	uchar	data[1500];
	uchar	crc[4];
} Etherpkt;

extern uchar broadcast[Eaddrlen];

typedef struct Ureg Ureg;
#pragma incomplete Ureg

typedef struct Segdesc {
	ulong	d0;
	ulong	d1;
} Segdesc;

typedef struct Mach {
	ulong	ticks;			/* of the clock since boot time */
	void	*alarm;			/* alarms bound to this clock */
} Mach;

extern Mach *m;

#define I_MAGIC		((((4*11)+0)*11)+7)

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

/*
 *  a parsed .ini line
 */
#define ISAOPTLEN	32
#define NISAOPT		8

typedef struct  ISAConf {
	char	type[NAMELEN];
	ulong	port;
	ulong	irq;
	ulong	mem;
	ulong	size;
	uchar	ea[6];

	int	nopt;
	char	opt[NISAOPT][ISAOPTLEN];
} ISAConf;

typedef struct Pcidev Pcidev;
typedef struct PCMmap PCMmap;
typedef struct PCMslot PCMslot;

#define BOOTLINE	((char*)CONFADDR)

enum {
	MB =		(1024*1024),
};
#define ROUND(s, sz)	(((s)+((sz)-1))&~((sz)-1))


typedef struct Type Type;
typedef struct Medium Medium;
typedef struct Boot Boot;

enum {					/* type */
	Tnil		= 0x00,

	Tfloppy		= 0x01,
	Tsd		= 0x02,
	Tether		= 0x03,
	Tcd		= 0x04,
	Tbios		= 0x05,

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
	int	type;
	int	flag;
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
extern void (*floppydetach)(void);
extern void (*sddetach)(void);

typedef struct Lock {	/* for ilock, iunlock */
	int locked;
	int spl;
} Lock;

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

extern int	debug;
extern Apminfo	apm;
extern char	*defaultpartition;
extern int	iniread;
extern int	pxe;
