#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"
#include "io.h"


/*
 *  read and crack the card information structure enough to set
 *  important parameters like power
 */
static void	tcfig(PCMslot*, Cisdat*, int);
static void	tentry(PCMslot*, Cisdat*, int);
static void	tvers1(PCMslot*, Cisdat*, int);

struct {
	int n;
	void (*parse)(PCMslot*, Cisdat*, int);
} cistab[] = {
	0x15, tvers1,
	0x1A, tcfig,
	0x1B, tentry,
};

static int
readc(Cisdat *pp, uchar *x)
{
	if(pp->cispos >= pp->cislen)
		return 0;
	*x = pp->cisbase[pp->cisskip*pp->cispos];
	pp->cispos++;
	return 1;
}

static int
xcistuple(int slotno, int tuple, int subtuple, void *v, int nv, int attr)
{
	PCMmap *m;
	Cisdat cis;
	int i, l;
	uchar *p;
	uchar type, link, n, c;
	int this, subtype;

	m = pcmmap(slotno, 0, 0, attr);
	if(m == 0)
		return -1;

	cis.cisbase = KADDR(m->isa);
	cis.cispos = 0;
	cis.cisskip = attr ? 2 : 1;
	cis.cislen = m->len;

	/* loop through all the tuples */
	for(i = 0; i < 1000; i++){
		this = cis.cispos;
		if(readc(&cis, &type) != 1)
			break;
		if(type == 0xFF)
			break;
		if(readc(&cis, &link) != 1)
			break;
		if(link == 0xFF)
			break;

		n = link;
		if (link > 1 && subtuple != -1) {
			if (readc(&cis, &c) != 1)
				break;
			subtype = c;
			n--;
		} else
			subtype = -1;

		if(type == tuple && subtype == subtuple) {
			p = v;
			for(l=0; l<nv && l<n; l++)
				if(readc(&cis, p++) != 1)
					break;
			pcmunmap(slotno, m);
			return nv;
		}
		cis.cispos = this + (2+link);
	}
	pcmunmap(slotno, m);
	return -1;
}

int
pcmcistuple(int slotno, int tuple, int subtuple, void *v, int nv)
{
	int n;

	/* try attribute space, then memory */
	if((n = xcistuple(slotno, tuple, subtuple, v, nv, 1)) >= 0)
		return n;
	return xcistuple(slotno, tuple, subtuple, v, nv, 0);
}

void
pcmcisread(PCMslot *pp)
{
	uchar v[256];
	int i, nv;
	Cisdat cis;

	memset(pp->ctab, 0, sizeof(pp->ctab));
	pp->caddr = 0;
	pp->cpresent = 0;
	pp->configed = 0;
	pp->nctab = 0;
	pp->verstr[0] = 0;

	for(i = 0; i < nelem(cistab); i++) {
		if((nv = pcmcistuple(pp->slotno, cistab[i].n, -1, v, sizeof(v))) >= 0) {
			cis.cisbase = v;
			cis.cispos = 0;
			cis.cisskip = 1;
			cis.cislen = nv;
			
			(*cistab[i].parse)(pp, &cis, cistab[i].n);
		}
	}
}

static ulong
getlong(Cisdat *cis, int size)
{
	uchar c;
	int i;
	ulong x;

	x = 0;
	for(i = 0; i < size; i++){
		if(readc(cis, &c) != 1)
			break;
		x |= c<<(i*8);
	}
	return x;
}

static void
tcfig(PCMslot *pp, Cisdat *cis, int )
{
	uchar size, rasize, rmsize;
	uchar last;

	if(readc(cis, &size) != 1)
		return;
	rasize = (size&0x3) + 1;
	rmsize = ((size>>2)&0xf) + 1;
	if(readc(cis, &last) != 1)
		return;
	pp->caddr = getlong(cis, rasize);
	pp->cpresent = getlong(cis, rmsize);
}

static ulong vexp[8] =
{
	1, 10, 100, 1000, 10000, 100000, 1000000, 10000000
};
static ulong vmant[16] =
{
	10, 12, 13, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 70, 80, 90,
};

static ulong
microvolt(Cisdat *cis)
{
	uchar c;
	ulong microvolts;
	ulong exp;

	if(readc(cis, &c) != 1)
		return 0;
	exp = vexp[c&0x7];
	microvolts = vmant[(c>>3)&0xf]*exp;
	while(c & 0x80){
		if(readc(cis, &c) != 1)
			return 0;
		switch(c){
		case 0x7d:
			break;		/* high impedence when sleeping */
		case 0x7e:
		case 0x7f:
			microvolts = 0;	/* no connection */
			break;
		default:
			exp /= 10;
			microvolts += exp*(c&0x7f);
		}
	}
	return microvolts;
}

static ulong
nanoamps(Cisdat *cis)
{
	uchar c;
	ulong nanoamps;

	if(readc(cis, &c) != 1)
		return 0;
	nanoamps = vexp[c&0x7]*vmant[(c>>3)&0xf];
	while(c & 0x80){
		if(readc(cis, &c) != 1)
			return 0;
		if(c == 0x7d || c == 0x7e || c == 0x7f)
			nanoamps = 0;
	}
	return nanoamps;
}

/*
 * only nominal voltage (feature 1) is important for config,
 * other features must read card to stay in sync.
 */
