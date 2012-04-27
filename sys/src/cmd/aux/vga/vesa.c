#include <u.h>
#include <libc.h>
#include <bio.h>
#include </386/include/ureg.h>
typedef struct Ureg Ureg;

#include "pci.h"
#include "vga.h"

typedef struct Vbe Vbe;
typedef struct Vmode Vmode;
typedef struct Modelist Modelist;
typedef struct Edid Edid;

enum
{
	MemSize = 1024*1024,
	PageSize = 4096,
	RealModeBuf = 0x9000,
};

struct Vbe
{
	int	rmfd;	/* /dev/realmode */
	int	memfd;	/* /dev/realmem */
	uchar	*mem;	/* copy of memory; 1MB */
	uchar	*isvalid;	/* 1byte per 4kB in mem */
	uchar	*buf;
	uchar	*modebuf;
};

struct Vmode
{
	char	name[32];
	char	chan[32];
	int	id;
	int	attr;	/* flags */
	int	bpl;
	int	dx, dy;
	int	depth;
	char	*model;
	int	r, g, b, x;
	int	ro, go, bo, xo;
	int	directcolor;	/* flags */
	ulong	paddr;
};

struct Edid {
	char		mfr[4];		/* manufacturer */
	char		serialstr[16];	/* serial number as string (in extended data) */
	char		name[16];		/* monitor name as string (in extended data) */
	ushort	product;		/* product code, 0 = unused */
	ulong	serial;		/* serial number, 0 = unused */
	uchar	version;		/* major version number */
	uchar	revision;		/* minor version number */
	uchar	mfrweek;		/* week of manufacture, 0 = unused */
	int		mfryear;		/* year of manufacture, 0 = unused */
	uchar 	dxcm;		/* horizontal image size in cm. */
	uchar	dycm;		/* vertical image size in cm. */
	int		gamma;		/* gamma*100 */
	int		rrmin;		/* minimum vertical refresh rate */
	int		rrmax;		/* maximum vertical refresh rate */
	int		hrmin;		/* minimum horizontal refresh rate */
	int		hrmax;		/* maximum horizontal refresh rate */
	ulong	pclkmax;		/* maximum pixel clock */
	int		flags;

	Modelist	*modelist;		/* list of supported modes */
};

struct Modelist
{
	Mode;
	Modelist *next;
};

enum {
	Fdigital = 1<<0,		/* is a digital display */
	Fdpmsstandby = 1<<1,	/* supports DPMS standby mode */
	Fdpmssuspend = 1<<2,	/* supports DPMS suspend mode */
	Fdpmsactiveoff = 1<<3,	/* supports DPMS active off mode */
	Fmonochrome = 1<<4,	/* is a monochrome display */
	Fgtf = 1<<5,		/* supports VESA GTF: see /public/doc/vesa/gtf10.pdf */
};

#define WORD(p) ((p)[0] | ((p)[1]<<8))
#define LONG(p) ((p)[0] | ((p)[1]<<8) | ((p)[2]<<16) | ((p)[3]<<24))
#define PWORD(p, v) (p)[0] = (v); (p)[1] = (v)>>8
#define PLONG(p, v) (p)[0] = (v); (p)[1] = (v)>>8; (p)[2] = (v)>>16; (p)[3] = (v)>>24

static Vbe *vbe;
static Edid edid;

Vbe *mkvbe(void);
int vbecheck(Vbe*);
uchar *vbemodes(Vbe*);
int vbemodeinfo(Vbe*, int, Vmode*);
int vbegetmode(Vbe*);
int vbesetmode(Vbe*, int);
void vbeprintinfo(Vbe*);
void vbeprintmodeinfo(Vbe*, int, char*);
int vbesnarf(Vbe*, Vga*);
void vesaddc(void);
int vbeddcedid(Vbe *vbe, Edid *e);
void printedid(Edid*);

int
dbvesa(Vga* vga)
{
	vbe = mkvbe();
	if(vbe == nil){
		fprint(2, "mkvbe: %r\n");
		return 0;
	}
	if(vbecheck(vbe) < 0){
		fprint(2, "dbvesa: %r\n");
		return 0;
	}
	vga->link = alloc(sizeof(Ctlr));
	*vga->link = vesa;
	vga->vesa = vga->link;
	vga->ctlr = vga->link;

	vga->link->link = alloc(sizeof(Ctlr));
	*vga->link->link = softhwgc;
	vga->hwgc = vga->link->link;

	return 1;
}

