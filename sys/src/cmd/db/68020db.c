/*
 * code to keep track of registers
 */

#include "defs.h"
#include "fns.h"

extern	void	m68020localaddr(char*, char*);
extern	void	m68020fpprint(int);
extern	int	framelen(void);
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

	0,			/* init */
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


static char * excep[] = {
	[2]	"bus error",
	[3]	"address error",
	[4]	"illegal instruction",
	[5]	"zero divide",
	[6]	"CHK",
	[7]	"TRAP",
	[8]	"privilege violation",
	[9]	"Trace",
	[10]	"line 1010",
	[11]	"line 1011",
	[13]	"coprocessor protocol violation",
	[24]	"spurious",
	[25]	"incon",
	[26]	"tac",
	[27]	"auto 3",
	[28]	"clock",
	[29]	"auto 5",
	[30]	"parity",
	[31]	"mouse",
	[32]	"sys call",
	[33]	"sys call 1",
	[48]	"FPCP branch",
	[49]	"FPCP inexact",
	[50]	"FPCP zero div",
	[51]	"FPCP underflow",
	[52]	"FPCP operand err",
	[53]	"FPCP overflow",
	[54]	"FPCP signal NAN",
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


int
framelen(void)
{
	struct ftype *ft;
	int i, size, vec;
	ulong efl[2], stktop;
	uchar *ef=(uchar*)efl;
	long l;
	short fvo;

		/* The kernel proc pointer on a 68020 is always
		 * at #8xxxxxxx; on the 68040 NeXT, the address
		 * is always #04xxxxxx.  the sun3 port at sidney
		 * uses 0xf8xxxxxx to 0xffxxxxxx.
		 */
	m68020vec = 0;

	if (get4(cormap, mach->kbase, SEGDATA, (&l)) == 0)
		chkerr();
	if ((l&0xfc000000) == 0x04000000)	/* if NeXT */
		size = 30*2;
	else
		size = 46*2;		/* 68020 */

	stktop = mach->kbase+mach->pgsize;
	for(i=3; i<100; i++){
		get1(cormap, stktop-i*4, SEGDATA, (uchar*)&l, 4);

		if(machdata->swal(l) == 0xBADC0C0A){
			get1(cormap, stktop-(i-1)*4, SEGDATA, (uchar *)&efl[0], 4);
			get1(cormap, stktop-(i-2)*4, SEGDATA, (uchar *)&efl[1], 4);
			fvo = (ef[6]<<8)|ef[7];
			vec = fvo & 0xfff;
			vec >>= 2;
			if(vec >= 256)
				continue;

			for(ft=ftype; ft->name; ft++) {
				if(ft->fmt == ((fvo>>12) & 0xF)){
					m68020vec = vec;
					return size+ft->len;
				}
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
	if(excep[m68020vec])
		dprint("%s\n", excep[m68020vec]);
}
