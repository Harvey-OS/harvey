#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"

#include	<libg.h>
#include	<gnot.h>
#include	"screen.h"
#include	"vga.h"

#define	MINX	8

struct{
	Point	pos;
	int	bwid;
}out;

/* imported */
extern	GSubfont defont0;
extern	Cursor arrow;
extern	GBitmap cursorback;

/* exported */
GSubfont *defont;
int islittle = 1;		/* little endian bit ordering in bytes */
GBitmap	gscreen;

/* local */
static	Lock vgalock;
static	GBitmap	vgascreen;
static	ulong colormap[256][3];
static	int cga = 1;		/* true if in cga mode */

/*
 *  screen dimensions
 */
#define MAXX	640
#define MAXY	480

/*
 *  'soft' screen bitmap
 */

typedef struct VGAmode	VGAmode;
struct VGAmode
{
	uchar	general[2];
	uchar	sequencer[5];
	uchar	crt[0x19];
	uchar	graphics[9];
	uchar	attribute[0x15];
};

/*
 *  640x480 display, 1, 2, or 4 bit color.
 */
VGAmode mode12 = 
{
	/* general */
	0xe7, 0x00,
	/* sequence */
	0x03, 0x01, 0x0f, 0x00, 0x06,
	/* crt */
	0x65, 0x4f, 0x50, 0x88, 0x55, 0x9a, 0x09, 0x3e,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xe8, 0x8b, 0xdf, 0x28, 0x00, 0xe7, 0x04, 0xe3,
	0xff,
	/* graphics */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x0f,
	0xff,
	/* attribute */
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
	0x01, 0x10, 0x0f, 0x00, 0x00,
};

/*
 *  320x200 display, 8 bit color.
 */
VGAmode mode13 = 
{
	/* general */
	0x63, 0x00,
	/* sequence */
	0x03, 0x01, 0x0f, 0x00, 0x0e,
	/* crt */
	0x5f, 0x4f, 0x50, 0x82, 0x54, 0x80, 0xbf, 0x1f,
	0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x28,
	0x9c, 0x8e, 0x8f, 0x28, 0x40, 0x96, 0xb9, 0xa3,
	0xff,
	/* graphics */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0f,
	0xff,
	/* attribute */
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
	0x41, 0x10, 0x0f, 0x00, 0x00,
};

static	Rectangle mbb;
static	Rectangle NULLMBB = {10000, 10000, -10000, -10000};
static	void nopage(int), tsengpage(int), tridentpage(int), parapage(int);
static	void vgaupdate(void);

/*
 *  definitions of known cards
 */
typedef struct Vgacard	Vgacard;
struct Vgacard
{
	char	*name;
	void	(*setpage)(int);
};

enum
{
	Ati,		/* ATI */
	Pvga1a,		/* paradise */
	Trident,	/* Trident 8900 */
	Tseng,		/* tseng labs te4000 */
	Generic,
};

Vgacard vgacards[] =
{
[Ati]		{ "ati", nopage, },
[Pvga1a]	{ "pvga1a", parapage, },
[Trident]	{ "trident", tridentpage, },
[Tseng]		{ "tseng", tsengpage, },
[Generic]	{ "generic", nopage, },
		{ 0, 0, },
};

Vgacard	*vgacard;	/* current vga card */

/*
 *  vga device directory
 */
enum
{
	Qdir=		0,
	Qvgasize=	1,
	Qvgatype=	2,
	Qvgaport=	3,
	Nvga=		4,
};
Dirtab vgadir[]={
	"vgasize",	{Qvgasize},	0,		0666,
	"vgatype",	{Qvgatype},	0,		0666,
	"vgaport",	{Qvgaport},	0,		0666,
};

void
vgareset(void)
{
	vgacard = &vgacards[Generic];
}

void
vgainit(void)
{
}

Chan*
vgaattach(char *upec)
{
	return devattach('v', upec);
}

Chan*
vgaclone(Chan *c, Chan *nc)
{
	return devclone(c, nc);
}

int
vgawalk(Chan *c, char *name)
{
	return devwalk(c, name, vgadir, Nvga, devgen);
}

void
vgastat(Chan *c, char *dp)
{
	devstat(c, dp, vgadir, Nvga, devgen);
}

Chan*
vgaopen(Chan *c, int omode)
{
	return devopen(c, omode, vgadir, Nvga, devgen);
}

