/*
 * 3210.h
 */

typedef struct	Alu		Alu;
typedef struct	Breakpoint	Breakpoint;
typedef struct	Freg		Freg;
typedef struct	Fpumem		Fpumem;
typedef struct	Fpu		Fpu;
typedef struct	Fspec		Fspec;
typedef struct	Inst		Inst;
typedef struct	Namedreg	Namedreg;
typedef struct	Refs		Refs;
typedef struct	Registers	Registers;
typedef struct	Segment		Segment;

enum
{
	Instruction	= 1,
	Read		= 2,
	Write		= 4,
	Access		= 2|4,
	Equal		= 4|8,
};

struct Alu
{
	void 	(*func)(int, int, ulong, ulong);
	void	(*fmt)(char*, int, char*, char*, char*, char*, char*, char*);
	char	*name;
	int	rcount;		/* count for condition reg operations */
	int	taken;		/* number with condition true */
	int	icount;		/* count for immediate ops */
};

struct Breakpoint
{
	int		type;		/* Instruction/Read/Access/Write/Equal */
	ulong		addr;		/* Place at address */
	int		count;		/* To execute count times or value */
	int		done;		/* How many times passed through */
	Breakpoint	*next;		/* Link to next one */
};

struct Freg
{
	ulong	val;		/* standard dsp format */
	uchar	guard;		/* extra bits in register */
};

struct Fpumem
{
	ulong	a;		/* address if memory ref */
	Freg	v;
};

struct Fpu
{
	ulong	inst;
	char	zinc;		/* increment for rz++ */
	Fpumem	x;
	Fpumem	y;
	Fpumem	z;
	Freg	accum;		/* value of source accumulator */
	int	(*fetch)(Fpu*);	/* 1 cycle delayed operand fetch */
	void	(*op)(Fpu*);	/* 3 cycle delayed fpu operation */
};

struct Fspec
{
	void	(*op)(Fpu*);	/* 3 cycle delayed fpu operation */
	int	(*fetch)(Fpu*);	/* 1 cycle delayed operand fetch */
	int	inc;		/* increment for *ry++ */
	int	zinc;		/* increment for *rz++ */
	char	*name;
	int	count;
};

enum
{
	Tillegal,
	Tcall,
	Tcalli,
	Tgoto,
	Tcgoto,
	Tdecgoto,
	Tdo,
	Taddi,
	Tarith,
	Tshiftor,
	Tset,
	Tfloat,
	Tfspec,
	Tmem,
	Tsys,
	Ntype
};

struct Inst
{
	void 	(*func)(ulong);
	void	(*print)(char*, int, ulong, ulong, int);
	char	*name;
	int	type;
	int	count;			/* number of instructions executed */
	int	taken;			/* number of conditionals true */
};

enum
{
	NREG	= 23,
	NFREG	= 4,
	NCREG	= 16,

	REGSP	= 1,
	REGLINK	= 18,
	REGRET	= 3,
	REGARG	= 3,

	FREGRET	= 0,

	PS	= 0,		/* status */
	EMR	= 8,		/* exception mask register */
	PCW	= 12,		/* processor control word */
	DAUC	= 14,		/* floating point control */
	CTR	= 15,		/* clip test register */
};

struct Registers
{
	ulong	pc;
	ulong	ip;		/* pointer to instuction executing */
	ulong	ir;		/* instruction executing */
	ulong	last;		/* last instuction executed */
	Inst	*inst;		/* ditto */
	ulong	pcsh;
	ulong	r[NREG];
	Freg	f[NFREG];
	ulong	c[NCREG];	/* control registers */

	ulong	loop;		/* loop instruction */
	ulong	start;		/* loop starting address */
	ulong	count;		/* number of instructios left in this iteration */
	ulong	size;		/* number of instructions in loop */
	ulong	iter;		/* iterations left */
};

struct	Namedreg
{
	char	*name;
	ulong	*reg;
};

enum{
	PCMEMMAP	= 1 << 10,	/* internal memory mapped at 0 */
	PCBIGEND	= 1 << 8,	/* big or little endian */
};

enum
{
	MemRead,
	MemReadstring,
	MemWrite,
};