static ulong
power(Cisdat *cis)
{
	uchar feature;
	ulong mv;

	mv = 0;
	if(readc(cis, &feature) != 1)
		return 0;
	if(feature & 1)
		mv = microvolt(cis);
	if(feature & 2)
		microvolt(cis);
	if(feature & 4)
		microvolt(cis);
	if(feature & 8)
		nanoamps(cis);
	if(feature & 0x10)
		nanoamps(cis);
	if(feature & 0x20)
		nanoamps(cis);
	if(feature & 0x40)
		nanoamps(cis);
	return mv/1000000;
}

static ulong mantissa[16] =
{ 0, 10, 12, 13, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 70, 80, };

static ulong exponent[8] =
{ 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, };

static ulong
ttiming(Cisdat *cis, int scale)
{
	uchar unscaled;
	ulong nanosecs;

	if(readc(cis, &unscaled) != 1)
		return 0;
	nanosecs = (mantissa[(unscaled>>3)&0xf]*exponent[unscaled&7])/10;
	nanosecs = nanosecs * vexp[scale];
	return nanosecs;
}

static void
timing(Cisdat *cis, PCMconftab *ct)
{
	uchar c, i;

	if(readc(cis, &c) != 1)
		return;
	i = c&0x3;
	if(i != 3)
		ct->maxwait = ttiming(cis, i);		/* max wait */
	i = (c>>2)&0x7;
	if(i != 7)
		ct->readywait = ttiming(cis, i);		/* max ready/busy wait */
	i = (c>>5)&0x7;
	if(i != 7)
		ct->otherwait = ttiming(cis, i);		/* reserved wait */
}

static void
iospaces(Cisdat *cis, PCMconftab *ct)
{
	uchar c;
	int i, nio;

	ct->nio = 0;
	if(readc(cis, &c) != 1)
		return;

	ct->bit16 = ((c>>5)&3) >= 2;
	if(!(c & 0x80)){
		ct->io[0].start = 0;
		ct->io[0].len = 1<<(c&0x1f);
		ct->nio = 1;
		return;
	}

	if(readc(cis, &c) != 1)
		return;

	nio = (c&0xf)+1;
	for(i = 0; i < nio; i++){
		ct->io[i].start = getlong(cis, (c>>4)&0x3);
		ct->io[0].len = getlong(cis, (c>>6)&0x3);
	}
	ct->nio = nio;
}

static void
irq(Cisdat *cis, PCMconftab *ct)
{
	uchar c;

	if(readc(cis, &c) != 1)
		return;
	ct->irqtype = c & 0xe0;
	if(c & 0x10)
		ct->irqs = getlong(cis, 2);
	else
		ct->irqs = 1<<(c&0xf);
	ct->irqs &= 0xDEB8;		/* levels available to card */
}

static void
memspace(Cisdat *cis, int asize, int lsize, int host)
{
	ulong haddress, address, len;

	len = getlong(cis, lsize)*256;
	address = getlong(cis, asize)*256;
	USED(len, address);
	if(host){
		haddress = getlong(cis, asize)*256;
		USED(haddress);
	}
}

static void
tentry(PCMslot *pp, Cisdat *cis, int )
{
	uchar c, i, feature;
	PCMconftab *ct;

	if(pp->nctab >= nelem(pp->ctab))
		return;
	if(readc(cis, &c) != 1)
		return;
	ct = &pp->ctab[pp->nctab++];

	/* copy from last default config */
	if(pp->def)
		*ct = *pp->def;

	ct->index = c & 0x3f;

	/* is this the new default? */
	if(c & 0x40)
		pp->def = ct;

	/* memory wait specified? */
	if(c & 0x80){
		if(readc(cis, &i) != 1)
			return;
		if(i&0x80)
			ct->memwait = 1;
	}

	if(readc(cis, &feature) != 1)
		return;
	switch(feature&0x3){
	case 1:
		ct->vpp1 = ct->vpp2 = power(cis);
		break;
	case 2:
		power(cis);
		ct->vpp1 = ct->vpp2 = power(cis);
		break;
	case 3:
		power(cis);
		ct->vpp1 = power(cis);
		ct->vpp2 = power(cis);
		break;
	default:
		break;
	}
	if(feature&0x4)
		timing(cis, ct);
	if(feature&0x8)
		iospaces(cis, ct);
	if(feature&0x10)
		irq(cis, ct);
	switch((feature>>5)&0x3){
	case 1:
		memspace(cis, 0, 2, 0);
		break;
	case 2:
		memspace(cis, 2, 2, 0);
		break;
	case 3:
		if(readc(cis, &c) != 1)
			return;
		for(i = 0; i <= (c&0x7); i++)
			memspace(cis, (c>>5)&0x3, (c>>3)&0x3, c&0x80);
		break;
	}
	pp->configed++;
}

static void
tvers1(PCMslot *pp, Cisdat *cis, int )
{
	uchar c, major, minor, last;
	int  i;

	if(readc(cis, &major) != 1)
		return;
	if(readc(cis, &minor) != 1)
		return;
	last = 0;
	for(i = 0; i < sizeof(pp->verstr)-1; i++){
		if(readc(cis, &c) != 1)
			return;
		if(c == 0)
			c = ';';
		if(c == '\n')
			c = ';';
		if(c == 0xff)
			break;
		if(c == ';' && last == ';')
			continue;
		pp->verstr[i] = c;
		last = c;
	}
	pp->verstr[i] = 0;
}