Mode*
dbvesamode(char *mode)
{
	int i;
	uchar *p, *ep;
	Vmode vm;
	Mode *m;
	
	if(vbe == nil)
		return nil;

	p = vbemodes(vbe);
	if(p == nil)
		return nil;
	for(ep=p+1024; (p[0]!=0xFF || p[1]!=0xFF) && p<ep; p+=2){
		if(vbemodeinfo(vbe, WORD(p), &vm) < 0)
			continue;
		if(strcmp(vm.name, mode) == 0)
			goto havemode;
	}
	if(1){
		fprint(2, "warning: scanning for unoffered vesa modes\n");
		for(i=0x100; i<0x200; i++){
			if(vbemodeinfo(vbe, i, &vm) < 0)
				continue;
			if(strcmp(vm.name, mode) == 0)
				goto havemode;
		}
	}
	werrstr("no such vesa mode");
	return nil;

havemode:
	m = alloc(sizeof(Mode));
	strcpy(m->type, "vesa");
	strcpy(m->size, vm.name);
	strcpy(m->chan, vm.chan);
	m->frequency = 100;
	m->x = vm.dx;
	m->y = vm.dy;
	m->z = vm.depth;
	m->ht = m->x;
	m->shb = m->x;
	m->ehb = m->x;
	m->shs = m->x;
	m->ehs = m->x;
	m->vt = m->y;
	m->vrs = m->y;
	m->vre = m->y;

	m->attr = alloc(sizeof(Attr));
	m->attr->attr = "id";
	m->attr->val = alloc(32);
	sprint(m->attr->val, "0x%x", vm.id);
	return m;
}

static void
snarf(Vga* vga, Ctlr* ctlr)
{
	if(!vbe)
		vbe = mkvbe();
	if(vbe)
		vga->vesa = ctlr;
	vbesnarf(vbe, vga);
	vga->linear = 1;
	ctlr->flag |= Hlinear|Ulinear;
}

static void
load(Vga* vga, Ctlr* ctlr)
{
	if(vbe == nil)
		error("no vesa bios\n");
	if(vbesetmode(vbe, atoi(dbattr(vga->mode->attr, "id"))) < 0){
		ctlr->flag |= Ferror;
		fprint(2, "vbesetmode: %r\n");
	}
}

static void
dump(Vga*, Ctlr*)
{
	int i;
	char did[0x200];
	uchar *p, *ep;

	if(!vbe){
		Bprint(&stdout, "no vesa bios\n");
		return;
	}

	memset(did, 0, sizeof did);
	vbeprintinfo(vbe);
	p = vbemodes(vbe);
	if(p){
		for(ep=p+1024; (p[0]!=0xFF || p[1]!=0xFF) && p<ep; p+=2){
			vbeprintmodeinfo(vbe, WORD(p), "");
			if(WORD(p) < nelem(did))
				did[WORD(p)] = 1;
		}
	}
	for(i=0x100; i<0x1FF; i++)
		if(!did[i])
			vbeprintmodeinfo(vbe, i, " (unoffered)");
				
	
	if(vbeddcedid(vbe, &edid) < 0)
		fprint(2, "warning: reading edid: %r\n");
	else
		printedid(&edid);
}

Ctlr vesa = {
	"vesa",			/* name */
	snarf,				/* snarf */
	0,			/* options */
	0,				/* init */
	load,				/* load */
	dump,				/* dump */
};

Ctlr softhwgc = {
	"soft",
};

/*
 * VESA bios extension
 */

typedef struct Flag Flag;
struct Flag {
	int bit;
	char *desc;
};

static Flag capabilityflag[] = {
	0x01, "8-bit-dac",
	0x02, "not-vga",
	0x04, "ramdac-needs-blank",
	0x08, "stereoscopic",
	0x10, "stereo-evc",
	0
};

