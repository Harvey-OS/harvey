#include <lib9.h>
#include <bio.h>
#include <mach.h>

/*
 * 68020-specific debugger interface
 */

static	char	*m68020excep(Map*, Rgetter);

static	int	m68020foll(Map*, uvlong, Rgetter, uvlong*);
static	int	m68020inst(Map*, uvlong, char, char*, int);
static	int	m68020das(Map*, uvlong, char*, int);
static	int	m68020instlen(Map*, uvlong);

Machdata m68020mach =
{
	{0x48,0x48,0,0},	/* break point #0 instr. */
	2,			/* size of break point instr. */

	beswab,			/* convert short to local byte order */
	beswal,			/* convert long to local byte order */
	beswav,			/* convert vlong to local byte order */
	cisctrace,		/* C traceback */
	ciscframe,		/* frame finder */
	m68020excep,		/* print exception */
	0,			/* breakpoint fixup */
	beieeesftos,
	beieeedftos,
	m68020foll,		/* follow-set calculation */
	m68020inst,		/* print instruction */
	m68020das,		/* dissembler */
	m68020instlen,		/* instruction size */
};

/*
 * 68020 exception frames
 */

#define BPTTRAP	4		/* breakpoint gives illegal inst */

static char * excep[] = {
	0,			/* 0 */
	0,			/* 1 */
	"bus error",		/* 2 */
	"address error",	/* 3 */
	"illegal instruction",	/* 4 */
	"zero divide",		/* 5 */
	"CHK",			/* 6 */
	"TRAP",			/* 7 */
	"privilege violation",	/* 8 */
	"Trace",		/* 9 */
	"line 1010",		/* 10 */
	"line 1011",		/* 11 */
	0,			/* 12 */
	"coprocessor protocol violation",	/* 13 */
	0,0,0,0,0,0,0,0,0,0,	/* 14-23 */
	"spurious",		/* 24 */
	"incon",		/* 25 */
	"tac",			/* 26 */
	"auto 3",		/* 27 */
	"clock",		/* 28 */
	"auto 5",		/* 29 */
	"parity",		/* 30 */
	"mouse",		/* 31 */
	"system call",		/* 32 */
	"system call 1",	/* 33 */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 34-47 */
	"FPCP branch",		/* 48 */
	"FPCP inexact",		/* 49 */
	"FPCP zero div",	/* 50 */
	"FPCP underflow",	/* 51 */
	"FPCP operand err",	/* 52 */
	"FPCP overflow",	/* 53 */
	"FPCP signal NAN",	/* 54 */
};

static int m68020vec;
static
struct ftype{
	short	fmt;
	short	len;
	char	*name;
} ftype[] = {		/* section 6.5.7 page 6-24 */
	{  0,  4*2, "Short Format" },
	{  1,  4*2, "Throwaway" },
	{  2,  6*2, "Instruction Exception" },
	{  3,  6*2, "MC68040 Floating Point Exception" },
	{  8, 29*2, "MC68010 Bus Fault" },
	{  7, 30*2, "MC68040 Bus Fault" },
	{  9, 10*2, "Coprocessor mid-Instruction" },
	{ 10, 16*2, "MC68020 Short Bus Fault" },
	{ 11, 46*2, "MC68020 Long Bus Fault" },
	{  0,    0, 0 }
};

static int
m68020ufix(Map *map)
{
	struct ftype *ft;
	int i, size, vec;
	ulong efl[2];
	uchar *ef=(uchar*)efl;
	ulong l;
	uvlong stktop;
	short fvo;

		/* The kernel proc pointer on a 68020 is always
		 * at #8xxxxxxx; on the 68040 NeXT, the address
		 * is always #04xxxxxx.  the sun3 port at sydney
		 * uses 0xf8xxxxxx to 0xffxxxxxx.
		 */
	m68020vec = 0;

	if (get4(map, mach->kbase, (&l)) < 0)
		return -1;
	if ((l&0xfc000000) == 0x04000000)	/* if NeXT */
		size = 30*2;
	else
		size = 46*2;			/* 68020 */
	USED(size);

	stktop = mach->kbase+mach->pgsize;
	for(i=3; i<100; i++){
		if (get1(map, stktop-i*4, (uchar*)&l, 4)< 0)
			return -1;

		if(machdata->swal(l) == 0xBADC0C0A){
			if (get1(map, stktop-(i-1)*4, (uchar *)&efl[0], 4) < 0)
				return -1;
			if (get1(map, stktop-(i-2)*4, (uchar *)&efl[1], 4) < 0)
				return -1;
			fvo = (ef[6]<<8)|ef[7];
			vec = fvo & 0xfff;
			vec >>= 2;
			if(vec >= 256)
				continue;

			for(ft=ftype; ft->name; ft++) {
				if(ft->fmt == ((fvo>>12) & 0xF)){
					m68020vec = vec;
					return 1;
				}
			}
			break;
		}
	}
	return -1;
}

static char *
m68020excep(Map *map, Rgetter rget)
{
	uvlong pc;
	uchar buf[4];

	if (m68020ufix(map) < 0)
		return "bad exception frame";

	if(excep[m68020vec] == 0)
		return "bad exeception type";

	if(m68020vec == BPTTRAP) {
		pc = (*rget)(map, "PC");
		if (get1(map, pc, buf, machdata->bpsize) > 0)
		if(memcmp(buf, machdata->bpinst, machdata->bpsize) == 0)
			return "breakpoint";
	}
	return excep[m68020vec];
}
	/* 68020 Disassembler and related functions */
/*
not supported: cpBcc, cpDBcc, cpGEN, cpScc, cpTRAPcc, cpRESTORE, cpSAVE 

opcode:					1 1 1 1 1 1
					5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
%y - register number						  x x x
%f - trap vector						  x x x
%e - destination eff addr				    x x x x x x
%p - conditional predicate				    x x x x x x
%s - size code						x x
%C - cache code						x x
%E - source eff addr.				x x x x x x
%d - direction bit				      x
%c - condition code				x x x x
%x - register number				x x x
%b - shift count				x x x
%q - daffy 3-bit quick operand or shift count	x x x
%i - immediate operand <varies>
%t - offset(PC) <varies>

word 1:					1 1 1 1 1 1
					5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
%a - register number						  x x x
%w - bit field width					      x x x x x
%L - MMU function code (SFC/DFC/D%a/#[0-3])		      x x x x x
%P - conditional predicate				    x x x x x x
%k - k factor						  x x x x x x x
%m - register mask					x x x x x x x x
%N - control register id			x x x x x x x x x x x x
%j - (Dq != Dr) ? Dq:Dr : Dr		  x x x			  x x x
%K - dynamic k register					  x x x
%h - register number					x x x
%I - MMU function code mask			      x x x x
%o - bit field offset				  x x x x x
%u - register number				      x x x
%D - float dest reg				    x x x
%F - (fdr==fsr) ? "F%D" :"F%B,F%D" 	   x x x x x x
%S - float source type			      x x x
%B - float source register		      x x x
%Z - ATC level number			      x x x
%H - MMU register			    x x x x
%r - register type/number		x x x x

word 2:					1 1 1 1 1 1
					5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
%A - register number						  x x x
%U - register number				      x x x
%R - register type,number		x x x x

-----------------------------------------------------------------------------

%a	-	register [word 1: 0-2]
%c	-	condition code [opcode: 8-11]
%d	-	direction [opcode: 8]
%e	-	destination effective address [opcode: 0-5]
%f	-	trap vector [opcode: 0-3]
%h	-	register [word 1: 5-7]
%i	-	immediate operand (1, 2, or 4 bytes)
%j	-	Dq:Dr if Dq != Dr; else Dr => Dr [word 1: 0-2] Dq [word 1: 12-14]
%k	-	k factor [word 1: 0-6]
%m	-	register mask [word 1: 0-7]
%o	-	bit field offset [word 1: 6-10]
%p	-	conditional predicate [opcode: 0-5]
%q	-	daffy 3-bit quick operand [opcode: 9-11]
%r	-	register type, [word 1: 15], register [word 1: 12-14]
%s	-	size [opcode: 6-7]
%t	-	offset beyond pc (text address) (2 or 4 bytes)
%u	-	register [word 1: 6-8]
%w	-	bit field width [word 1: 0-4]
%x	-	register [opcode: 9-11]
%y	-	register [opcode: 0-2]
%A	-	register [word 2: 0-2]
%B	-	float source register [word 1: 10-12]
%C	-	cache identifier [opcode: 6-7] (IC, DC, or BC)
%D	-	float dest reg [word 1: 7-9]
%E	-	dest effective address [opcode: 6-11]
%F	-	float dest reg == float src reg => "F%D"; else "F%B,F%D"
%H	-	MMU reg [word 1: 10-13] (see above & p 4-53/54)
%I	-	MMU function code mask [word 1: 5-8]
%K	-	dynamic k factor register [word 1: 4-6]
%L	-	MMU function code [word 1: 0-4] (SFC, DFC, D%a, or #[0-3])
%N	-	control register [word 1: 0-11]
%P	-	conditional predicate [word 1: 0-5]
%R	-	register type, [word 2: 15], register [word 2: 12-14]
%S	-	float source type code [word 1: 10-12]
%U	-	register [word 2: 6-8]
%Z	-	ATC level number [word 1: 10-12]
%1	-	Special case: EA as second operand
*/
	/* Operand classes */
enum {
	EAPI = 1,	/* extended address: pre decrement only */
	EACA,		/* extended address: control alterable */
	EACAD,		/* extended address: control alterable or Dreg */
	EACAPI,		/* extended address: control alterable or post-incr */
	EACAPD,		/* extended address: control alterable or pre-decr */
	EAMA,		/* extended address: memory alterable */
	EADA,		/* extended address: data alterable */
	EAA,		/* extended address: alterable */
	EAC,		/* extended address: control addressing */
	EACPI,		/* extended address: control addressing or post-incr */
	EACD,		/* extended address: control addressing or Dreg */
	EAD,		/* extended address: data addressing */
	EAM,		/* extended address: memory addressing */
	EAM_B,		/* EAM with byte immediate data */
	EADI,		/* extended address: data addressing or immediate */
	EADI_L,		/* EADI with long immediate data */
	EADI_W,		/* EADI with word immediate data */
	EAALL,		/* extended address: all modes */
	EAALL_L,	/* EAALL with long immediate data */
	EAALL_W,	/* EAALL with word immediate data */
	EAALL_B,	/* EAALL with byte immediate date */
		/* special codes not directly used for validation */
	EAFLT,		/* extended address: EADI for B, W, L, or S; else EAM */
	EADDA,		/* destination extended address: EADA */
	BREAC,		/* EAC operand for JMP or CALL */
	OP8,		/* low 8 bits of op word */
	I8,		/* low 8-bits of first extension word */
	I16,		/* 16 bits in first extension word */
	I32,		/* 32 bits in first and second extension words */
	IV,		/* 8, 16 or 32 bit data in first & 2nd extension words */
	C16,		/* CAS2 16 bit immediate with bits 9-11 & 3-5 zero */
	BR8,		/* 8 bits in op word or 16 or 32 bits in extension words
				branch instruction format (p. 2-25) */
	BR16,		/* 16-bit branch displacement */
	BR32,		/* 32-bit branch displacement */
	STACK,		/* return PC on stack - follow set only */
};
		/* validation bit masks for various EA classes */
enum {
	Dn	= 0x0001,	/* Data register */
	An	= 0x0002,	/* Address register */
	Ind	= 0x0004,	/* Address register indirect */
	Pinc	= 0x0008,	/* Address register indirect post-increment */
	Pdec	= 0x0010,	/* Address register indirect pre-decrement */
	Bdisp	= 0x0020,	/* Base/Displacement in all its forms */
	PCrel	= 0x0040,	/* PC relative addressing in all its forms */
	Imm	= 0x0080,	/* Immediate data */
	Abs	= 0x0100,	/* Absolute */
};
	/* EA validation table indexed by operand class number */

