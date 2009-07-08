/*
 * power sim.h
 *
 * The integer instruction side of this emulator is portable if sizeof(long) >= 4
 * Floating point emulation assumes:
 *	sizeof(ulong) == sizeof(float)
 *	sizeof(ulong)*2 == sizeof(double) <= sizeof(vlong)
 *	unions of double & vlong have no padding
 *	vlongs provide at least 64 bits precision
 */
#include "/power/include/ureg.h"
#define	USERADDR	0xC0000000
#define	UREGADDR	(USERADDR+BY2PG-4-0xA0)
#define USER_REG(x)	(UREGADDR+(ulong)(x))
#define	REGOFF(x)	(USER_REG(&((struct Ureg *) 0)->x))

typedef struct Registers Registers;
typedef struct Segment Segment;
typedef struct Memory Memory;
typedef struct Inset Inset;
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
	Ilog,
	Ibranch,
	Ireg,
	Isyscall,
	Ifloat,
	Inop,
	Icontrol,
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

struct Inset
{
	Inst	*tab;		/* indexed by extended opcode */
	int	nel;
};

struct Inst
{
	void 	(*func)(ulong);
	char	*name;
	int	type;
	int	count;
	int	taken;
};

struct Registers
{
	ulong	pc;
	ulong	ir;
	Inst	*ip;
	long	r[32];
	ulong	ctr;
	ulong	cr;
	ulong	xer;
	ulong	lr;
	ulong	fpscr;
	ulong	dec;
	ulong	tbl;
	ulong	tbu;
	double	fd[32];
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
void		fpreginit(void);
void		run(void);
void		undef(ulong);
void		unimp(ulong);
void		dumpreg(void);
void		dumpfreg(void);
void		dumpdreg(void);
void*		emalloc(ulong);
void*		erealloc(void*, ulong, ulong);
void*		vaddr(ulong);
void		itrace(char *, ...);
void		segsum(void);
void		sc(ulong);
char*		memio(char*, ulong, int, int);
ulong		getmem_w(ulong);
ulong		ifetch(ulong);
ushort		getmem_h(ulong);
void		putmem_w(ulong, ulong);
uchar		getmem_b(ulong);
void		putmem_b(ulong, uchar);
uvlong	getmem_v(ulong);
ulong		getmem_4(ulong);
ulong		getmem_2(ulong);
void	putmem_v(ulong, uvlong);
void		putmem_h(ulong, short);
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
Extern		ulong	iprofsize;
Extern		ulong loadlock;
extern		int datasize;		
extern		int printcol;
Extern		Map *symmap;
extern		ulong bits[];

extern		Inset ops0, ops19, ops31, ops59, ops63a, ops63b;

/* Plan9 Kernel constants */
#define	BY2PG		4096
#define BY2WD		4
#define UTZERO		0x1000
#define TSTKSIZ		32
#define TSTACKTOP	0x20000000
#define STACKTOP	(TSTACKTOP-TSTKSIZ*BY2PG)
#define STACKSIZE	(4*1024*1024)

#define PROFGRAN	4
#define NOP		0x80300000
#define SIGNBIT		0x80000000


enum {
	CRLT = 1<<31,
	CRGT = 1<<30,
	CREQ = 1<<29,
	CRSO = 1<<28,
	CRFU = CRSO,

	CRFX = 1<<27,
	CRFEX = 1<<26,
	CRVX = 1<<25,
	CROX = 1<<24,
};

#define getCR(x,w) (((w)>>(28-(x*4)))&0xF)
#define mkCR(x,v) (((v)&0xF)<<(28-(x*4)))

#define simm(xx, ii)	xx = (short)(ii&0xFFFF);
#define uimm(xx, ii)	xx = ii&0xFFFF;
#define imms(xx, ii) xx = ii<<16;
#define getairr(i)	rd = (i>>21)&0x1f; ra = (i>>16)&0x1f; simm(imm,i)
#define getarrr(i)	rd = (i>>21)&0x1f; ra = (i>>16)&0x1f; rb = (i>>11)&0x1f;
#define getbobi(i)	bo = (i>>21)&0x1f; bi = (i>>16)&0x1f; xx = (i>>11)&0x1f;
#define getlirr(i)	rs = (i>>21)&0x1f; ra = (i>>16)&0x1f; uimm(imm,i)
#define getlrrr(i)	rs = (i>>21)&0x1f; ra = (i>>16)&0x1f; rb = (i>>11)&0x1f;

#define OP(o,xo) ((o<<26)|(xo<<1))	/* build an operation */
#define xop(a,b) ((b<<6)|a)	/* compact form for use in a decoding switch on op/xo */
#define getop(i) ((i>>26)&0x3F)
#define getxo(i) ((i>>1)&0x3FF)

#define	FPS_FX	(1<<31)	/* exception summary (sticky) */
#define	FPS_EX	(1<<30)	/* enabled exception summary */
#define	FPS_VX	(1<<29)	/* invalid operation exception summary */
#define	FPS_OX	(1<<28)	/* overflow exception OX (sticky) */
#define	FPS_UX	(1<<27)	/* underflow exception UX (sticky) */
#define	FPS_ZX	(1<<26)	/* zero divide exception ZX (sticky) */
#define	FPS_XX	(1<<25)	/* inexact exception XX (sticky) */
#define	FPS_VXSNAN (1<<24)	/* invalid operation exception for SNaN (sticky) */
#define	FPS_VXISI	(1<<23)	/* invalid operation exception for ∞-∞ (sticky) */
#define	FPS_VXIDI	(1<<22)	/* invalid operation exception for ∞/∞ (sticky) */
#define	FPS_VXZDZ (1<<21)	/* invalid operation exception for 0/0 (sticky) */
#define	FPS_VXIMZ	(1<<20)	/* invalid operation exception for ∞*0 (sticky) */
#define	FPS_VXVC	(1<<19)	/* invalid operation exception for invalid compare (sticky) */
#define	FPS_FR	(1<<18)	/* fraction rounded */
#define	FPS_FI	(1<<17)	/* fraction inexact */
#define	FPS_FPRF	(1<<16)	/* floating point result class */
#define	FPS_FPCC	(0xF<<12)	/* <, >, =, unordered */
#define	FPS_VXCVI	(1<<8)	/* enable exception for invalid integer convert (sticky) */
#define	FPS_VE	(1<<7)	/* invalid operation exception enable */
#define	FPS_OE	(1<<6)	/* enable overflow exceptions */
#define	FPS_UE	(1<<5)	/* enable underflow */
#define	FPS_ZE	(1<<4)	/* enable zero divide */
#define	FPS_XE	(1<<3)	/* enable inexact exceptions */
#define	FPS_RN	(3<<0)	/* rounding mode */

#define	XER_SO	(1<<31)
#define	XER_OV	(1<<30)
#define	XER_CA	(1<<29)

#define	Rc	1
#define	OE	0x400