static Flag modeattributesflags[] = {
	1<<0, "supported",
	1<<2, "tty",
	1<<3, "color",
	1<<4, "graphics",
	1<<5, "not-vga",
	1<<6, "no-windowed-vga",
	1<<7, "linear",
	1<<8, "double-scan",
	1<<9, "interlace",
	1<<10, "triple-buffer",
	1<<11, "stereoscopic",
	1<<12, "dual-start-addr",
	0
};

static Flag winattributesflags[] = {
	1<<0, "relocatable",
	1<<1, "readable",
	1<<2, "writeable",
	0
};

static Flag directcolorflags[] = {
	1<<0, "programmable-color-ramp",
	1<<1, "x-usable",
	0
};

static char *modelstr[] = {
	"text", "cga", "hercules", "planar", "packed", "non-chain4", "direct", "YUV"
};

static void
printflags(Flag *f, int b)
{
	int i;

	for(i=0; f[i].bit; i++)
		if(f[i].bit & b)
			Bprint(&stdout, " %s", f[i].desc);
	Bprint(&stdout, "\n");
}

Vbe*
mkvbe(void)
{
	Vbe *vbe;

	vbe = alloc(sizeof(Vbe));
	if((vbe->rmfd = open("/dev/realmode", ORDWR)) < 0)
		return nil;
	if((vbe->memfd = open("/dev/realmodemem", ORDWR)) < 0)
		return nil;
	vbe->mem = alloc(MemSize);
	vbe->isvalid = alloc(MemSize/PageSize);
	vbe->buf = alloc(PageSize);
	vbe->modebuf = alloc(PageSize);
	return vbe;
}

static void
loadpage(Vbe *vbe, int p)
{
	if(p >= MemSize/PageSize || vbe->isvalid[p])
		return;
	if(pread(vbe->memfd, vbe->mem+p*PageSize, PageSize, p*PageSize) != PageSize)
		error("read /dev/realmodemem: %r\n");
	vbe->isvalid[p] = 1;
}

static void*
unfarptr(Vbe *vbe, uchar *p)
{
	int seg, off;

	seg = WORD(p+2);
	off = WORD(p);
	if(seg==0 && off==0)
		return nil;
	off += seg<<4;
	if(off >= MemSize)
		return nil;
	loadpage(vbe, off/PageSize);
	loadpage(vbe, off/PageSize+1);	/* just in case */
	return vbe->mem+off;
}

uchar*
vbesetup(Vbe *vbe, Ureg *u, int ax)
{
	memset(vbe->buf, 0, PageSize);
	memset(u, 0, sizeof *u);
	u->ax = ax;
	u->es = (RealModeBuf>>4)&0xF000;
	u->di = RealModeBuf&0xFFFF;
	return vbe->buf;
}

int
vbecall(Vbe *vbe, Ureg *u)
{
	u->trap = 0x10;
	if(pwrite(vbe->memfd, vbe->buf, PageSize, RealModeBuf) != PageSize)
		error("write /dev/realmodemem: %r\n");
	if(pwrite(vbe->rmfd, u, sizeof *u, 0) != sizeof *u)
		error("write /dev/realmode: %r\n");
	if(pread(vbe->rmfd, u, sizeof *u, 0) != sizeof *u)
		error("read /dev/realmode: %r\n");
	if(pread(vbe->memfd, vbe->buf, PageSize, RealModeBuf) != PageSize)
		error("read /dev/realmodemem: %r\n");
	if((u->ax&0xFFFF) != 0x004F){
		werrstr("VBE error %#.4lux", u->ax&0xFFFF);
		return -1;
	}
	memset(vbe->isvalid, 0, MemSize/PageSize);
	return 0;
}

int
vbecheck(Vbe *vbe)
{
	uchar *p;
	Ureg u;

	p = vbesetup(vbe, &u, 0x4F00);
	strcpy((char*)p, "VBE2");
	if(vbecall(vbe, &u) < 0)
		return -1;
	if(memcmp(p, "VESA", 4) != 0 || p[5] < 2){
		werrstr("invalid vesa signature %.4H %.4H\n", p, p+4);
		return -1;
	}
	return 0;
}

int
vbesnarf(Vbe *vbe, Vga *vga)
{
	uchar *p;
	Ureg u;

	p = vbesetup(vbe, &u, 0x4F00);
	strcpy((char*)p, "VBE2");
	if(vbecall(vbe, &u) < 0)
		return -1;
	if(memcmp(p, "VESA", 4) != 0 || p[5] < 2)
		return -1;
	vga->apz = WORD(p+18)*0x10000UL;
	return 0;
}

