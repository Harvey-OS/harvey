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

/*S00600004844521B*/
static	ulong	ucode1[] = {
/*S30902202000*/ 0x3FFF0000,
/*S30902202004*/ 0x3FFD0000,
/*S30902202008*/ 0x3FFB0000,
/*S3090220200C*/ 0x3FF90000,
/*S30902202010*/ 0x5F13EFF8,
/*S30902202014*/ 0x5EB5EFF8,
/*S30902202018*/ 0x5F88ADF7,
/*S3090220201C*/ 0x5FEFADF7,
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
/*S30902202048*/ 0xB0925D8D,
/*S3090220204C*/ 0xDFD079F7,
/*S30902202050*/ 0xB090E6BB,
/*S30902202054*/ 0xE5BBE74F,
/*S30902202058*/ 0x9E046F0F,
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
/*S30902202138*/ 0x5D8FDF61,
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
/*S309022021D8*/ 0x3A11E710,
/*S309022021DC*/ 0xEDF0CCDD,
/*S309022021E0*/ 0xF3186D0A,
/*S309022021E4*/ 0x7F0E5F06,
/*S309022021E8*/ 0x7FEDBB38,
/*S309022021EC*/ 0x3AFE7468,
/*S309022021F0*/ 0x7FEDF4FC,
/*S309022021F4*/ 0x8FFBB951,
/*S309022021F8*/ 0xB85F77FD,
/*S309022021FC*/ 0xB0DF5DDD,
/*S30902202200*/ 0xDEFE7FED,
/*S30902202204*/ 0x90E1E74D,
/*S30902202208*/ 0x6F0DCBF7,
/*S3090220220C*/ 0xE7DECFED,
/*S30902202210*/ 0xCB74CFED,
/*S30902202214*/ 0xCFEDDF6D,
/*S30902202218*/ 0x91714F74,
/*S3090220221C*/ 0x5DD2DEEF,
/*S30902202220*/ 0x9E04E7DF,
/*S30902202224*/ 0xEFBB6FFB,
/*S30902202228*/ 0xE7EF7F0E,
/*S3090220222C*/ 0x9E097FED,
/*S30902202230*/ 0xEBDBEFFA,
/*S30902202234*/ 0xEB54AFFB,
/*S30902202238*/ 0x7FEA90D7,
/*S3090220223C*/ 0x7E0CF0C3,
/*S30902202240*/ 0xBFFFF318,
/*S30902202244*/ 0x5FFFDFFF,
/*S30902202248*/ 0xAC59EFEA,
/*S3090220224C*/ 0x7FCE1EE5,
/*S30902202250*/ 0xE2FF5EE1,
/*S30902202254*/ 0xAFFBE2FF,
/*S30902202258*/ 0x5EE3AFFB,
/*S3090220225C*/ 0xF9CC7D0F,
/*S30902202260*/ 0xAEF8770F,
/*S30902202264*/ 0x7D0FB0C6,
/*S30902202268*/ 0xEFFBBFFF,
/*S3090220226C*/ 0xCFEF5EDE,
/*S30902202270*/ 0x7D0FBFFF,
/*S30902202274*/ 0x5EDE4CF8,
/*S30902202278*/ 0x7FDDD0BF,
/*S3090220227C*/ 0x49F847FD,
/*S30902202280*/ 0x7EFDF0BB,
/*S30902202284*/ 0x7FEDFFFD,
/*S30902202288*/ 0x7DFDF0B7,
/*S3090220228C*/ 0xEF7E7E1E,
/*S30902202290*/ 0x5EDE7F0E,
/*S30902202294*/ 0x3A11E710,
/*S30902202298*/ 0xEDF0CCAB,
/*S3090220229C*/ 0xFB18AD2E,
/*S309022022A0*/ 0x1EA9BBB8,
/*S309022022A4*/ 0x74283B7E,
/*S309022022A8*/ 0x73C2E4BB,
/*S309022022AC*/ 0x2ADA4FB8,
/*S309022022B0*/ 0xDC21E4BB,
/*S309022022B4*/ 0xB2A1FFBF,
/*S309022022B8*/ 0x5E2C43F8,
/*S309022022BC*/ 0xFC87E1BB,
/*S309022022C0*/ 0xE74FFD91,
/*S309022022C4*/ 0x6F0F4FE8,
/*S309022022C8*/ 0xC7BA32E2,
/*S309022022CC*/ 0xF396EFEB,
/*S309022022D0*/ 0x600B4F78,
/*S309022022D4*/ 0xE5BB760B,
/*S309022022D8*/ 0x53ACAEF8,
/*S309022022DC*/ 0x4EF88B0E,
/*S309022022E0*/ 0xCFEF9E09,
/*S309022022E4*/ 0xABF8751F,
/*S309022022E8*/ 0xEFEF5BAC,
/*S309022022EC*/ 0x741F4FE8,
/*S309022022F0*/ 0x751E760D,
/*S309022022F4*/ 0x7FDBF081,
/*S309022022F8*/ 0x741CAFCE,
/*S309022022FC*/ 0xEFCC7FCE,
/*S30902202300*/ 0x751E70AC,
/*S30902202304*/ 0x741CE7BB,
/*S30902202308*/ 0x3372CFED,
/*S3090220230C*/ 0xAFDBEFEB,
/*S30902202310*/ 0xE5BB760B,
/*S30902202314*/ 0x53F2AEF8,
/*S30902202318*/ 0xAFE8E7EB,
/*S3090220231C*/ 0x4BF8771E,
/*S30902202320*/ 0x7E247FED,
/*S30902202324*/ 0x4FCBE2CC,
/*S30902202328*/ 0x7FBC30A9,
/*S3090220232C*/ 0x7B0F7A0F,
/*S30902202330*/ 0x34D577FD,
/*S30902202334*/ 0x308B5DB7,
/*S30902202338*/ 0xDE553E5F,
/*S3090220233C*/ 0xAF78741F,
/*S30902202340*/ 0x741F30F0,
/*S30902202344*/ 0xCFEF5E2C,
/*S30902202348*/ 0x741F3EAC,
/*S3090220234C*/ 0xAFB8771E,
/*S30902202350*/ 0x5E677FED,
/*S30902202354*/ 0x0BD3E2CC,
/*S30902202358*/ 0x741CCFEC,
/*S3090220235C*/ 0xE5CA53CD,
/*S30902202360*/ 0x6FCB4F74,
/*S30902202364*/ 0x5DADDE4B,
/*S30902202368*/ 0x2AB63D38,
/*S3090220236C*/ 0x4BB3DE30,
/*S30902202370*/ 0x751F741C,
/*S30902202374*/ 0x6C42EFFA,
/*S30902202378*/ 0xEFEA7FCE,
/*S3090220237C*/ 0x6FFC30BE,
/*S30902202380*/ 0xEFEC3FCA,
/*S30902202384*/ 0x30B3DE2E,
/*S30902202388*/ 0xADF85D9E,
/*S3090220238C*/ 0xAF7DAEFD,
/*S30902202390*/ 0x5D9EDE2E,
/*S30902202394*/ 0x5D9EAFDD,
/*S30902202398*/ 0x761F10AC,
/*S3090220239C*/ 0x1DA07EFD,
/*S309022023A0*/ 0x30ADFFFE,
/*S309022023A4*/ 0x4908FB18,
/*S309022023A8*/ 0x5FFFDFFF,
/*S309022023AC*/ 0xAFBB709B,
/*S309022023B0*/ 0x4EF85E67,
/*S309022023B4*/ 0xADF814AD,
/*S309022023B8*/ 0x7A0F70AD,
/*S309022023BC*/ 0xCFEF50AD,
/*S309022023C0*/ 0x7A0FDE30,
/*S309022023C4*/ 0x5DA0AFED,
/*S309022023C8*/ 0x3C12780F,
/*S309022023CC*/ 0xEFEF780F,
/*S309022023D0*/ 0xEFEF790F,
/*S309022023D4*/ 0xA7F85E0F,
/*S309022023D8*/ 0xFFEF790F,
/*S309022023DC*/ 0xEFEF790F,
/*S309022023E0*/ 0x14ADDE2E,
/*S309022023E4*/ 0x5D9EADFD,
/*S309022023E8*/ 0x5E2DFFFB,
/*S309022023EC*/ 0xE79ADDFD,
/*S309022023F0*/ 0xEFF96079,
/*S309022023F4*/ 0x607AE79A,
/*S309022023F8*/ 0xDDFCEFF9,
/*S309022023FC*/ 0x60795DFF,
/*S30902202400*/ 0x607ACFEF,
/*S30902202404*/ 0xEFEFEFDF,
/*S30902202408*/ 0xEFBFEF7F,
/*S3090220240C*/ 0xEEFFEDFF,
/*S30902202410*/ 0xEBFFE7FF,
/*S30902202414*/ 0xAFEFAFDF,
/*S30902202418*/ 0xAFBFAF7F,
/*S3090220241C*/ 0xAEFFADFF,
/*S30902202420*/ 0xABFFA7FF,
/*S30902202424*/ 0x6FEF6FDF,
/*S30902202428*/ 0x6FBF6F7F,
/*S3090220242C*/ 0x6EFF6DFF,
/*S30902202430*/ 0x6BFF67FF,
/*S30902202434*/ 0x2FEF2FDF,
/*S30902202438*/ 0x2FBF2F7F,
/*S3090220243C*/ 0x2EFF2DFF,
/*S30902202440*/ 0x2BFF27FF,
/*S30902202444*/ 0x4E08FD1F,
/*S30902202448*/ 0xE5FF6E0F,
/*S3090220244C*/ 0xAFF87EEF,
/*S30902202450*/ 0x7E0FFDEF,
/*S30902202454*/ 0xF11F6079,
/*S30902202458*/ 0xABF8F542,
/*S3090220245C*/ 0x7E0AF11C,
/*S30902202460*/ 0x37CFAE3A,
/*S30902202464*/ 0x7FEC90BE,
/*S30902202468*/ 0xADF8EFDC,
/*S3090220246C*/ 0xCFEAE52F,
/*S30902202470*/ 0x7D0FE12B,
/*S30902202474*/ 0xF11C6079,
/*S30902202478*/ 0x7E0A4DF8,
/*S3090220247C*/ 0xCFEA5DC4,
/*S30902202480*/ 0x7D0BEFEC,
/*S30902202484*/ 0xCFEA5DC6,
/*S30902202488*/ 0xE522EFDC,
/*S3090220248C*/ 0x5DC6CFDA,
/*S30902202490*/ 0x4E08FD1F,
/*S30902202494*/ 0x6E0FAFF8,
/*S30902202498*/ 0x7C1F761F,
/*S3090220249C*/ 0xFDEFF91F,
/*S309022024A0*/ 0x6079ABF8,
/*S309022024A4*/ 0x761CEE24,
/*S309022024A8*/ 0xF91F2BFB,
/*S309022024AC*/ 0xEFEFCFEC,
/*S309022024B0*/ 0xF91F6079,
/*S309022024B4*/ 0x761C27FB,
/*S309022024B8*/ 0xEFDF5DA7,
/*S309022024BC*/ 0xCFDC7FDD,
/*S309022024C0*/ 0xD09C4BF8,
/*S309022024C4*/ 0x47FD7C1F,
/*S309022024C8*/ 0x761CCFCF,
/*S309022024CC*/ 0x7EEF7FED,
/*S309022024D0*/ 0x7DFDF093,
/*S309022024D4*/ 0xEF7E7F1E,
/*S309022024D8*/ 0x771EFB18,
/*S309022024DC*/ 0x6079E722,
/*S309022024E0*/ 0xE6BBE5BB,
/*S309022024E4*/ 0xAE0AE5BB,
/*S309022024E8*/ 0x600BAE85,
/*S309022024EC*/ 0xE2BBE2BB,
/*S309022024F0*/ 0xE2BBE2BB,
/*S309022024F4*/ 0xAF02E2BB,
/*S309022024F8*/ 0xE2BB2FF9,
/*S309022024FC*/ 0x6079E2BB,
};

