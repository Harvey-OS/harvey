#include <u.h>
#include <libc.h>
#include <bio.h>
#include "debug.h"
#include "lebo.h"
#include "tap.h"
#include "chain.h"
#include "jtag.h"
#include "icert.h"
#include "mpsse.h"

/*
	See schematics, openocd code.
	Guessing for the GuruDisp...
 */

static uchar cablingH[][4] = {
	[Sheeva]		{[TRST] 0x2, [SRST] 0x0, [TRSTnOE]0x0, [SRSTnOE] 0x04,},
	[GuruDisp]	{[TRST] 0x2, [SRST] 0x0, [TRSTnOE]0x0, [SRSTnOE] 0x04,},
};

static uchar cablingL[][5] = {
	[Sheeva]		{[TCK] 0x01, [TDI] 0x02, [TDO]0x04, [TMS] 0x08, [nOE] 0x10,},
	[GuruDisp]	{[TCK] 0x01, [TDI] 0x02, [TDO]0x04, [TMS] 0x08, [nOE] 0x10,},
};

static uchar valcabling[][2] = {
	[Sheeva]		{isPushPull, isOpenDrain},
	[GuruDisp]	{isPushPull, isPushPull},
};

static u32int cpuids[] = {
	[Sheeva]		FeroceonId,
	[GuruDisp]	ArmadaId,
};

static uchar
hinitvalH(int motherb, int trst, int srst)
{
	uchar hout;

	hout = 0;

	if(trst){
		hout |= cablingH[motherb][TRSTnOE];
		hout &= ~cablingH[motherb][TRST];
	}
	else{
		hout &= ~cablingH[motherb][TRSTnOE];
		hout |= cablingH[motherb][TRST];
	}
	if(srst){
		hout &= ~cablingH[motherb][SRSTnOE];
		hout |= cablingH[motherb][SRST];
	}
	else{
		hout |= cablingH[motherb][SRSTnOE];
		hout &= ~cablingH[motherb][SRST];
	}

	return hout;
}

static uchar
hinitdirH(int motherb)
{
	USED(motherb);
	//int i;
	//uchar dir;
	//dir = 0;
	//for(i = 0; i < 4; i++)
	//	dir |= cablingH[motherb][i];	/* BUG: TODO */
	return 0xf;
}

int
mpsseflush(void *mdata)
{
	int r, nb;
	ushort s;
	Mpsse *mpsse;

	mpsse = mdata;
	s = 0xbebe;

	r = 0;

	nb = Bbuffered(&mpsse->bout);
	if(debug[DFile] || debug[DAll])
		fprint(2, "Flush %d\n", nb);

	if(debug[DXFlush])
		Bwrite(&mpsse->bout, &s, sizeof s);
	if(nb && Bflush(&mpsse->bout) == Beof)
		r = -1;
	
	return r;
}

static int
mpsseresets(void *mdata, int trst, int srst)
{
	char cmd[128];
	uchar hout, hdir;
	int motherb;
	Mpsse *mpsse;

	mpsse = mdata;
	motherb = mpsse->motherb;

	hdir = hinitdirH(motherb);
	hout = hinitvalH(motherb, trst, srst);

	snprint(cmd, sizeof(cmd), "SetBitsH %#2.2ux %#2.2ux", hout, hdir);
	if(pushcmd(mpsse, cmd) < 0)
		return -1;
	if(mpsseflush(mpsse) < 0)
		return -1;
	return 0;
}


static int
initpins(Mpsse *mpsse)
{
	char cmd[128];
	uchar hout, hdir;
	int motherb;
	Biobufhdr *bout;

	motherb = mpsse->motherb;
	bout = &mpsse->bout;

	/* value of this also depends on opendrain etc?, seems it does not */
	hout = cablingL[motherb][TMS];
	hdir =  cablingL[motherb][TCK]|cablingL[motherb][TDI];
	hdir |= cablingL[motherb][TMS]|cablingL[motherb][nOE];
	snprint(cmd, sizeof(cmd), "SetBitsL %#2.2ux %#2.2ux", hout, hdir);
	if(pushcmd(mpsse, cmd) < 0){
		Bterm(bout);
		return -1;
	}
	if(mpsseflush(mpsse) < 0)
		return -1;

	/* is this Board dependant? */
	if(mpsseresets(mpsse, 0, 0) < 0)
		return -1;

	return 0;
}

static int
mpsseterm(void *mdata)
{
	Mpsse *mpsse;
	int r;
	
	mpsse = mdata;
	r =  Bterm(&mpsse->bout);
	free(mpsse);
	
	return r;
}