void
vbeprintinfo(Vbe *vbe)
{
	uchar *p;
	Ureg u;

	p = vbesetup(vbe, &u, 0x4F00);
	strcpy((char*)p, "VBE2");
	if(vbecall(vbe, &u) < 0)
		return;

	printitem("vesa", "sig");
	Bprint(&stdout, "%.4s %d.%d\n", (char*)p, p[5], p[4]);
	if(p[5] < 2)
		return;

	printitem("vesa", "oem");
	Bprint(&stdout, "%s %d.%d\n", unfarptr(vbe, p+6), p[21], p[20]);
	printitem("vesa", "vendor");
	Bprint(&stdout, "%s\n", unfarptr(vbe, p+22));
	printitem("vesa", "product");
	Bprint(&stdout, "%s\n", unfarptr(vbe, p+26));
	printitem("vesa", "rev");
	Bprint(&stdout, "%s\n", unfarptr(vbe, p+30));

	printitem("vesa", "cap");
	printflags(capabilityflag, p[10]);

	printitem("vesa", "mem");
	Bprint(&stdout, "%lud\n", WORD(p+18)*0x10000UL);
}

uchar*
vbemodes(Vbe *vbe)
{
	uchar *p;
	Ureg u;

	p = vbesetup(vbe, &u, 0x4F00);
	strcpy((char*)p, "VBE2");
	if(vbecall(vbe, &u) < 0)
		return nil;
	memmove(vbe->modebuf, unfarptr(vbe, p+14), 1024);
	return vbe->modebuf;
}

int
vbemodeinfo(Vbe *vbe, int id, Vmode *m)
{
	int o;
	ulong d, c, x;
	uchar *p;
	char tmp[sizeof m->chan];
	Ureg u;

	p = vbesetup(vbe, &u, 0x4F01);
	u.cx = id;
	if(vbecall(vbe, &u) < 0)
		return -1;

	m->id = id;
	m->attr = WORD(p);
	m->bpl = WORD(p+16);
	m->dx = WORD(p+18);
	m->dy = WORD(p+20);
	m->depth = p[25];
	m->model = p[27] < nelem(modelstr) ? modelstr[p[27]] : "unknown";
	m->r = p[31];
	m->g = p[33];
	m->b = p[35];
	m->x = p[37];
	m->ro = p[32];
	m->go = p[34];
	m->bo = p[36];
	m->xo = p[38];
	m->directcolor = p[39];
	m->paddr = LONG(p+40);
	snprint(m->name, sizeof m->name, "%dx%dx%d",
		m->dx, m->dy, m->depth);
	if(m->depth <= 8) {
		snprint(m->chan, sizeof m->chan, "m%d", m->depth);
		return 0;
	}

	m->xo = m->x = 0;
	d = 1 << (m->depth - 1);
	d |= d - 1;
	c  = ((1<<m->r)-1) << m->ro;
	c |= ((1<<m->g)-1) << m->go;
	c |= ((1<<m->b)-1) << m->bo;
	x = d ^ c;
	if(x != 0){
		for(; (x & 1) == 0; x >>= 1)
			m->xo++;
		for(; x & 1; x >>= 1)
			m->x++;
	}

	m->chan[0] = o = 0;
	while(o < m->depth){
		if(m->r && m->ro == o){
			snprint(tmp, sizeof tmp, "r%d%s", m->r, m->chan);
			o += m->r;
		}else if(m->g && m->go == o){
			snprint(tmp, sizeof tmp, "g%d%s", m->g, m->chan);
			o += m->g;
		}else if(m->b && m->bo == o){
			snprint(tmp, sizeof tmp, "b%d%s", m->b, m->chan);
			o += m->b;
		}else if(m->x && m->xo == o){
			snprint(tmp, sizeof tmp, "x%d%s", m->x, m->chan);
			o += m->x;
		}else
			break;
		strncpy(m->chan, tmp, sizeof m->chan);
	}
	return 0;
}

