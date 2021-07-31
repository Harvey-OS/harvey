/*
 * code to keep track of registers
 */

#include "defs.h"
#include "fns.h"

extern	void	m68020localaddr(char*, char*);
extern	void	m68020fpprint(int);
extern	int	framelen(void);
extern	void	setvec(void);
extern	void	m68020rsnarf(Reglist *);
extern	void	m68020rrest(Reglist *);
extern	void	m68020excep(void);
extern	int	m68020foll(ulong, ulong*);
extern	void	m68020printins(Map *, char, int);
extern	void	m68020printdas(Map *, int);

#define	INTREG	21		/* 21 fixed-location registers */
#define	SHREG	2		/* 2 two-byte registers */


Machdata m68020mach =
{
	{0x48,0x48,0,0},	/* break point #0 instr. */
	2,			/* size of break point instr. */

	setvec,			/* init */
	beswab,			/* convert short to local byte order */
	beswal,			/* convert long to local byte order */
	ctrace,			/* print C traceback */
	findframe,		/* frame finder */
	m68020fpprint,		/* print floating registers */
	m68020rsnarf,		/* grab registers */
	m68020rrest,		/* restore registers */
	m68020excep,		/* print exception */
	0,			/* breakpoint fixup */
	beieeesfpout,
	beieeedfpout,
	m68020foll,		/* following addresses */
	m68020printins,		/* print instruction */
	m68020printdas,		/* dissembler */
};

int	flen;

/*
 * Do the variable-offset guys here; call the default for
 * the floating point registers, which are at a known offset.
 */

void
m68020rsnarf(Reglist *rp)
{
	ushort sh;

	flen = framelen()-8;
	while(rp < mach->reglist+SHREG) {
		get2(cormap, rp->roffs-flen, SEGREGS, &sh);
		rp->rval = sh;
		rp++;
	}
	while (rp < mach->reglist+INTREG) {
		get4(cormap, rp->roffs-flen, SEGREGS, (long *) &rp->rval);
		rp++;
	}
	rsnarf4(rp);
}

void
m68020rrest(Reglist *rp)
{
	if(!regdirty)
		return;
	flen = framelen()-8;
	while (rp < mach->reglist+SHREG) {
		put2(cormap, rp->roffs-flen, SEGREGS, rp->rval);
		rp++;
	}
	while (rp < mach->reglist+INTREG) {
		put4(cormap, rp->roffs-flen, SEGREGS, rp->rval);
		rp++;
	}
	rrest4(rp);
}

void
m68020fpprint(int modif)
{
	int j;
	char buf[10];
	char reg[12];

	USED(modif);
	for(j=0; j<8; j++){
		sprint(buf, "F%dH", j);
		get1(cormap, rname(buf), SEGREGS, (uchar *)reg, 12);
		chkerr();
		dprint("\tF%d\t", j);
		beieee80fpout(reg);
		dprint("\n");
	}
}
/*
 * 68020 C stack frame	- uses generic frame functions
 */
/*
 * 68020 exception frames
 */

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

static char *vecname[256];
static char vec2fmt2[256];
static char vec2fmt4[256];
static char *vec2fmt = vec2fmt2;

