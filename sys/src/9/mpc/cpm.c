#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

enum {
	BDSIZE=	1024,	/* IO memory reserved for buffer descriptors */
	CPMSIZE=	1024,	/* IO memory reserved for other uses */
};

static	Map	bdmapv[BDSIZE/sizeof(BD)];
static	RMap	bdmap = {"buffer descriptors"};

static	Map	cpmmapv[CPMSIZE/sizeof(ulong)];
static	RMap	cpmmap = {"CPM memory"};

static	Lock	cpmlock;

static struct {
	Lock;
	ulong	avail;
} brgens;

static struct {
	Lock;
	ulong	clash;	/* disable bits for mutually-exclusive options */
	ulong	active;	/* disable bit for current owner, or 0 */
	void	(*stop)(void*);
	void*	arg;
} scc2owner;

static	void	i2cspireloc(void);

/*
 * initialise the communications processor module
 * and associated device registers
 */
void
cpminit(void)
{
	IMM *io;

	io = m->iomem;
	io->rccr = 0;
	io->rmds = 0;
//	io->lccr &= ~1;	/* disable LCD */
	io->sdcr = 1;
	io->pcint = 0;	/* disable all port C interrupts */
	io->pcso = 0;
	io->pcdir =0;
	io->pcpar = 0;
	io->papar = 0;
	io->padir = 0;
	io->pbpar = 0;
	io->pbdir = 0;
	io->tgcr = 0x2222;	/* reset timers, low-power stop */
	eieio();

	for(io->cpcr = 0x8001; io->cpcr & 1;)	/* reset all CPM channels */
		eieio();

	mapinit(&bdmap, bdmapv, sizeof(bdmapv));
	mapfree(&bdmap, DPBASE, BDSIZE);
	mapinit(&cpmmap, cpmmapv, sizeof(cpmmapv));
	mapfree(&cpmmap, DPBASE+BDSIZE, CPMSIZE);

	if(m->cputype == 0x50 && (getimmr() & 0xFFFF) <= 0x2001)
		brgens.avail = 0x3;
	else
		brgens.avail = 0xF;

	scc2owner.clash = DisableEther|DisableIR|DisableRS232b;

	i2cspireloc();
}

/*
 * issue a request to a CPM device
 */
void
cpmop(int op, int cno, int param)
{
	IMM *io;

	ilock(&cpmlock);
	io = m->iomem;
	while(io->cpcr & 1)
		eieio();
	io->cpcr = (op<<8)|(cno<<4)|(param<<1)|1;
	eieio();
	while(io->cpcr & 1)
		eieio();
	iunlock(&cpmlock);
}

/*
 * lock the shared IO memory and return a reference to it
 */
IMM*
ioplock(void)
{
	ilock(&cpmlock);
	return m->iomem;
}

/*
 * release the lock on the shared IO memory
 */
void
iopunlock(void)
{
	eieio();
	iunlock(&cpmlock);
}

/*
 * connect SCCx clocks in NSMI mode (x=1 for USB)
 */
void
sccnmsi(int x, int rcs, int tcs)
{
	IMM *io;
	ulong v;
	int sh;

	sh = (x-1)*8;	/* each SCCx field in sicr is 8 bits */
	v = (((rcs&7)<<3) | (tcs&7)) << sh;
	io = ioplock();
	io->sicr = (io->sicr & ~(0xFF<<sh)) | v;
	iopunlock();
}

/*
 * request or grab (if force!=0) the use of SCC2
 * for the mode (uart, ether, etc) defined by the enable bits.
 * the `stop' function is invoked with the given argument
 * when the device is lost to another driver.
 */
int
scc2claim(int force, ulong enable, void (*stop)(void*), void *arg)
{
	ulong clash;

	clash = scc2owner.clash & ~enable;
	ilock(&scc2owner);
	if(/* ~m->bcsr[1] & clash */ 0){
		if(!force){
			iunlock(&scc2owner);
			return 0;
		}
		/* notify current owner */
		if(scc2owner.stop)
			scc2owner.stop(scc2owner.arg);
//		m->bcsr[1] |= clash;
	}
	scc2owner.stop = stop;
	scc2owner.arg = arg;
	scc2owner.active = enable;
//	m->bcsr[1] &= ~enable;	/* the bits are active low */
	iunlock(&scc2owner);
	/* beware: someone else can claim it during the `successful' return */
	return 1;
}

/*
 * if SCC2 is currently enabled, shut it down
 */
void
scc2stop(void *a)
{
	SCC *scc;

	scc = a;
	if(scc->gsmrl & (3<<4)){
		cpmop(GracefulStopTx, SCC2ID, 0);
		cpmop(CloseRxBD, SCC2ID, 0);
		delay(1);
		scc->gsmrl &= ~(3<<4);	/* disable current use */
//		m->bcsr[1] |= scc2owner.clash;
	}
}