void
vbeprintmodeinfo(Vbe *vbe, int id, char *suffix)
{
	Vmode m;

	if(vbemodeinfo(vbe, id, &m) < 0){
	//	Bprint(&stdout, "vesa: cannot get mode 0x%ux: %r\n", id);
		return;
	}
	printitem("vesa", "mode");
	Bprint(&stdout, "0x%ux %s %s %s%s\n",
		m.id, m.name, m.chan, m.model, suffix);
}

int
vbegetmode(Vbe *vbe)
{
	Ureg u;

	vbesetup(vbe, &u, 0x4F03);
	if(vbecall(vbe, &u) < 0)
		return 0;
	return u.bx;
}

int
vbesetmode(Vbe *vbe, int id)
{
	uchar *p;
	Ureg u;

	p = vbesetup(vbe, &u, 0x4F02);
	if(id != 3)
		id |= 3<<14;	/* graphics: use linear, do not clear */
	u.bx = id;
	USED(p);
	/*
	 * can set mode specifics (ht hss hse vt vss vse 0 clockhz refreshhz):
	 *
		u.bx |= 1<<11;
		n = atoi(argv[2]); PWORD(p, n); p+=2;
		n = atoi(argv[3]); PWORD(p, n); p+=2;
		n = atoi(argv[4]); PWORD(p, n); p+=2;
		n = atoi(argv[5]); PWORD(p, n); p+=2;
		n = atoi(argv[6]); PWORD(p, n); p+=2;
		n = atoi(argv[7]); PWORD(p, n); p+=2;
		*p++ = atoi(argv[8]);
		n = atoi(argv[9]); PLONG(p, n); p += 4;
		n = atoi(argv[10]); PWORD(p, n); p += 2;
		if(p != vbe.buf+19){
			fprint(2, "prog error\n");
			return;
		}
	 *
	 */
	return vbecall(vbe, &u);
}

void
vesatextmode(void)
{
	if(vbe == nil){
		vbe = mkvbe();
		if(!vbe)
			error("mkvbe: %r\n");
	}
	if(vbecheck(vbe) < 0)
		error("vbecheck: %r\n");
	if(vbesetmode(vbe, 3) < 0)
		error("vbesetmode: %r\n");
}

static Flag edidflags[] = {
	Fdigital, "digital",
	Fdpmsstandby, "standby",
	Fdpmssuspend, "suspend",
	Fdpmsactiveoff, "activeoff",
	Fmonochrome, "monochrome",
	Fgtf, "gtf",
	0
};

int parseedid128(Edid *e, void *v);

int
vbeddcedid(Vbe *vbe, Edid *e)
{
	uchar *p;
	Ureg u;
	
	p = vbesetup(vbe, &u, 0x4F15);
	u.bx = 0x0001;
	if(vbecall(vbe, &u) < 0)
		return -1;
	if(parseedid128(e, p) < 0){
		werrstr("parseedid128: %r");
		return -1;
	}
	return 0;
}
	
void
printedid(Edid *e)
{
	Modelist *l;
	
	printitem("edid", "mfr");
	Bprint(&stdout, "%s\n", e->mfr);
	printitem("edid", "serialstr");
	Bprint(&stdout, "%s\n", e->serialstr);
	printitem("edid", "name");
	Bprint(&stdout, "%s\n", e->name);
	printitem("edid", "product");
	Bprint(&stdout, "%d\n", e->product);
	printitem("edid", "serial");
	Bprint(&stdout, "%lud\n", e->serial);
	printitem("edid", "version");
	Bprint(&stdout, "%d.%d\n", e->version, e->revision);
	printitem("edid", "mfrdate");
	Bprint(&stdout, "%d.%d\n", e->mfryear, e->mfrweek);
	printitem("edid", "size (cm)");
	Bprint(&stdout, "%dx%d\n", e->dxcm, e->dycm);
	printitem("edid", "gamma");
	Bprint(&stdout, "%.2f\n", e->gamma/100.);
	printitem("edid", "vert (Hz)");
	Bprint(&stdout, "%d-%d\n", e->rrmin, e->rrmax);
	printitem("edid", "horz (Hz)");
	Bprint(&stdout, "%d-%d\n", e->hrmin, e->hrmax);
	printitem("edid", "pclkmax");
	Bprint(&stdout, "%lud\n", e->pclkmax);
	printitem("edid", "flags");
	printflags(edidflags, e->flags);
	
	for(l=e->modelist; l; l=l->next){
		printitem("edid", l->name);
		Bprint(&stdout, "\n\t\tclock=%g\n\t\tshb=%d ehb=%d ht=%d\n\t\tvrs=%d vre=%d vt=%d\n\t\thsync=%c vsync=%c %s\n",
			l->frequency/1.e6, l->shb, l->ehb, l->ht, l->vrs, l->vre, l->vt, l->hsync?l->hsync:'?', l->vsync?l->vsync:'?', l->interlace?"interlace=v" : "");
	}
}

