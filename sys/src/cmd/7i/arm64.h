/*
 * arm64.h
 */
#include "/arm64/include/ureg.h"
#define	USERADDR	0xC0000000
#define	UREGADDR	(USERADDR+BY2PG-4-0xA0)
#define	REGOFF(x)	offsetof(Ureg, x)

typedef struct Pstate Pstate;
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
	uvlong		addr;		/* Place at address */
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
	Isys,
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

struct Pstate
{
	char	N, Z, C, V;
};

struct Registers
{
	ulong	pc;
	ulong	prevpc;
	ulong	ir;
	Inst	*ip;
	vlong	r[32];
	Pstate;
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
	uvlong	base;
	uvlong	end;
	uvlong	fileoff;
	uvlong	fileend;
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
void		dumpreg(void);
void		dumpfreg(void);
void		dumpdreg(void);
void*		emalloc(ulong);
void*		erealloc(void*, ulong, ulong);
void*		vaddr(uvlong);
void		itrace(char *, ...);
void		segsum(void);
void	syscall(ulong);
char*		memio(char*, uvlong, int, int);
ulong		getmem_w(uvlong);
ulong		ifetch(uvlong);
ushort		getmem_h(uvlong);
void		putmem_w(uvlong, ulong);
uchar		getmem_b(uvlong);
void		putmem_b(uvlong, uchar);
uvlong	getmem_v(uvlong);
ulong		getmem_4(uvlong);
ulong		getmem_2(uvlong);
ulong	getxo(ulong);
void	putmem_v(uvlong, uvlong);
void		putmem_h(uvlong, short);
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
uvlong		expr(char*);
void		initstk(int, char**);
void		initmap(void);
void		inithdr(int);
void		reset(void);
void		dobplist(void);
void		procinit(int);
void		printsource(long);
void		printparams(Symbol *, uvlong);
void		printlocals(Symbol *, uvlong);
void		stktrace(int);
void		iprofile(void);

/* Globals */
Extern 		Registers reg;
Extern 		Memory memory;
Extern		int text;
Extern		int trace;
Extern 		int sysdbg;
Extern 		int calltree;
Extern 		int regdb;
Extern		Icache icache;
Extern		int count;
Extern		jmp_buf errjmp;
Extern		Breakpoint *bplist;
Extern		int atbpt;
Extern		int membpt;
Extern		int cmdcount;
Extern		int nopcount;
Extern		uvlong dot;
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

/* Plan9 Kernel constants */
#define	BY2PG		4096
#define BY2WD		8
#define STACKTOP	0x00007ffffffff000ull
#define STACKSIZE	0x10000

#define PROFGRAN	4

/* instruction classes: iota << 6. */
enum {
	Ccmpb  =    0,	/* compare and branch */
	Ccb    =   64,	/* conditional branch */
	Cex    =  128,	/* exception generation */
	Csys   =  192,	/* system */
	Ctb    =  256,	/* test and branch */
	Cubi   =  320,	/* unconditional branch imm */
	Cubr   =  384,	/* unconditional branch reg */
	Cai    =  448,	/* add/sub imm */
	Cab    =  512,	/* bitfield */
	Cax    =  576,	/* extract */
	Cali   =  640,	/* logic imm */
	Camwi  =  704,	/* move wide imm */
	Capcr  =  768,	/* PC-rel addr */
	Car    =  832,	/* add/sub extended reg */
	Casr   =  896,	/* add/sub shift-reg */
	Cac    =  960,	/* add/sub carry */
	Caci   = 1024,	/* cond compare imm */
	Cacr   = 1088,	/* cond compare reg */
	Cacs   = 1152,	/* cond select */
	Ca1    = 1216,	/* data proc 1 src */
	Ca2    = 1280,	/* data proc 2 src */
	Ca3    = 1344,	/* data proc 3 src */
	Calsr  = 1408,	/* logic shift-reg */
	Clsr   = 1472,	/* load/store reg */
	Clsx   = 1536,	/* load/store ex */
	Clsnp  = 1600,	/* load/store no-alloc pair (off) */
	Clspos = 1664,	/* load/store reg (imm post-index) */
	Clspre = 1728,	/* load/store reg (imm pre-index) */
	Clso   = 1792,	/* load/store reg (off) */
	Clsu   = 1856,	/* load/store reg (unpriv) */
	Clsuci = 1920,	/* load/store reg (unscaled imm) */
	Clsusi = 1984,	/* load/store reg (unsigned imm) */
	Clsrpo = 2048,	/* load/store reg-pair (off) */
	Clsppo = 2112,	/* load/store reg-pair (post-index) */
	Clsppr = 2176,	/* load/store reg-pair (pre-index) */
	Cundef = 2208	/* undefined instruction */
};

/* initialize variables in Inst functions */
#define getcmpb(i) 	sf = (i>>31)&0x1; op = (i>>24)&0x1; imm19 = (i>>5)&0x7ffff; Rt = i&0x1f; 
#define getcb(i)   	o1 = (i>>24)&0x1; imm19 = (i>>5)&0x7ffff; o0 = (i>>4)&0x1; cond = i&0xf; 
#define getex(i)   	opc = (i>>21)&0x7; imm16 = (i>>5)&0xffff; LL = i&0x3; 
#define getsys(i)  	CRn = (i>>12)&0x7; CRm = (i>>8)&0xf; op2 = (i>>5)&0x7; Rt = i&0x1f;  
#define gettb(i)   	b5 = (i>>31)&0x1; op = (i>>24)&0x1; b40 = (i>>19)&0x1f; imm14 = (i>>5)&0x3fff; Rt = i&0x1f; 
#define getubi(i)  	op = (i>>31)&0x1; imm26 = i&0x3ffffff; 
#define getubr(i)  	opc = (i>>21)&0xf; op2 = (i>>16)&0x1f; Rn = (i>>5)&0x1f; 
#define getai(i)   	sf = (i>>31)&0x1; op = (i>>30)&0x1; S = (i>>29)&0x1; shift = (i>>22)&0x3; imm12 = (i>>10)&0xfff; Rn = (i>>5)&0x1f; Rd = i&0x1f; 
#define getab(i)   	sf = (i>>31)&0x1; opc = (i>>29)&0x3; N = (i>>22)&0x1; immr = (i>>16)&0x3f; imms = (i>>10)&0x3f; Rn = (i>>5)&0x1f; Rd = i&0x1f; 
#define getax(i)   	sf = (i>>31)&0x1; op21 = (i>>29)&0x3; N = (i>>22)&0x1; o0 = (i>>21)&0x1; Rm = (i>>16)&0x1f; imms = (i>>10)&0x3f; Rn = (i>>5)&0x1f; Rd = i&0x1f; 
#define getali(i)  	sf = (i>>31)&0x1; opc = (i>>29)&0x3; N = (i>>22)&0x1; immr = (i>>16)&0x3f; imms = (i>>10)&0x3f; Rn = (i>>5)&0x1f; Rd = i&0x1f; 
#define getamwi(i) 	sf = (i>>31)&0x1; opc = (i>>29)&0x3; hw = (i>>21)&0x3; imm16 = (i>>5)&0xffff; Rd = i&0x1f; 
#define getapcr(i) 	op = (i>>31)&0x1; immlo = (i>>29)&0x3; immhi = (i>>5)&0x7ffff; Rd = i&0x1f; 
#define getar(i)   	sf = (i>>31)&0x1; op = (i>>30)&0x1; S = (i>>29)&0x1; opt = (i>>22)&0x3; Rm = (i>>16)&0x1f; option = (i>>13)&0x7; imm3 = (i>>10)&0x7; Rn = (i>>5)&0x1f; Rd = i&0x1f; 
#define getasr(i)  	sf = (i>>31)&0x1; op = (i>>30)&0x1; S = (i>>29)&0x1; shift = (i>>22)&0x3; Rm = (i>>16)&0x1f; imm6 = (i>>10)&0x3f; Rn = (i>>5)&0x1f; Rd = i&0x1f; 
#define getac(i)   	sf = (i>>31)&0x1; op = (i>>30)&0x1; S = (i>>29)&0x1; Rm = (i>>16)&0x1f; Rn = (i>>5)&0x1f; Rd = i&0x1f; 
#define getaci(i)  	sf = (i>>31)&0x1; op = (i>>30)&0x1; S = (i>>29)&0x1; imm5 = (i>>16)&0x1f; cond = (i>>12)&0xf; Rn = (i>>5)&0x1f; nzcv = i&0xf; 
#define getacr(i)  	sf = (i>>31)&0x1; op = (i>>30)&0x1; S = (i>>29)&0x1; Rm = (i>>16)&0x1f; cond = (i>>12)&0xf; Rn = (i>>5)&0x1f; nzcv = i&0xf; 
#define getacs(i)  	sf = (i>>31)&0x1; op = (i>>30)&0x1; S = (i>>29)&0x1; Rm = (i>>16)&0x1f; cond = (i>>12)&0xf; op2 = (i>>10)&0x3; Rn = (i>>5)&0x1f; Rd = i&0x1f; 
#define geta1(i)   	sf = (i>>31)&0x1; S = (i>>29)&0x1; opcode = (i>>10)&0x7; Rn = (i>>5)&0x1f; Rd = i&0x1f; 
#define geta2(i)   	sf = (i>>31)&0x1; Rm = (i>>16)&0x1f; opcode = (i>>10)&0x1f; Rn = (i>>5)&0x1f; Rd = i&0x1f; 
#define geta3(i)   	sf = (i>>31)&0x1; op31 = (i>>21)&0x7; Rm = (i>>16)&0x1f; o0 = (i>>15)&0x1; Ra = (i>>10)&0x1f; Rn = (i>>5)&0x1f; Rd = i&0x1f; 
#define getalsr(i) 	sf = (i>>31)&0x1; opc = (i>>29)&0x3; shift = (i>>22)&0x3; N = (i>>21)&0x1; Rm = (i>>16)&0x1f; imm6 = (i>>10)&0x3f; Rn = (i>>5)&0x1f; Rd = i&0x1f; 
#define getlsr(i)  	opc = (i>>30)&0x3; V = (i>>26)&0x1; imm19 = (i>>5)&0x7ffff; Rt = i&0x1f; 
#define getlsx(i)  	size = (i>>30)&0x3; o2 = (i>>23)&0x1; L = (i>>22)&0x1; o1 = (i>>21)&0x1; Rs = (i>>16)&0x1f; o0 = (i>>15)&0x1; Rt2 = (i>>10)&0x1f; Rn = (i>>5)&0x1f; Rt = i&0x1f; 
#define getlsnp(i) 	opc = (i>>30)&0x3; V = (i>>26)&0x1; L = (i>>22)&0x1; imm7 = (i>>15)&0x7f; Rt2 = (i>>10)&0x1f; Rn = (i>>5)&0x1f; Rt = i&0x1f; 
#define getlspos(i)	size = (i>>30)&0x3; V = (i>>26)&0x1; opc = (i>>22)&0x3; imm9 = (i>>12)&0x1ff; Rn = (i>>5)&0x1f; Rt = i&0x1f; 
#define getlspre(i)	size = (i>>30)&0x3; V = (i>>26)&0x1; opc = (i>>22)&0x3; imm9 = (i>>12)&0x1ff; Rn = (i>>5)&0x1f; Rt = i&0x1f; 
#define getlso(i)  	size = (i>>30)&0x3; V = (i>>26)&0x1; opc = (i>>22)&0x3; Rm = (i>>16)&0x1f; option = (i>>13)&0x7; S = (i>>12)&0x1; Rn = (i>>5)&0x1f; Rt = i&0x1f; 
#define getlsu(i)  	size = (i>>30)&0x3; V = (i>>26)&0x1; opc = (i>>22)&0x3; imm9 = (i>>12)&0x1ff; Rn = (i>>5)&0x1f; Rt = i&0x1f; 
#define getlsuci(i)	size = (i>>30)&0x3; V = (i>>26)&0x1; opc = (i>>22)&0x3; imm9 = (i>>12)&0x1ff; Rn = (i>>5)&0x1f; Rt = i&0x1f; 
#define getlsusi(i)	size = (i>>30)&0x3; V = (i>>26)&0x1; opc = (i>>22)&0x3; imm12 = (i>>10)&0xfff; Rn = (i>>5)&0x1f; Rt = i&0x1f; 
#define getlsrpo(i)	opc = (i>>30)&0x3; V = (i>>26)&0x1; L = (i>>22)&0x1; imm7 = (i>>15)&0x7f; Rt2 = (i>>10)&0x1f; Rn = (i>>5)&0x1f; Rt = i&0x1f; 
#define getlsppo(i)	opc = (i>>30)&0x3; V = (i>>26)&0x1; L = (i>>22)&0x1; imm7 = (i>>15)&0x7f; Rt2 = (i>>10)&0x1f; Rn = (i>>5)&0x1f; Rt = i&0x1f; 
#define getlsppr(i)	opc = (i>>30)&0x3; V = (i>>26)&0x1; L = (i>>22)&0x1; imm7 = (i>>15)&0x7f; Rt2 = (i>>10)&0x1f; Rn = (i>>5)&0x1f; Rt = i&0x1f; 

/* extract a particular's class opcode; to be OR-ed with the instruction
class to get the super-opcode (xo) */
#define opcmpb(i) 	((i>>24)&0x1|(((i>>31)&0x1)<<1))
#define opcb(i)   	((i>>4)&0x1)
#define opex(i)   	(i&0x3|(((i>>21)&0x7)<<2))
#define opsys(i)  	((i>>5)&0x7|(((i>>12)&0x7)<<3))
#define optb(i)   	((i>>24)&0x1)
#define opubi(i)  	((i>>31)&0x1)
#define opubr(i)  	((i>>16)&0x1f|(((i>>21)&0xf)<<5))
#define opai(i)   	((i>>29)&0x1|(((i>>30)&0x1)<<1)|(((i>>31)&0x1)<<2))
#define opab(i)   	((i>>22)&0x1|(((i>>29)&0x3)<<1)|(((i>>31)&0x1)<<3))
#define opax(i)   	((i>>21)&0x1|(((i>>22)&0x1)<<1)|(((i>>29)&0x3)<<2)|(((i>>31)&0x1)<<4))
#define opali(i)  	((i>>29)&0x3|(((i>>31)&0x1)<<2))
#define opamwi(i) 	((i>>29)&0x3|(((i>>31)&0x1)<<2))
#define opapcr(i) 	((i>>31)&0x1)
#define opar(i)   	((i>>22)&0x3|(((i>>29)&0x1)<<2)|(((i>>30)&0x1)<<3)|(((i>>31)&0x1)<<4))
#define opasr(i)  	((i>>29)&0x1|(((i>>30)&0x1)<<1)|(((i>>31)&0x1)<<2))
#define opac(i)   	((i>>29)&0x1|(((i>>30)&0x1)<<1)|(((i>>31)&0x1)<<2))
#define opaci(i)  	((i>>29)&0x1|(((i>>30)&0x1)<<1)|(((i>>31)&0x1)<<2))
#define opacr(i)  	((i>>29)&0x1|(((i>>30)&0x1)<<1)|(((i>>31)&0x1)<<2))
#define opacs(i)  	((i>>10)&0x3|(((i>>29)&0x1)<<2)|(((i>>30)&0x1)<<3)|(((i>>31)&0x1)<<4))
#define opa1(i)   	((i>>10)&0x7|(((i>>29)&0x1)<<3)|(((i>>31)&0x1)<<4))
#define opa2(i)   	((i>>10)&0x1f|(((i>>31)&0x1)<<5))
#define opa3(i)   	((i>>15)&0x1|(((i>>21)&0x7)<<1)|(((i>>31)&0x1)<<4))
#define opalsr(i) 	((i>>21)&0x1|(((i>>29)&0x3)<<1)|(((i>>31)&0x1)<<3))
#define oplsr(i)  	((i>>26)&0x1|(((i>>30)&0x3)<<1))
#define oplsx(i)  	((i>>15)&0x1|(((i>>21)&0x1)<<1)|(((i>>22)&0x1)<<2)|(((i>>23)&0x1)<<3)|(((i>>30)&0x3)<<4))
#define oplsnp(i) 	((i>>22)&0x1|(((i>>26)&0x1)<<1)|(((i>>30)&0x3)<<2))
#define oplspos(i)	((i>>22)&0x3|(((i>>26)&0x1)<<2)|(((i>>30)&0x3)<<3))
#define oplspre(i)	((i>>22)&0x3|(((i>>26)&0x1)<<2)|(((i>>30)&0x3)<<3))
#define oplso(i)  	((i>>22)&0x3|(((i>>26)&0x1)<<2)|(((i>>30)&0x3)<<3))
#define oplsu(i)  	((i>>22)&0x3|(((i>>26)&0x1)<<2)|(((i>>30)&0x3)<<3))
#define oplsuci(i)	((i>>22)&0x3|(((i>>26)&0x1)<<2)|(((i>>30)&0x3)<<3))
#define oplsusi(i)	((i>>22)&0x3|(((i>>26)&0x1)<<2)|(((i>>30)&0x3)<<3))
#define oplsrpo(i)	((i>>22)&0x1|(((i>>26)&0x1)<<1)|(((i>>30)&0x3)<<2))
#define oplsppo(i)	((i>>22)&0x1|(((i>>26)&0x1)<<1)|(((i>>30)&0x3)<<2))
#define oplsppr(i)	((i>>22)&0x1|(((i>>26)&0x1)<<1)|(((i>>30)&0x3)<<2))