/* I always work with bits on the lsb side, nbits count the
 * n less significant bits, no matter msb or lsb order
 * I know that mpsse works with lsb on the lsb side and
 * msb I am not sure how it is codified msb on lsb side would be
 * 00001234, instead of lsb on msb side 12340000
 * In any case I never use msb on the mpsse,
 * just on the paths for convenience and they are lsb side
 * like 000001234
 * in other words, nbits always counts from lsb bit
 */
static ulong
msb2lsb(ulong msbl, int nbits)
{
	int i;
	ulong lsbl, bit;

	lsbl = 0;
	for(i = 0; i < nbits; i++){
		bit = (msbl >> (nbits - i - 1))&1;
		lsbl |= bit << i;
	}
	return lsbl;
}

/* how many clk bits takes to clock out a data bit */
static int 
tmsdata2clk(int nbits)
{
	return ((nbits + 6)/7) * 3;
}

#define takebits(byte, nbits, offset) (((byte) >> (offset)) & ((1U << (nbits))-1))

static void
dropbits(Mpsse *mpsse, int nbits)
{
	int nby, nbi;

	nby = nbits / 8;
	nbi = nbits % 8;

	assert(mpsse->nbits >= nbits);
	
	mpsse->nbits -= 8*nby;
	mpsse->rb += nby;

	mpsse->rbits = (mpsse->rbits + nbi) %8;
	
	mpsse->nbits -= nbi;
}

static int
runpath(JMedium *jmed, SmPath *pth, int isrd, int issend)
{
	SmPath pref;
	uchar tmslsb, lastbit;
	int nclkbits;
	char cmd[128];
	Mpsse *mpsse;
	
	mpsse = jmed->mdata;
	lastbit = 0;
	nclkbits = 0;
	if(issend && mpsse->nbits != 0){
		lastbit = mpsse->lastbit;
		mpsse->nbits--;
		nclkbits = 1;
	}
	
	while(pth->ptmslen != 0){
		pref = takepathpref(pth, MaxNbitsT);
		tmslsb = msb2lsb(pref.ptms, pref.ptmslen);

		if(issend && nclkbits--){
			tmslsb |= lastbit<<7;
		}
		if(isrd)
			snprint(cmd, sizeof(cmd), "TmsCsOutIn EdgeDown EdgeUp LSB B%#2.2ux %#2.2ux",
				(uchar)pref.ptmslen, tmslsb);
		else
			snprint(cmd, sizeof(cmd), "TmsCsOut EdgeDown LSB B%#2.2ux %#2.2ux",
				(uchar)pref.ptmslen, tmslsb);
		
		if(pushcmd(mpsse, cmd) < 0)
			return -1;
	}

	moveto(jmed, pth->st);
	return 0;
}

static int
sendbytes(Mpsse *mpsse, int nbytes, int op)
{
	char cmd[128];
	int totbytes, nwbytes;
	uchar *buf;

	buf = mpsse->rb;
	totbytes = mpsse->nbits/8;
	nwbytes = nbytes;
	if(totbytes < nbytes){
		werrstr("sendbytes: not enough %d<%d", totbytes, nbytes);
		return -1;
	}
	if( (op&ShiftIn ) && !(op&ShiftOut) ){
		snprint(cmd, sizeof(cmd), "DataIn EdgeUp LSB %#2.2ux",
				nbytes);
		nwbytes = 0;
	}
	else if( !(op&ShiftIn) && (op&ShiftOut) )
		snprint(cmd, sizeof(cmd), "DataOut EdgeDown LSB %#2.2ux @",
				nbytes);
	else if( (op&ShiftIn) && (op&ShiftOut) )
		
		snprint(cmd, sizeof(cmd), "DataOutIn EdgeDown EdgeUp LSB %#2.2ux @",
				nbytes);
	else
		return -1;


	if(pushcmdwdata(mpsse, cmd, buf, nwbytes) < 0)
		return -1;

	dropbits(mpsse, 8*nbytes);
	return 0;
}


static int
sendbits(Mpsse *mpsse, int nbits, int op)
{
	char cmd[128];
	uchar lastbyte, obyte;

	lastbyte = *mpsse->rb;
	if(nbits > 8){
		werrstr("too many bits %d>%d", nbits, MaxNbitsS);
		return -1;
	}


	obyte = takebits(lastbyte, nbits, mpsse->rbits);

	if( (op&ShiftIn ) && !(op&ShiftOut) )
		snprint(cmd, sizeof(cmd), "DataIn EdgeUp LSB B%#2.2ux",
				nbits);
	else  if( !(op&ShiftIn) && (op&ShiftOut) )
		snprint(cmd, sizeof(cmd), "DataOut EdgeDown LSB B%#2.2ux %#2.2ux",
				nbits, obyte);
	else if( (op&ShiftIn) && (op&ShiftOut) )
		
		snprint(cmd, sizeof(cmd), "DataOutIn EdgeDown EdgeUp LSB B%#2.2ux %#2.2ux",
				nbits, obyte);
	else
		return -1;


	if(pushcmd(mpsse, cmd) < 0)
		return -1;

	dropbits(mpsse, nbits);
	return 0;
}