Modelist*
addmode(Modelist *l, Mode m)
{
	int rr;
	Modelist **lp;

//m.z = 8; // BUG
	rr = (m.frequency+m.ht*m.vt/2)/(m.ht*m.vt);
	snprint(m.name, sizeof m.name, "%dx%dx%d@%dHz", m.x, m.y, m.z, rr);

	if(m.shs == 0)
		m.shs = m.shb;
	if(m.ehs == 0)
		m.ehs = m.ehb;
	if(m.vbs == 0)
		m.vbs = m.vrs;
	if(m.vbe == 0)
		m.vbe = m.vbs+1;

	for(lp=&l; *lp; lp=&(*lp)->next){
		if(strcmp((*lp)->name, m.name) == 0){
			(*lp)->Mode = m;
			return l;
		}
	}

	*lp = alloc(sizeof(**lp));
	(*lp)->Mode = m;
	return l;
}

/*
 * Parse VESA EDID information.  Based on the VESA
 * Extended Display Identification Data standard, Version 3,
 * November 13, 1997.  See /public/doc/vesa/edidv3.pdf.
 *
 * This only handles 128-byte EDID blocks.  Until I find
 * a monitor that produces 256-byte blocks, I'm not going
 * to try to decode them.
 */

/*
 * Established timings block.  There is a bitmap
 * that says whether each mode is supported.  Most
 * of these have VESA definitions.  Those that don't are marked
 * as such, and we ignore them (the lookup fails).
 */
static char *estabtime[] = {
	"720x400@70Hz",	/* non-VESA: IBM, VGA */
	"720x400@88Hz",	/* non-VESA: IBM, XGA2 */
	"640x480@60Hz",
	"640x480@67Hz",	/* non-VESA: Apple, Mac II */
	"640x480@72Hz",
	"640x480@75Hz",
	"800x600@56Hz",
	"800x600@60Hz",

	"800x600@72Hz",
	"800x600@75Hz",
	"832x624@75Hz",	/* non-VESA: Apple, Mac II */
	"1024x768i@87Hz",	/* non-VESA: IBM */
	"1024x768@60Hz",
	"1024x768@70Hz",
	"1024x768@75Hz",
	"1280x1024@75Hz",

	"1152x870@75Hz",	/* non-VESA: Apple, Mac II */
};

/*
 * Decode the EDID detailed timing block.  See pp. 20-21 of the standard.
 */