void
setvec(void)
{
	char *v, **vn = vecname;
	int i;

	v = vec2fmt2;
	for (i = 0; i < 256; ++i)
		v[i] = -1;
	v[2]  = 11;	/* bus error -> MC68020 Long Bus Fault  */
	v[3]  = 11;	/* address error -> MC68020 Long Bus Fault */
	v[4]  = 2;	/* illegal instruction  -> Instruction Exception */
	v[5]  = 2;	/* zero divide -> Instruction Exception */
	v[6]  = 2;	/* CHK -> Instruction Exception */
	v[7]  = 2;	/* TRAP -> Instruction Exception */
	v[8]  = 2;	/* privilege violation -> Instruction Exception */
	v[9]  = 2;	/* Trace -> Instruction Exception */
	v[10] = 0;	/* line 1010 -> Instruction Exception */
	v[11] = 0;	/* line 1011 -> Instruction Exception */
	v[13] = 9;	/* coproc viol */
	v[24] = 0;	/* spurious */
	v[25] = 0;	/* incon -> Short Format */
	v[26] = 0;	/* tac -> Short Format */
	v[27] = 0;	/* auto 3 -> Short Format */
	v[28] = 0;	/* clock -> Short Format */
	v[29] = 0;	/* auto 5 -> Short Format */
	v[30] = 0;	/* parity -> Short Format */
	v[31] = 0;	/* mouse -> Short Format */
	v[32] = 0;	/* sys call -> Short Format */
	v[33] = 0;	/* sys call -> Short Format */

	v[48] = 9;	/* FPCP branch */
	v[49] = 9;	/* FPCP inexact */
	v[50] = 9;	/* FPCP zero div */
	v[51] = 9;	/* FPCP underflow */
	v[52] = 9;	/* FPCP operand err */
	v[53] = 9;	/* FPCP overflow */
	v[54] = 9;	/* FPCP signal NAN */

	v = vec2fmt4;
	for (i = 0; i < 256; ++i)
		v[i] = -1;
	v[2]  = 7;	/* bus error -> MC68040 Long Bus Fault  */
	v[3]  = 2;	/* address error -> MC68040 Long Bus Fault */
	v[4]  = 0;	/* illegal instruction  -> Instruction Exception */
	v[5]  = 2;	/* zero divide -> Instruction Exception */
	v[6]  = 2;	/* CHK -> Instruction Exception */
	v[7]  = 0;	/* TRAP -> Instruction Exception */
	v[8]  = 0;	/* privilege violation -> Instruction Exception */
	v[9]  = 2;	/* Trace -> Instruction Exception */
	v[10] = 0;	/* line 1010 -> Instruction Exception */
	v[11] = 0;	/* line 1011 -> Instruction Exception */
	v[13] = 9;	/* coproc viol */
	v[24] = 0;	/* spurious */
	v[25] = 0;	/* incon -> Short Format */
	v[26] = 0;	/* tac -> Short Format */
	v[27] = 0;	/* auto 3 -> Short Format */
	v[28] = 0;	/* clock -> Short Format */
	v[29] = 0;	/* auto 5 -> Short Format */
	v[30] = 0;	/* parity -> Short Format */
	v[31] = 0;	/* mouse -> Short Format */
	v[32] = 0;	/* sys call -> Short Format */
	v[33] = 0;	/* sys call -> Short Format */

	v[48] = 9;	/* FPCP branch */
	v[49] = 9;	/* FPCP inexact */
	v[50] = 9;	/* FPCP zero div */
	v[51] = 9;	/* FPCP underflow */
	v[52] = 9;	/* FPCP operand err */
	v[53] = 9;	/* FPCP overflow */
	v[54] = 9;	/* FPCP signal NAN */

	vn[2]  = "bus error";
	vn[3]  = "address error";
	vn[4]  = "illegal instruction";
	vn[5]  = "zero divide";
	vn[6]  = "CHK";
	vn[7]  = "TRAP";
	vn[8]  = "privilege violation";
	vn[9]  = "Trace";
	vn[10]  = "line 1010";
	vn[11]  = "line 1011";
	vn[13]  = "coproc proto viol";
	vn[24] = "spurious";
	vn[25] = "incon";
	vn[26] = "tac";
	vn[27] = "auto 3";
	vn[28] = "clock";
	vn[29] = "auto 5";
	vn[30] = "parity";
	vn[31] = "mouse";
	vn[32] = "sys call";
	vn[33] = "sys call 1";

	vn[48] = "FPCP branch";
	vn[49] = "FPCP inexact";
	vn[50] = "FPCP zero div";
	vn[51] = "FPCP underflow";
	vn[52] = "FPCP operand err";
	vn[53] = "FPCP overflow";
	vn[54] = "FPCP signal NAN";
}

int	m68020vec;

int
framelen(void)
{
	struct ftype *ft;
	int i, size, vec;
	ulong efl[2];	/* short sr; long pc; short fvo */
	uchar *ef=(uchar*)efl;
	long l;
	short fvo;

		/* The kernel proc pointer on a 68020 is always
		 * at #8xxxxxxx; on the 68040 NeXT, the address
		 * is always #04xxxxxx.
		 */
	if (get4(cormap, 0, SEGREGS, (&l)) == 0)
		chkerr();
	if ((l&0x80000000) == 0) {	/* if NeXT */
		vec2fmt = vec2fmt4;
		size = 30*2;
	} else
		size = 46*2;		/* 68020 */
	for(i=3; i<100; i++){
		get1(cormap, mach->pgsize-i*mach->szreg, SEGREGS, (uchar*)&l, mach->szreg);
		if(machdata->swal(l) == 0xBADC0C0A){
			get1(cormap, mach->pgsize-(i-1)*mach->szreg, SEGREGS, (uchar *)&efl[0], mach->szreg);
			get1(cormap, mach->pgsize-(i-2)*mach->szreg, SEGREGS, (uchar *)&efl[1], mach->szreg);
			fvo = (ef[6]<<8)|ef[7];
			vec = fvo & 0xfff;
			vec >>= 2;
			if(vec >= 256)
				continue;
			for(ft=ftype; ft->name; ft++)
				if(ft->fmt == ((fvo>>12) & 0xF)){
					m68020vec = vec;
					return size+ft->len;
				}
			break;
		}
	}
	dprint("unknown exception frame type\n");
	return size+8;
}

void
m68020excep(void)
{
	if(vecname[m68020vec])
		dprint("%s\n", vecname[m68020vec]);
}