void
vgacreate(Chan *c, char *name, int omode, ulong perm)
{
	USED(c, name, omode, perm);
	error(Eperm);
}

void
vgaclose(Chan *c)
{
	USED(c);
}

long
vgaread(Chan *c, void *buf, long n, ulong offset)
{
	int port;
	uchar *cp = buf;
	char cbuf[64];

	switch(c->qid.path&~CHDIR){
	case Qdir:
		return devdirread(c, buf, n, vgadir, Nvga, devgen);
	case Qvgasize:
		sprint(cbuf, "%dx%dx%d", gscreen.r.max.x, gscreen.r.max.y,
			1<<gscreen.ldepth);
		return readstr(offset, buf, n, cbuf);
	case Qvgatype:
		return readstr(offset, buf, n, vgacard->name);
	case Qvgaport:
		/* at the moment, I need freedom for the ATI card - ches
		if (offset + n >= 0x8000 || offset < 0x300)
			error(Ebadarg);
		*/
		for (port=offset; port<offset+n; port++)
			*cp++ = inb(port);
		return n;
	}
	error(Eperm);
	return 0;
}

long
vgawrite(Chan *c, void *buf, long n, ulong offset)
{
	char cbuf[64], *cp;
	Vgacard *vp;
	int port, maxx, maxy, ldepth;

	switch(c->qid.path&~CHDIR){
	case Qdir:
		error(Eperm);
	case Qvgatype:
		if(offset != 0 || n >= sizeof(cbuf) || n < 1)
			error(Ebadarg);
		memmove(cbuf, buf, n);
		cbuf[n] = 0;
		if(cp = strchr(cbuf, '\n'))
			*cp = 0;
		for(vp = vgacards; vp->name; vp++)
			if(strcmp(cbuf, vp->name) == 0){
				vgacard = vp;
				return n;
			}
		error(Ebadarg);
	case Qvgasize:
		if(offset != 0 || n >= sizeof(cbuf))
			error(Ebadarg);
		memmove(cbuf, buf, n);
		cbuf[n] = 0;
		cp = cbuf;
		maxx = strtoul(cp, &cp, 0);
		if(*cp!=0)
			cp++;
		maxy = strtoul(cp, &cp, 0);
		if(*cp!=0)
			cp++;
		switch(strtoul(cp, &cp, 0)){
		case 1:
			ldepth = 0;
			break;
		case 2:
			ldepth = 1;
			break;
		case 4:
			ldepth = 2;
			break;
		case 8:
			ldepth = 3;
			break;
		default:
			ldepth = -1;
		}
		if(maxx == 0 || maxy == 0
		|| maxx > 1280 || maxy > 1024
		|| ldepth > 3 || ldepth < 0)
			error(Ebadarg);
		cursoroff(1);
		setscreen(maxx, maxy, ldepth);
		cursoron(1);
		return n;
	case Qvgaport:
		cp = buf;
		/*
		if (offset + n >= 0x8000 || offset < 0x300)
			error(Ebadarg);
		*/
		for (port=offset; port<offset+n; port++) {
			outb(port, *cp++);
		}
		return n;
	}
	error(Eperm);
	return 0;
}

void
vgaremove(Chan *c)
{
	USED(c);
	error(Eperm);
}

void
vgawstat(Chan *c, char *dp)
{
	USED(c, dp);
	error(Eperm);
}

void
genout(int reg, int val)
{
	if(reg == 0)
		outb(EMISCW, val);
	else if (reg == 1)
		outb(EFCW, val);
}
void
srout(int reg, int val)
{
	outb(SRX, reg);
	outb(SR, val);
}
void
grout(int reg, int val)
{
	outb(GRX, reg);
	outb(GR, val);
}
void
arout(int reg, int val)
{
	inb(0x3DA);
	if (reg <= 0xf) {
		outb(ARW, reg | 0x0);
		outb(ARW, val);
		inb(0x3DA);
		outb(ARW, reg | 0x20);
	} else {
		outb(ARW, reg | 0x20);
		outb(ARW, val);
	}
}

void
crout(int reg, int val)
{
	outb(CRX, reg);
	outb(CR, val);
}