enum
{
	Ram0,
	Ram1,
	Mema,
	Memb,
	Nseg
};

struct Segment
{
	char	*name;
	int	rss;
	int	refs;
	ulong	ntbl;		/* number of entries in table */
	uchar	**table;
};

enum
{
	Fetch,
	Load,
	Store,
	Nref
};

/*
 * number of references in different segments
 */
struct Refs
{
	ulong	nops;
	ulong	refs[Nref][Nseg];
};

/*
 * fpu exposed pipeline and delayed load
 */
int	ldlast(int, int, int, int);
int	last(int, int);
int	fpufetch(void);
void	fpuupdate(int, int, int, int);
void	fpuop(void);

/*
 * instruction decode & exec
 */
void	Daddi(char*, int, ulong, ulong, int);
void	Dalui(char*, int, ulong, ulong, int);
void	Dcall(char*, int, ulong, ulong, int);
void	Dcalli(char*, int, ulong, ulong, int);
void	Dcalu(char*, int, ulong, ulong, int);
void	Dcgoto(char*, int, ulong, ulong, int);
void	Ddecgoto(char*, int, ulong, ulong, int);
void	Ddo(char*, int, ulong, ulong, int);
void	Ddoi(char*, int, ulong, ulong, int);
void	Dfspecial(char*, int, ulong, ulong, int);
void	Dfmadd(char*, int, ulong, ulong, int);
void	Dgoto(char*, int, ulong, ulong, int);
void	Dloadi(char*, int, ulong, ulong, int);
void	Dmove(char*, int, ulong, ulong, int);
void	Dmovi(char*, int, ulong, ulong, int);
void	Dmovio(char*, int, ulong, ulong, int);
void	Dmovr(char*, int, ulong, ulong, int);
void	Dmovrio(char*, int, ulong, ulong, int);
void	Dshiftor(char*, int, ulong, ulong, int);
void	Dsyscall(char*, int, ulong, ulong, int);
void	Dundef(char*, int, ulong, ulong, int);

void	Iaddi(ulong);
void	Ialui(ulong);
void	Icall(ulong);
void	Icalli(ulong);
void	Icalu(ulong);
void	Icgoto(ulong);
void	Idecgoto(ulong);
void	Ido(ulong);
void	Idoi(ulong);
void	Ifspecial(ulong);
void	Ifmadd(ulong);
void	Igoto(ulong);
void	Iloadi(ulong);
void	Imove(ulong);
void	Imovi(ulong);
void	Imovio(ulong);
void	Imovr(ulong);
void	Imovrio(ulong);
void	Ishiftor(ulong);
void	Isyscall(ulong);
void	Iundef(ulong);

/*
 * bpt.c
 */
void	breakpoint(char*);
void	brkchk(ulong, int);
void	delbpt(void);
void	dobplist(void);

/*
 * cmd.c
 */
void	cmd(void);
ulong	expr(char*);
char*	nextc(char*);

/*
 * convfloat.c
 */
double	alawtod(int);
double	dsptod(ulong);
int	dtoalaw(double);
int	dtochar(double);
ulong	dtodsp(double);
int	dtolong(double);
int	dtoshort(double);
int	dtoulaw(double);
double	ulawtod(int);

/*
 * float.c
 */
void	Ffloat16(Fpu*);
void	Ffloat32(Fpu*);
void	Fint16(Fpu*);
void	Fint32(Fpu*);
void	Fround(Fpu*);
void	Fseed(Fpu*);
void	addtap(Fpu*);
int	axyfetch(Fpu*);
ulong	crack(int, int);
Freg	faccum(ulong);
void	fargname(char*, int, int);
Freg	ffetch(int, ulong);
int	increg(int);
int	mfetch(Fpu*);
void	multaccstore(Fpu*);
void	multacctap(Fpu*);
void	multaddstore(Fpu*);
void	resetfpu(void);
int	sfetch(Fpu*);
void	storeaccum(ulong, Freg);
int	update(int, long, int);
int	xyfetch(Fpu*);
void	yfetch(Fpu*);

/*
 * fpu.c
 */
