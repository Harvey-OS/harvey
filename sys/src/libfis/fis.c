/*
 * sata fises and sas frames
 * copyright © 2009-2010 erik quanstrom
 */
#include <u.h>
#include <libc.h>
#include <fis.h>

static char *flagname[9] = {
	"lba",
	"llba",
	"smart",
	"power",
	"nop",
	"atapi",
	"atapi16",
	"ata8",
	"sct",
};

/*
 * ata8 standard (llba) cmd layout
 *
 * feature	16 bits
 * count		16 bits
 * lba		48 bits
 * device		8 bits
 * command	8 bits
 *
 * response:
 *
 * status		8 bits
 * error		8 bits
 * reason		8 bits
 * count		8 bits
 * sstatus		8 bits
 * sactive		8 bits
*/

/*
 * sata fis layout for fistype 0x27: host-to-device:
 *
 * 0	fistype
 * 1	fis flags
 * 2	ata command
 * 3	features
 * 4	sector	lba low	7:0
 * 5	cyl low	lba mid	15:8
 * 6	cyl hi	lba hi	23:16
 * 7	device / head
 * 8	sec exp	lba	31:24
 * 9	cy low e	lba	39:32
 * 10	cy hi e	lba	48:40
 * 11	features (exp)
 * 12	sector count
 * 13	sector count (exp)
 * 14	r
 * 15	control
 */

void
setfissig(Sfis *x, uint sig)
{
	x->sig = sig;
}

void
skelfis(uchar *c)
{
	memset(c, 0, Fissize);
	c[Ftype] = H2dev;
	c[Fflags] = Fiscmd;
	c[Fdev] = Ataobs;
}

int
nopfis(Sfis*, uchar *c, int srst)
{
	skelfis(c);
	if(srst){
		c[Fflags] &= ~Fiscmd;
		c[Fcontrol] = 1<<2;
		return Preset|P28;
	}
	return Pnd|P28;
}

int
txmodefis(Sfis *f, uchar *c, uchar d)
{
	int m;

	/* hack */
	if((f->sig >> 16) == 0xeb14)
		return -1;
	m = 0x40;
	if(d == 0xff){
		d = 0;
		m = 0;
	}
	skelfis(c);
	c[Fcmd] = 0xef;
	c[Ffeat] = 3;			/* set transfer mode */
	c[Fsc] = m | d;			/* sector count */
	return Pnd|P28;
}

int
featfis(Sfis*, uchar *c, uchar f)
{
	skelfis(c);
	c[Fcmd] = 0xef;
	c[Ffeat] = f;
	return Pnd|P28;
}

int
identifyfis(Sfis *f, uchar *c)
{
	static uchar tab[] = { 0xec, 0xa1, };

	skelfis(c);
	c[Fcmd] = tab[f->sig>>16 == 0xeb14];
	return Pin|Ppio|P28|P512;
}

int
flushcachefis(Sfis *f, uchar *c)
{
	static uchar tab[2] = {0xe7, 0xea};
	static uchar ptab[2] = {Pnd|P28, Pnd|P48};
	int llba;

	llba = (f->feat & Dllba) != 0;
	skelfis(c);
	c[Fcmd] = tab[llba];
	return ptab[llba];
}

static ushort
gbit16(void *a)
{
	ushort j;
	uchar *i;

	i = a;
	j  = i[1] << 8;
	j |= i[0];
	return j;
}

static uint
gbit32(void *a)
{
	uint j;
	uchar *i;

	i = a;
	j  = i[3] << 24;
	j |= i[2] << 16;
	j |= i[1] << 8;
	j |= i[0];
	return j;
}

static uvlong
gbit64(void *a)
{
	uchar *i;

	i = a;
	return (uvlong)gbit32(i+4) << 32 | gbit32(a);
}

ushort
id16(ushort *id, int i)
{
	return gbit16(id+i);
}

uint
id32(ushort *id, int i)
{
	return gbit32(id+i);
}

uvlong
id64(ushort *id, int i)
{
	return gbit64(id+i);
}

/* acs-2 §7.18.7.4 */
static ushort puistab[] = {
	0x37c8,	Pspinup,
	0x738c,	Pspinup | Pidready,
	0x8c73,	0,
	0xc837,	Pidready,
};

int
idpuis(ushort *id)
{
	ushort u, i;

	u = gbit16(id + 2);
	for(i = 0; i < nelem(puistab); i += 2)
		if(u == puistab[i])
			return puistab[i + 1];
	return Pidready;	/* annoying cdroms */
}