void
setmode(VGAmode *v)
{
	int i;

	/* turn screen off (to avoid damage) */
	srout(1, 0x21);

	for(i = 0; i < sizeof(v->general); i++)
		genout(i, v->general[i]);

	for(i = 0; i < sizeof(v->sequencer); i++)
		if(i == 1)
			srout(i, v->sequencer[i]|0x20);		/* avoid enabling screen */
		else
			srout(i, v->sequencer[i]);

	crout(Cvre, 0);	/* allow writes to CRT registers 0-7 */
	for(i = 0; i < sizeof(v->crt); i++)
		crout(i, v->crt[i]);

	for(i = 0; i < sizeof(v->graphics); i++)
		grout(i, v->graphics[i]);

	for(i = 0; i < sizeof(v->attribute); i++)
		arout(i, v->attribute[i]);

	/* turn screen on */
	srout(1, v->sequencer[1]);
}

void
getmode(VGAmode *v) {
	int i;
	v->general[0] = inb(0x3cc);
	v->general[1] = inb(0x3ca);
	for(i = 0; i < sizeof(v->sequencer); i++) {
		outb(SRX, i);
		v->sequencer[i] = inb(SR);
	}
	for(i = 0; i < sizeof(v->crt); i++) {
		outb(CRX, i);
		v->crt[i] = inb(CR);
	}
	for(i = 0; i < sizeof(v->graphics); i++) {
		outb(GRX, i);
		v->graphics[i] = inb(GR);
	}
	for(i = 0; i < sizeof(v->attribute); i++) {
		inb(0x3DA);
		outb(ARW, i | 0x20);
		v->attribute[i] = inb(ARR);
	}
}

void
printmode(VGAmode *v) {
	int i;
	print("g %2.2x %2x\n",
		v->general[0], v->general[1]);

	print("s ");
	for(i = 0; i < sizeof(v->sequencer); i++) {
		print(" %2.2x", v->sequencer[i]);
	}

	print("\nc ");
	for(i = 0; i < sizeof(v->crt); i++) {
		print(" %2.2x", v->crt[i]);
	}

	print("\ng ");
	for(i = 0; i < sizeof(v->graphics); i++) {
		print(" %2.2x", v->graphics[i]);
	}

	print("\na ");
	for(i = 0; i < sizeof(v->attribute); i++) {
		print(" %2.2x", v->attribute[i]);
	}
	print("\n");
}

/*
 *  expand 3 and 6 bits of color to 32
 */
static ulong
x3to32(uchar x)
{
	ulong y;

	x = x&7;
	x= (x<<3)|x;
	y = (x<<(32-6))|(x<<(32-12))|(x<<(32-18))|(x<<(32-24))|(x<<(32-30));
	return y;
}
static ulong
x6to32(uchar x)
{
	ulong y;

	x = x&0x3f;
	y = (x<<(32-6))|(x<<(32-12))|(x<<(32-18))|(x<<(32-24))|(x<<(32-30));
	return y;
}

void
setscreen(int maxx, int maxy, int ldepth)
{
	int len, vgamaxy, width, i, x, s;
	ulong *p, *oldp;

	if(ldepth == 3)
		setmode(&mode13);
	else
		setmode(&mode12);

	/* allocate a new soft bitmap area */
	width = (maxx*(1<<ldepth))/32;
	len = width * BY2WD * maxy;
	p = xalloc(len);
	if(p == 0)
		error(Enobitstore);
	mbb = NULLMBB;

	/*
	 *  zero hard screen and setup a bitmap for the new size
	 */
	if(ldepth == 3)
		vgascreen.ldepth = 3;
	else
		vgascreen.ldepth = 0;
	vgascreen.width = (maxx*(1<<vgascreen.ldepth))/32;
	if(maxy > (64*1024)/(vgascreen.width*BY2WD))
		vgamaxy = (64*1024)/(vgascreen.width*BY2WD);
	else
		vgamaxy = maxy;
	vgascreen.base = (void*)SCREENMEM;
	vgascreen.r.min = Pt(0, 0);
	vgascreen.r.max = Pt(maxx, vgamaxy);
	vgascreen.clipr = vgascreen.r;

	/*
	 *  setup new soft screen, free memory for old screen
	 */
	oldp = gscreen.base;
	s = splhi();
	gscreen.ldepth = ldepth;
	gscreen.width = width;
	gscreen.r.min = Pt(0, 0);
	gscreen.r.max = Pt(maxx, maxy);
	gscreen.clipr = gscreen.r;
	gscreen.base = p;
	splx(s);
	if(oldp)
		xfree(oldp);

	/*
	 *  set depth of cursor backup area
	 */
	bitdepth();

	/*
	 *  set string pointer to upper left
	 */
	out.pos.x = MINX;
	out.pos.y = 0;
	out.bwid = defont0.info[' '].width;

	/*
	 *  default color map
	 */
	switch(ldepth){
	case 3:
		for(i = 0; i < 256; i++)
			setcolor(i, x3to32(i>>5), x3to32(i>>2), x3to32(i<<1));
		break;
	case 2:
	case 1:
	case 0:
		gscreen.ldepth = 3;
		for(i = 0; i < 16; i++){
			x = x6to32((i*63)/15);
			setcolor(i, x, x, x);
		}
		gscreen.ldepth = ldepth;
		break;
	}
	cga = 0;

	/*
	 * clear screen
	 */
	mbb = gscreen.r;
	vgaupdate();
}

