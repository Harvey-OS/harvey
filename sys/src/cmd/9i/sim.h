/*
 * 29000 sim.h
 */
#include "/29000/include/ureg.h"
#define	USERADDR	0xC0000000
#define	UREGADDR	(USERADDR+BY2PG-4-0xA0)
#define	USER_REG(x)	(UREGADDR+(ulong)(x))
#define	REGOFF(x)	(USER_REG(&((struct Ureg *) 0)->x))

typedef struct Registers Registers;
typedef struct Segment Segment;
typedef struct Memory Memory;
typedef struct Mul Mul;
typedef struct Mulu Mulu;
typedef struct Inst Inst;
typedef struct Icache Icache;
typedef struct Tlb Tlb;
typedef struct Breakpoint Breakpoint;
typedef struct Namedreg		Namedreg;

struct	Namedreg
{
	char	*name;
	ulong	*reg;
};

enum
{
	Instruction	= 1,
	Read		= 2,
	Write		= 4,
	Access		= 2|4,
	Equal		= 4|8,
};

struct Breakpoint
{
	int		type;		/* Instruction/Read/Access/Write/Equal */
	ulong		addr;		/* Place at address */
	int		count;		/* To execute count times or value */
	int		done;		/* How many times passed through */
	Breakpoint	*next;		/* Link to next one */
};

enum
{
	Iload,
	Istore,
	Iarith,
	Ibranch,
	Ireg,
	Isyscall,
	Ifloat,
};

enum
{
	NREG	= 256,
	REGSP	= 65,
	REGLINK	= 64,
	REGARG	= 69,
	REGRET	= 69,

	Nmaxtlb = 64,
	True	= 1L<<31,

	Sipc	= 128,
	Sipa	= 129,
	Sipb	= 130,
	Sq	= 131,
	Salusr	= 132,
		DF	= 0x80,
		N	= 0x20,

};

struct Tlb
{
	int	on;			/* Being updated */
	int	tlbsize;		/* Number of entries */
	ulong	tlbent[Nmaxtlb];	/* Virtual address tags */
	int	hit;			/* Number of successful tag matches */
	int	miss;			/* Number of failed tag matches */
};		

struct Icache
{
	int	on;			/* Turned on */
	int	linesize;		/* Line size in bytes */
	int	stall;			/* Cache stalls */
	int	*lines;			/* Tag array */
	int*	(*hash)(ulong);		/* Hash function */
	char	*hashtext;		/* What the function looks like */
};

struct Inst
{
	void 	(*func)(ulong);
	char	*name;
	int	type;
	int	count;
	int	taken;
	int	useddelay;
};

struct Registers
{
	ulong	pc;
	ulong	ir;
	Inst	*ip;
	long	r[NREG];
	long	s[NREG];

	ulong	fpsr;
	union {
		double	fd[16];
		float	fl[32];
		ulong	di[32];
	};
	char	ft[32];
};

enum
{
	FPd	= 0,
	FPs,
	FPmemory,
};
#define dreg(r)	((r)>>1)

struct Mulu
{
	ulong lo;
	ulong hi;
};

struct Mul
{
	long lo;
	long hi;
};

enum
{
	MemRead,
	MemReadstring,
	MemWrite,
};

enum
{
	Stack,
	Text,
	Data,
	Bss,
	Nseg,
};

struct Segment
{
	short	type;
	ulong	base;
	ulong	end;
	ulong	fileoff;
	ulong	fileend;
	int	rss;
	int	refs;
	uchar	**table;
};

struct Memory
{
	Segment	seg[Nseg];
};