static	ulong	ubase2 = 0x2F00;
static	ulong	ucode2[] = {
/*S30902202F00*/ 0x30303030,
/*S30902202F04*/ 0x3E3E3434,
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
/*S30902202F74*/ 0x6935AF78,
/*S30902202F78*/ 0xB9B3BAA3,
/*S30902202F7C*/ 0xB8788683,
/*S30902202F80*/ 0x368F78F7,
/*S30902202F84*/ 0x87778733,
/*S30902202F88*/ 0x3FFFFB3B,
/*S30902202F8C*/ 0x8E8F78B8,
/*S30902202F90*/ 0x1D118E13,
/*S30902202F94*/ 0xF3FF3F8B,
/*S30902202F98*/ 0x6BD8E173,
/*S30902202F9C*/ 0xD1366856,
/*S30902202FA0*/ 0x68D1687B,
/*S30902202FA4*/ 0x3DAF78B8,
/*S30902202FA8*/ 0x3A3A3F87,
/*S30902202FAC*/ 0x8F81378F,
/*S30902202FB0*/ 0xF876F887,
/*S30902202FB4*/ 0x77FD8778,
/*S30902202FB8*/ 0x737DE8D6,
/*S30902202FBC*/ 0xBBF8BFFF,
/*S30902202FC0*/ 0xD8DF87F7,
/*S30902202FC4*/ 0xFD876F7B,
/*S30902202FC8*/ 0x8BFFF8BD,
/*S30902202FCC*/ 0x8683387D,
/*S30902202FD0*/ 0xB873D87B,
/*S30902202FD4*/ 0x3B8FD7F8,
/*S30902202FD8*/ 0xF7338883,
/*S30902202FDC*/ 0xBB8EE1F8,
/*S30902202FE0*/ 0xEF837377,
/*S30902202FE4*/ 0x3337B836,
/*S30902202FE8*/ 0x817D11F8,
/*S30902202FEC*/ 0x7378B878,
/*S30902202FF0*/ 0xD3368B7D,
/*S30902202FF4*/ 0xED731B7D,
/*S30902202FF8*/ 0x833731F3,
/*S30902202FFC*/ 0xF22F3F23,
/*S30902202E00*/ 0x27EEEEEE,
/*S30902202E04*/ 0xEEEEEEEE,
/*S30902202E08*/ 0xEEEEEEEE,
/*S30902202E0C*/ 0xEEEEEEEE,
/*S30902202E10*/ 0xEE4BF4FB,
/*S30902202E14*/ 0xDBD259BB,
/*S30902202E18*/ 0x1979577F,
/*S30902202E1C*/ 0xDFD2D573,
/*S30902202E20*/ 0xB773F737,
/*S30902202E24*/ 0x4B4FBDBD,
/*S30902202E28*/ 0x25B9B177,
/*S30902202E2C*/ 0xD2D17376,
/*S30902202E30*/ 0x956BBFDD,
/*S30902202E34*/ 0x697BDD2F,
/*S30902202E38*/ 0xFF9F79FF,
/*S30902202E3C*/ 0xFF9FF22F,
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

	io->rctr1 = 0x8080;	/* relocate SPI */
	io->rctr2 = 0x808a;	/* relocate SPI */
	io->rctr3 = 0x8028;	/* relocate I2C */
	io->rctr4 = 0x802a;	/* relocate I2C */
	io->rccr |= 3;
	done = 1;
}
