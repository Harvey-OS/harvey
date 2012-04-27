/*
 * sparc sim.h
 *
 * The integer instruction side of this emulator is portable if sizeof(long) >= 4
 * Floating point emulation however is not. Assumptions made are:
 * sizeof(ulong) == sizeof(float)
 * sizeof(ulong)*2 == sizeof(double)
 * bits of floating point in memory may be reversed lsw/msw
 * unions of double & 2*float & 2*long have no padding
 */
#include "/sparc/include/ureg.h"
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
	Inop,
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
	long	r[32];
	ulong	Y;
	ulong	psr;
	ulong	fpsr;

	union {
		double	fd[16];
		float	fl[32];
		ulong	di[32];
	};
};

struct Mulu{
	ulong lo;
	ulong hi;
};

struct Mul{
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
void		ta(ulong);
char*		memio(char*, ulong, int, int);
ulong		getmem_w(ulong);
ulong		ifetch(ulong);
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
long		lnrand(long);
void		randseed(long, long);
void		cmd(void);
void		brkchk(ulong, int);
void		delbpt(char*);
void		breakpoint(char*, char*);
char*		nextc(char*);
ulong		expr(char*);
void		initstk(int, char**);
void		initmap(void);
void		inithdr(int);
void		reset(void);
void		dobplist(void);
void		procinit(int);
void		printsource(long);
void		printparams(Symbol *, ulong);
void		printlocals(Symbol *, ulong);
void		stktrace(int);
void		delay(ulong);
void		iprofile(void);

/* Globals */
Extern 		Registers reg;
Extern 		Memory memory;
Extern		int text;
Extern		int trace;
Extern 		int sysdbg;
Extern 		int calltree;
Extern		Icache icache;
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
Extern		Inst *ci;
Extern		ulong *iprof;
Extern		ulong loadlock;
Extern		ulong anulled;
extern		int datasize;		
extern		int printcol;
Extern		Map *symmap;

/* Plan9 Kernel constants */
#define	BY2PG		4096
#define BY2WD		4
#define UTZERO		0x1000
#define TSTKSIZ		32
#define TSTACKTOP	0x10000000
#define STACKTOP	(TSTACKTOP-TSTKSIZ*BY2PG)
#define STACKSIZE	(4*1024*1024)

#define ANUL		(1<<29)
#define PROFGRAN	4
#define NOP		0x80300000
#define SIGNBIT		0x80000000
#define IMMBIT		(1<<13)
#define getrop23(i)	rd = (i>>25)&0x1f; rs1 = (i>>14)&0x1f; rs2 = i&0x1f;
#define ximm(xx, ii)	xx = ii&0x1FFF; if(xx&0x1000) xx |= ~0x1FFF

#define PSR_n		(1<<23)
#define PSR_z		(1<<22)
#define PSR_v		(1<<21)
#define PSR_c		(1<<20)

#define FP_U		3
#define FP_L		1
#define FP_G		2
#define FP_E		0
