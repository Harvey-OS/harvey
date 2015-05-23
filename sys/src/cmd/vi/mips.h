/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

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
	uint32_t		addr;		/* Place at address */
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
};

struct Tlb
{
	int	on;			/* Being updated */
	int	tlbsize;		/* Number of entries */
	uint32_t	tlbent[Nmaxtlb];	/* Virtual address tags */
	int	hit;			/* Number of successful tag matches */
	int	miss;			/* Number of failed tag matches */
};		

struct Icache
{
	int	on;			/* Turned on */
	int	linesize;		/* Line size in bytes */
	int	stall;			/* Cache stalls */
	int	*lines;			/* Tag array */
	int*	(*hash)(uint32_t);		/* Hash function */
	char	*hashtext;		/* What the function looks like */
};

struct Inst
{
	void 	(*func)(uint32_t);
	char	*name;
	int	type;
	int	count;
	int	taken;
	int	useddelay;
};

struct Registers
{
	uint32_t	pc;
	uint32_t	ir;
	Inst	*ip;
	long	r[32];
	uint32_t	mhi;
	uint32_t	mlo;

	uint32_t	fpsr;
	union {
		double	fd[16];
		float	fl[32];
		uint32_t	di[32];
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
	uint32_t lo;
	uint32_t hi;
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
	uint32_t	base;
	uint32_t	end;
	uint32_t	fileoff;
	uint32_t	fileend;
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
void		undef(uint32_t);
void		dumpreg(void);
void		dumpfreg(void);
void		dumpdreg(void);
void*		emalloc(uint32_t);
void*		erealloc(void*, uint32_t, uint32_t);
void*		vaddr(uint32_t);
int		badvaddr(uint32_t, int);
void		itrace(char *, ...);
void		segsum(void);
void		Ssyscall(uint32_t);
char*		memio(char*, uint32_t, int, int);
uint32_t		ifetch(uint32_t);
uint32_t		getmem_w(uint32_t);
ushort		getmem_h(uint32_t);
void		putmem_w(uint32_t, uint32_t);
uchar		getmem_b(uint32_t);
void		putmem_b(uint32_t, uchar);
uint32_t		getmem_4(uint32_t);
uint32_t		getmem_2(uint32_t);
void		putmem_h(uint32_t, short);
Mul		mul(long, long);
Mulu		mulu(uint32_t, uint32_t);
void		isum(void);
void		initicache(void);
void		updateicache(uint32_t addr);
void		tlbsum(void);
long		lnrand(long);
void		randseed(long, long);
void		cmd(void);
void		brkchk(uint32_t, int);
void		delbpt(char*);
void		breakpoint(char*, char*);
char*		nextc(char*);
uint32_t		expr(char*);
void		initmap(void);
void		inithdr(int);
void		initstk(int, char**);
void		reset(void);
void		dobplist(void);
int		_mipscoinst(Map*, uvlong, char*, int);
void		procinit(int);
void		printsource(long);
void		printparams(Symbol *, uint32_t);
void		printlocals(Symbol *, uint32_t);
void		stktrace(int);
void		iprofile(void);

/* Globals */
Extern 		Registers reg;
Extern 		Memory memory;
Extern		int text;
Extern		int trace;
Extern 		int sysdbg;
Extern 		int calltree;
Extern		Inst itab[];
Extern		Inst ispec[];
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
Extern		int rtrace;

/* Plan9 Kernel constants */
#define	BY2PG		(16*1024)
#define BY2WD		4
#define UTZERO		0x1000
#define STACKTOP	0x80000000
#define STACKSIZE	0x10000

#define PROFGRAN	4
/* Opcode decoders */
#define Getrsrt(s,t,i)		s = (i>>21)&0x1f; t = (i>>16)&0x1f;
#define Getrbrt(b,t,i)		b = (i>>21)&0x1f; t = (i>>16)&0x1f;
#define Get3(s, t, d, i)	s = (i>>21)&0x1f; t = (i>>16)&0x1f; d = (i>>11)&0x1f;
#define Getf3(s, t, d, i)	s = (i>>11)&0x1f; t = (i>>16)&0x1f; d = (i>>6)&0x1f;
#define Getf2(s, d, i)		s = (i>>11)&0x1f; d = (i>>6)&0x1f;
#define SpecialGetrtrd(t, d, i)	t = (i>>16)&0x1f; d = (i>>11)&0x1f;

#define INOPINST	"nor"
#define INOP		0x00000027	/* Instruction used as nop */
#define SIGNBIT		0x80000000
#define Iexec(ir)	{Inst *i; i = &itab[(ir)>>26]; reg.ip = i; i->count++; (*i->func)(ir); }
#define Statbra()	reg.ip->taken++; if(reg.ir != INOP) reg.ip->useddelay++;

#define FP_U		3
#define FP_L		1
#define FP_G		2
#define FP_E		0
#define FP_CBIT		(1<<23)