void
screeninit(void)
{
	int i;
	ulong *l;

	/*
	 *  arrow is defined as a big endian
	 */
	pixreverse(arrow.set, 2*16, 0);
	pixreverse(arrow.clr, 2*16, 0);

	/*
	 *  swizzle the font longs.  we do both byte and bit swizzling
	 *  since the font is initialized with big endian longs.
	 */
	defont = &defont0;
	l = defont->bits->base;
	for(i = defont->bits->width*Dy(defont->bits->r); i > 0; i--, l++)
		*l = (*l<<24) | ((*l>>8)&0x0000ff00) | ((*l<<8)&0x00ff0000) | (*l>>24);
	pixreverse((uchar*)defont->bits->base,
		defont->bits->width*BY2WD*Dy(defont->bits->r), 0);

	cga = 1;
	crout(0x0a, 0xff);		/* turn off cursor */
	memset(CGASCREEN, 0, CGAWIDTH*CGAHEIGHT);
}

/*
 *  collect changes to the 'soft' screen
 */
void
mbbrect(Rectangle r)
{
	if (r.min.x < mbb.min.x)
		mbb.min.x = r.min.x;
	if (r.min.y < mbb.min.y)
		mbb.min.y = r.min.y;
	if (r.max.x > mbb.max.x)
		mbb.max.x = r.max.x;
	if (r.max.y > mbb.max.y)
		mbb.max.y = r.max.y;
	mousescreenupdate();
}

void
mbbpt(Point p)
{
	if (p.x < mbb.min.x)
		mbb.min.x = p.x;
	if (p.y < mbb.min.y)
		mbb.min.y = p.y;
	if (p.x >= mbb.max.x)
		mbb.max.x = p.x+1;
	if (p.y >= mbb.max.y)
		mbb.max.y = p.y+1;
}

/*
 *  paging routines for different cards
 */
static void
nopage(int page)
{
	USED(page);
}
static void
tridentpage(int page)
{
	srout(0x0e, page^0x02);	/* yes, the page bit needs to be toggled! */
}
static void
tsengpage(int page)
{
	outb(0x3cd, (page<<4)|page);
}
static void
parapage(int page)
{
	grout(9, page<<4);
}

/*
 *  copy litte endian soft screen to big endian hard screen
 */