Freg	fmuladd(int, int, Freg, Freg, Freg);
Freg	fpuflags(Freg);
Freg	fround(Freg);
Freg	normalize(ulong, ulong, int);

/*
 * mem.c
 */
uchar	getmem_1(ulong);
ushort	getmem_2(ulong);
ulong	getmem_4(ulong);
uchar	getmem_b(ulong);
double	getmem_dsp(ulong);
ushort	getmem_h(ulong);
ulong	getmem_w(ulong);
ulong	ifetch(ulong);
char*	memio(char*, ulong, int, int);
void	putmem_1(ulong, uchar);
void	putmem_2(ulong, ushort);
void	putmem_4(ulong, ulong);
void	putmem_b(ulong, uchar);
void	putmem_dsp(ulong, double);
void	putmem_h(ulong, ushort);
void	putmem_w(ulong, ulong);
void*	vaddr(ulong, int, int);
int	membrk(ulong);

/*
 * run.c
 */
void	addflags(int, ulong, ulong, ulong);
int	fmtins(char*, int, int, ulong, ulong);
char	*condname(int);
int	condset(int);
void	cpuflags(ulong);
ulong	fetch(ulong, int);
void	memfetch(int, int, int, int);
int	regno(int);
ulong	regval(int, int);
void	resetld(void);
ulong	run(void);
void	store(ulong, ulong, int);
void	subflags(int, ulong, ulong, ulong);
void	textname(char*, int, long, ulong);
void	undef(ulong);

/*
 * stats.c
 */
void	clearprof(void);
void	iprofile(void);
void	isum(void);
ulong	icalc(Inst*);
void	segsum(void);
void	summary(ulong, ulong, char*);
void	tlbsum(void);
void	cachesum(void);

/*
 * symbols.c
 */
ulong	findframe(ulong);
void	printparams(Symbol*, ulong);
void	printsource(long);
int	psymoff(ulong, int, char*);
void	stktrace(int);
void	tracecall(ulong);
void	traceret(ulong, int);

/*
 * syscall.c
 */
int	syscall(ulong, ulong);

/*
 * xi.c
 */
void	dumpFreg(void);
void	dumpfreg(void);
void	dumpreg(void);
void*	emalloc(ulong);
void*	erealloc(void*, ulong);
void	error(char*, ...);
void	fatal(char*, ...);
void	init(int, char**);
void	inithdr(int);
void	itrace(char *, ...);
void	reset(void);
int	Xconv(void*, Fconv*);
void	warn(char*, ...);
int	ffmt(char*, int, int, ulong, ulong);
int	Ffmt(char*, int, int, ulong, ulong);

Extern	Biobuf		*bin;
Extern	Biobuf		*bioout;
extern	char		*file;
extern	char		*fregname[];
extern	char		*incname[];
extern	char		*iorname[];
Extern	Refs		*iprof;
Extern	Refs		ramprof[(8*1024)/4];
Extern	Refs		*mprof;
extern	char		*regname[];
Extern	ulong		Bdata;
Extern	ulong		Btext;
Extern	ulong		Edata;
Extern	ulong		Etext;
Extern	ulong		Bram;
Extern	ulong		Eram;
Extern	ulong		dspmem;
Extern	ulong		Tstart;
Extern	ulong		Dstart;
extern	Alu		alutab[];
Extern	int		atbpt;
extern	Inst		biginst[];
Extern	int		calltree;
Extern	int		cmdcount;
Extern	ulong		count;
Extern	ulong		dot;
Extern	jmp_buf		errjmp;
extern	Fspec		fputab[];
Extern	ulong		iloads;
Extern	int		isbigend;
Extern	ulong		istores;
Extern	int		lock;			/* is the bus locked? */
Extern	ulong		locked[Nref];
extern	Inst		itab[];
Extern	ulong		loads;
Extern	int		membpt;
Extern	int		nopcount;
Extern	Refs		notext;
extern	int		regmap[];
extern	Segment		segments[];
Extern	ulong		stores;
Extern	int		syscalls;
Extern	int		sysdbg;
Extern	int		text;
Extern	int		trace;
Extern	Breakpoint	*bplist;
extern	Namedreg	namedreg[];
Extern	Registers	reg;

