/*
 * arm.h
 */
#ifndef	EXTERN
#define	EXTERN	extern
#endif

typedef	struct	Registers	Registers;
typedef	struct	Segment		Segment;
typedef	struct	Memory		Memory;
typedef	struct	Mul		Mul;
typedef	struct	Mulu		Mulu;
typedef	struct	Inst		Inst;
typedef	struct	Icache		Icache;
typedef	struct	Tlb		Tlb;
typedef	struct	Breakpoint	Breakpoint;

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
	Breakpoint*	next;		/* Link to next one */
};

enum
{
	Imem,
	Iarith,
	Ibranch,
	Isyscall,
};

enum
{
	Nmaxtlb = 64,
	REGARG	= 0,
	REGRET	= 0,
	REGPC	= 15,
	REGLINK	= 14,
	REGSP	= 13,
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
	int*	lines;			/* Tag array */
	int*	(*hash)(ulong);		/* Hash function */
	char*	hashtext;		/* What the function looks like */
};

struct Inst
{
	void 	(*func)(ulong);
	char*	name;
	int	type;
	int	count;
	int	taken;
	int	useddelay;
};

struct Registers
{
	ulong	ar;
	ulong	ir;
	Inst*	ip;
	long	r[16];
	long	cc1;
	long	cc2;
	int	class;
	int	cond;
	int	compare_op;
	int	cbit;
	int	cout;
};

enum
{
	FPd	= 0,
	FPs,
	FPmemory,
};

enum
{
	MemRead,
	MemReadstring,
	MemWrite,
};

enum
{
	CCcmp, 
	CCtst,
	CCteq,
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
	uchar**	table;
};

struct Memory
{
	Segment	seg[Nseg];
};

void		Ssyscall(ulong);
int		armclass(long);
void		breakpoint(char*, char*);
void		brkchk(ulong, int);
void		cmd(void);
void		delbpt(char*);
void		dobplist(void);
void		dumpdreg(void);
void		dumpfreg(void);
void		dumpreg(void);
void*		emalloc(ulong);
void*		erealloc(void*, ulong, ulong);
ulong		expr(char*);
void		fatal(int, char*, ...);
ulong		getmem_2(ulong);
ulong		getmem_4(ulong);
uchar		getmem_b(ulong);
ushort		getmem_h(ulong);
uvlong		getmem_v(ulong);
ulong		getmem_w(ulong);
ulong		ifetch(ulong);
void		inithdr(int);
void		initicache(void);
void		initmap(void);
void		initstk(int, char**);
void		iprofile(void);
void		isum(void);
void		itrace(char*, ...);
long		lnrand(long);
char*		memio(char*, ulong, int, int);
int		_mipscoinst(Map*, ulong, char*, int);
Mul		mul(long, long);
Mulu		mulu(ulong, ulong);
char*		nextc(char*);
void		printlocals(Symbol*, ulong);
void		printparams(Symbol*, ulong);
void		printsource(long);
void		procinit(int);
void		putmem_b(ulong, uchar);
void		putmem_h(ulong, ushort);
void		putmem_v(ulong, uvlong);
void		putmem_w(ulong, ulong);
void		reset(void);
void		run(void);
void		segsum(void);
void		stktrace(int);
void		tlbsum(void);
void		undef(ulong);
void		updateicache(ulong addr);
void*		vaddr(ulong);

/* Globals */
EXTERN	Registers	reg;
EXTERN	Memory		memory;
EXTERN	int		text;
EXTERN	int		trace;
EXTERN	int		sysdbg;
EXTERN	int		calltree;
EXTERN	Inst		itab[];
EXTERN	Inst		ispec[];
EXTERN	Icache		icache;
EXTERN	Tlb		tlb;
EXTERN	int		count;
EXTERN	jmp_buf		errjmp;
EXTERN	Breakpoint*	bplist;
EXTERN	int		atbpt;
EXTERN	int		membpt;
EXTERN	int		cmdcount;
EXTERN	int		nopcount;
EXTERN	ulong		dot;
EXTERN	char*		file;
EXTERN	Biobuf*		bioout;
EXTERN	Biobuf*		bin;
EXTERN	ulong*		iprof;
EXTERN	int		datasize;
EXTERN	Map*		symmap;	

/* Plan9 Kernel constants */
enum
{
	BY2PG		= 4096,
	BY2WD		= 4,
	UTZERO		= 0x1000,
	STACKTOP	= 0x80000000,
	STACKSIZE	= 0x10000,

	PROFGRAN	= 4,
	Sbit		= 1<<20,
	SIGNBIT		= 0x80000000,

	FP_U		= 3,
	FP_L		= 1,
	FP_G		= 2,
	FP_E		= 0,
	FP_CBIT		= 1<<23,
};