static int
decodedtb(Mode *m, uchar *p)
{
	int ha, hb, hso, hspw, rr, va, vb, vso, vspw;
	/* int dxmm, dymm, hbord, vbord; */

	memset(m, 0, sizeof *m);

	m->frequency = ((p[1]<<8) | p[0]) * 10000;

	ha = ((p[4] & 0xF0)<<4) | p[2];		/* horizontal active */
	hb = ((p[4] & 0x0F)<<8) | p[3];		/* horizontal blanking */
	va = ((p[7] & 0xF0)<<4) | p[5];		/* vertical active */
	vb = ((p[7] & 0x0F)<<8) | p[6];		/* vertical blanking */
	hso = ((p[11] & 0xC0)<<2) | p[8];	/* horizontal sync offset */
	hspw = ((p[11] & 0x30)<<4) | p[9];	/* horizontal sync pulse width */
	vso = ((p[11] & 0x0C)<<2) | ((p[10] & 0xF0)>>4);	/* vertical sync offset */
	vspw = ((p[11] & 0x03)<<4) | (p[10] & 0x0F);		/* vertical sync pulse width */

	/* dxmm = (p[14] & 0xF0)<<4) | p[12]; 	/* horizontal image size (mm) */
	/* dymm = (p[14] & 0x0F)<<8) | p[13];	/* vertical image size (mm) */
	/* hbord = p[15];		/* horizontal border (pixels) */
	/* vbord = p[16];		/* vertical border (pixels) */

	m->x = ha;
	m->y = va;

	m->ht = ha+hb;
	m->shs = ha;
	m->shb = ha+hso;
	m->ehb = ha+hso+hspw;
	m->ehs = ha+hb;

	m->vt = va+vb;
	m->vbs = va;	
	m->vrs = va+vso;
	m->vre = va+vso+vspw;
	m->vbe = va+vb;

	if(p[17] & 0x80)	/* interlaced */
		m->interlace = 'v';

	if(p[17] & 0x60)	/* some form of stereo monitor mode; no support */
		return -1;

	/*
	 * Sync signal description.  I have no idea how to properly handle the 
	 * first three cases, which I think are aimed at things other than
	 * canonical SVGA monitors.
	 */
	switch((p[17] & 0x18)>>3) {
	case 0:	/* analog composite sync signal*/
	case 1:	/* bipolar analog composite sync signal */
		/* p[17] & 0x04 means serration: hsync during vsync */
		/* p[17] & 0x02 means sync pulse appears on RGB not just G */
		break;

	case 2:	/* digital composite sync signal */
		/* p[17] & 0x04 means serration: hsync during vsync */
		/* p[17] & 0x02 means hsync positive outside vsync */
		break;

	case 3:	/* digital separate sync signal; the norm */
		m->vsync = (p[17] & 0x04) ? '+' : '-';
		m->hsync = (p[17] & 0x02) ? '+' : '-';
		break;
	}
	/* p[17] & 0x01 is another stereo bit, only referenced if p[17] & 0x60 != 0 */

	rr = (m->frequency+m->ht*m->vt/2) / (m->ht*m->vt);

	snprint(m->name, sizeof m->name, "%dx%d@%dHz", m->x, m->y, rr);

	return 0;
}

extern Mode *vesamodes[];

int
vesalookup(Mode *m, char *name)
{
	Mode **p;

	for(p=vesamodes; *p; p++)
		if(strcmp((*p)->name, name) == 0) {
			*m = **p;
			return 0;
		}

	return -1;
}

static int
decodesti(Mode *m, uchar *p)
{
	int x, y, rr;
	char str[20];

	x = (p[0]+31)*8;
	switch((p[1]>>6) & 3){
	default:
	case 0:
		y = x;
		break;
	case 1:
		y = (x*4)/3;
		break;
	case 2:
		y = (x*5)/4;
		break;
	case 3:
		y = (x*16)/9;
		break;
	}
	rr = (p[1] & 0x1F) + 60;

	sprint(str, "%dx%d@%dHz", x, y, rr);
	return vesalookup(m, str);
}