static void
vgaupdate(void)
{
	uchar *sp, *hp, *edisp;
	int y, len, incs, inch, off, page;
	Rectangle r;
	void* (*f)(void*, void*, long);

	r = mbb;
	mbb = NULLMBB;

	if(Dy(r) < 0)
		return;

	if(r.min.x < 0)
		r.min.x = 0;
	if(r.min.y < 0)
		r.min.y = 0;
	if(r.max.x > gscreen.r.max.x)
		r.max.x = gscreen.r.max.x;
	if(r.max.y > gscreen.r.max.y)
		r.max.y = gscreen.r.max.y;

	incs = gscreen.width * BY2WD;
	inch = vgascreen.width * BY2WD;

	switch(gscreen.ldepth){
	case 0:
		r.min.x &= ~15;		/* 16 pixel allignment for l0update() */
		f = l0update;
		len = (r.max.x + 7)/8 - r.min.x/8;
		if(len & 1)
			len++;		/* 16 bit allignment for l0update() */
		break;
	case 1:
		r.min.x &= ~15;		/* 8 pixel allignment for l1update() */
		f = l1update;
		len = (r.max.x + 7)/8 - r.min.x/8;
		if(len & 1)
			len++;		/* 8 pixel allignment for l1update() */
		break;
	case 2:
		r.min.x &= ~7;		/* 8 pixel allignment for l2update() */
		f = l2update;
		len = (r.max.x + 7)/8 - r.min.x/8;
		break;
	case 3:
		f = memmove;
		len = r.max.x - r.min.x;
		break;
	default:
		return;
	}
	if(len < 1)
		return;

	off = r.min.y * vgascreen.width * BY2WD + (r.min.x>>(3 - vgascreen.ldepth));
	page = off>>16;
	off &= (1<<16)-1;
	hp = ((uchar*)vgascreen.base) + off;
	off = r.min.y * gscreen.width * BY2WD
		+ (r.min.x>>(3 - gscreen.ldepth));
	sp = ((uchar*)gscreen.base) + off;

	edisp = ((uchar*)vgascreen.base) + 64*1024;
	vgacard->setpage(page);
	for(y = r.min.y; y < r.max.y; y++){
		if(hp + inch < edisp){
			f(hp, sp, len);
			sp += incs;
			hp += inch;
		} else {
			off = edisp - hp;
			if(off <= len){
				if(off > 0)
					f(hp, sp, off);
				vgacard->setpage(++page);
				if(len - off > 0)
					f(vgascreen.base, sp+off, len - off);
			} else {
				f(hp, sp, len);
				vgacard->setpage(++page);
			}
			sp += incs;
			hp += inch - 64*1024;
		}
	}
}

void
screenupdate(void)
{
	lock(&vgalock);
	vgaupdate();
	unlock(&vgalock);
}

void
mousescreenupdate(void)
{
	if(canlock(&vgalock)){
		vgaupdate();
		unlock(&vgalock);
	}
}

static void
cgascreenputc(int c)
{
	int i;
	static int color;
	static int pos;

	if(c == '\n'){
		pos = pos/CGAWIDTH;
		pos = (pos+1)*CGAWIDTH;
	} else if(c == '\t'){
		i = 8 - ((pos/2)&7);
		while(i-->0)
			cgascreenputc(' ');
	} else if(c == '\b'){
		if(pos >= 2)
			pos -= 2;
		cgascreenputc(' ');
		pos -= 2;
	} else {
		CGASCREEN[pos++] = c;
		CGASCREEN[pos++] = 2;	/* green on black */
	}
	if(pos >= CGAWIDTH*CGAHEIGHT){
		memmove(CGASCREEN, &CGASCREEN[CGAWIDTH], CGAWIDTH*(CGAHEIGHT-1));
		memset(&CGASCREEN[CGAWIDTH*(CGAHEIGHT-1)], 0, CGAWIDTH);
		pos = CGAWIDTH*(CGAHEIGHT-1);
	}
}

void
cgascreenputs(char *s, int n)
{
	while(n-- > 0)
		cgascreenputc(*s++);
}

void
screenputnl(void)
{
	Rectangle r;

	out.pos.x = MINX;
	out.pos.y += defont0.height;
	if(out.pos.y > gscreen.r.max.y-defont0.height){
		vgaupdate();
		out.pos.y = gscreen.r.min.y;
	}
	r = Rect(0, out.pos.y, gscreen.r.max.x, out.pos.y+2*defont0.height);
	gbitblt(&gscreen, r.min, &gscreen, r, flipD[0]);
	mbbrect(r);
	vgaupdate();
}

void
screenputs(char *s, int n)
{
	Rune r;
	int i;
	char buf[4];
	Rectangle rect;

	if(cga) {
		cgascreenputs(s, n);
		return;
	}

	while(n > 0){
		i = chartorune(&r, s);
		if(i == 0){
			s++;
			--n;
			continue;
		}
		memmove(buf, s, i);
		buf[i] = 0;
		n -= i;
		s += i;
		if(r == '\n')
			screenputnl();
		else if(r == '\t'){
			out.pos.x += (8-((out.pos.x-MINX)/out.bwid&7))*out.bwid;
			if(out.pos.x >= gscreen.r.max.x)
				screenputnl();
		}else if(r == '\b'){
			if(out.pos.x >= out.bwid+MINX){
				out.pos.x -= out.bwid;
				gsubfstring(&gscreen, out.pos, defont, " ", flipD[S]);
			}
			rect.min = Pt(out.pos.x, out.pos.y);
			rect.max = Pt(out.pos.x+out.bwid, out.pos.y+defont0.height);
			mbbrect(rect);
		}else{
			if(out.pos.x >= gscreen.r.max.x-out.bwid)
				screenputnl();
			rect.min = Pt(out.pos.x, out.pos.y);
			rect.max = Pt(out.pos.x+out.bwid, out.pos.y+defont0.height);
			out.pos = gsubfstring(&gscreen, out.pos, defont, buf, flipD[S]);
			mbbrect(rect);
		}
	}
	vgaupdate();
}