static ushort
onesc(ushort *id)
{
	ushort u;

	u = gbit16(id);
	if(u == 0xffff)
		u = 0;
	return u;
}

enum{
	Idmasp	= 1<<8,
	Ilbasp	= 1<<9,
	Illba	= 1<<10,

	Usepsec	= 1,
};

vlong
idfeat(Sfis *f, ushort *id)
{
	int i, j;
	vlong s;

	f->feat = 0;
	if(f->sig>>16 == 0xeb14)
		f->feat |= Datapi;
	i = gbit16(id + 49);
	if((i & Ilbasp) == 0){
		if(gbit16(id + 53) & 1){
			f->c = gbit16(id + 1);
			f->h = gbit16(id + 3);
			f->s = gbit16(id + 6);
		}else{
			f->c = gbit16(id + 54);
			f->h = gbit16(id + 55);
			f->s = gbit16(id + 56);
		}
		s = f->c*f->h*f->s;
	}else{
		f->c = f->h = f->s = 0;
		f->feat |= Dlba;
		j = gbit16(id + 83) | gbit16(id + 86);
		if(j & Illba){
			f->feat |= Dllba;
			s = gbit64(id + 100);
		}else
			s = gbit32(id + 60);
	}
	f->udma = 0xff;
	if(i & Idmasp)
	if(gbit16(id + 53) & 4)
		for(i = gbit16(id + 88) & 0x7f; i; i >>= 1)
			f->udma++;

	if(f->feat & Datapi){
		i = gbit16(id + 0);
		if(i & 1)
			f->feat |= Datapi16;
	}

	i = gbit16(id+83);
	if((i>>14) == 1){
		if(i & (1<<3))
			f->feat |= Dpower;
		i = gbit16(id + 82);
		if(i & 1)
			f->feat |= Dsmart;
		if(i & (1<<14))
			f->feat |= Dnop;
	}
	i = onesc(id + 80);
	if(i & 1<<8){
		f->feat |= Data8;
		i = onesc(id + 222);			/* sata? */
		j = onesc(id + 76);
		if(i != 0 && i >> 12 == 1 && j != 0){
			j >>= 1;
			f->speeds = j & 7;
			i = gbit16(id + 78) & gbit16(id + 79);
			/*
			 * not acceptable for comreset to
			 * wipe out device configuration.
			 * reject drive.
			 */
			if((i & 1<<6) == 0)
				return -1;
		}
	}
	if(gbit16(id + 206) & 1)
		f->feat |= Dsct;
	idss(f, id);
	if(Usepsec)
		return s>>f->physshift;
	return s;
}

int
idss(Sfis *f, ushort *id)
{
	uint sw, i;

	if(f->sig>>16 == 0xeb14)
		return 0;
	f->lsectsz = 512;
	f->physshift = 0;
	i = gbit16(id + 106);
	if(i >> 14 != 1)
		return f->lsectsz;
	if((sw = gbit32(id + 117)) >= 256)
		f->lsectsz = sw * 2;
	if(i & 1<<13)
		f->physshift = i & 7;
	if(Usepsec)
		return f->lsectsz << f->physshift;
	return f->lsectsz;
}

uvlong
idwwn(Sfis*, ushort *id)
{
	uvlong u;

	u = 0;
	if(id[108]>>12 == 5){
		u |= (uvlong)gbit16(id + 108) << 48;
		u |= (uvlong)gbit16(id + 109) << 32;
		u |= gbit16(id + 110) << 16;
		u |= gbit16(id + 111) << 0;
	}
	return u;
}

void
idmove(char *p, ushort *u, int n)
{
	int i;
	char *op, *e, *s;

	op = p;
	s = (char*)u;
	for(i = 0; i < n; i += 2){
		*p++ = s[i + 1];
		*p++ = s[i + 0];
	}
	*p = 0;
	while(p > op && *--p == ' ')
		*p = 0;
	e = p;
	p = op;
	while(*p == ' ')
		p++;
	memmove(op, p, n - (e - p));
}

char*
pflag(char *s, char *e, Sfis *f)
{
	ushort i, u;

	u = f->feat;
	for(i = 0; i < Dnflag; i++)
		if(u & (1 << i))
			s = seprint(s, e, "%s ", flagname[i]);
	return seprint(s, e, "\n");
}

