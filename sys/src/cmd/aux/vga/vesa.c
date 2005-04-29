#include <u.h>
#include <libc.h>
#include <bio.h>
#include </386/include/ureg.h>
typedef struct Ureg Ureg;

#include "pci.h"
#include "vga.h"

typedef struct Vbe Vbe;
typedef struct Vmode Vmode;

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


#define WORD(p) ((p)[0] | ((p)[1]<<8))
#define LONG(p) ((p)[0] | ((p)[1]<<8) | ((p)[2]<<16) | ((p)[3]<<24))
#define PWORD(p, v) (p)[0] = (v); (p)[1] = (v)>>8
#define PLONG(p, v) (p)[0] = (v); (p)[1] = (v)>>8; (p)[2] = (v)>>16; (p)[3] = (v)>>24

static Vbe *vbe;

Vbe *mkvbe(void);
int vbecheck(Vbe*);
uchar *vbemodes(Vbe*);
int vbemodeinfo(Vbe*, int, Vmode*);
int vbegetmode(Vbe*);
int vbesetmode(Vbe*, int);
void vbeprintinfo(Vbe*);
void vbeprintmodeinfo(Vbe*, int);
int vbesnarf(Vbe*, Vga*);

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
		error("no vesa bios");
	if(vbesetmode(vbe, atoi(dbattr(vga->mode->attr, "id"))) < 0){
		ctlr->flag |= Ferror;
		fprint(2, "vbesetmode: %r\n");
	}
}

static void
dump(Vga*, Ctlr*)
{
	uchar *p, *ep;

	if(!vbe){
		Bprint(&stdout, "no vesa bios\n");
		return;
	}

	vbeprintinfo(vbe);
	p = vbemodes(vbe);
	if(p){
		for(ep=p+1024; (p[0]!=0xFF || p[1]!=0xFF) && p<ep; p+=2)
			vbeprintmodeinfo(vbe, WORD(p));
	}
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
	uchar *p;
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
	if(m->depth <= 8)
		snprint(m->chan, sizeof m->chan, "m%d", m->depth);
	else if(m->xo)
		snprint(m->chan, sizeof m->chan, "x%dr%dg%db%d", m->x, m->r, m->g, m->b);
	else
		snprint(m->chan, sizeof m->chan, "r%dg%db%d", m->r, m->g, m->b);
	return 0;
}

void
vbeprintmodeinfo(Vbe *vbe, int id)
{
	Vmode m;

	if(vbemodeinfo(vbe, id, &m) < 0)
		return;
	printitem("vesa", "mode");
	Bprint(&stdout, "0x%ux %s %s %s\n",
		m.id, m.name, m.chan, m.model);
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