/*
 * yield up control of SCC2
 */
void
scc2free(void)
{
	ilock(&scc2owner);
//	m->bcsr[1] |= scc2owner.active;
	scc2owner.active = 0;
	iunlock(&scc2owner);
}

/*
 * allocate a buffer descriptor
 */
BD *
bdalloc(int n)
{
	ulong a;

	a = rmapalloc(&bdmap, 0, n*sizeof(BD), sizeof(BD));
	if(a == 0)
		panic("bdalloc");
	return KADDR(a);
}

/*
 * free a buffer descriptor
 */
void
bdfree(BD *b, int n)
{
	if(b){
		eieio();
		mapfree(&bdmap, PADDR(b), n*sizeof(BD));
	}
}

/*
 * allocate memory from the shared IO memory space
 */
void *
cpmalloc(int n, int align)
{
	ulong a;

	a = rmapalloc(&cpmmap, 0, n, align);
	if(a == 0)
		panic("cpmalloc");
	return KADDR(a);
}

/*
 * free previously allocated shared memory
 */
void
cpmfree(void *p, int n)
{
	if(p != nil && n > 0){
		eieio();
		mapfree(&cpmmap, PADDR(p), n);
	}
}

/*
 * allocate a baud rate generator, returning its index
 * (or -1 if none is available)
 */
int
brgalloc(void)
{
	int n;

	lock(&brgens);
	for(n=0; brgens.avail!=0; n++)
		if(brgens.avail & (1<<n)){
			brgens.avail &= ~(1<<n);
			unlock(&brgens);
			return n;
		}
	unlock(&brgens);
	return -1;
}

/*
 * free a previously allocated baud rate generator
 */
void
brgfree(int n)
{
	if(n >= 0){
		if(n > 3 || brgens.avail & (1<<n))
			panic("brgfree");
		lock(&brgens);
		brgens.avail |= 1 << n;
		unlock(&brgens);
	}
}

/*
 * return a value suitable for loading into a baud rate
 * generator to produce the given rate if the generator
 * is prescaled by the given amount (typically 16).
 * the value must be or'd with BaudEnable to start the generator.
 */
ulong
baudgen(int rate, int scale)
{
	int d;

	rate *= scale;
	d = (2*m->cpuhz+rate)/(2*rate) - 1;
	if(d < 0)
		d = 0;
	if(d >= (1<<12))
		return ((d+15)>>(4-1))|1;	/* divider too big: enable prescale by 16 */
	return d<<1;
}

/*
 * initialise receive and transmit buffer rings.
 */
int
ioringinit(Ring* r, int nrdre, int ntdre, int bufsize)
{
	int i, x;

	/* the ring entries must be aligned on sizeof(BD) boundaries */
	r->nrdre = nrdre;
	if(r->rdr == nil)
		r->rdr = bdalloc(nrdre);
	if(r->rrb == nil)
		r->rrb = malloc(nrdre*bufsize);
	dcflush(r->rrb, nrdre*bufsize);
	if(r->rdr == nil || r->rrb == nil)
		return -1;
	x = PADDR(r->rrb);
	for(i = 0; i < nrdre; i++){
		r->rdr[i].length = 0;
		r->rdr[i].addr = x;
		r->rdr[i].status = BDEmpty|BDInt;
		x += bufsize;
	}
	r->rdr[i-1].status |= BDWrap;
	r->rdrx = 0;

	r->ntdre = ntdre;
	if(r->tdr == nil)
		r->tdr = bdalloc(ntdre);
	if(r->txb == nil)
		r->txb = malloc(ntdre*sizeof(Block*));
	if(r->tdr == nil || r->txb == nil)
		return -1;
	for(i = 0; i < ntdre; i++){
		r->txb[i] = nil;
		r->tdr[i].addr = 0;
		r->tdr[i].length = 0;
		r->tdr[i].status = 0;
	}
	r->tdr[i-1].status |= BDWrap;
	r->tdrh = 0;
	r->tdri = 0;
	r->ntq = 0;
	return 0;
}

/*
 * Allocate a new parameter block for I2C or SPI,
 * and plant a pointer to it for the microcode,
 * returning the kernel address.
 * See motorola errata and microcode package:
 * the design botch is that the parameters for the SCC2 ethernet overlap the
 * SPI/I2C parameter space; this compensates by relocating the latter.
 * This routine may be used iff i2cspireloc is used (and it is, above).
 */
void*
cpmparam(ulong olda, int nb)
{
	void *p;

	if(olda < INTMEM)
		olda += INTMEM;
	if(olda != SPIP && olda != I2CP)
		return (void*)(olda);
	p = cpmalloc(nb, 32);	/* ``RPBASE must be multiple of 32'' */
	if(p == nil)
		return p;
	*(ushort*)(olda+0x2C) = PADDR(p);	/* set RPBASE */
	eieio();
	return p;
}