static	short	validea[] =
{
	0,						/* none */
	Pdec,						/* EAPI */
	Abs|Bdisp|Ind,					/* EACA */
	Abs|Bdisp|Ind|Dn,				/* EACAD */
	Abs|Bdisp|Pinc|Ind,				/* EACAPI */
	Abs|Bdisp|Pdec|Ind,				/* EACAPD */
	Abs|Bdisp|Pdec|Pinc|Ind,			/* EAMA */
	Abs|Bdisp|Pdec|Pinc|Ind|Dn,			/* EADA */
	Abs|Bdisp|Pdec|Pinc|Ind|An|Dn,			/* EAA */
	Abs|PCrel|Bdisp|Ind,				/* EAC */
	Abs|PCrel|Bdisp|Pinc|Ind,			/* EACPI */
	Abs|PCrel|Bdisp|Ind|Dn,				/* EACD */
	Abs|PCrel|Bdisp|Pdec|Pinc|Ind|Dn,		/* EAD */
	Abs|Imm|PCrel|Bdisp|Pdec|Pinc|Ind,		/* EAM */
	Abs|Imm|PCrel|Bdisp|Pdec|Pinc|Ind,		/* EAM_B */
	Abs|Imm|PCrel|Bdisp|Pdec|Pinc|Ind|Dn,		/* EADI */
	Abs|Imm|PCrel|Bdisp|Pdec|Pinc|Ind|Dn,		/* EADI_L */
	Abs|Imm|PCrel|Bdisp|Pdec|Pinc|Ind|Dn,		/* EADI_W */
	Abs|Imm|PCrel|Bdisp|Pdec|Pinc|Ind|An|Dn,	/* EAALL */
	Abs|Imm|PCrel|Bdisp|Pdec|Pinc|Ind|An|Dn,	/* EAALL_L */
	Abs|Imm|PCrel|Bdisp|Pdec|Pinc|Ind|An|Dn,	/* EAALL_W */
	Abs|Imm|PCrel|Bdisp|Pdec|Pinc|Ind|An|Dn,	/* EAALL_B */
};
	/* EA types */
enum
{
	Dreg,		/* Dn */
	Areg,		/* An */
	AInd,		/* (An) */
	APdec,		/* -(An) */
	APinc,		/* (An)+ */
	ADisp,		/* Displacement beyond (An) */
	BXD,		/* Base, Index, Displacement */
	PDisp,		/* Displacement beyond PC */
	PXD,		/* PC, Index, Displacement */
	ABS,		/* absolute */
	IMM,		/* immediate */
	IREAL,		/* single precision real immediate */
	IEXT,		/* extended precision real immediate */
	IPACK,		/* packed real immediate */
	IDBL,		/* double precision real immediate */
};
	
typedef	struct optable	Optable;
typedef	struct operand	Operand;
typedef	struct inst	Inst;

struct optable
{
	ushort	opcode;
	ushort	mask0;
	ushort	op2;
	ushort	mask1;
	char	opdata[2];
	char	*format;
};

struct	operand
{
	int	eatype;
	short	ext;
	/*union {*/
		long	immediate;	/* sign-extended integer byte/word/long */
		/*struct	{*/		/* index mode displacements */
			long	disp;
			long	outer;
		/*};*/
		char	floater[24];	/* floating point immediates */
	/*};*/
};

struct	inst
{
	int	n;		/* # bytes in instruction */
	uvlong	addr;		/* addr of start of instruction */
	ushort	raw[4+12];	/* longest instruction: 24 byte packed immediate */
	Operand	and[2];
	char	*end;		/* end of print buffer */
	char	*curr;		/* current fill point in buffer */
	char	*errmsg;
};
	/* class 0: bit field, MOVEP & immediate instructions */
static Optable t0[] = {
{ 0x003c, 0xffff, 0x0000, 0xff00, {I8},		"ORB	%i,CCR" },
{ 0x007c, 0xffff, 0x0000, 0x0000, {I16},	"ORW	%i,SR" },
{ 0x023c, 0xffff, 0x0000, 0xff00, {I8},		"ANDB	%i,CCR" },
{ 0x027c, 0xffff, 0x0000, 0x0000, {I16},	"ANDW	%i,SR" },
{ 0x0a3c, 0xffff, 0x0000, 0xff00, {I8},		"EORB	%i,CCR" },
{ 0x0a7c, 0xffff, 0x0000, 0x0000, {I16},	"EORW	%i,SR" },
{ 0x0cfc, 0xffff, 0x0000, 0x0000, {C16,C16},	"CAS2W	R%a:R%A,R%u:R%U,(%r):(%R)"} ,
{ 0x0efc, 0xffff, 0x0000, 0x0000, {C16,C16},	"CAS2L	R%a:R%A,R%u:R%U,(%r):(%R)"} ,

{ 0x06c0, 0xfff8, 0x0000, 0x0000, {0},		"RTM	R%y" },
{ 0x06c8, 0xfff8, 0x0000, 0x0000, {0},		"RTM	A%y" },
{ 0x0800, 0xfff8, 0x0000, 0x0000, {I16},	"BTSTL	%i,R%y" },
{ 0x0840, 0xfff8, 0x0000, 0x0000, {I16},	"BCHGL	%i,R%y" },
{ 0x0880, 0xfff8, 0x0000, 0x0000, {I16},	"BCLRL	%i,R%y" },

{ 0x00c0, 0xffc0, 0x0000, 0x0fff, {EAC},	"CMP2B	%e,%r" },
{ 0x00c0, 0xffc0, 0x0800, 0x0fff, {EAC},	"CHK2B	%e,%r" },
{ 0x02c0, 0xffc0, 0x0000, 0x0fff, {EAC},	"CMP2W	%e,%r" },
{ 0x02c0, 0xffc0, 0x0800, 0x0fff, {EAC},	"CHK2W	%e,%r" },
{ 0x04c0, 0xffc0, 0x0000, 0x0fff, {EAC},	"CMP2L	%e,%r" },
{ 0x04c0, 0xffc0, 0x0800, 0x0fff, {EAC},	"CHK2L	%e,%r" },
{ 0x06c0, 0xffc0, 0x0000, 0x0000, {I16, BREAC},	"CALLM	%i,%e" },
{ 0x0800, 0xffc0, 0x0000, 0x0000, {I16, EAD},	"BTSTB	%i,%e" },
{ 0x0840, 0xffc0, 0x0000, 0x0000, {I16, EADA},	"BCHG	%i,%e" },
{ 0x0880, 0xffc0, 0x0000, 0x0000, {I16, EADA},	"BCLR	%i,%e" },
{ 0x08c0, 0xffc0, 0x0000, 0x0000, {I16, EADA},	"BSET	%i,%e" },
{ 0x0ac0, 0xffc0, 0x0000, 0xfe38, {EAMA},	"CASB	R%a,R%u,%e" },
{ 0x0cc0, 0xffc0, 0x0000, 0xfe38, {EAMA},	"CASW	R%a,R%u,%e" },
{ 0x0ec0, 0xffc0, 0x0000, 0xfe38, {EAMA},	"CASL	R%a,R%u,%e" },

{ 0x0000, 0xff00, 0x0000, 0x0000, {IV, EADA},	"OR%s	%i,%e" },
{ 0x0200, 0xff00, 0x0000, 0x0000, {IV, EADA},	"AND%s	%i,%e" },
{ 0x0400, 0xff00, 0x0000, 0x0000, {IV, EADA},	"SUB%s	%i,%e" },
{ 0x0600, 0xff00, 0x0000, 0x0000, {IV, EADA},	"ADD%s	%i,%e" },
{ 0x0a00, 0xff00, 0x0000, 0x0000, {IV, EADA},	"EOR%s	%i,%e" },
{ 0x0c00, 0xff00, 0x0000, 0x0000, {IV, EAD},	"CMP%s	%i,%e" },
{ 0x0e00, 0xff00, 0x0000, 0x0800, {EAMA},	"MOVES%s	%e,%r" },
{ 0x0e00, 0xff00, 0x0800, 0x0800, {EAMA},	"MOVES%s	%r,%e" },

{ 0x0108, 0xf1f8, 0x0000, 0x0000, {I16},	"MOVEPW	(%i,A%y),R%x" },
{ 0x0148, 0xf1f8, 0x0000, 0x0000, {I16},	"MOVEPL	(%i,A%y),R%x" },
{ 0x0188, 0xf1f8, 0x0000, 0x0000, {I16},	"MOVEPW	R%x,(%i,A%y)" },
{ 0x01c8, 0xf1f8, 0x0000, 0x0000, {I16},	"MOVEPL	R%x,(%i,A%y)" },
{ 0x0100, 0xf1f8, 0x0000, 0x0000, {0},		"BTSTL	R%x,R%y" },
{ 0x0140, 0xf1f8, 0x0000, 0x0000, {0},		"BCHGL	R%x,R%y" },
{ 0x0180, 0xf1f8, 0x0000, 0x0000, {0},		"BCLRL	R%x,R%y" },
{ 0x01c0, 0xf1f8, 0x0000, 0x0000, {0},		"BSET	R%x,R%y" },

{ 0x0100, 0xf1c0, 0x0000, 0x0000, {EAM_B},	"BTSTB	R%x,%e" },
{ 0x0140, 0xf1c0, 0x0000, 0x0000, {EAMA},	"BCHG	R%x,%e" },
{ 0x0180, 0xf1c0, 0x0000, 0x0000, {EAMA},	"BCLR	R%x,%e" },
{ 0x01c0, 0xf1c0, 0x0000, 0x0000, {EAMA},	"BSET	R%x,%e" },
{ 0,0,0,0,{0},0 },
};
	/* class 1: move byte */
static Optable t1[] = {
{ 0x1000, 0xf000, 0x0000, 0x0000, {EAALL_B,EADDA},"MOVB	%e,%E" },
{ 0,0,0,0,{0},0 },
};
	/* class 2: move long */
static Optable t2[] = {
{ 0x2040, 0xf1c0, 0x0000, 0x0000, {EAALL_L},	  "MOVL	%e,A%x" },

{ 0x2000, 0xf000, 0x0000, 0x0000, {EAALL_L,EADDA},"MOVL	%e,%E" },
{ 0,0,0,0,{0},0 },
};
	/* class 3: move word */
static Optable t3[] = {
{ 0x3040, 0xf1c0, 0x0000, 0x0000, {EAALL_W},	  "MOVW	%e,A%x" },

{ 0x3000, 0xf000, 0x0000, 0x0000, {EAALL_W,EADDA},"MOVW	%e,%E" },
{ 0,0,0,0,{0},0 },
};
	/* class 4: miscellaneous */
