typedef struct Iarg Iarg;
typedef struct Inst Inst;
typedef struct Bus Bus;
typedef struct Cpu Cpu;
typedef struct Pit Pit;

enum {
	RAX,
	RCX,
	RDX,
	RBX,
	RSP,
	RBP,
	RSI,
	RDI,

	RES,
	RCS,
	RSS,
	RDS,
	RFS,
	RGS,

	R0S,	/* 0 segment */

	RIP,
	RFL,

	NREG,
};

struct Iarg
{
	Cpu *cpu;

	uchar tag;
	uchar len;
	uchar atype;

	union {
		uchar reg;
		struct {
			uchar sreg;
			ulong seg, off;
		};
		ulong val;
	};
};

struct Inst
{
	uchar op;
	uchar code;
	uchar olen;
	uchar alen;

	Iarg *a1, *a2, *a3;

	uchar rep;

	uchar mod;
	uchar reg;
	uchar rm;

	uchar scale;
	uchar index;
	uchar base;

	uchar sreg;
	uchar dsreg;

	ulong off;
	long disp;
};

struct Bus
{
	void *aux;
	ulong (*r)(void *aux, ulong off, int len);
	void (*w)(void *aux, ulong off, ulong data, int len);
};

struct Cpu
{
	ulong reg[NREG];

	/* instruction counter */
	ulong ic;

	/* mem[16], one entry for each 64k block */
	Bus *mem;

	/* port[1], in/out */
	Bus *port;

	int trap;
	ulong oldip;
	jmp_buf jmp;

	/* default operand, address and stack pointer length */
	uchar olen, alen, slen;

	/* argument buffers */
	ulong iabuf;
	Iarg abuf[0x80];
};

struct Pit
{
	ulong	count;

	/* set by setgate(), cleared by clockpit() */
	uchar	gateraised;

	/* signals */
	uchar	gate;
	uchar	out;

	/* mode and flags */
	uchar	count0;

	uchar	bcd;
	uchar	amode;
	uchar	omode;

	/* latch for wpit initial count */
	uchar	wcount;
	uchar	wlatched;
	uchar	wlatch[2];

	/* latch for rpit status/count */
	uchar	rcount;
	uchar	rlatched;
	uchar	rlatch[2];
};

/* processor flags */
enum {
	CF = 1<<0,	/* carry flag */
	PF = 1<<2,	/* parity flag */
	AF = 1<<4,	/* aux carry flag */
	ZF = 1<<6,	/* zero flag */
	SF = 1<<7,	/* sign flag */
	TF = 1<<8,	/* trap flag */
	IF = 1<<9, 	/* interrupts enabled flag */
	DF = 1<<10,	/* direction flag */
	OF = 1<<11,	/* overflow flag */
	IOPL= 3<<12,	/* I/O privelege level */
	NT = 1<<14,	/* nested task */
	RF = 1<<16,	/* resume flag */
	VM = 1<<17,	/* virtual-8086 mode */
	AC = 1<<18,	/* alignment check */
	VIF = 1<<19,	/* virtual interrupt flag */
	VIP = 1<<20,	/* virtual interrupt pending */
	ID = 1<<21,	/* ID flag */
};

/* interrupts/traps */
enum {
	EDIV0,
	EDEBUG,
	ENMI,
	EBRK,
	EINTO,
	EBOUND,
	EBADOP,
	ENOFPU,
	EDBLF,
	EFPUSEG,
	EBADTSS,
	ENP,
	ESTACK,
	EGPF,
	EPF,

	EHALT = 256,	/* pseudo-interrupts */
	EMEM,
	EIO,
};

/* argument tags */
enum {
	TREG,
	TMEM,
	TCON,

	TH = 0x80,	/* special flag for AH,BH,CH,DH */
};

/* argument types */
enum {
	ANONE,	/* no argument */
	A0,		/* constant 0 */
	A1,		/* constant 1 */
	A2,		/* constant 2 */
	A3,		/* constant 3 */
	A4,		/* constant 4 */
	AAp,	/* 32-bit or 48-bit direct address */
	AEb,	/* r/m8 from modrm byte */
	AEv,	/* r/m16 or r/m32 from modrm byte */
	AEw,	/* r/m16 */
	AFv,	/* flag word */
	AGb,	/* r8 from modrm byte */
	AGv,	/* r16 or r32 from modrm byte */
	AGw, /* r/m16 */
	AIb,	/* immediate byte */
	AIc,	/* immediate byte sign-extended */
	AIw,	/* immediate 16-bit word */
	AIv,	/* immediate 16-bit or 32-bit word */
	AJb,	/* relative offset byte */
	AJv,	/* relative offset 16-bit or 32-bit word */
	AJr,	/* r/m16 or r/m32 register */
	AM,		/* memory address from modrm */
	AMa,	/* something for bound */
	AMa2,
	AMp,	/* 32-bit or 48-bit memory address */
	AOb,	/* immediate word-sized offset to a byte */
	AOv,	/* immediate word-size offset to a word */
	ASw,	/* segment register selected by r field of modrm */
	AXb,	/* byte at DS:SI */
	AXv,	/* word at DS:SI */
	AYb,	/* byte at ES:DI */
	AYv,	/* word at ES:DI */

	AAL,
	ACL,
	ADL,
	ABL,
	AAH,
	ACH,
	ADH,
	ABH,

	AAX,
	ACX,
	ADX,
	ABX,
	ASP,
	ABP,
	ASI,
	ADI,

	AES,
	ACS,
	ASS,
	ADS,
	AFS,
	AGS,

	NATYPE,
};

/* operators */
enum {
	OBAD,
	O0F,
	OAAA,
	OAAD,
	OAAM,
	OAAS,
	OADC,
	OADD,
	OAND,
	OARPL,
	OASIZE,
	OBOUND,
	OBT,
	OBTS,
	OBTR,
	OBTC,
	OBSF,
	OBSR,
	OCALL,
	OCBW,
	OCLC,
	OCLD,
	OCLI,
	OCMC,
	OCMOV,
	OCMP,
	OCMPS,
	OCPUID,
	OCWD,
	ODAA,
	ODAS,
	ODEC,
	ODIV,
	OENTER,
	OGP1,
	OGP2,
	OGP3b,
	OGP3v,
	OGP4,
	OGP5,
	OGP8,
	OGP10,
	OGP12,
	OHLT,
	OIDIV,
	OIMUL,
	OIN,
	OINC,
	OINS,
	OINT,
	OIRET,
	OJUMP,
	OLAHF,
	OLEA,
	OLEAVE,
	OLFP,
	OLOCK,
	OLODS,
	OLOOP,
	OLOOPNZ,
	OLOOPZ,
	OMOV,
	OMOVS,
	OMOVZX,
	OMOVSX,
	OMUL,
	ONEG,
	ONOP,
	ONOT,
	OOR,
	OOSIZE,
	OOUT,
	OOUTS,
	OPOP,
	OPOPA,
	OPOPF,
	OPUSH,
	OPUSHA,
	OPUSHF,
	ORCL,
	ORCR,
	OREPE,
	OREPNE,
	ORET,
	ORETF,
	OROL,
	OROR,
	OSAHF,
	OSAR,
	OSBB,
	OSCAS,
	OSEG,
	OSET,
	OSHL,
	OSHLD,
	OSHR,
	OSHRD,
	OSTC,
	OSTD,
	OSTI,
	OSTOS,
	OSUB,
	OTEST,
	OWAIT,
	OXCHG,
	OXLAT,
	OXOR,
	NUMOP,
};