int
screenbits(void)
{
	return 1<<gscreen.ldepth;	/* bits per pixel */
}

void
getcolor(ulong p, ulong *pr, ulong *pg, ulong *pb)
{
	p &= (1<<(1<<gscreen.ldepth))-1;
	lock(&vgalock);
	*pr = colormap[p][0];
	*pg = colormap[p][1];
	*pb = colormap[p][2];
	unlock(&vgalock);
}

int
setcolor(ulong p, ulong r, ulong g, ulong b)
{
	p &= (1<<(1<<gscreen.ldepth))-1;
	lock(&vgalock);
	colormap[p][0] = r;
	colormap[p][1] = g;
	colormap[p][2] = b;
	outb(CMWX, p);
	outb(CM, r>>(32-6));
	outb(CM, g>>(32-6));
	outb(CM, b>>(32-6));
	unlock(&vgalock);
	return ~0;
}

int
hwcursset(uchar *s, uchar *c, int ox, int oy)
{
	USED(s, c, ox, oy);
	return 0;
}

int
hwcursmove(int x, int y)
{
	USED(x, y);
	return 0;
}

void
mouseclock(void)
{
	spllo();	/* so we don't cause lost chars on the uart */
	mouseupdate(1);
}

/*
 *  a fatter than usual cursor for the safari
 */
Cursor fatarrow = {
	{ -1, -1 },
	{
		0xff, 0xff, 0x80, 0x01, 0x80, 0x02, 0x80, 0x0c, 
		0x80, 0x10, 0x80, 0x10, 0x80, 0x08, 0x80, 0x04, 
		0x80, 0x02, 0x80, 0x01, 0x80, 0x02, 0x8c, 0x04, 
		0x92, 0x08, 0x91, 0x10, 0xa0, 0xa0, 0xc0, 0x40, 
	},
	{
		0x00, 0x00, 0x7f, 0xfe, 0x7f, 0xfc, 0x7f, 0xf0, 
		0x7f, 0xe0, 0x7f, 0xe0, 0x7f, 0xf0, 0x7f, 0xf8, 
		0x7f, 0xfc, 0x7f, 0xfe, 0x7f, 0xfc, 0x73, 0xf8, 
		0x61, 0xf0, 0x60, 0xe0, 0x40, 0x40, 0x00, 0x00, 
	},
};

void
bigcursor(void)
{
	memmove(&arrow, &fatarrow, sizeof(fatarrow));
	pixreverse(arrow.set, 2*16, 0);
	pixreverse(arrow.clr, 2*16, 0);
}

/*
 *  Table for separating and reversing bits in a ldepth 1 bitmap.
 *  This aids in coverting a little endian ldepth 1 bitmap into the
 *  2 big-endian ldepth 0 bitmaps used for the VGA bit planes.
 *
 *	if the bits in uchar x are labeled
 *		76543210
 *	then l1revsep[x] yields a ushort with bits
 *		________1357________0246
 *	where _ represents a bit whose value is 0.
 *
 *  This table is used by l1update() in l.s.  l1update is implemented
 *  in assembler for speed.
 *
 */