int		membrk(ulong);
int		fmtins(char*, int, int, ulong, ulong);
int		ffmt(char*, int, int, ulong, ulong);
int		Ffmt(char*, int, int, ulong, ulong);
void		Ssyscall(ulong);
void		breakpoint(char*);
void		brkchk(ulong, int);
void		cachesum(void);
void		clearprof(void);
void		cmd(void);
void		delbpt(void);
void		dobplist(void);
void		dumpdreg(void);
void		dumpfreg(void);
void		dumpreg(void);
void*		emalloc(ulong);
void*		erealloc(void*, ulong);
void		error(char*, ...);
ulong		expr(char*);
void		fatal(char*, ...);
uchar		getmem_1(ulong);
ushort		getmem_2(ulong);
ulong		getmem_4(ulong);
uchar		getmem_b(ulong);
ushort		getmem_h(ulong);
ulong		getmem_w(ulong);
ulong		ifetch(ulong);
void		inithdr(int);
void		initicache(void);
void		initmap(void);
void		initstk(int, char**);
void		iprofile(void);
void		isum(void);
void		itrace(char *, ...);
long		lnrand(long);
char*		memio(char*, ulong, int, int);
Mul		mul(long, long);
Mulu		mulu(ulong, ulong);
char*		nextc(char*);
void		printlocals(Symbol *, ulong);
void		printparams(Symbol *, ulong);
void		printsource(long);
void		procinit(int);
void		putmem_1(ulong, uchar);
void		putmem_2(ulong, ushort);
void		putmem_4(ulong, ulong);
void		putmem_b(ulong, uchar);
void		putmem_h(ulong, ushort);
void		putmem_w(ulong, ulong);
void		randseed(long, long);
void		reset(void);
ulong		run(void);
void		segsum(void);
void		stktrace(int);
void		tlbsum(void);
void		undef(ulong);
void		updateicache(ulong addr);
void*		vaddr(ulong, int);

/* Globals */
Extern	ulong		Bdata;
Extern	ulong		Btext;
Extern	ulong		Edata;
Extern	ulong		Etext;
Extern 	Registers		reg;
Extern 	Memory		memory;
Extern	int		text;
Extern	int		trace;
Extern	int		sysdbg;
Extern	int		calltree;
Extern	Inst		itab[];
Extern	Inst		ispec[];
Extern	Icache		icache;
Extern	Tlb		tlb;
extern	Namedreg	namedreg[];
Extern	int		count;
Extern	jmp_buf		errjmp;
Extern	Breakpoint	*bplist;
Extern	int		atbpt;
Extern	int		membpt;
Extern	int		cmdcount;
Extern	int		nopcount;
Extern	ulong		dot;
extern	char		*file;
Extern	Biobuf		*bioout;
Extern	Biobuf		*bin;
Extern	ulong		*iprof;
Extern	int		datasize;
Extern	Map		*symmap;		
Extern	char		errbuf[ERRLEN];

/* Plan9 Kernel constants */
#define	BY2PG		4096
#define BY2WD		4
#define UTZERO		0x1000
#define STACKTOP	0x80000000
#define STACKSIZE	0x10000

#define PROFGRAN	4
/* Opcode decoders */
#define Getrrr(c,a,b,i)\
	a = (i>>8)&0xff;\
	b = i&0xff;\
	c = (i>>16)&0xff;

#define Getir(im,a,i)\
	a = (i>>8)&0xff;\
	im = (i&0xff) | ((i>>8)&0xff00);

#define	GetIa()\
	if(ra == 0) ra = (reg.s[Sipa]>>2) & 0xff;

#define	GetIb()\
	if(rb == 0) rb = (reg.s[Sipb]>>2) & 0xff;

#define	GetIc()\
	if(rc == 0) rc = (reg.s[Sipc]>>2) & 0xff;

#define	GetIab()\
	if(ra == 0) ra = (reg.s[Sipa]>>2) & 0xff;\
	if(rb == 0) rb = (reg.s[Sipb]>>2) & 0xff;

#define	GetIac()\
	if(ra == 0) ra = (reg.s[Sipa]>>2) & 0xff;\
	if(rc == 0) rc = (reg.s[Sipc]>>2) & 0xff;

#define	GetIbc()\
	if(rb == 0) rb = (reg.s[Sipb]>>2) & 0xff;\
	if(rc == 0) rc = (reg.s[Sipc]>>2) & 0xff;

#define	GetIabc()\
	if(ra == 0) ra = (reg.s[Sipa]>>2) & 0xff;\
	if(rb == 0) rb = (reg.s[Sipb]>>2) & 0xff;\
	if(rc == 0) rc = (reg.s[Sipc]>>2) & 0xff;

#define INOPINST	"nor"
#define INOP		0x921c1c1c	/* Instruction used as nop */
#define SIGNBIT		0x80000000
#define Iexec(ir)	{Inst *i; i = &itab[(ir)>>24]; reg.ip = i; i->count++; (*i->func)(ir); }
#define Statbra()	reg.ip->taken++; if(reg.ir != INOP) reg.ip->useddelay++;