int
parseedid128(Edid *e, void *v)
{
	static uchar magic[8] = { 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00 };
	uchar *p, *q, sum;
	int dpms, estab, i, m, vid;
	Mode mode;

	memset(e, 0, sizeof *e);

	p = (uchar*)v;
	if(memcmp(p, magic, 8) != 0) {
		werrstr("bad edid header");
		return -1;
	}

	sum = 0;
	for(i=0; i<128; i++) 
		sum += p[i];
	if(sum != 0) {
		werrstr("bad edid checksum");
		return -1;
	}
	p += 8;

	assert(p == (uchar*)v+8);	/* assertion offsets from pp. 12-13 of the standard */
	/*
	 * Manufacturer name is three 5-bit ascii letters, packed
	 * into a big endian [sic] short in big endian order.  The high bit is unused.
	 */
	i = (p[0]<<8) | p[1];
	p += 2;
	e->mfr[0] = 'A'-1 + ((i>>10) & 0x1F);
	e->mfr[1] = 'A'-1 + ((i>>5) & 0x1F);
	e->mfr[2] = 'A'-1 + (i & 0x1F);
	e->mfr[3] = '\0';

	/*
	 * Product code is a little endian short.
	 */
	e->product = (p[1]<<8) | p[0];
	p += 2;

	/*
	 * Serial number is a little endian long, 0x01010101 = unused.
	 */
	e->serial = (p[3]<<24) | (p[2]<<16) | (p[1]<<8) | p[0];
	p += 4;
	if(e->serial == 0x01010101)
		e->serial = 0;

	e->mfrweek = *p++;
	e->mfryear = 1990 + *p++;

	assert(p == (uchar*)v+8+10);
	/*
	 * Structure version is next two bytes: major.minor.
	 */
	e->version = *p++;
	e->revision = *p++;

	assert(p == (uchar*)v+8+10+2);
	/*
	 * Basic display parameters / features.
	 */
	/*
	 * Video input definition byte: 0x80 tells whether it is
	 * an analog or digital screen; we ignore the other bits.
	 * See p. 15 of the standard.
	 */
	vid = *p++;
	if(vid & 0x80)
		e->flags |= Fdigital;

	e->dxcm = *p++;
	e->dycm = *p++;
	e->gamma = 100 + *p++;
	dpms = *p++;
	if(dpms & 0x80)
		e->flags |= Fdpmsstandby;
	if(dpms & 0x40)
		e->flags |= Fdpmssuspend;
	if(dpms & 0x20)
		e->flags |= Fdpmsactiveoff;
	if((dpms & 0x18) == 0x00)
		e->flags |= Fmonochrome;
	if(dpms & 0x01)
		e->flags |= Fgtf;

	assert(p == (uchar*)v+8+10+2+5);
	/*
	 * Color characteristics currently ignored.
	 */
	p += 10;

	assert(p == (uchar*)v+8+10+2+5+10);
	/*
	 * Established timings: a bitmask of 19 preset timings.
	 */
	estab = (p[0]<<16) | (p[1]<<8) | p[2];
	p += 3;
	
	for(i=0, m=1<<23; i<nelem(estabtime); i++, m>>=1)
		if(estab & m)
			if(vesalookup(&mode, estabtime[i]) == 0)
				e->modelist = addmode(e->modelist,  mode);

	assert(p == (uchar*)v+8+10+2+5+10+3);
	/*
	 * Standard Timing Identifications: eight 2-byte selectors
	 * of more standard timings.
	 */
	for(i=0; i<8; i++, p+=2)
		if(decodesti(&mode, p+2*i) == 0)
			e->modelist = addmode(e->modelist, mode);

	assert(p == (uchar*)v+8+10+2+5+10+3+16);
	/*
	 * Detailed Timings
	 */
fprint(2, "dt\n");
	for(i=0; i<4; i++, p+=18) {
fprint(2, "%.8H\n", p);
		if(p[0] || p[1]) {	/* detailed timing block: p[0] or p[1] != 0 */
			if(decodedtb(&mode, p) == 0)
				e->modelist = addmode(e->modelist, mode);
		} else if(p[2]==0) {	/* monitor descriptor block */
			switch(p[3]) {
			case 0xFF:	/* monitor serial number (13-byte ascii, 0A terminated) */
				if(q = memchr(p+5, 0x0A, 13))
					*q = '\0';
				memset(e->serialstr, 0, sizeof(e->serialstr));
				strncpy(e->serialstr, (char*)p+5, 13);
				break;
			case 0xFE:	/* ascii string (13-byte ascii, 0A terminated) */
				break;
			case 0xFD:	/* monitor range limits */
				print("fd %.18H\n", p);
				e->rrmin = p[5];
				e->rrmax = p[6];
				e->hrmin = p[7]*1000;
				e->hrmax = p[8]*1000;
				if(p[9] != 0xFF)
					e->pclkmax = p[9]*10*1000000;
				break;
			case 0xFC:	/* monitor name (13-byte ascii, 0A terminated) */
				if(q = memchr(p+5, 0x0A, 13))
					*q = '\0';
				memset(e->name, 0, sizeof(e->name));
				strncpy(e->name, (char*)p+5, 13);
				break;
			case 0xFB:	/* extra color point data */
				break;
			case 0xFA:	/* extra standard timing identifications */
				for(i=0; i<6; i++)
					if(decodesti(&mode, p+5+2*i) == 0)
						e->modelist = addmode(e->modelist, mode);
				break;
			}
		}
	}

	assert(p == (uchar*)v+8+10+2+5+10+3+16+72);
	return 0;
}