ulong l1revsep[] = {
 0x00000, 0x00008, 0x80000, 0x80008, 0x00004, 0x0000c, 0x80004, 0x8000c,
 0x40000, 0x40008, 0xc0000, 0xc0008, 0x40004, 0x4000c, 0xc0004, 0xc000c,
 0x00002, 0x0000a, 0x80002, 0x8000a, 0x00006, 0x0000e, 0x80006, 0x8000e,
 0x40002, 0x4000a, 0xc0002, 0xc000a, 0x40006, 0x4000e, 0xc0006, 0xc000e,
 0x20000, 0x20008, 0xa0000, 0xa0008, 0x20004, 0x2000c, 0xa0004, 0xa000c,
 0x60000, 0x60008, 0xe0000, 0xe0008, 0x60004, 0x6000c, 0xe0004, 0xe000c,
 0x20002, 0x2000a, 0xa0002, 0xa000a, 0x20006, 0x2000e, 0xa0006, 0xa000e,
 0x60002, 0x6000a, 0xe0002, 0xe000a, 0x60006, 0x6000e, 0xe0006, 0xe000e,
 0x00001, 0x00009, 0x80001, 0x80009, 0x00005, 0x0000d, 0x80005, 0x8000d,
 0x40001, 0x40009, 0xc0001, 0xc0009, 0x40005, 0x4000d, 0xc0005, 0xc000d,
 0x00003, 0x0000b, 0x80003, 0x8000b, 0x00007, 0x0000f, 0x80007, 0x8000f,
 0x40003, 0x4000b, 0xc0003, 0xc000b, 0x40007, 0x4000f, 0xc0007, 0xc000f,
 0x20001, 0x20009, 0xa0001, 0xa0009, 0x20005, 0x2000d, 0xa0005, 0xa000d,
 0x60001, 0x60009, 0xe0001, 0xe0009, 0x60005, 0x6000d, 0xe0005, 0xe000d,
 0x20003, 0x2000b, 0xa0003, 0xa000b, 0x20007, 0x2000f, 0xa0007, 0xa000f,
 0x60003, 0x6000b, 0xe0003, 0xe000b, 0x60007, 0x6000f, 0xe0007, 0xe000f,
 0x10000, 0x10008, 0x90000, 0x90008, 0x10004, 0x1000c, 0x90004, 0x9000c,
 0x50000, 0x50008, 0xd0000, 0xd0008, 0x50004, 0x5000c, 0xd0004, 0xd000c,
 0x10002, 0x1000a, 0x90002, 0x9000a, 0x10006, 0x1000e, 0x90006, 0x9000e,
 0x50002, 0x5000a, 0xd0002, 0xd000a, 0x50006, 0x5000e, 0xd0006, 0xd000e,
 0x30000, 0x30008, 0xb0000, 0xb0008, 0x30004, 0x3000c, 0xb0004, 0xb000c,
 0x70000, 0x70008, 0xf0000, 0xf0008, 0x70004, 0x7000c, 0xf0004, 0xf000c,
 0x30002, 0x3000a, 0xb0002, 0xb000a, 0x30006, 0x3000e, 0xb0006, 0xb000e,
 0x70002, 0x7000a, 0xf0002, 0xf000a, 0x70006, 0x7000e, 0xf0006, 0xf000e,
 0x10001, 0x10009, 0x90001, 0x90009, 0x10005, 0x1000d, 0x90005, 0x9000d,
 0x50001, 0x50009, 0xd0001, 0xd0009, 0x50005, 0x5000d, 0xd0005, 0xd000d,
 0x10003, 0x1000b, 0x90003, 0x9000b, 0x10007, 0x1000f, 0x90007, 0x9000f,
 0x50003, 0x5000b, 0xd0003, 0xd000b, 0x50007, 0x5000f, 0xd0007, 0xd000f,
 0x30001, 0x30009, 0xb0001, 0xb0009, 0x30005, 0x3000d, 0xb0005, 0xb000d,
 0x70001, 0x70009, 0xf0001, 0xf0009, 0x70005, 0x7000d, 0xf0005, 0xf000d,
 0x30003, 0x3000b, 0xb0003, 0xb000b, 0x30007, 0x3000f, 0xb0007, 0xb000f,
 0x70003, 0x7000b, 0xf0003, 0xf000b, 0x70007, 0x7000f, 0xf0007, 0xf000f,
};

/*
 *  Table for separating and reversing bits in a ldepth 2 bitmap.
 *  This aids in coverting a little endian ldepth 1 bitmap into the
 *  4 big-endian ldepth 0 bitmaps used for the VGA bit planes.
 *
 *	if the bits in uchar x are labeled
 *		76543210
 *	then l1revsep[x] yields a ushort with bits
 *		______37______26______15______04
 *	where _ represents a bit whose value is 0.
 *
 *  This table is used by l2update() in l.s.  l2update is implemented
 *  in assembler for speed.
 *
 */