int
atapirwfis(Sfis *f, uchar *c, uchar *cdb, int cdblen, int ndata)
{
	int fill, len;

	fill = f->feat&Datapi16? 16: 12;
	if((len = cdblen) > fill)
		len = fill;
	memmove(c + 0x40, cdb, len);
	memset(c + 0x40 + len, 0, fill - len);

	c[Ftype] = H2dev;
	c[Fflags] = Fiscmd;
	c[Fcmd] = Ataobs;
	if(ndata != 0)
		c[Ffeat] = 1;	/* dma */
	else
		c[Ffeat] = 0;	/* features (exp); */
	c[Flba0] = 0;	
	c[Flba8] = ndata;
	c[Flba16] = ndata >> 8;
	c[Fdev] = Ataobs;
	memset(c + 8, 0, Fissize - 8);
	return P28|Ppkt;
}

int
rwfis(Sfis *f, uchar *c, int rw, int nsect, uvlong lba)
{
	uchar acmd, llba, udma;
	static uchar tab[2][2][2] = { 0x20, 0x24, 0x30, 0x34, 0xc8, 0x25, 0xca, 0x35, };
	static uchar ptab[2][2][2] = {
		Pin|Ppio|P28,	Pin|Ppio|P48,
		Pout|Ppio|P28,	Pout|Ppio|P48,
		Pin|Pdma|P28,	Pin|Pdma|P48,
		Pout|Pdma|P28,	Pout|Pdma|P48,
	};

	if(Usepsec){
		nsect <<= f->physshift;
		lba <<= f->physshift;
	}

	udma = f->udma != 0xff;
	llba = (f->feat & Dllba) != 0;
	acmd = tab[udma][rw][llba];

	c[Ftype] = 0x27;
	c[Fflags] = 0x80;
	c[Fcmd] = acmd;
	c[Ffeat] = 0;

	c[Flba0] = lba;
	c[Flba8] = lba >> 8;
	c[Flba16] = lba >> 16;
	c[Fdev] = Ataobs | Atalba;
	if(llba == 0)
		c[Fdev] |= (lba>>24) & 0xf;

	c[Flba24] = lba >> 24;
	c[Flba32] = lba >> 32;
	c[Flba40] = lba >> 48;
	c[Ffeat8] = 0;

	c[Fsc] = nsect;
	c[Fsc8] = nsect >> 8;
	c[Ficc] = 0;
	c[Fcontrol] = 0;

	memset(c + 16, 0, Fissize - 16);
	return ptab[udma][rw][llba];
}

uvlong
fisrw(Sfis *f, uchar *c, int *n)
{
	uvlong lba;

	lba = c[Flba0];
	lba |= c[Flba8] << 8;
	lba |= c[Flba16] << 16;
	lba |= c[Flba24] << 24;
	lba |= (uvlong)(c[Flba32] | c[Flba40]<<8) << 32;

	*n = c[Fsc];
	*n |= c[Fsc8] << 8;

	if(Usepsec){
		*n >>= f->physshift;
		lba >>= f->physshift;
	}

	return lba;
}

void
sigtofis(Sfis *f, uchar *c)
{
	uint u;

	u = f->sig;
	memset(c, 0, Fissize);
	c[Ftype] = 0x34;
	c[Fflags] = 0x00;
	c[Fcmd] = 0x50;
	c[Ffeat] = 0x01;
	c[Flba0] = u >> 8;
	c[Flba8] = u >> 16;
	c[Flba16] = u >> 24;
	c[Fdev] = Ataobs;
	c[Fsc] = u;
}

uint
fistosig(uchar *u)
{
	return u[Fsc] | u[Flba0]<<8 | u[Flba8]<<16 | u[Flba16]<<24;
}


/* sas smp */
void
smpskelframe(Cfis *f, uchar *c, int m)
{
	memset(c, 0, Fissize);
	c[Ftype] = 0x40;
	c[Fflags] = m;
	if(f->phyid)
		c[Flba32] = f->phyid;
}

uint
sashash(uvlong u)
{
	uint poly, msb, l, r;
	uvlong m;

	r = 0;
	poly = 0x01db2777;
	msb = 0x01000000;
	for(m = 1ull<<63; m > 0; m >>= 1){
		l = 0;
		if(m & u)
			l = msb;
		r <<= 1;
		r ^= l;
		if(r & msb)
			r ^= poly;
	}
	return r & 0xffffff;
}

uchar*
sasbhash(uchar *t, uchar *s)
{
	uint poly, msb, l, r, i, j;

	r = 0;
	poly = 0x01db2777;
	msb = 0x01000000;
	for(i = 0; i < 8; i++)
		for(j = 0x80; j != 0; j >>= 1){
			l = 0;
			if(s[i] & j)
				l = msb;
			r <<= 1;
			r ^= l;
			if(r & msb)
				r ^= poly;
		}
	t[0] = r>>16;
	t[1] = r>>8;
	t[2] = r;
	return t;
}
