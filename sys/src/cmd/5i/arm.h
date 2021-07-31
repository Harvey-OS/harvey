/*
 * mipsim.h
 */
#include "/mips/include/ureg.h"
#define	USERADDR	0xC0000000
#define	UREGADDR	(USERADDR+BY2PG-4-0xA0)
#define USER_REG(x)	(UREGADDR+(ulong)(x))
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
	Nmaxtlb = 64,
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
	ulong	ar;
	ulong	ir;
	Inst	*ip;
	long	r[16];
	long	cc1;
	long	cc2;
	int	cond;
	int	compare_op; 
};

enum
{
	FPd	= 0,
	FPs,
	FPmemory,
};
#define dreg(r)	((r)>>1)

enum
{
	MemRead,
	MemReadstring,
	MemWrite,
};

enum
{
	CCcmp, 
	CCcmn,
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
	uchar	**table;
};

struct Memory
{
	Segment	seg[Nseg];
};

void		fatal(int, char*, ...);
void		run(void);
void		undef(ulong);
void		dumpreg(void);
void		dumpfreg(void);
void		dumpdreg(void);
void*		emalloc(ulong);
void*		erealloc(void*, ulong, ulong);
void*		vaddr(ulong);
void		itrace(char *, ...);
void		segsum(void);
void		Ssyscall(ulong);
char*		memio(char*, ulong, int, int);
ulong		ifetch(ulong);
ulong		getmem_w(ulong);
ushort		getmem_h(ulong);
void		putmem_w(ulong, ulong);
uchar		getmem_b(ulong);
void		putmem_b(ulong, uchar);
ulong		getmem_4(ulong);
ulong		getmem_2(ulong);
void		putmem_h(ulong, short);
Mul		mul(long, long);
Mulu		mulu(ulong, ulong);
void		isum(void);
void		initicache(void);
void		updateicache(ulong addr);
void		tlbsum(void);
long		lnrand(long);
void		randseed(long, long);
void		cmd(void);
void		brkchk(ulong, int);
void		delbpt(char*);
void		breakpoint(char*, char*);
char*		nextc(char*);
ulong		expr(char*);
void		initmap(void);
void		inithdr(int);
void		initstk(int, char**);
void		reset(void);
void		dobplist(void);
int		_mipscoinst(Map*, ulong, char*, int);
void		procinit(int);
void		printsource(long);
void		printparams(Symbol *, ulong);
void		printlocals(Symbol *, ulong);
void		stktrace(int);
void		iprofile(void);
int		_armclass(int, long);

/* Globals */
Extern 		Registers reg;
Extern 		Memory memory;
Extern		int text;
Extern		int trace;
Extern 		int sysdbg;
Extern 		int calltree;
Extern		Inst itab[];
Extern  	Inst ispec[];
Extern		Icache icache;
Extern		Tlb tlb;
Extern		int count;
Extern		jmp_buf errjmp;
Extern		Breakpoint *bplist;
Extern		int atbpt;
Extern		int membpt;
Extern		int cmdcount;
Extern		int nopcount;
Extern		ulong dot;
extern		char *file;
Extern		Biobuf *bioout;
Extern		Biobuf *bin;
Extern		ulong *iprof;
extern		int datasize;
Extern		Map *symmap;		

/* Plan9 Kernel constants */
#define	BY2PG		4096
#define BY2WD		4
#define UTZERO		0x1000
#define STACKTOP	0x80000000
#define STACKSIZE	0x10000

#define PROFGRAN	4
#define Sbit		(1<<20)
#define SIGNBIT		0x80000000

#define FP_U		3
#define FP_L		1
#define FP_G		2
#define FP_E		0
#define FP_CBIT		(1<<23)