static Optable t4[] = {
{ 0x4e75, 0xffff, 0x0000, 0x0000, {STACK},	"RTS" },
{ 0x4e77, 0xffff, 0x0000, 0x0000, {STACK},	"RTR" },
{ 0x4afc, 0xffff, 0x0000, 0x0000, {0},		"ILLEGAL" },
{ 0x4e71, 0xffff, 0x0000, 0x0000, {0},		"NOP" },
{ 0x4e74, 0xffff, 0x0000, 0x0000, {I16, STACK},	"RTD	%i" },
{ 0x4e76, 0xffff, 0x0000, 0x0000, {0},		"TRAPV" },
{ 0x4e70, 0xffff, 0x0000, 0x0000, {0},		"RESET" },
{ 0x4e72, 0xffff, 0x0000, 0x0000, {I16},	"STOP	%i" },
{ 0x4e73, 0xffff, 0x0000, 0x0000, {0},		"RTE" },
{ 0x4e7a, 0xffff, 0x0000, 0x0000, {I16},	"MOVEL	%N,%r" },
{ 0x4e7b, 0xffff, 0x0000, 0x0000, {I16},	"MOVEL	%r,%N" },

{ 0x4808, 0xfff8, 0x0000, 0x0000, {I32},	"LINKL	A%y,%i" },
{ 0x4840, 0xfff8, 0x0000, 0x0000, {0},		"SWAPW	R%y" },
{ 0x4848, 0xfff8, 0x0000, 0x0000, {0},		"BKPT	#%y" },
{ 0x4880, 0xfff8, 0x0000, 0x0000, {0},		"EXTW	R%y" },
{ 0x48C0, 0xfff8, 0x0000, 0x0000, {0},		"EXTL	R%y" },
{ 0x49C0, 0xfff8, 0x0000, 0x0000, {0},		"EXTBL	R%y" },
{ 0x4e50, 0xfff8, 0x0000, 0x0000, {I16},	"LINKW	A%y,%i" },
{ 0x4e58, 0xfff8, 0x0000, 0x0000, {0},		"UNLK	A%y" },
{ 0x4e60, 0xfff8, 0x0000, 0x0000, {0},		"MOVEL	(A%y),USP" },
{ 0x4e68, 0xfff8, 0x0000, 0x0000, {0},		"MOVEL	USP,(A%y)" },

{ 0x4e40, 0xfff0, 0x0000, 0x0000, {0},		"SYS	%f" },

{ 0x40c0, 0xffc0, 0x0000, 0x0000, {EADA},	"MOVW	SR,%e" },
{ 0x42c0, 0xffc0, 0x0000, 0x0000, {EADA},	"MOVW	CCR,%e" },
{ 0x44c0, 0xffc0, 0x0000, 0x0000, {EADI_W},	"MOVW	%e,CCR" },
{ 0x46c0, 0xffc0, 0x0000, 0x0000, {EADI_W},	"MOVW	%e,SR" },
{ 0x4800, 0xffc0, 0x0000, 0x0000, {EADA},	"NBCDB	%e" },
{ 0x4840, 0xffc0, 0x0000, 0x0000, {EAC},	"PEA	%e" },
{ 0x4880, 0xffc0, 0x0000, 0x0000, {I16, EACAPD},"MOVEMW	%i,%e" },
{ 0x48c0, 0xffc0, 0x0000, 0x0000, {I16, EACAPD},"MOVEML	%i,%e" },
{ 0x4ac0, 0xffc0, 0x0000, 0x0000, {EADA},	"TAS	%e" },
{ 0x4a00, 0xffc0, 0x0000, 0x0000, {EAD},	"TSTB	%e" },
{ 0x4c00, 0xffc0, 0x0000, 0x8ff8, {EADI_L},	"MULUL	%e,%r" },
{ 0x4c00, 0xffc0, 0x0400, 0x8ff8, {EADI_L},	"MULUL	%e,R%a:%r" },
{ 0x4c00, 0xffc0, 0x0800, 0x8ff8, {EADI_L},	"MULSL	%e,%r" },
{ 0x4c00, 0xffc0, 0x0c00, 0x8ff8, {EADI_L},	"MULSL	%e,R%a:%r" },
{ 0x4c40, 0xffc0, 0x0000, 0x8ff8, {EADI_L},	"DIVUL	%e,%j" },
{ 0x4c40, 0xffc0, 0x0400, 0x8ff8, {EADI_L},	"DIVUD	%e,%r:R%a" },
{ 0x4c40, 0xffc0, 0x0800, 0x8ff8, {EADI_L},	"DIVSL	%e,%j" },
{ 0x4c40, 0xffc0, 0x0c00, 0x8ff8, {EADI_L},	"DIVSD	%e,%r:R%a" },
{ 0x4c80, 0xffc0, 0x0000, 0x0000, {I16, EACPI}, "MOVEMW	%1,%i" },
{ 0x4cc0, 0xffc0, 0x0000, 0x0000, {I16, EACPI}, "MOVEML	%1,%i" },
{ 0x4e80, 0xffc0, 0x0000, 0x0000, {BREAC},	"JSR	%e" },
{ 0x4ec0, 0xffc0, 0x0000, 0x0000, {BREAC},	"JMP	%e" },

{ 0x4000, 0xff00, 0x0000, 0x0000, {EADA},	"NEGX%s	%e" },
{ 0x4200, 0xff00, 0x0000, 0x0000, {EADA},	"CLR%s	%e" },
{ 0x4400, 0xff00, 0x0000, 0x0000, {EADA},	"NEG%s	%e" },
{ 0x4600, 0xff00, 0x0000, 0x0000, {EADA},	"NOT%s	%e" },
{ 0x4a00, 0xff00, 0x0000, 0x0000, {EAALL},	"TST%s	%e" },

{ 0x4180, 0xf1c0, 0x0000, 0x0000, {EADI_W},	"CHKW	%e,R%x" },
{ 0x41c0, 0xf1c0, 0x0000, 0x0000, {EAC},	"LEA	%e,A%x" },
{ 0x4100, 0xf1c0, 0x0000, 0x0000, {EADI_L},	"CHKL	%e,R%x" },
{ 0,0,0,0,{0},0 },
};
	/* class 5:  miscellaneous quick, branch & trap instructions */
static Optable t5[] = {
{ 0x5000, 0xf1c0, 0x0000, 0x0000, {EADA},	"ADDB	$Q#%q,%e" },
{ 0x5100, 0xf1c0, 0x0000, 0x0000, {EADA},	"SUBB	$Q#%q,%e" },

{ 0x50c8, 0xf1f8, 0x0000, 0x0000, {BR16},	"DB%c	R%y,%t" },
{ 0x51c8, 0xf1f8, 0x0000, 0x0000, {BR16},	"DB%c	R%y,%t" },

{ 0x5000, 0xf1c0, 0x0000, 0x0000, {EAA},	"ADDB	$Q#%q,%e" },
{ 0x5040, 0xf1c0, 0x0000, 0x0000, {EAA},	"ADDW	$Q#%q,%e" },
{ 0x5080, 0xf1c0, 0x0000, 0x0000, {EAA},	"ADDL	$Q#%q,%e" },
{ 0x5100, 0xf1c0, 0x0000, 0x0000, {EAA},	"SUBB	$Q#%q,%e" },
{ 0x5140, 0xf1c0, 0x0000, 0x0000, {EAA},	"SUBW	$Q#%q,%e" },
{ 0x5180, 0xf1c0, 0x0000, 0x0000, {EAA},	"SUBL	$Q#%q,%e" },

{ 0x50fa, 0xf0ff, 0x0000, 0x0000, {I16},	"TRAP%cW	%i" },
{ 0x50fb, 0xf0ff, 0x0000, 0x0000, {I32},	"TRAP%cL	%i" },
{ 0x50fc, 0xf0ff, 0x0000, 0x0000, {0},		"TRAP%c" },

{ 0x50c0, 0xf0c0, 0x0000, 0x0000, {EADA},	"S%c	%e" },
{ 0,0,0,0,{0},0 },
};
	/* class 6: branch instructions */
static Optable t6[] = {
{ 0x6000, 0xff00, 0x0000, 0x0000, {BR8},	"BRA	%t" },
{ 0x6100, 0xff00, 0x0000, 0x0000, {BR8},	"BSR	%t" },
{ 0x6000, 0xf000, 0x0000, 0x0000, {BR8},	"B%c	%t" },
{ 0,0,0,0,{0},0 },
};
	/* class 7: move quick */
static Optable t7[] = {
{ 0x7000, 0xf100, 0x0000, 0x0000, {OP8},	"MOVL	$Q%i,R%x" },
{ 0,0,0,0,{0},0 },
};
	/* class 8: BCD operations, DIV, and OR instructions */
static Optable t8[] = {
{ 0x8100, 0xf1f8, 0x0000, 0x0000, {0},		"SBCDB	R%y,R%x" },
{ 0x8108, 0xf1f8, 0x0000, 0x0000, {0},		"SBCDB	-(A%y),-(A%x)" },
{ 0x8140, 0xf1f8, 0x0000, 0x0000, {I16},	"PACK	R%y,R%x,%i" },
{ 0x8148, 0xf1f8, 0x0000, 0x0000, {I16},	"PACK	-(A%y),-(A%x),%i" },
{ 0x8180, 0xf1f8, 0x0000, 0x0000, {I16},	"UNPK	R%y,R%x,%i" },
{ 0x8188, 0xf1f8, 0x0000, 0x0000, {I16},	"UNPK	-(A%y),-(A%x),%i" },

{ 0x80c0, 0xf1c0, 0x0000, 0x0000, {EADI_W},	"DIVUW	%e,R%x" },
{ 0x81c0, 0xf1c0, 0x0000, 0x0000, {EADI_W},	"DIVSW	%e,R%x" },

{ 0x8000, 0xf100, 0x0000, 0x0000, {EADI},	"OR%s	%e,R%x" },
{ 0x8100, 0xf100, 0x0000, 0x0000, {EAMA},	"OR%s	R%x,%e" },
{ 0,0,0,0,{0},0 },
};
	/* class 9: subtract instruction */
static Optable t9[] = {
{ 0x90c0, 0xf1c0, 0x0000, 0x0000, {EAALL_W},	"SUBW	%e,A%x" },
{ 0x91c0, 0xf1c0, 0x0000, 0x0000, {EAALL_L},	"SUBL	%e,A%x" },

{ 0x9100, 0xf138, 0x0000, 0x0000, {0},		"SUBX%s	R%y,R%x" },
{ 0x9108, 0xf138, 0x0000, 0x0000, {0},		"SUBX%s	-(A%y),-(A%x)" },

{ 0x9000, 0xf100, 0x0000, 0x0000, {EAALL},	"SUB%s	%e,R%x" },
{ 0x9100, 0xf100, 0x0000, 0x0000, {EAMA},	"SUB%s	R%x,%e" },
{ 0,0,0,0,{0},0 },
};
	/* class b: CMP & EOR */
static Optable tb[] = {
{ 0xb000, 0xf1c0, 0x0000, 0x0000, {EADI},	"CMPB	R%x,%e" },
{ 0xb040, 0xf1c0, 0x0000, 0x0000, {EAALL_W},	"CMPW	R%x,%e" },
{ 0xb080, 0xf1c0, 0x0000, 0x0000, {EAALL_L},	"CMPL	R%x,%e" },
{ 0xb0c0, 0xf1c0, 0x0000, 0x0000, {EAALL_W},	"CMPW	A%x,%e" },
{ 0xb1c0, 0xf1c0, 0x0000, 0x0000, {EAALL_L},	"CMPL	A%x,%e" },

{ 0xb108, 0xf138, 0x0000, 0x0000, {0},		"CMP%s	(A%y)+,(A%x)+" },

{ 0xb100, 0xf100, 0x0000, 0x0000, {EADA},	"EOR%s	%e,R%x" },
{ 0,0,0,0,{0},0 },
};
	/* class c: AND, MUL, BCD & Exchange */
static Optable tc[] = {
{ 0xc100, 0xf1f8, 0x0000, 0x0000, {0},		"ABCDB	R%y,R%x" },
{ 0xc108, 0xf1f8, 0x0000, 0x0000, {0},		"ABCDB	-(A%y),-(A%x)" },
{ 0xc140, 0xf1f8, 0x0000, 0x0000, {0},		"EXG	R%x,R%y" },
{ 0xc148, 0xf1f8, 0x0000, 0x0000, {0},		"EXG	A%x,A%y" },
{ 0xc188, 0xf1f8, 0x0000, 0x0000, {0},		"EXG	R%x,A%y" },

{ 0xc0c0, 0xf1c0, 0x0000, 0x0000, {EADI_W},	"MULUW	%e,R%x" },
{ 0xc1c0, 0xf1c0, 0x0000, 0x0000, {EADI_W},	"MULSW	%e,R%x" },

{ 0xc000, 0xf100, 0x0000, 0x0000, {EADI},	"AND%s	%e,R%x" },
{ 0xc100, 0xf100, 0x0000, 0x0000, {EAMA},	"AND%s	R%x,%e" },
{ 0,0,0,0,{0},0 },
};
	/* class d: addition  */
static Optable td[] = {
{ 0xd000, 0xf1c0, 0x0000, 0x0000, {EADI},	"ADDB	%e,R%x" },
{ 0xd0c0, 0xf1c0, 0x0000, 0x0000, {EAALL_W},	"ADDW	%e,A%x" },
{ 0xd1c0, 0xf1c0, 0x0000, 0x0000, {EAALL_L},	"ADDL	%e,A%x" },

{ 0xd100, 0xf138, 0x0000, 0x0000, {0},		"ADDX%s	R%y,R%x" },
{ 0xd108, 0xf138, 0x0000, 0x0000, {0},		"ADDX%s	-(A%y),-(A%x)" },

{ 0xd000, 0xf100, 0x0000, 0x0000, {EAALL},	"ADD%s	%e,R%x" },
{ 0xd100, 0xf100, 0x0000, 0x0000, {EAMA},	"ADD%s	R%x,%e" },
{ 0,0,0,0,{0},0 },
};
	/* class e: shift, rotate, bit field operations */