static int
movetost(JMedium *jmed, int dst, int isrd, int issend)
{
	SmPath pth;
	int len;

	pth = pathto(jmed, dst);
	len = pth.ptmslen;
	if(runpath(jmed, &pth, isrd, issend) < 0)
		return -1;
	return len;
}

static int
stayinst(JMedium *jmed, int nclock)
{
	SmPath pth;

	pth.ptms = 0;
	pth.ptmslen = nclock;
	if(runpath(jmed, &pth, 0, 0) < 0)
		return -1;
	return nclock;
}

static int
mpsserdshiftrep(JMedium *jmed, uchar *buf, ShiftRDesc *rep)
{
	int nr, npartial, nby;
	uchar msk;

	Mpsse *mpsse;

	mpsse = jmed->mdata;
	dprint(DFile, "Reading %d\n", rep->nbyread);

	nr = readn(mpsse->jtagfd, buf, rep->nbyread);

	npartial = 0;

	dprint(DFile, "Read %d\n", nr);
	dumpbuf(DFile, buf, nr);
	if(nr <= 0)
		return -1;
	if(rep->nbiprelast != 0){
		npartial++;
	}
	if(rep->nbilast != 0){
		npartial++;
	}
	nby = rep->nbyread - npartial;
	if(rep->nbiprelast != 0){
		buf[nby] = buf[nby] >> (8 - rep->nbiprelast);
	}
	if(rep->nbilast != 0){
		msk = MSK(rep->nbilast);
		buf[nby] |= (buf[nby+1] & msk) << (8 - rep->nbilast);
	}
	return (7 + nby + npartial) / 8;	/* readjust after compacting */
}

/*
 * May need a couple of two or three bytes of margin for reading,
 * do not call it with just enough bytes
*/

static int
mpsseregshift(JMedium *jmed, ShiftTDesc *req, ShiftRDesc *rep)
{
	int npsend, nr, nl, isrd, nback, nby, ntrail, reg, nbits, op;
	ShiftRDesc rr;
	uchar *buf;
	Mpsse *mpsse;

	reg = req->reg;
	buf = req->buf;
	nbits = req->nbits;
	op = req->op;

	mpsse = jmed->mdata;
	if(rep == nil)
		rep = &rr;

	nr = 0;
	nby = 0;
	nback = 0;
	npsend = 0;


	if(reg != TapDR && reg != TapIR)
		sysfatal("unknown register");

	isrd = op&ShiftIn;

	/* 	
	 *	May need to go to Pause to cross capture before starting
	 *	May still have a trailing bit from last shifting
	 */

	if(op&ShiftPauseIn){
		nl = movetost(jmed, TapPauseDR+reg, 0, mpsse->nbits != 0);
		if(nl < 0){
			werrstr("regshift: going to pause in");
			return -1;
		}
	}

	if(movetost(jmed, TapShiftDR+reg, 0, mpsse->nbits != 0) < 0){
		werrstr("regshift: going to shift");
		return -1;
	}

	mpsse->nbits = nbits;
	mpsse->rb = buf;
	mpsse->rbits = 0;

	if((mpsse->nbits / 8)  > 1){
		nby = mpsse->nbits / 8;
		if(mpsse->nbits % 8 == 0)
			nby--;
		if(sendbytes(mpsse, nby, op) < 0){
			werrstr("regshift: shifting bytes");
			return -1;
		}
		nback = nby;
	}


	if(mpsse->nbits > 1){
		nback++;				/* each op gives a partial byte */
		npsend = mpsse->nbits;
		if(mpsse->nbits > 7)		/* apparently hw does not like 8 bytes */
			npsend = MaxNbitsS;
		npsend -= 1;	/* one is for the way */
		if(sendbits(mpsse, npsend, op) < 0){
			werrstr("regshift: shifting bits");
			return -1;
		}
		mpsse->lastbit = takebits(*mpsse->rb, 1, mpsse->rbits);
	}


	/* 	I only go through pause if I need extra time to shift
	 *	for example, I need a command to read or if I am told
	*/

	if(isrd || (op&ShiftPauseOut)){
		nback++;
		nl = movetost(jmed, TapPauseDR+reg, isrd, mpsse->nbits != 0);
		if(nl < 0){
			werrstr("regshift: going to pause out");
			return -1;
		}
	}

	if(! (op&ShiftNoCommit)){
		nl = movetost(jmed, TapIdle, 0, mpsse->nbits != 0);
		if(nl < 0){
			werrstr("regshift: going to idle out");
			return -1;
		}
	}
	if(isrd){
		if(mpsseflush(mpsse) < 0){
			werrstr("regshift: flushing");
			return -1;
		}

		ntrail = nbits - npsend - 8*nby;
		rep->nbyread = nback;
		rep->nbiprelast = npsend;
		rep->nbilast = ntrail;
		if( !(op&ShiftAsync) )
			nr = mpsserdshiftrep(jmed, buf, rep);
	}
	return nr;
}