/*
 * I2C/SPI microcode package from Motorola
 * (to relocate I2C/SPI parameters), which was distributed
 * on their web site in S-record format.
 *
 *	May 1998
 */

static	ulong	ubase1 = 0x2000;

static	ulong	ucode1[] = {
/*S30902202000*/ 0x7FFFEFD9,
/*S30902202004*/ 0x3FFD0000,
/*S30902202008*/ 0x7FFB49F7,
/*S3090220200C*/ 0x7FF90000,
/*S30902202010*/ 0x5FEFADF7,
/*S30902202014*/ 0x5F88ADF7,
/*S30902202018*/ 0x5FEFAFF7,
/*S3090220201C*/ 0x5F88AFF7,
/*S30902202020*/ 0x3A9CFBC8,
/*S30902202024*/ 0x77CAE1BB,
/*S30902202028*/ 0xF4DE7FAD,
/*S3090220202C*/ 0xABAE9330,
/*S30902202030*/ 0x4E08FDCF,
/*S30902202034*/ 0x6E0FAFF8,
/*S30902202038*/ 0x7CCF76CF,
/*S3090220203C*/ 0xFDAFF9CF,
/*S30902202040*/ 0xABF88DC8,
/*S30902202044*/ 0xAB5879F7,
/*S30902202048*/ 0xB0926A27,
/*S3090220204C*/ 0xDFD079F7,
/*S30902202050*/ 0xB090E6BB,
/*S30902202054*/ 0xE5BBE74F,
/*S30902202058*/ 0xAA616F0F,
/*S3090220205C*/ 0x6FFB76CE,
/*S30902202060*/ 0xEE0CF9CF,
/*S30902202064*/ 0x2BFBEFEF,
/*S30902202068*/ 0xCFEEF9CF,
/*S3090220206C*/ 0x76CEAD23,
/*S30902202070*/ 0x90B3DF99,
/*S30902202074*/ 0x7FDDD0C1,
/*S30902202078*/ 0x4BF847FD,
/*S3090220207C*/ 0x7CCF76CE,
/*S30902202080*/ 0xCFEF77CA,
/*S30902202084*/ 0x7EAF7FAD,
/*S30902202088*/ 0x7DFDF0B7,
/*S3090220208C*/ 0xEF7A7FCA,
/*S30902202090*/ 0x77CAFBC8,
/*S30902202094*/ 0x6079E722,
/*S30902202098*/ 0xFBC85FFF,
/*S3090220209C*/ 0xDFFF5FB3,
/*S309022020A0*/ 0xFFFBFBC8,
/*S309022020A4*/ 0xF3C894A5,
/*S309022020A8*/ 0xE7C9EDF9,
/*S309022020AC*/ 0x7F9A7FAD,
/*S309022020B0*/ 0x5F36AFE8,
/*S309022020B4*/ 0x5F5BFFDF,
/*S309022020B8*/ 0xDF95CB9E,
/*S309022020BC*/ 0xAF7D5FC3,
/*S309022020C0*/ 0xAFED8C1B,
/*S309022020C4*/ 0x5FC3AFDD,
/*S309022020C8*/ 0x5FC5DF99,
/*S309022020CC*/ 0x7EFDB0B3,
/*S309022020D0*/ 0x5FB3FFFE,
/*S309022020D4*/ 0xABAE5FB3,
/*S309022020D8*/ 0xFFFE5FD0,
/*S309022020DC*/ 0x600BE6BB,
/*S309022020E0*/ 0x600B5FD0,
/*S309022020E4*/ 0xDFC827FB,
/*S309022020E8*/ 0xEFDF5FCA,
/*S309022020EC*/ 0xCFDE3A9C,
/*S309022020F0*/ 0xE7C9EDF9,
/*S309022020F4*/ 0xF3C87F9E,
/*S309022020F8*/ 0x54CA7FED,
/*S309022020FC*/ 0x2D3A3637,
/*S30902202100*/ 0x756F7E9A,
/*S30902202104*/ 0xF1CE37EF,
/*S30902202108*/ 0x2E677FEE,
/*S3090220210C*/ 0x10EBADF8,
/*S30902202110*/ 0xEFDECFEA,
/*S30902202114*/ 0xE52F7D9F,
/*S30902202118*/ 0xE12BF1CE,
/*S3090220211C*/ 0x5F647E9A,
/*S30902202120*/ 0x4DF8CFEA,
/*S30902202124*/ 0x5F717D9B,
/*S30902202128*/ 0xEFEECFEA,
/*S3090220212C*/ 0x5F73E522,
/*S30902202130*/ 0xEFDE5F73,
/*S30902202134*/ 0xCFDA0B61,
/*S30902202138*/ 0x6A29DF61,
/*S3090220213C*/ 0xE7C9EDF9,
/*S30902202140*/ 0x7E9A30D5,
/*S30902202144*/ 0x1458BFFF,
/*S30902202148*/ 0xF3C85FFF,
/*S3090220214C*/ 0xDFFFA7F8,
/*S30902202150*/ 0x5F5BBFFE,
/*S30902202154*/ 0x7F7D10D0,
/*S30902202158*/ 0x144D5F33,
/*S3090220215C*/ 0xBFFFAF78,
/*S30902202160*/ 0x5F5BBFFD,
/*S30902202164*/ 0xA7F85F33,
/*S30902202168*/ 0xBFFE77FD,
/*S3090220216C*/ 0x30BD4E08,
/*S30902202170*/ 0xFDCFE5FF,
/*S30902202174*/ 0x6E0FAFF8,
/*S30902202178*/ 0x7EEF7E9F,
/*S3090220217C*/ 0xFDEFF1CF,
/*S30902202180*/ 0x5F17ABF8,
/*S30902202184*/ 0x0D5B5F5B,
/*S30902202188*/ 0xFFEF79F7,
/*S3090220218C*/ 0x309EAFDD,
/*S30902202190*/ 0x5F3147F8,
/*S30902202194*/ 0x5F31AFED,
/*S30902202198*/ 0x7FDD50AF,
/*S3090220219C*/ 0x497847FD,
/*S309022021A0*/ 0x7F9E7FED,
/*S309022021A4*/ 0x7DFD70A9,
/*S309022021A8*/ 0xEF7E7ECE,
/*S309022021AC*/ 0x6BA07F9E,
/*S309022021B0*/ 0x2D227EFD,
/*S309022021B4*/ 0x30DB5F5B,
/*S309022021B8*/ 0xFFFD5F5B,
/*S309022021BC*/ 0xFFEF5F5B,
/*S309022021C0*/ 0xFFDF0C9C,
/*S309022021C4*/ 0xAFED0A9A,
/*S309022021C8*/ 0xAFDD0C37,
/*S309022021CC*/ 0x5F37AFBD,
/*S309022021D0*/ 0x7FBDB081,
/*S309022021D4*/ 0x5F8147F8,
};