static Optable te[] = {
{ 0xe8c0, 0xffc0, 0x0820, 0xfe38, {EACD},	"BFTST	%e{R%u:R%a}" },
{ 0xe8c0, 0xffc0, 0x0800, 0xfe20, {EACD},	"BFTST	%e{R%u:%w}" },
{ 0xe8c0, 0xffc0, 0x0020, 0xf838, {EACD},	"BFTST	%e{%o:R%a}" },
{ 0xe8c0, 0xffc0, 0x0000, 0xf820, {EACD},	"BFTST	%e{%o:%w}" },

{ 0xe9c0, 0xffc0, 0x0820, 0x8e38, {EACD},	"BFEXTU	%e{R%u:R%a},%r" },
{ 0xe9c0, 0xffc0, 0x0800, 0x8e20, {EACD},	"BFEXTU	%e{R%u:%w},%r" },
{ 0xe9c0, 0xffc0, 0x0020, 0x8838, {EACD},	"BFEXTU	%e{%o:R%a},%r" },
{ 0xe9c0, 0xffc0, 0x0000, 0x8820, {EACD},	"BFEXTU	%e{%o:%w},%r" },

{ 0xeac0, 0xffc0, 0x0820, 0xfe38, {EACAD},	"BFCHG	%e{R%u:R%a}" },
{ 0xeac0, 0xffc0, 0x0800, 0xfe20, {EACAD},	"BFCHG	%e{R%u:%w}" },
{ 0xeac0, 0xffc0, 0x0020, 0xf838, {EACAD},	"BFCHG	%e{%o:R%a}" },
{ 0xeac0, 0xffc0, 0x0000, 0xf820, {EACAD},	"BFCHG	%e{%o:%w}" },

{ 0xebc0, 0xffc0, 0x0820, 0x8e38, {EACD},	"BFEXTS	%e{R%u:R%a},%r" },
{ 0xebc0, 0xffc0, 0x0800, 0x8e20, {EACD},	"BFEXTS	%e{R%u:%w},%r" },
{ 0xebc0, 0xffc0, 0x0020, 0x8838, {EACD},	"BFEXTS	%e{%o:R%a},%r" },
{ 0xebc0, 0xffc0, 0x0000, 0x8820, {EACD},	"BFEXTS	%e{%o:%w},%r" },

{ 0xecc0, 0xffc0, 0x0820, 0xfe38, {EACAD},	"BFCLR	%e{R%u:R%a}" },
{ 0xecc0, 0xffc0, 0x0800, 0xfe20, {EACAD},	"BFCLR	%e{R%u:%w}" },
{ 0xecc0, 0xffc0, 0x0020, 0xf838, {EACAD},	"BFCLR	%e{%o:R%a}" },
{ 0xecc0, 0xffc0, 0x0000, 0xf820, {EACAD},	"BFCLR	%e{%o:%w}" },

{ 0xedc0, 0xffc0, 0x0820, 0x8e38, {EACAD},	"BFFFO	%e{R%u:R%a},%r" },
{ 0xedc0, 0xffc0, 0x0800, 0x8e20, {EACAD},	"BFFFO	%e{R%u:%w},%r" },
{ 0xedc0, 0xffc0, 0x0020, 0x8838, {EACAD},	"BFFFO	%e{%o:R%a},%r" },
{ 0xedc0, 0xffc0, 0x0000, 0x8820, {EACAD},	"BFFFO	%e{%o:%w},%r" },

{ 0xeec0, 0xffc0, 0x0820, 0xfe38, {EACAD},	"BFSET	%e{R%u:R%a}" },
{ 0xeec0, 0xffc0, 0x0800, 0xfe20, {EACAD},	"BFSET	%e{R%u:%w}" },
{ 0xeec0, 0xffc0, 0x0020, 0xf838, {EACAD},	"BFSET	%e{%o:R%a}" },
{ 0xeec0, 0xffc0, 0x0000, 0xf820, {EACAD},	"BFSET	%e{%o:%w}" },

{ 0xefc0, 0xffc0, 0x0820, 0x8e38, {EACAD},	"BFINS	%r,%e{R%u:R%a}" },
{ 0xefc0, 0xffc0, 0x0800, 0x8e20, {EACAD},	"BFINS	%r,%e{R%u:%w}" },
{ 0xefc0, 0xffc0, 0x0020, 0x8838, {EACAD},	"BFINS	%r,%e{%o:R%a}" },
{ 0xefc0, 0xffc0, 0x0000, 0x8820, {EACAD},	"BFINS	%r,%e{%o:%w}" },

{ 0xe0c0, 0xfec0, 0x0000, 0x0000, {EAMA},	"AS%dW	%e" },
{ 0xe2c0, 0xfec0, 0x0000, 0x0000, {EAMA},	"LS%dW	%e" },
{ 0xe4c0, 0xfec0, 0x0000, 0x0000, {EAMA},	"ROX%dW	%e" },
{ 0xe6c0, 0xfec0, 0x0000, 0x0000, {EAMA},	"RO%dW	%e" },

{ 0xe000, 0xf038, 0x0000, 0x0000, {0},		"AS%d%s	#%q,R%y" },
{ 0xe008, 0xf038, 0x0000, 0x0000, {0},		"LS%d%s	#%q,R%y" },
{ 0xe010, 0xf038, 0x0000, 0x0000, {0},		"ROX%d%s	#%q,R%y" },
{ 0xe018, 0xf038, 0x0000, 0x0000, {0},		"RO%d%s	#%q,R%y" },
{ 0xe020, 0xf038, 0x0000, 0x0000, {0},		"AS%d%s	R%x,R%y" },
{ 0xe028, 0xf038, 0x0000, 0x0000, {0},		"LS%d%s	R%x,R%y" },
{ 0xe030, 0xf038, 0x0000, 0x0000, {0},		"ROX%d%s	R%x,R%y" },
{ 0xe038, 0xf038, 0x0000, 0x0000, {0},		"RO%d%s	R%x,R%y" },
{ 0,0,0,0,{0},0 },
};
	/* class f: coprocessor and mmu instructions */