JMedium *
newmpsse(int fd, int motherb)
{
	Mpsse *mpsse;
	Biobufhdr *bout;
	JMedium *jmed;
	int i;

	jmed = mallocz(sizeof(JMedium), 1);
	if(jmed == nil)
		return nil;

	mpsse = malloc(sizeof(Mpsse));
	if(mpsse == nil)
		return nil;

	jmed->motherb = motherb;
	jmed->mdata = mpsse;
	jmed->regshift = mpsseregshift;
	jmed->rdshiftrep = mpsserdshiftrep;
	jmed->flush = mpsseflush;
	jmed->term = mpsseterm;
	jmed->resets = mpsseresets;

	/* BUG: configuration file? */
	if(motherb == Sheeva){
		jmed->ntaps = 1;
		jmed->tapcpu = 0;
		jmed->taps[0].hwid = FeroceonId;
		jmed->taps[0].irlen = InLen;
	}
	else if(motherb == GuruDisp){
		/* BUG: set concatenating mode */
		jmed->ntaps = 3;
		jmed->taps[0].hwid = 0;
		jmed->taps[0].irlen = 1;
		jmed->tapcpu = 1;
		jmed->taps[1].hwid = ArmadaId;
		jmed->taps[1].irlen = InLen;
		jmed->taps[2].hwid = 0;
		jmed->taps[2].irlen = 9;
	}
	else
		sysfatal("Unkwown motherboard");

	for(i = 0; i < jmed->ntaps; i++)
		jmed->taps[i].state = TapUnknown;
	jmed->state = TapUnknown;

	mpsse->jtagfd = fd;
	mpsse->motherb = motherb;
	

	bout = &mpsse->bout;

	if(Binits(bout, fd, OWRITE, mpsse->bp, MpsseBufSz) == Beof){
		free(mpsse);
		return nil;
	}
	return jmed;
}

JMedium *
initmpsse(int fd, int motherb)
{
	Mpsse *mpsse;
	JMedium *jmed;
	uchar buf[32];
	ShiftTDesc req;


	jmed = newmpsse(fd, motherb);
	if(jmed == nil){
		free(jmed);
		return nil;
	}
	mpsse = jmed->mdata;

	if(initpins(mpsse) < 0)
		goto Err;

	if(pushcmd(mpsse, "TckSkDiv 0x0200") < 0)
		goto Err;
	if(mpsseflush(mpsse) < 0)
		goto Err;

	if(pushcmd(mpsse, "BreakLoop") < 0)
		goto Err;
	if(mpsseflush(mpsse) < 0)
		goto Err;

	/* Board dependant ? */
	if(mpsseresets(mpsse, 0, 0) < 0)
		goto Err;

	if(motherb == GuruDisp){	/* set concat mode */
		req.reg = TapIR;
		hleputs(buf, InGuruTapctl);
		req.buf = buf;
		req.nbits = InGuruLen;
		req.op = ShiftOut;
		
		jmed->regshift(jmed, &req, nil);
		req.reg = TapDR;
		hleputs(buf, DrGuruTapctl);
		req.buf = buf;
		req.nbits = 16;
		req.op = ShiftOut;
		jmed->regshift(jmed, &req, nil);
		jmed->taps[2].TapSm = jmed->TapSm;
	}
	return jmed;
Err:
	mpsseterm(mpsse);
	free(jmed);
	return nil;
}

static void
termmpsse(JMedium *jmed)
{
	mpsseterm(jmed->mdata);
	free(jmed);
}


JMedium *
resetmpsse(JMedium *jmed)
{
	Mpsse mpsse;
	JMedium *jmnew;

	mpsse = *(Mpsse *)jmed->mdata;
	mpsseflush((Mpsse *)jmed->mdata);
	free(jmed->mdata);

	jmnew = initmpsse(mpsse.jtagfd, mpsse.motherb);
	return jmnew;
}