static	ulong	ubase2 = 0x2F00;
static	ulong	ucode2[] = {
/*S30902202F00*/ 0x3E303430,
/*S30902202F04*/ 0x34343737,
/*S30902202F08*/ 0xABBF9B99,
/*S30902202F0C*/ 0x4B4FBDBD,
/*S30902202F10*/ 0x59949334,
/*S30902202F14*/ 0x9FFF37FB,
/*S30902202F18*/ 0x9B177DD9,
/*S30902202F1C*/ 0x936956BB,
/*S30902202F20*/ 0xFBDD697B,
/*S30902202F24*/ 0xDD2FD113,
/*S30902202F28*/ 0x1DB9F7BB,
/*S30902202F2C*/ 0x36313963,
/*S30902202F30*/ 0x79373369,
/*S30902202F34*/ 0x3193137F,
/*S30902202F38*/ 0x7331737A,
/*S30902202F3C*/ 0xF7BB9B99,
/*S30902202F40*/ 0x9BB19795,
/*S30902202F44*/ 0x77FDFD3D,
/*S30902202F48*/ 0x573B773F,
/*S30902202F4C*/ 0x737933F7,
/*S30902202F50*/ 0xB991D115,
/*S30902202F54*/ 0x31699315,
/*S30902202F58*/ 0x31531694,
/*S30902202F5C*/ 0xBF4FBDBD,
/*S30902202F60*/ 0x35931497,
/*S30902202F64*/ 0x35376956,
/*S30902202F68*/ 0xBD697B9D,
/*S30902202F6C*/ 0x96931313,
/*S30902202F70*/ 0x19797937,
/*S30902202F74*/ 0x69350000,
};

/*
 * compensate for chip design botch by installing
 * microcode to relocate I2C and SPI parameters away
 * from the ethernet parameters
 */
static void
i2cspireloc(void)
{
	IMM *io;
	static int done;

	if(done)
		return;
	io = m->iomem;
	io->rccr &= ~3;
	memmove((uchar*)m->iomem+ubase1, ucode1, sizeof(ucode1));
	memmove((uchar*)m->iomem+ubase2, ucode2, sizeof(ucode2));

	io->rctr1 = 0x802a;	/* relocate SPI */
	io->rctr2 = 0x8028;	/* relocate SPI */
	io->rctr3 = 0x802e;	/* relocate I2C */
	io->rctr4 = 0x802c;	/* relocate I2C */
	io->rccr |= 1;
	done = 1;
}