static Optable tf[] = {
{ 0xf280, 0xffff, 0x0000, 0xffff, {0},		"FNOP" },
{ 0xf200, 0xffff, 0x5c00, 0xfc00, {0},		"FMOVECRX	%k,F%D" },
{ 0xf27a, 0xffff, 0x0000, 0xffc0, {I16},	"FTRAP%P	%i" },
{ 0xf27b, 0xffff, 0x0000, 0xffc0, {I32},	"FTRAP%P	%i" },
{ 0xf27c, 0xffff, 0x0000, 0xffc0, {0},		"FTRAP%P" },

{ 0xf248, 0xfff8, 0x0000, 0xffc0, {BR16},	"FDB%P	R%y,%t" },
{ 0xf620, 0xfff8, 0x8000, 0x8fff, {0},		"MOVE16	(A%y)+,(%r)+" },
{ 0xf500, 0xfff8, 0x0000, 0x0000, {0},		"PFLUSHN	(A%y)" },
{ 0xf508, 0xfff8, 0x0000, 0x0000, {0},		"PFLUSH	(A%y)" },
{ 0xf510, 0xfff8, 0x0000, 0x0000, {0},		"PFLUSHAN" },
{ 0xf518, 0xfff8, 0x0000, 0x0000, {0},		"PFLUSHA" },
{ 0xf548, 0xfff8, 0x0000, 0x0000, {0},		"PTESTW	(A%y)" },
{ 0xf568, 0xfff8, 0x0000, 0x0000, {0},		"PTESTR	(A%y)" },
{ 0xf600, 0xfff8, 0x0000, 0x0000, {I32},	"MOVE16	(A%y)+,$%i" },
{ 0xf608, 0xfff8, 0x0000, 0x0000, {I32},	"MOVE16	$%i,(A%y)-" },
{ 0xf610, 0xfff8, 0x0000, 0x0000, {I32},	"MOVE16	(A%y),$%i" },
{ 0xf618, 0xfff8, 0x0000, 0x0000, {I32},	"MOVE16	$%i,(A%y)" },

{ 0xf000, 0xffc0, 0x0800, 0xffff, {EACA},	"PMOVE	%e,TT0" },
{ 0xf000, 0xffc0, 0x0900, 0xffff, {EACA},	"PMOVEFD	%e,TT0" },
{ 0xf000, 0xffc0, 0x0a00, 0xffff, {EACA},	"PMOVE	TT0,%e" },
{ 0xf000, 0xffc0, 0x0b00, 0xffff, {EACA},	"PMOVEFD	TT0,%e" },
{ 0xf000, 0xffc0, 0x0c00, 0xffff, {EACA},	"PMOVE	%e,TT1" },
{ 0xf000, 0xffc0, 0x0d00, 0xffff, {EACA},	"PMOVEFD	%e,TT1" },
{ 0xf000, 0xffc0, 0x0e00, 0xffff, {EACA},	"PMOVE	TT1,%e" },
{ 0xf000, 0xffc0, 0x0f00, 0xffff, {EACA},	"PMOVEFD	TT1,%e" },
{ 0xf000, 0xffc0, 0x2400, 0xffff, {0},		"PFLUSHA" },
{ 0xf000, 0xffc0, 0x2800, 0xffff, {EACA},	"PVALID	VAL,%e" },
{ 0xf000, 0xffc0, 0x6000, 0xffff, {EACA},	"PMOVE	%e,MMUSR" },
{ 0xf000, 0xffc0, 0x6200, 0xffff, {EACA},	"PMOVE	MMUSR,%e" },
{ 0xf000, 0xffc0, 0x2800, 0xfff8, {EACA},	"PVALID	A%a,%e" },
{ 0xf000, 0xffc0, 0x2000, 0xffe0, {EACA},	"PLOADW	%L,%e" },
{ 0xf000, 0xffc0, 0x2200, 0xffe0, {EACA},	"PLOADR	%L,%e" },
{ 0xf000, 0xffc0, 0x8000, 0xffe0, {EACA},	"PTESTW	%L,%e,#0" },
{ 0xf000, 0xffc0, 0x8200, 0xffe0, {EACA},	"PTESTR	%L,%e,#0" },
{ 0xf000, 0xffc0, 0x3000, 0xfe00, {0},		"PFLUSH	%L,#%I" },
{ 0xf000, 0xffc0, 0x3800, 0xfe00, {EACA},	"PFLUSH	%L,#%I,%e" },
{ 0xf000, 0xffc0, 0x8000, 0xe300, {EACA},	"PTESTW	%L,%e,#%Z" },
{ 0xf000, 0xffc0, 0x8100, 0xe300, {EACA},	"PTESTW	%L,%e,#%Z,A%h" },
{ 0xf000, 0xffc0, 0x8200, 0xe300, {EACA},	"PTESTR	%L,%e,#%Z" },
{ 0xf000, 0xffc0, 0x8300, 0xe300, {EACA},	"PTESTR	%L,%e,#%Z,A%h" },
{ 0xf000, 0xffc0, 0x4000, 0xc3ff, {EACA},	"PMOVE	%e,%H" },
{ 0xf000, 0xffc0, 0x4100, 0xc3ff, {EACA},	"PMOVEFD	%e,%H" },
{ 0xf000, 0xffc0, 0x4200, 0xc3ff, {EACA},	"PMOVE	%H,%e" },

	/* floating point (coprocessor 1)*/

{ 0xf200, 0xffc0, 0x8400, 0xffff, {EAALL_L},	"FMOVEL	%e,FPIAR" },
{ 0xf200, 0xffc0, 0x8800, 0xffff, {EADI_L},	"FMOVEL	%e,FPSR" },
{ 0xf200, 0xffc0, 0x9000, 0xffff, {EADI_L},	"FMOVEL	%e,FPCR" },
{ 0xf200, 0xffc0, 0xa400, 0xffff, {EAA},	"FMOVEL	FPIAR,%e" },
{ 0xf200, 0xffc0, 0xa800, 0xffff, {EADA},	"FMOVEL	FPSR,%e" },
{ 0xf200, 0xffc0, 0xb000, 0xffff, {EADA},	"FMOVEL	FPCR,%e" },

{ 0xf240, 0xffc0, 0x0000, 0xffc0, {EADA},	"FS%P	%e" },

{ 0xf200, 0xffc0, 0xd000, 0xff00, {EACPI},	"FMOVEMX	%e,%m" },
{ 0xf200, 0xffc0, 0xd800, 0xff00, {EACPI},	"FMOVEMX	%e,R%K" },
{ 0xf200, 0xffc0, 0xe000, 0xff00, {EAPI},	"FMOVEMX	%m,-(A%y)" },
{ 0xf200, 0xffc0, 0xe800, 0xff00, {EAPI},	"FMOVEMX	R%K,-(A%y)" },
{ 0xf200, 0xffc0, 0xf000, 0xff00, {EACAPD},	"FMOVEMX	%m,%e" },
{ 0xf200, 0xffc0, 0xf800, 0xff00, {EACAPD},	"FMOVEMX	R%K,%e" },

{ 0xf200, 0xffc0, 0x6800, 0xfc00, {EAMA},	"FMOVEX	F%D,%e" },
{ 0xf200, 0xffc0, 0x6c00, 0xfc00, {EAMA},	"FMOVEP	F%D,%e,{%k}" },
{ 0xf200, 0xffc0, 0x7400, 0xfc00, {EAMA},	"FMOVED	F%D,%e" },
{ 0xf200, 0xffc0, 0x7c00, 0xfc00, {EAMA},	"FMOVEP	F%D,%e,{R%K}" },

{ 0xf200, 0xffc0, 0x8000, 0xe3ff, {EAM},	"FMOVEML	#%B,%e" },
{ 0xf200, 0xffc0, 0xa000, 0xe3ff, {EAMA},	"FMOVEML	%e,#%B" },

{ 0xf200, 0xffc0, 0x0000, 0xe07f, {0},		"FMOVE	F%B,F%D" },
{ 0xf200, 0xffc0, 0x0001, 0xe07f, {0},		"FINTX	%F" },
{ 0xf200, 0xffc0, 0x0002, 0xe07f, {0},		"FSINHX	%F" },
{ 0xf200, 0xffc0, 0x0003, 0xe07f, {0},		"FINTRZ	%F" },
{ 0xf200, 0xffc0, 0x0004, 0xe07f, {0},		"FSQRTX	%F" },
{ 0xf200, 0xffc0, 0x0006, 0xe07f, {0},		"FLOGNP1X	%F" },
{ 0xf200, 0xffc0, 0x0009, 0xe07f, {0},		"FTANHX	%F" },
{ 0xf200, 0xffc0, 0x000a, 0xe07f, {0},		"FATANX	%F" },
{ 0xf200, 0xffc0, 0x000c, 0xe07f, {0},		"FASINX	%F" },
{ 0xf200, 0xffc0, 0x000d, 0xe07f, {0},		"FATANHX	%F" },
{ 0xf200, 0xffc0, 0x000e, 0xe07f, {0},		"FSINX	%F" },
{ 0xf200, 0xffc0, 0x000f, 0xe07f, {0},		"FTANX	%F" },
{ 0xf200, 0xffc0, 0x0010, 0xe07f, {0},		"FETOXX	%F" },
{ 0xf200, 0xffc0, 0x0011, 0xe07f, {0},		"FTWOTOXX	%F" },
{ 0xf200, 0xffc0, 0x0012, 0xe07f, {0},		"FTENTOXX	%F" },
{ 0xf200, 0xffc0, 0x0014, 0xe07f, {0},		"FLOGNX	%F" },
{ 0xf200, 0xffc0, 0x0015, 0xe07f, {0},		"FLOG10X	%F" },
{ 0xf200, 0xffc0, 0x0016, 0xe07f, {0},		"FLOG2X	%F" },
{ 0xf200, 0xffc0, 0x0018, 0xe07f, {0},		"FABSX	%F" },
{ 0xf200, 0xffc0, 0x0019, 0xe07f, {0},		"FCOSHX	%F" },
{ 0xf200, 0xffc0, 0x001a, 0xe07f, {0},		"FNEGX	%F" },
{ 0xf200, 0xffc0, 0x001c, 0xe07f, {0},		"FACOSX	%F" },
{ 0xf200, 0xffc0, 0x001d, 0xe07f, {0},		"FCOSX	%F" },
{ 0xf200, 0xffc0, 0x001e, 0xe07f, {0},		"FGETEXPX	%F" },
{ 0xf200, 0xffc0, 0x001f, 0xe07f, {0},		"FGETMANX	%F" },
{ 0xf200, 0xffc0, 0x0020, 0xe07f, {0},		"FDIVX	F%B,F%D" },
{ 0xf200, 0xffc0, 0x0021, 0xe07f, {0},		"FMODX	F%B,F%D" },
{ 0xf200, 0xffc0, 0x0022, 0xe07f, {0},		"FADDX	F%B,F%D" },
{ 0xf200, 0xffc0, 0x0023, 0xe07f, {0},		"FMULX	F%B,F%D" },
{ 0xf200, 0xffc0, 0x0024, 0xe07f, {0},		"FSGLDIVX	F%B,F%D" },
{ 0xf200, 0xffc0, 0x0025, 0xe07f, {0},		"FREMX	F%B,F%D" },
{ 0xf200, 0xffc0, 0x0026, 0xe07f, {0},		"FSCALEX	F%B,F%D" },
{ 0xf200, 0xffc0, 0x0027, 0xe07f, {0},		"FSGLMULX	F%B,F%D" },
{ 0xf200, 0xffc0, 0x0028, 0xe07f, {0},		"FSUBX	F%B,F%D" },
{ 0xf200, 0xffc0, 0x0038, 0xe07f, {0},		"FCMPX	F%B,F%D" },
{ 0xf200, 0xffc0, 0x003a, 0xe07f, {0},		"FTSTX	F%B" },
{ 0xf200, 0xffc0, 0x0040, 0xe07f, {0},		"FSMOVE	F%B,F%D" },
{ 0xf200, 0xffc0, 0x0041, 0xe07f, {0},		"FSSQRTX	%F"},
{ 0xf200, 0xffc0, 0x0044, 0xe07f, {0},		"FDMOVE	F%B,F%D" },
{ 0xf200, 0xffc0, 0x0045, 0xe07f, {0},		"FDSQRTX	%F" },
{ 0xf200, 0xffc0, 0x0058, 0xe07f, {0},		"FSABSX	%F" },
{ 0xf200, 0xffc0, 0x005a, 0xe07f, {0},		"FSNEGX	%F" },
{ 0xf200, 0xffc0, 0x005c, 0xe07f, {0},		"FDABSX	%F" },
{ 0xf200, 0xffc0, 0x005e, 0xe07f, {0},		"FDNEGX	%F" },
{ 0xf200, 0xffc0, 0x0060, 0xe07f, {0},		"FSDIVX	F%B,F%D" },
{ 0xf200, 0xffc0, 0x0062, 0xe07f, {0},		"FSADDX	F%B,F%D" },
{ 0xf200, 0xffc0, 0x0063, 0xe07f, {0},		"FSMULX	F%B,F%D" },
{ 0xf200, 0xffc0, 0x0064, 0xe07f, {0},		"FDDIVX	F%B,F%D" },
{ 0xf200, 0xffc0, 0x0066, 0xe07f, {0},		"FDADDX	F%B,F%D" },
{ 0xf200, 0xffc0, 0x0067, 0xe07f, {0},		"FDMULX	F%B,F%D" },
{ 0xf200, 0xffc0, 0x0068, 0xe07f, {0},		"FSSUBX	F%B,F%D" },
{ 0xf200, 0xffc0, 0x006c, 0xe07f, {0},		"FDSUBX	F%B,F%D" },
{ 0xf200, 0xffc0, 0x4000, 0xe07f, {EAFLT},	"FMOVE%S	%e,F%D" },
{ 0xf200, 0xffc0, 0x4001, 0xe07f, {EAFLT},	"FINT%S	%e,F%D" },
{ 0xf200, 0xffc0, 0x4002, 0xe07f, {EAFLT},	"FSINH%S	%e,F%D" },
{ 0xf200, 0xffc0, 0x4003, 0xe07f, {EAFLT},	"FINTRZ%S	%e,F%D" },
{ 0xf200, 0xffc0, 0x4004, 0xe07f, {EAFLT},	"FSQRT%S	%e,F%D" },
{ 0xf200, 0xffc0, 0x4006, 0xe07f, {EAFLT},	"FLOGNP1%S	%e,F%D" },
{ 0xf200, 0xffc0, 0x4009, 0xe07f, {EAFLT},	"FTANH%S	%e,F%D" },
{ 0xf200, 0xffc0, 0x400a, 0xe07f, {EAFLT},	"FATAN%S	%e,F%D" },
{ 0xf200, 0xffc0, 0x400c, 0xe07f, {EAFLT},	"FASIN%S	%e,F%D" },
{ 0xf200, 0xffc0, 0x400d, 0xe07f, {EAFLT},	"FATANH%S	%e,F%D" },
{ 0xf200, 0xffc0, 0x400e, 0xe07f, {EAFLT},	"FSIN%S	%e,F%D" },
{ 0xf200, 0xffc0, 0x400f, 0xe07f, {EAFLT},	"FTAN%S	%e,F%D" },
{ 0xf200, 0xffc0, 0x4010, 0xe07f, {EAFLT},	"FETOX%S	%e,F%D" },
{ 0xf200, 0xffc0, 0x4011, 0xe07f, {EAFLT},	"FTWOTOX%S	%e,F%D" },
{ 0xf200, 0xffc0, 0x4012, 0xe07f, {EAFLT},	"FTENTOX%S	%e,F%D" },
{ 0xf200, 0xffc0, 0x4014, 0xe07f, {EAFLT},	"FLOGN%S	%e,F%D" },
{ 0xf200, 0xffc0, 0x4015, 0xe07f, {EAFLT},	"FLOG10%S	%e,F%D" },
{ 0xf200, 0xffc0, 0x4016, 0xe07f, {EAFLT},	"FLOG2%S	%e,F%D" },
{ 0xf200, 0xffc0, 0x4018, 0xe07f, {EAFLT},	"FABS%S	%e,F%D" },
{ 0xf200, 0xffc0, 0x4019, 0xe07f, {EAFLT},	"FCOSH%S	%e,F%D" },
{ 0xf200, 0xffc0, 0x401a, 0xe07f, {EAFLT},	"FNEG%S	%e,F%D" },
{ 0xf200, 0xffc0, 0x401c, 0xe07f, {EAFLT},	"FACOS%S	%e,F%D" },
{ 0xf200, 0xffc0, 0x401d, 0xe07f, {EAFLT},	"FCOS%S	%e,F%D" },
{ 0xf200, 0xffc0, 0x401e, 0xe07f, {EAFLT},	"FGETEXP%S	%e,F%D" },
{ 0xf200, 0xffc0, 0x401f, 0xe07f, {EAFLT},	"FGETMAN%S	%e,F%D" },
{ 0xf200, 0xffc0, 0x4020, 0xe07f, {EAFLT},	"FDIV%S	%e,F%D" },
{ 0xf200, 0xffc0, 0x4021, 0xe07f, {EAFLT},	"FMOD%S	%e,F%D" },
{ 0xf200, 0xffc0, 0x4022, 0xe07f, {EAFLT},	"FADD%S	%e,F%D" },
{ 0xf200, 0xffc0, 0x4023, 0xe07f, {EAFLT},	"FMUL%S	%e,F%D" },
{ 0xf200, 0xffc0, 0x4024, 0xe07f, {EAFLT},	"FSGLDIV%S	%e,F%D" },
{ 0xf200, 0xffc0, 0x4025, 0xe07f, {EAFLT},	"FREM%S	%e,F%D" },
{ 0xf200, 0xffc0, 0x4026, 0xe07f, {EAFLT},	"FSCALE%S	%e,F%D" },
{ 0xf200, 0xffc0, 0x4027, 0xe07f, {EAFLT},	"FSGLMUL%S	%e,F%D" },
{ 0xf200, 0xffc0, 0x4028, 0xe07f, {EAFLT},	"FSUB%S	%e,F%D" },
{ 0xf200, 0xffc0, 0x4038, 0xe07f, {EAFLT},	"FCMP%S	%e,F%D" },
{ 0xf200, 0xffc0, 0x403a, 0xe07f, {EAFLT},	"FTST%S	%e" },
{ 0xf200, 0xffc0, 0x4040, 0xe07f, {EAFLT},	"FSMOVE%S	%e,F%D" },
{ 0xf200, 0xffc0, 0x4041, 0xe07f, {EAFLT},	"FSSQRT%S	%e,F%D" },
{ 0xf200, 0xffc0, 0x4044, 0xe07f, {EAFLT},	"FDMOVE%S	%e,F%D" },
{ 0xf200, 0xffc0, 0x4045, 0xe07f, {EAFLT},	"FDSQRT%S	%e,F%D" },
{ 0xf200, 0xffc0, 0x4058, 0xe07f, {EAFLT},	"FSABS%S	%e,F%D" },
{ 0xf200, 0xffc0, 0x405a, 0xe07f, {EAFLT},	"FSNEG%S	%e,F%D" },
{ 0xf200, 0xffc0, 0x405c, 0xe07f, {EAFLT},	"FDABS%S	%e,F%D" },
{ 0xf200, 0xffc0, 0x405e, 0xe07f, {EAFLT},	"FDNEG%S	%e,F%D" },
{ 0xf200, 0xffc0, 0x4060, 0xe07f, {EAFLT},	"FSDIV%S	%e,F%D" },
{ 0xf200, 0xffc0, 0x4062, 0xe07f, {EAFLT},	"FSADD%S	%e,F%D" },
{ 0xf200, 0xffc0, 0x4063, 0xe07f, {EAFLT},	"FSMUL%S	%e,F%D" },
{ 0xf200, 0xffc0, 0x4064, 0xe07f, {EAFLT},	"FDDIV%S	%e,F%D" },
{ 0xf200, 0xffc0, 0x4066, 0xe07f, {EAFLT},	"FDADD%S	%e,F%D" },
{ 0xf200, 0xffc0, 0x4067, 0xe07f, {EAFLT},	"FDMUL%S	%e,F%D" },
{ 0xf200, 0xffc0, 0x4068, 0xe07f, {EAFLT},	"FSSUB%S	%e,F%D" },
{ 0xf200, 0xffc0, 0x406c, 0xe07f, {EAFLT},	"FDSUB%S	%e,F%D" },

{ 0xf200, 0xffc0, 0x0030, 0xe078, {0},		"FSINCOSX	F%B,F%a:F%D" },
{ 0xf200, 0xffc0, 0x4030, 0xe078, {EAFLT},	"FSINCOS%S	%e,F%a:F%D" },

{ 0xf200, 0xffc0, 0x6000, 0xe000, {EADA},	"FMOVE%S	F%D,%e" },

{ 0xf300, 0xffc0, 0x0000, 0x0000, {EACAPD},	"FSAVE	%e" },
{ 0xf340, 0xffc0, 0x0000, 0x0000, {EACAPI},	"FRESTORE	%e" },

{ 0xf280, 0xffc0, 0x0000, 0x0000, {BR16},	"FB%p	%t" },
{ 0xf2c0, 0xffc0, 0x0000, 0x0000, {BR32},	"FB%p	%t" },

{ 0xf408, 0xff38, 0x0000, 0x0000, {0},		"CINVL	%C,(A%y)" },
{ 0xf410, 0xff38, 0x0000, 0x0000, {0},		"CINVP	%C,(A%y)" },
{ 0xf418, 0xff38, 0x0000, 0x0000, {0},		"CINVA	%C" },
{ 0xf428, 0xff38, 0x0000, 0x0000, {0},		"CPUSHL	%C,(A%y)" },
{ 0xf430, 0xff38, 0x0000, 0x0000, {0},		"CPUSHP	%C,(A%y)" },
{ 0xf438, 0xff38, 0x0000, 0x0000, {0},		"CPUSHA	%C" },
{ 0,0,0,0,{0},0 },
};