ulong l2revsep[] = {
 0x0000000, 0x0000002, 0x0000200, 0x0000202, 0x0020000, 0x0020002, 0x0020200, 0x0020202,
 0x2000000, 0x2000002, 0x2000200, 0x2000202, 0x2020000, 0x2020002, 0x2020200, 0x2020202,
 0x0000001, 0x0000003, 0x0000201, 0x0000203, 0x0020001, 0x0020003, 0x0020201, 0x0020203,
 0x2000001, 0x2000003, 0x2000201, 0x2000203, 0x2020001, 0x2020003, 0x2020201, 0x2020203,
 0x0000100, 0x0000102, 0x0000300, 0x0000302, 0x0020100, 0x0020102, 0x0020300, 0x0020302,
 0x2000100, 0x2000102, 0x2000300, 0x2000302, 0x2020100, 0x2020102, 0x2020300, 0x2020302,
 0x0000101, 0x0000103, 0x0000301, 0x0000303, 0x0020101, 0x0020103, 0x0020301, 0x0020303,
 0x2000101, 0x2000103, 0x2000301, 0x2000303, 0x2020101, 0x2020103, 0x2020301, 0x2020303,
 0x0010000, 0x0010002, 0x0010200, 0x0010202, 0x0030000, 0x0030002, 0x0030200, 0x0030202,
 0x2010000, 0x2010002, 0x2010200, 0x2010202, 0x2030000, 0x2030002, 0x2030200, 0x2030202,
 0x0010001, 0x0010003, 0x0010201, 0x0010203, 0x0030001, 0x0030003, 0x0030201, 0x0030203,
 0x2010001, 0x2010003, 0x2010201, 0x2010203, 0x2030001, 0x2030003, 0x2030201, 0x2030203,
 0x0010100, 0x0010102, 0x0010300, 0x0010302, 0x0030100, 0x0030102, 0x0030300, 0x0030302,
 0x2010100, 0x2010102, 0x2010300, 0x2010302, 0x2030100, 0x2030102, 0x2030300, 0x2030302,
 0x0010101, 0x0010103, 0x0010301, 0x0010303, 0x0030101, 0x0030103, 0x0030301, 0x0030303,
 0x2010101, 0x2010103, 0x2010301, 0x2010303, 0x2030101, 0x2030103, 0x2030301, 0x2030303,
 0x1000000, 0x1000002, 0x1000200, 0x1000202, 0x1020000, 0x1020002, 0x1020200, 0x1020202,
 0x3000000, 0x3000002, 0x3000200, 0x3000202, 0x3020000, 0x3020002, 0x3020200, 0x3020202,
 0x1000001, 0x1000003, 0x1000201, 0x1000203, 0x1020001, 0x1020003, 0x1020201, 0x1020203,
 0x3000001, 0x3000003, 0x3000201, 0x3000203, 0x3020001, 0x3020003, 0x3020201, 0x3020203,
 0x1000100, 0x1000102, 0x1000300, 0x1000302, 0x1020100, 0x1020102, 0x1020300, 0x1020302,
 0x3000100, 0x3000102, 0x3000300, 0x3000302, 0x3020100, 0x3020102, 0x3020300, 0x3020302,
 0x1000101, 0x1000103, 0x1000301, 0x1000303, 0x1020101, 0x1020103, 0x1020301, 0x1020303,
 0x3000101, 0x3000103, 0x3000301, 0x3000303, 0x3020101, 0x3020103, 0x3020301, 0x3020303,
 0x1010000, 0x1010002, 0x1010200, 0x1010202, 0x1030000, 0x1030002, 0x1030200, 0x1030202,
 0x3010000, 0x3010002, 0x3010200, 0x3010202, 0x3030000, 0x3030002, 0x3030200, 0x3030202,
 0x1010001, 0x1010003, 0x1010201, 0x1010203, 0x1030001, 0x1030003, 0x1030201, 0x1030203,
 0x3010001, 0x3010003, 0x3010201, 0x3010203, 0x3030001, 0x3030003, 0x3030201, 0x3030203,
 0x1010100, 0x1010102, 0x1010300, 0x1010302, 0x1030100, 0x1030102, 0x1030300, 0x1030302,
 0x3010100, 0x3010102, 0x3010300, 0x3010302, 0x3030100, 0x3030102, 0x3030300, 0x3030302,
 0x1010101, 0x1010103, 0x1010301, 0x1010303, 0x1030101, 0x1030103, 0x1030301, 0x1030303,
 0x3010101, 0x3010103, 0x3010301, 0x3010303, 0x3030101, 0x3030103, 0x3030301, 0x3030303,
};
