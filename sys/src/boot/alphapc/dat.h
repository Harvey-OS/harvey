typedef struct Hwrpb	Hwrpb;
typedef struct Hwcpu	Hwcpu;
typedef struct Hwcrb	Hwcrb;
typedef struct Hwdsr	Hwdsr;
typedef struct Procdesc	Procdesc;
typedef struct Memdsc	Memdsc;
typedef struct Memclust	Memclust;
typedef struct PCB		PCB;

struct Hwrpb
{
	uvlong	phys;
	uvlong	sign;
	uvlong	rev;
	uvlong	size;
	uvlong	cpu0;
	uvlong	by2pg;
	uvlong	pabits;
	uvlong	maxasn;
	char		ssn[16];
	uvlong	systype;
	uvlong	sysvar;
	uvlong	sysrev;
	uvlong	ifreq;
	uvlong	cfreq;
	uvlong	vptb;
	uvlong	resv;
	uvlong	tbhint;
	uvlong	ncpu;
	uvlong	cpulen;
	uvlong	cpuoff;
	uvlong	nctb;
	uvlong	ctblen;
	uvlong	ctboff;
	uvlong	crboff;
	uvlong	memoff;
	uvlong	confoff;
	uvlong	fruoff;
	uvlong	termsaveva;
	uvlong	termsavex;
	uvlong	termrestva;
	uvlong	termrestx;
	uvlong	termresetva;
	uvlong	termresetx;
	uvlong	sysresv;
	uvlong	hardresv;
	uvlong	csum;
	uvlong	rxrdymsk;
	uvlong	txrdymsk;
	uvlong	dsroff;		/* rev 6 or higher */
};

extern Hwrpb* hwrpb;

struct Hwcpu
{
	uvlong	hwpcb[16];
	uvlong	state;
	uvlong	palmainlen;
	uvlong	palscratchlen;
	uvlong	palmainpa;
	uvlong	palscratchpa;
	uvlong	palrev;
	uvlong	cputype;
	uvlong	cpuvar;
	uvlong	cpurev;
	uvlong	serial[2];
	/* more crap ... */
};

struct Hwdsr
{
	vlong	smm;
	uvlong	lurtoff;
	uvlong	sysnameoff;
};

struct Hwcrb
{
	uvlong	dispatchva;
	uvlong	dispatchpa;
	uvlong	fixupva;
	uvlong	fixuppa;
	/* more, uninteresting crud */
};

struct Procdesc
{
	uvlong	bollocks;
	uvlong	addr;
};

struct Memclust
{
	uvlong	pfn;
	uvlong	npages;
	uvlong	ntest;
	uvlong	vabitm;
	uvlong	pabitm;
	uvlong	csumbitm;
	uvlong	usage;
};

struct Memdsc
{
	uvlong	csum;
	uvlong	opt;
	uvlong	nclust;
	Memclust	clust[1];
};

enum
{
	PRINTSIZE =	256,
	MB =		(1024*1024),
};

#define L_MAGIC		((((4*23)+0)*23)+7)

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

enum {
	Eaddrlen	= 6,
	ETHERMINTU	= 60,		/* minimum transmit size */
	ETHERMAXTU	= 1514,		/* maximum transmit size */
	ETHERHDRSIZE	= 14,		/* size of an ethernet header */

	MaxEther	= 2,
};

typedef struct {
	uchar	d[Eaddrlen];
	uchar	s[Eaddrlen];
	uchar	type[2];
	uchar	data[1500];
	uchar	crc[4];
} Etherpkt;

/*
 *	Process Control Block, used by OSF/1 PALcode when we switch to it
 */
struct PCB {
	uvlong	ksp;
	uvlong	usp;
	uvlong	ptbr;
	ulong	asn;
	ulong	pcc;
	uvlong	unique;
	ulong	fen;
	ulong	dummy;
	uvlong	rsrv1;
	uvlong	rsrv2;
};


#include	"conf.h"

extern Bootconf	conf;