static Optable	*optables[] =
{
	t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, 0, tb, tc, td, te, tf,
};

static	Map	*mymap;

static int
dumpinst(Inst *ip, char *buf, int n)
{
	int i;

	if (n <= 0)
		return 0;

	*buf++ = '#';
	for (i = 0; i < ip->n && i*4+1 < n-4; i++, buf += 4)
		_hexify(buf, ip->raw[i], 3);
	*buf = 0;
	return i*4+1;
}

static int
getword(Inst *ip, uvlong offset)
{
	if (ip->n < nelem(ip->raw)) {
		if (get2(mymap, offset, &ip->raw[ip->n++]) > 0)
			return 1;
		werrstr("can't read instruction: %r");
	} else
		werrstr("instruction too big: %r");
	return -1;
}

static int
getshorts(Inst *ip, void *where, int n)
{
	if (ip->n+n < nelem(ip->raw)) {
		if (get1(mymap, ip->addr+ip->n*2, (uchar*)&ip->raw[ip->n], n*2) < 0) {
			werrstr("can't read instruction: %r");
			return 0;
		}
		memmove(where, &ip->raw[ip->n], n*2);
		ip->n += n;
		return 1;
	}
	werrstr("instruction too big: %r");
	return 0;
}

static int
i8(Inst *ip, long *l)
{
	if (getword(ip, ip->addr+ip->n*2) < 0)
		return -1;
	*l = ip->raw[ip->n-1]&0xff;
	if (*l&0x80)
		*l |= ~0xff;
	return 1;
}

static int
i16(Inst *ip, long *l)
{
	if (getword(ip, ip->addr+ip->n*2) < 0)
		return -1;
	*l = ip->raw[ip->n-1];
	if (*l&0x8000)
		*l |= ~0xffff;
	return 1;
}
static int
i32(Inst *ip, long *l)
{
	if (getword(ip, ip->addr+ip->n*2) < 0)
		return -1;
	if (getword(ip, ip->addr+ip->n*2) < 0)
		return -1;
	*l = (ip->raw[ip->n-2]<<16)|ip->raw[ip->n-1];
	return 1;
}

static int
getimm(Inst *ip, Operand *ap, int mode)
{
	ap->eatype = IMM;
	switch(mode)
	{
	case EAM_B:	/* byte */
	case EAALL_B:
		return i8(ip, &ap->immediate);
	case EADI_W:	/* word */
	case EAALL_W:
		return i16(ip, &ap->immediate);
	case EADI_L:	/* long */
	case EAALL_L:
		return i32(ip, &ap->immediate);
	case EAFLT:	/* floating point - size in bits 10-12 or word 1 */
		switch((ip->raw[1]>>10)&0x07)
		{
		case 0:	/* long integer */
			return i32(ip, &ap->immediate);
		case 1:	/* single precision real */
			ap->eatype = IREAL;
			return getshorts(ip, ap->floater, 2);
		case 2:	/* extended precision real - not supported */
			ap->eatype = IEXT;
			return getshorts(ip, ap->floater, 6);
		case 3: /* packed decimal real - not supported */
			ap->eatype = IPACK;
			return getshorts(ip, ap->floater, 12);
		case 4:	/* integer word */
			return i16(ip, &ap->immediate);
		case 5:	/* double precision real */
			ap->eatype = IDBL;
			return getshorts(ip, ap->floater, 4);
		case 6:	/* integer byte */
			return i8(ip, &ap->immediate);
		default:
			ip->errmsg = "bad immediate float data";
			return -1;
		}
		/* not reached */
	case IV:	/* size encoded in bits 6&7 of opcode word */
	default:
		switch((ip->raw[0]>>6)&0x03)
		{
		case 0x00:	/* integer byte */
			return i8(ip, &ap->immediate);
		case 0x01:	/* integer word */
			return i16(ip, &ap->immediate);
		case 0x02:	/* integer long */
			return i32(ip, &ap->immediate);
		default:
			ip->errmsg = "bad immediate size";
			return -1;
		}
		/* not reached */
	}
}

static int
getdisp(Inst *ip, Operand *ap)
{
	short ext;

	if (getword(ip, ip->addr+ip->n*2) < 0)
		return -1;
	ext = ip->raw[ip->n-1];
	ap->ext = ext;
	if ((ext&0x100) == 0) {		/* indexed with 7-bit displacement */
		ap->disp = ext&0x7f;
		if (ap->disp&0x40)
			ap->disp |= ~0x7f;
		return 1;
	}
	switch(ext&0x30)	/* first (inner) displacement  */
	{
	case 0x10:
		break;
	case 0x20:
		if (i16(ip, &ap->disp) < 0)
			return -1;
		break;
	case 0x30:
		if (i32(ip, &ap->disp) < 0)
			return -1;
		break;
	default:
		ip->errmsg = "bad EA displacement";
		return -1;
	}
	switch (ext&0x03)	/* outer displacement */
	{
	case 0x02:		/* 16 bit displacement */
		return i16(ip, &ap->outer);
	case 0x03:		/* 32 bit displacement */
		return i32(ip, &ap->outer);
	default:
		break;
	}
	return 1;
}

static int
ea(Inst *ip, int ea, Operand *ap, int mode)
{
	int type, size;

	type = 0;
	ap->ext = 0;
	switch((ea>>3)&0x07)
	{
	case 0x00:
		ap->eatype = Dreg;
		type = Dn;
		break;
	case 0x01:
		ap->eatype = Areg;
		type = An;
		break;
	case 0x02:
		ap->eatype = AInd;
		type = Ind;
		break;
	case 0x03:
		ap->eatype = APinc;
		type = Pinc;
		break;
	case 0x04:
		ap->eatype = APdec;
		type = Pdec;
		break;
	case 0x05:
		ap->eatype = ADisp;
		type = Bdisp;
		if (i16(ip, &ap->disp) < 0)
			return -1;
		break;
	case 0x06:
		ap->eatype = BXD;
		type = Bdisp;
		if (getdisp(ip, ap) < 0)
			return -1;
		break;
	case 0x07:
		switch(ea&0x07)
		{
		case 0x00:
			type = Abs;
			ap->eatype = ABS;
			if (i16(ip, &ap->immediate) < 0)
				return -1;
			break;
		case 0x01:
			type = Abs;
			ap->eatype = ABS;
			if (i32(ip, &ap->immediate) < 0)
				return -1;
			break;
		case 0x02:
			type = PCrel;
			ap->eatype = PDisp;
			if (i16(ip, &ap->disp) < 0)
				return -1;
			break;
		case 0x03:
			type = PCrel;
			ap->eatype = PXD;
			if (getdisp(ip, ap) < 0)
				return -1;
			break;
		case 0x04:
			type = Imm;
			if (getimm(ip, ap, mode) < 0)
				return -1;
			break;
		default:
			ip->errmsg = "bad EA mode";
			return -1;
		}
	}
		/* Allowable floating point EAs are restricted for packed,
		 * extended, and double precision operands
		 */
	if (mode == EAFLT) {
		size = (ip->raw[1]>>10)&0x07;
		if (size == 2 || size == 3 || size == 5)
			mode = EAM;
		else
			mode = EADI;
	}
	if (!(validea[mode]&type)) {
		ip->errmsg = "invalid EA";
		return -1;
	}
	return 1;
}

static int
decode(Inst *ip, Optable *op)
{
	int i, t, mode;
	Operand *ap;
	short opcode;

	opcode = ip->raw[0];
	for (i = 0; i < nelem(op->opdata) && op->opdata[i]; i++) {
		ap = &ip->and[i];
		mode = op->opdata[i];
		switch(mode)
		{
		case EAPI:		/* normal EA modes */
		case EACA:
		case EACAD:
		case EACAPI:
		case EACAPD:
		case EAMA:
		case EADA:
		case EAA:
		case EAC:
		case EACPI:
		case EACD:
		case EAD:
		case EAM:
		case EAM_B:
		case EADI:
		case EADI_L:
		case EADI_W:
		case EAALL:
		case EAALL_L:
		case EAALL_W:
		case EAALL_B:
		case EAFLT:
			if (ea(ip, opcode&0x3f, ap, mode) < 0)
				return -1;
			break;
		case EADDA:	/* stupid bit flop required */
			t = ((opcode>>9)&0x07)|((opcode>>3)&0x38);
			if (ea(ip, t, ap, EADA)< 0)
					return -1;
			break;
		case BREAC:	/* EAC JMP or CALL operand */
			if (ea(ip, opcode&0x3f, ap, EAC) < 0)
				return -1;
			break;
		case OP8:	/* weird movq instruction */
			ap->eatype = IMM;
			ap->immediate = opcode&0xff;
			if (opcode&0x80)
				ap->immediate |= ~0xff;
			break;
		case I8:	/* must be two-word opcode */
			ap->eatype = IMM;
			ap->immediate = ip->raw[1]&0xff;
			if (ap->immediate&0x80)
				ap->immediate |= ~0xff;
			break;
		case I16:	/* 16 bit immediate */
		case BR16:
			ap->eatype = IMM;
			if (i16(ip, &ap->immediate) < 0)
				return -1;
			break;
		case C16:	/* CAS2 16 bit immediate */
			ap->eatype = IMM;
			if (i16(ip, &ap->immediate) < 0)
				return -1;
			if (ap->immediate & 0x0e38) {
				ip->errmsg = "bad CAS2W operand";
				return 0;
			}
			break;
		case I32:	/* 32 bit immediate */
		case BR32:
			ap->eatype = IMM;
			if (i32(ip, &ap->immediate) < 0)
				return -1;
			break;
		case IV:	/* immediate data depends on size field */
			if (getimm(ip, ap, IV) < 0)
				return -1;
			break;
		case BR8:	/* branch displacement format */
			ap->eatype = IMM;
			ap->immediate = opcode&0xff;
			if (ap->immediate == 0) {
				if (i16(ip, &ap->immediate) < 0)
					return -1;
			} else if (ap->immediate == 0xff) {
				if (i32(ip, &ap->immediate) < 0)
					return -1;
			} else if (ap->immediate & 0x80)
				ap->immediate |= ~0xff;
			break;
		case STACK:	/* Dummy operand type for Return instructions */
		default:
			break;
		}
	}
	return 1;
}