/* Kernel constants */
#define BY2WD		4
#define BY2PG		4096		/* convenient size */
#define STACKTOP	0x5003fffc

#define PROFGRAN	4

/* Opcode decoders */
#define	reg1(i)		((i)>> 0 & 0x1f)
#define	reg2(i)		((i)>>11 & 0x1f)
#define	reg3(i)		((i)>>16 & 0x1f)
#define	reg4(i)		((i)>>21 & 0x1f)
#define imm7(i)		(((i)>>11)&0x3f)
#define imm11(i)	((i)&0x3ff)
#define	imm16(i)	((short)((i)&0xffff))
#define	imm24(i)	((i)&0xffff | (i)>>5 & 0xff0000)
#define	cond1(i)	((i)>> 5 & 0x3f)
#define	cond2(i)	((i)>>21 & 0x3f)
#define alu32(i)	((i)>>31)
#define	aluop(i)	((i)>>21 & 0xf)
#define	mvwid(i)	((i)>>21 & 7)
#define	tomem(i)	((i) & 0x01000000)
#define	islock(i)	(((i)>>23) & 3)

#define	INOP		0x80000000

/*
 * load/store widths
 */
enum
{
	Uchar,
	Char,
	Ushort,
	Short,
	Hchar,
	Wres1,
	Wres2,
	Long,
};

/*
 * condition codes
 */
enum{
	Cfalse,
	Ctrue,
	Cpl,
	Cmi,
	Cne,
	Ceq,
	Cvc,
	Cvs,
	Ccc,
	Ccs,
	Cge,
	Clt,
	Cgt,
	Cle,
	Chi,
	Cls,
	Cauc,
	Caus,
	Cage,
	Calt,
	Cane,
	Caeq,
	Cavc,
	Cavs,
	Cagt,
	Cale,

	Cibe = 0x20,
	Cibf,
	Cobf,
	Cobe,
	Csyc = 0x28,
	Csys,
	Cfbc,
	Cfbs,
	Cir0c,
	Cir0s,
	Cir1c,
	Cir1s,

	Negsh = 0,
	Zerosh,
	Oversh,
	Carrysh,
	Fnegsh,
	Fzerosh,
	Fundersh,
	Foversh,

	Neg	= 1 << Negsh,
	Zero 	= 1 << Zerosh,
	Over	= 1 << Oversh,
	Carry	= 1 << Carrysh,
	Fneg	= 1 << Fnegsh,
	Fzero	= 1 << Fzerosh,
	Funder	= 1 << Fundersh,
	Fover	= 1 << Foversh,
};

#define	isneg()		((reg.c[PS]>>Negsh)&1)
#define	iszero()	((reg.c[PS]>>Zerosh)&1)
#define	isover()	((reg.c[PS]>>Oversh)&1)
#define	iscarry()	((reg.c[PS]>>Carrysh)&1)
#define	isfneg()	((reg.c[PS]>>Fnegsh)&1)
#define	isfzero()	((reg.c[PS]>>Fzerosh)&1)
#define	isfunder()	((reg.c[PS]>>Fundersh)&1)
#define	isfover()	((reg.c[PS]>>Foversh)&1)

/*
 * dau control register
 */
#define Convert()	(reg.c[DAUC]&7)
#define Round()		((reg.c[DAUC]>>4)&3)
#define Condwrite()	((reg.c[DAUC]>>6)&1)

/*
 * dsp format aids
 */
enum{
	Mbits	= 23,			/* bits in a dsp mantissa */
	Mmask	= (1<<Mbits) - 1,	/* mask for mantissa */
	Bias	= 128,			/* exponent bias */
	Ebits	= 8,			/* bits in a dsp exponent */
	Emask	= (1<<Ebits) - 1,		/* mask for eponent */
};

#define Mant(f)		(((f)>>Ebits)&Mmask)
#define Exp(f)		((f)&Emask)
#define Sign(f)		(((f)>>31)&1)
#define Fval(s,m,e)	(((s)<<31) | (((m)&Mmask)<<8) | ((e)&Emask))