static Optable *
instruction(Inst *ip)
{
	ushort opcode, op2;
	Optable *op;
	int class;

	ip->n = 0;
	if (getword(ip, ip->addr) < 0)
		return 0;
	opcode = ip->raw[0];
	if (get2(mymap, ip->addr+2, &op2) < 0)
		op2 = 0;
	class = (opcode>>12)&0x0f;
	for (op = optables[class]; op && op->format; op++) {
		if (op->opcode != (opcode&op->mask0))
			continue;
		if (op->op2 != (op2&op->mask1))
			continue;
		if (op->mask1)
			ip->raw[ip->n++] = op2;
		return op;
	}
	ip->errmsg = "Invalid opcode";
	return 0;
}

#pragma	varargck	argpos	bprint		2

static void
bprint(Inst *i, char *fmt, ...)
{
	va_list arg;

	va_start(arg, fmt);
	i->curr = vseprint(i->curr, i->end, fmt, arg);
	va_end(arg);
}

static	char	*regname[] =
{
	"R0", "R1", "R2", "R3", "R4", "R5", "R6", "R7", "A0",
	"A1", "A2", "A3", "A4", "A5", "A6", "A7", "PC", "SB"
};

static void
plocal(Inst *ip, Operand *ap)
{
	int ret;
	long offset;
	uvlong moved;
	Symbol s;

	offset = ap->disp;
	if (!findsym(ip->addr, CTEXT, &s))
		goto none;

	moved = pc2sp(ip->addr);
	if (moved == -1)
		goto none;

	if (offset > moved) {		/* above frame - must be argument */
		offset -= moved;
		ret = getauto(&s, offset-mach->szaddr, CPARAM, &s);
	} else				/* below frame - must be automatic */
		ret = getauto(&s, moved-offset, CPARAM, &s);
	if (ret)
		bprint(ip, "%s+%lux", s.name, offset);
	else
none:		bprint(ip, "%lux", ap->disp);
}

/*
 *	this guy does all the work of printing the base and index component
 *	of an EA.
 */
static int
pidx(Inst *ip, int ext, int reg, char *bfmt, char *ifmt, char *nobase)
{
	char *s;
	int printed;
	char buf[512];

	printed = 1;
	if (ext&0x80) {				/* Base suppressed */
		if (reg == 16)
			bprint(ip, bfmt, "(ZPC)");
		else if (nobase)
			bprint(ip, nobase);
		else
			printed = 0;
	} else					/* format base reg */
		bprint(ip, bfmt, regname[reg]);
	if (ext & 0x40)				/* index suppressed */
		return printed;
	switch ((ext>>9)&0x03)
	{
	case 0x01:
		s = "*2";
		break;
	case 0x02:
		s = "*4";
		break;
	case 0x03:
		s = "*8";
		break;
	default:
		if (ext&0x80)
			s = "*1";
		else
			s = "";
		break;
	}
	sprint(buf, "%s.%c%s", regname[(ext>>12)&0x0f], (ext&0x800) ? 'L' : 'W', s);
	if (!printed)
		bprint(ip, ifmt, buf);
	else
		bprint(ip, "(%s)", buf);
	return 1;
}

static void
prindex(Inst *ip, int reg, Operand *ap)
{
	short ext;
	int left;
	int disp;

	left = ip->end-ip->curr;
	if (left <= 0)
		return;
	ext = ap->ext;
	disp = ap->disp;
		/* look for static base register references */
	if ((ext&0xa0) == 0x20 && reg == 14 && mach->sb && disp) {
		reg = 17;		/* "A6" -> "SB" */
		disp += mach->sb;
	}
	if ((ext&0x100) == 0) {		/* brief form */
		if (reg == 15)
			plocal(ip, ap);
		else if (disp)
			ip->curr += symoff(ip->curr, left, disp, CANY);
		pidx(ip, ext&0xff00, reg, "(%s)", "(%s)", 0);
		return;
	}
	switch(ext&0x3f)	/* bd size, && i/is */
	{
	case 0x10:
		if (!pidx(ip, ext, reg, "(%s)", "(%s)", 0))
			bprint(ip, "#0");
		break;
	case 0x11:
		if (pidx(ip, ext, reg, "((%s)", "((%s)", 0))
			bprint(ip, ")");
		else
			bprint(ip, "#0");
		break;
	case 0x12:
	case 0x13:
		ip->curr += symoff(ip->curr, left, ap->outer, CANY);
		if (pidx(ip, ext, reg, "((%s)", "((%s)", 0))
			bprint(ip, ")");
		break;
	case 0x15:
		if (!pidx(ip, ext, reg, "((%s))", "(%s)", 0))
			bprint(ip, "#0");
		break;
	case 0x16:
	case 0x17:
		ip->curr += symoff(ip->curr, left, ap->outer, CANY);
		pidx(ip, ext, reg, "((%s))", "(%s)", 0);
		break;
	case 0x20:
	case 0x30:
		if (reg == 15)
			plocal(ip, ap);
		else
			ip->curr += symoff(ip->curr, left, disp, CANY);
		pidx(ip, ext, reg, "(%s)", "(%s)", 0);
		break;
	case 0x21:
	case 0x31:
		*ip->curr++ = '(';
		if (reg == 15)
			plocal(ip, ap);
		else
			ip->curr += symoff(ip->curr, left-1, disp, CANY);
		pidx(ip, ext, reg, "(%s)", "(%s)", 0);
		bprint(ip, ")");
		break;
	case 0x22:
	case 0x23:
	case 0x32:
	case 0x33:
		ip->curr += symoff(ip->curr, left, ap->outer, CANY);
		bprint(ip, "(");
		if (reg == 15)
			plocal(ip, ap);
		else
			ip->curr += symoff(ip->curr, ip->end-ip->curr, disp, CANY);
		pidx(ip, ext, reg, "(%s)", "(%s)", 0);
		bprint(ip, ")");
		break;
	case 0x25:
	case 0x35:
		*ip->curr++ = '(';
		if (reg == 15)
			plocal(ip, ap);
		else
			ip->curr += symoff(ip->curr, left-1, disp, CANY);
		if (!pidx(ip, ext, reg, "(%s))", "(%s)", "())"))
			bprint(ip, ")");
		break;
	case 0x26:
	case 0x27:
	case 0x36:
	case 0x37:
		ip->curr += symoff(ip->curr, left, ap->outer, CANY);
		bprint(ip, "(");
		if (reg == 15)
			plocal(ip, ap);
		else
			ip->curr += symoff(ip->curr, ip->end-ip->curr, disp, CANY);
		pidx(ip, ext, reg, "(%s))", "(%s)", "())");
		break;
	default:
		bprint(ip, "??%x??", ext);
		ip->errmsg = "bad EA";
		break;
	}
}

static	void
pea(int reg, Inst *ip, Operand *ap)
{
	int i, left;

	left = ip->end-ip->curr;
	if (left < 0)
		return;
	switch(ap->eatype)
	{
	case Dreg:
		bprint(ip, "R%d", reg);
		break;
	case Areg:
		bprint(ip, "A%d", reg);
		break;
	case AInd:
		bprint(ip, "(A%d)", reg);
		break;
	case APinc:
		bprint(ip, "(A%d)+", reg);
		break;
	case APdec:
		bprint(ip, "-(A%d)", reg);
		break;
	case PDisp:
		ip->curr += symoff(ip->curr, left, ip->addr+2+ap->disp, CANY);
		break;
	case PXD:
		prindex(ip, 16, ap);
		break;
	case ADisp:	/* references off the static base */
		if (reg == 6 && mach->sb && ap->disp) {
			ip->curr += symoff(ip->curr, left, ap->disp+mach->sb, CANY);
			bprint(ip, "(SB)");
			break;
		}
			/* reference autos and parameters off the stack */
		if (reg == 7)
			plocal(ip, ap);
		else
			ip->curr += symoff(ip->curr, left, ap->disp, CANY);
		bprint(ip, "(A%d)", reg);
		break;
	case BXD:
		prindex(ip, reg+8, ap);
		break;
	case ABS:
		ip->curr += symoff(ip->curr, left, ap->immediate, CANY);
		bprint(ip, "($0)");
		break;
	case IMM:
		*ip->curr++ = '$';
		ip->curr += symoff(ip->curr, left-1, ap->immediate, CANY);
		break;
	case IREAL:
		*ip->curr++ = '$';
		ip->curr += beieeesftos(ip->curr, left-1, (void*) ap->floater);
		break;
	case IDBL:
		*ip->curr++ = '$';
		ip->curr += beieeedftos(ip->curr, left-1, (void*) ap->floater);
		break;
	case IPACK:
		bprint(ip, "$#");
		for (i = 0; i < 24 && ip->curr < ip->end-1; i++) {
			_hexify(ip->curr, ap->floater[i], 1);
			ip->curr += 2;
		}
		break;
	case IEXT:
		bprint(ip, "$#");
		ip->curr += beieee80ftos(ip->curr, left-2, (void*)ap->floater);
		break;
	default:
		bprint(ip, "??%x??", ap->eatype);
		ip->errmsg = "bad EA type";
		break;
	}
}

static char *cctab[]  = { "F", "T", "HI", "LS", "CC", "CS", "NE", "EQ",
			  "VC", "VS", "PL", "MI", "GE", "LT", "GT", "LE" };
static	char *fcond[] =
{
	"F",	"EQ",	"OGT",	"OGE",	"OLT",	"OLE",	"OGL", "OR",
	"UN",	"UEQ",	"UGT",	"UGE",	"ULT",	"ULE",	"NE",	"T",
	"SF",	"SEQ",	"GT",	"GE",	"LT",	"LE",	"GL",	"GLE",
	"NGLE",	"NGL",	"NLE",	"NLT",	"NGE",	"NGT",	"SNE",	"ST"
};
static	char *cachetab[] =	{ "NC", "DC", "IC", "BC" };
static	char *mmutab[] =	{ "TC", "??", "SRP", "CRP" };
static	char *crtab0[] =
{
	"SFC", "DFC", "CACR", "TC", "ITT0", "ITT1", "DTT0", "DTT1",
};
static	char *crtab1[] =
{
	"USP", "VBR", "CAAR", "MSP", "ISP", "MMUSR", "URP", "SRP",
};
static	char typetab[] =	{ 'L', 'S', 'X', 'P', 'W', 'D', 'B', '?', };
static	char sztab[] =		{'?', 'B', 'W', 'L', '?' };

static	void
formatins(char *fmt, Inst *ip)
{
	short op, w1;
	int r1, r2;
	int currand;

	op = ip->raw[0];
	w1 = ip->raw[1];
	currand = 0;
	for (; *fmt && ip->curr < ip->end; fmt++) {
		if (*fmt != '%')
			*ip->curr++ = *fmt;
		else switch(*++fmt)
		{
		case '%':
			*ip->curr++ = '%';
			break;
		case 'a':	/* register number; word 1:[0-2] */
			*ip->curr++ = (w1&0x07)+'0';
			break;
		case 'c':	/* condition code; opcode: [8-11] */
			bprint(ip, cctab[(op>>8)&0x0f]);
			break;
		case 'd':	/* shift direction; opcode: [8] */
			if (op&0x100)
				*ip->curr++ = 'L';
			else
				*ip->curr++ = 'R';
			break;
		case 'e':	/* source effective address */
			pea(op&0x07, ip, &ip->and[currand++]);
			break;
		case 'f':	/* trap vector; op code: [0-3] */
			bprint(ip, "%x", op&0x0f);
			break;
		case 'h':	/* register number; word 1: [5-7] */
			*ip->curr++ = (w1>>5)&0x07+'0';
			break;
		case 'i':	/* immediate operand */
			ip->curr += symoff(ip->curr, ip->end-ip->curr,
					ip->and[currand++].immediate, CANY);
			break;
		case 'j':	/* data registers; word 1: [0-2] & [12-14] */
			r1 = w1&0x07;
			r2 = (w1>>12)&0x07;
			if (r1 == r2)
				bprint(ip, "R%d", r1);
			else
				bprint(ip, "R%d:R%d", r2, r1);
			break;
		case 'k':	/* k factor; word 1 [0-6] */
			bprint(ip, "%x", w1&0x7f);
			break;
		case 'm':	/* register mask; word 1 [0-7] */
			bprint(ip, "%x", w1&0xff);
			break;
		case 'o':	/* bit field offset; word1: [6-10] */
			bprint(ip, "%d", (w1>>6)&0x3f);
			break;
		case 'p':	/* conditional predicate; opcode: [0-5]
				   only bits 0-4 are defined  */
			bprint(ip, fcond[op&0x1f]);
			break;
		case 'q':	/* 3-bit immediate value; opcode[9-11] */
			r1 = (op>>9)&0x07;
			if (r1 == 0)
				*ip->curr++ = '8';
			else
				*ip->curr++ = r1+'0';
			break;
		case 'r':	/* register type & number; word 1: [12-15] */
			bprint(ip, regname[(w1>>12)&0x0f]);
			break;
		case 's':	/* size; opcode [6-7] */
			*ip->curr = sztab[((op>>6)&0x03)+1];
			if (*ip->curr++ == '?')
				ip->errmsg = "bad size code";
			break;
		case 't':	/* text offset */
			ip->curr += symoff(ip->curr, ip->end-ip->curr,
				ip->and[currand++].immediate+ip->addr+2, CTEXT);
			break;
		case 'u':	/* register number; word 1: [6-8] */
			*ip->curr++ = ((w1>>6)&0x07)+'0';
			break;
		case 'w':	/* bit field width; word 1: [0-4] */
			bprint(ip, "%d", w1&0x0f);
			break;
		case 'x':	/* register number; opcode: [9-11] */
			*ip->curr++ = ((op>>9)&0x07)+'0';
			break;
		case 'y':	/* register number; opcode: [0-2] */
			*ip->curr++ = (op&0x07)+'0';
			break;
		case 'z':	/* shift count; opcode: [9-11] */	
			*ip->curr++ = ((op>>9)&0x07)+'0';
			break;
		case 'A':	/* register number; word 2: [0-2] */
			*ip->curr++ = (ip->raw[2]&0x07)+'0';
			break;
		case 'B':	/* float source reg; word 1: [10-12] */
			*ip->curr++ = ((w1>>10)&0x07)+'0';
			break;
		case 'C':	/* cache identifier; opcode: [6-7] */
			bprint(ip, cachetab[(op>>6)&0x03]);
			break;
		case 'D':	/* float dest reg; word 1: [7-9] */
			*ip->curr++ = ((w1>>7)&0x07)+'0';
			break;
		case 'E':	/* destination EA; opcode: [6-11] */
			pea((op>>9)&0x07, ip, &ip->and[currand++]);
			break;
		case 'F':	/* float dest register(s); word 1: [7-9] & [10-12] */
			r1 = (w1>>7)&0x07;
			r2 = (w1>>10)&0x07;
			if (r1 == r2)
				bprint(ip, "F%d", r1);
			else
				bprint(ip, "F%d,F%d", r2, r1);
			break;
		case 'H':	/* MMU register; word 1 [10-13] */
			bprint(ip, mmutab[(w1>>10)&0x03]);
			if (ip->curr[-1] == '?')
				ip->errmsg = "bad mmu register";
			break;
		case 'I':	/* MMU function code mask; word 1: [5-8] */
			bprint(ip, "%x", (w1>>4)&0x0f);
			break;
		case 'K':	/* dynamic k-factor register; word 1: [5-8] */
			bprint(ip, "%d",  (w1>>4)&0x0f);
			break;
		case 'L':	/* MMU function code; word 1: [0-6] */
			if (w1&0x10)
				bprint(ip, "%x", w1&0x0f);
			else if (w1&0x08)
				bprint(ip, "R%d",w1&0x07);
			else if (w1&0x01)
				bprint(ip, "DFC");
			else
				bprint(ip, "SFC");
			break;
		case 'N':	/* control register; word 1: [0-11] */
			r1 = w1&0xfff;
			if (r1&0x800)
				bprint(ip, crtab1[r1&0x07]);
			else
				bprint(ip, crtab0[r1&0x07]);
			break;
		case 'P':	/* conditional predicate; word 1: [0-5] */
			bprint(ip, fcond[w1&0x1f]);
			break;
		case 'R':	/* register type & number; word 2 [12-15] */
			bprint(ip, regname[(ip->raw[2]>>12)&0x0f]);
			break;
		case 'S':	/* float source type code; word 1: [10-12] */
			*ip->curr = typetab[(w1>>10)&0x07];
			if (*ip->curr++ == '?')
				ip->errmsg = "bad float type";
			break;
		case 'U':	/* register number; word 2: [6-8] */
			*ip->curr++ = ((ip->raw[2]>>6)&0x07)+'0';
			break;
		case 'Z':	/* ATC level number; word 1: [10-12] */
			bprint(ip, "%x", (w1>>10)&0x07);
			break;
		case '1':	/* effective address in second operand*/
			pea(op&0x07, ip, &ip->and[1]);
			break;
		default:
			bprint(ip, "%%%c", *fmt);
			break;
		}
	}
	*ip->curr = 0;		/* there's always room for 1 byte */
}

static int
dispsize(Inst *ip)
{
	ushort ext;
	static int dsize[] = {0, 0, 1, 2};	/* in words */

	if (get2(mymap, ip->addr+ip->n*2, &ext) < 0)
		return -1;
	if ((ext&0x100) == 0)
		return 1;
	return dsize[(ext>>4)&0x03]+dsize[ext&0x03]+1;
}

static int
immsize(Inst *ip, int mode)
{
	static int fsize[] = { 2, 2, 6, 12, 1, 4, 1, -1 };
	static int isize[] = { 1, 1, 2, -1 };

	switch(mode)
	{
	case EAM_B:			/* byte */
	case EAALL_B:
	case EADI_W:			/* word */
	case EAALL_W:
		return 1;
	case EADI_L:			/* long */
	case EAALL_L:
		return 2;
	case EAFLT:	/* floating point - size in bits 10-12 or word 1 */
		return fsize[(ip->raw[1]>>10)&0x07];
	case IV:	/* size encoded in bits 6&7 of opcode word */
	default:
		return isize[(ip->raw[0]>>6)&0x03];
	}
}

static int
easize(Inst *ip, int ea, int mode)
{
	switch((ea>>3)&0x07)
	{
	case 0x00:
	case 0x01:
	case 0x02:
	case 0x03:
	case 0x04:
		return 0;
	case 0x05:
		return 1;
	case 0x06:
		return dispsize(ip);
	case 0x07:
		switch(ea&0x07)
		{
		case 0x00:
		case 0x02:
			return 1;
		case 0x01:
			return 2;
		case 0x03:
			return dispsize(ip);
		case 0x04:
			return immsize(ip, mode);
		default:
			return -1;
		}
	}
	return -1;
}

static int
instrsize(Inst *ip, Optable *op)
{
	int i, t, mode;
	short opcode;

	opcode = ip->raw[0];
	for (i = 0; i < nelem(op->opdata) && op->opdata[i]; i++) {
		mode = op->opdata[i];
		switch(mode)
		{
		case EAPI:		/* normal EA modes */
		case EACA:
		case EACAD:
		case EACAPI:
		case EACAPD:
		case EAMA:
		case EADA:
		case EAA:
		case EAC:
		case EACPI:
		case EACD:
		case EAD:
		case EAM:
		case EAM_B:
		case EADI:
		case EADI_L:
		case EADI_W:
		case EAALL:
		case EAALL_L:
		case EAALL_W:
		case EAALL_B:
		case EAFLT:
			t = easize(ip, opcode&0x3f, mode);
			if (t < 0)
				return -1;
			ip->n += t;
			break;
		case EADDA:	/* stupid bit flop required */
			t = ((opcode>>9)&0x07)|((opcode>>3)&0x38);
			t = easize(ip, t, mode);
			if (t < 0)
				return -1;
			ip->n += t;
			break;
		case BREAC:	/* EAC JMP or CALL operand */
				/* easy displacements for follow set */
			if ((opcode&0x038) == 0x28 || (opcode&0x3f) == 0x3a) {
				if (i16(ip, &ip->and[i].immediate) < 0)
					return -1;
			} else {
				t = easize(ip, opcode&0x3f, mode);
				if (t < 0)
					return -1;
				ip->n += t;
			}
			break;
		case I16:	/* 16 bit immediate */
		case C16:	/* CAS2 16 bit immediate */
			ip->n++;
			break;
		case BR16:	/* 16 bit branch displacement */
			if (i16(ip, &ip->and[i].immediate) < 0)
				return -1;
			break;
		case BR32:	/* 32 bit branch displacement */
			if (i32(ip, &ip->and[i].immediate) < 0)
				return -1;
			break;
		case I32:	/* 32 bit immediate */
			ip->n += 2;
			break;
		case IV:	/* immediate data depends on size field */
			t = (ip->raw[0]>>6)&0x03;
			if (t < 2)
				ip->n++;
			else if (t == 2)
				ip->n += 2;
			else 
				return -1;
			break;
		case BR8:	/* loony branch displacement format */
			t = opcode&0xff;
			if (t == 0) {
				if (i16(ip, &ip->and[i].immediate) < 0)
					return -1;
			} else if (t == 0xff) {
				if (i32(ip, &ip->and[i].immediate) < 0)
					return -1;
			} else {
				ip->and[i].immediate = t;
				if (t & 0x80)
					ip->and[i].immediate |= ~0xff;
			}
			break;
		case STACK:	/* Dummy operand for Return instructions */
		case OP8:	/* weird movq instruction */
		case I8:	/* must be two-word opcode */
		default:
			break;
		}
	}
	return 1;
}

static int
eaval(Inst *ip, Operand *ap, Rgetter rget)
{
	int reg;
	char buf[8];

	reg = ip->raw[0]&0x07;
	switch(ap->eatype)
	{
	case AInd:
		sprint(buf, "A%d", reg);
		return (*rget)(mymap, buf);
	case PDisp:
		return ip->addr+2+ap->disp;
	case ADisp:
		sprint(buf, "A%d", reg);
		return ap->disp+(*rget)(mymap, buf);
	case ABS:
		return ap->immediate;
	default:
		return 0;
	}
}

static int
m68020instlen(Map *map, uvlong pc)
{
	Inst i;
	Optable *op;

	mymap = map;
	i.addr = pc;
	i.errmsg = 0;
	op = instruction(&i);
	if (op && instrsize(&i, op) > 0)
			return i.n*2;
	return -1;
}

static int
m68020foll(Map *map, uvlong pc, Rgetter rget, uvlong *foll)
{
	int j;
	Inst i;
	ulong l;
	Optable *op;

	mymap = map;
	i.addr = pc;
	i.errmsg = 0;
	op = instruction(&i);
	if (op == 0  || instrsize(&i, op) < 0)
		return -1;
	for (j = 0; j < nelem(op->opdata) && op->opdata[j]; j++) {
		switch(op->opdata[j])
		{
		case BREAC:	/* CALL, JMP, JSR */
			foll[0] = pc+2+eaval(&i, &i.and[j], rget);
			return 1;
		case BR8:	/* Bcc, BSR, & BRA */
		case BR16:	/* FBcc, FDBcc, DBcc */
		case BR32:	/* FBcc */
			foll[0] = pc+i.n*2;
			foll[1] = pc+2+i.and[j].immediate;
			return 2;
		case STACK:	/* RTR, RTS, RTD */
			if (get4(map, (*rget)(map, mach->sp), &l) < 0)
				return -1;
			*foll = l;
			return 1;
		default:
			break;
		}
	}
	foll[0] = pc+i.n*2;			
	return 1;
}

static int
m68020inst(Map *map, uvlong pc, char modifier, char *buf, int n)
{
	Inst i;
	Optable *op;

	USED(modifier);
	mymap = map;
	i.addr = pc;
	i.curr = buf;
	i.end = buf+n-1;
	i.errmsg = 0;
	op = instruction(&i);
	if (!op)
		return -1;
	if (decode(&i, op) > 0)
		formatins(op->format, &i);
	if (i.errmsg) {
		if (i.curr != buf)
			bprint(&i, "\t\t;");
		bprint(&i, "%s: ", i.errmsg);
		dumpinst(&i, i.curr, i.end-i.curr);
	}
	return i.n*2;
}

static int
m68020das(Map *map, uvlong pc, char *buf, int n)
{
	Inst i;
	Optable *op;

	mymap = map;
	i.addr = pc;
	i.curr = buf;
	i.end = buf+n-1;
	i.errmsg = 0;
	
	op = instruction(&i);
	if (!op)
		return -1;
	decode(&i, op);
	if (i.errmsg)
		bprint(&i, "%s: ", i.errmsg);
	dumpinst(&i, i.curr, i.end-i.curr);
	return i.n*2;
}
