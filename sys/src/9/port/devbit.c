#include	"u.h"
#include	"../port/lib.h"
#include	<libg.h>
#include	<gnot.h>
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

#include	"devtab.h"

#include	"screen.h"

/*
 * Some monochrome screens are reversed from what we like:
 * We want 0's bright and 1's dark.
 * Indexed by an Fcode, these compensate for the source bitmap being wrong
 * (exchange S rows) and destination (exchange D columns and invert result)
 */
int flipS[] = {
	0x0, 0x4, 0x8, 0xC, 0x1, 0x5, 0x9, 0xD,
	0x2, 0x6, 0xA, 0xE, 0x3, 0x7, 0xB, 0xF
};

int flipD[] = {
	0xF, 0xD, 0xE, 0xC, 0x7, 0x5, 0x6, 0x4,
	0xB, 0x9, 0xA, 0x8, 0x3, 0x1, 0x2, 0x0, 
};

int flipping;	/* are flip tables being used to transform Fcodes? */

/*
 * Device (#b/bitblt) is exclusive use on open, so no locks are necessary
 * for i/o
 */

/*
 * Arena is a word containing N, followed by a pointer to the Arena,
 * followed by a pointer to the Bitmap, followed by N words.
 * The bitmap pointer is zero if block is free.
 * bit.map is an array of pointers to GBitmaps.  The GBitmaps are
 * freed individually and their corresponding entries in bit.map are zeroed.
 * The index into bit.map is the Bitmap id as seen in libg.  Subfonts and
 * fonts are handled similarly.
 */

typedef struct	Arena	Arena;
struct Arena
{
	ulong	*words;		/* storage */
	ulong	*wfree;		/* pointer to next free word */
	ulong	nwords;		/* total in arena */
	int	nbusy;		/* number of busy blocks */
};

typedef struct	BSubfont BSubfont;
struct BSubfont
{
	GSubfont;
	int	ref;		/* number of times this subfont is open */
	ulong	qid[2];		/* unique id used as a cache tag */
};

extern GSubfont	*defont;
       BSubfont	*bdefont;
       BSubfont bdefont0;


struct
{
	Ref;
	QLock;
	GBitmap	**map;		/* indexed array */
	int	nmap;		/* number allocated */
	GFont	**font;		/* indexed array */
	int	nfont;		/* number allocated */
	BSubfont**subfont;	/* indexed array */
	int	nsubfont;	/* number allocated */
	Arena	*arena;		/* array */
	int	narena;		/* number allocated */
	int	mouseopen;	/* flag: mouse open */
	int	bitbltopen;	/* flag: bitblt open */
	int	bid;		/* last allocated bitmap id */
	int	subfid;		/* last allocated subfont id */
	int	cacheid;	/* last cached subfont id */
	int	fid;		/* last allocated font id */
	int	init;		/* freshly opened; init message pending */
	int	rid;		/* read bitmap id */
	int	rminy;		/* read miny */
	int	rmaxy;		/* read maxy */
	int	mid;		/* colormap read bitmap id */
}bit;

#define	DMAP	16		/* delta increase in size of arrays */
#define	FREE	0x80000000

void	bitcompact(void);
int	bitalloc(Rectangle, int);
void	bitfree(GBitmap*);
void	fontfree(GFont*);
void	subfontfree(BSubfont*, int);
void	arenafree(Arena*);
void	bitstring(GBitmap*, Point, GFont*, uchar*, long, Fcode);
void	bitloadchar(GFont*, int, GSubfont*, int);
extern	GBitmap	gscreen;

Mouseinfo	mouse;
Cursorinfo	cursor;
int		mouseshifted;
int		mousetype;
int		islittle;

Cursor	arrow =
{
	{-1, -1},
	{0xFF, 0xE0, 0xFF, 0xE0, 0xFF, 0xC0, 0xFF, 0x00,
	 0xFF, 0x00, 0xFF, 0x80, 0xFF, 0xC0, 0xFF, 0xE0,
	 0xE7, 0xF0, 0xE3, 0xF8, 0xC1, 0xFC, 0x00, 0xFE,
	 0x00, 0x7F, 0x00, 0x3E, 0x00, 0x1C, 0x00, 0x08,
	},
	{0x00, 0x00, 0x7F, 0xC0, 0x7F, 0x00, 0x7C, 0x00,
	 0x7E, 0x00, 0x7F, 0x00, 0x6F, 0x80, 0x67, 0xC0,
	 0x43, 0xE0, 0x41, 0xF0, 0x00, 0xF8, 0x00, 0x7C,
	 0x00, 0x3E, 0x00, 0x1C, 0x00, 0x08, 0x00, 0x00,
	}
};

ulong setbits[16];
GBitmap	set =
{
	setbits,
	0,
	1,
	0,
	{0, 0, 16, 16},
	{0, 0, 16, 16}
};

ulong clrbits[16];
GBitmap	clr =
{
	clrbits,
	0,
	1,
	0,
	{0, 0, 16, 16},
	{0, 0, 16, 16}
};

ulong cursorbackbits[16*4];
GBitmap cursorback =
{
	cursorbackbits,
	0,
	1,
	0,
	{0, 0, 16, 16},
	{0, 0, 16, 16}
};

void	Cursortocursor(Cursor*);
int	mousechanged(void*);

enum{
	Qdir,
	Qbitblt,
	Qmouse,
	Qmousectl,
	Qscreen,
};

Dirtab bitdir[]={
	"bitblt",	{Qbitblt},	0,			0666,
	"mouse",	{Qmouse},	0,			0666,
	"mousectl",	{Qmousectl},	0,			0220,
	"screen",	{Qscreen},	0,			0444,
};

#define	NBIT	(sizeof bitdir/sizeof(Dirtab))
#define	NINFO	8192	/* max chars per subfont; sanity check only */
#define	HDR	3

void
lockedupdate(void)
{
	qlock(&bit);
	if(waserror()){
		qunlock(&bit);
		return;
	}
	screenupdate();
	qunlock(&bit);
	poperror();
}

void
bitfreeup(void)
{
	int i;
	BSubfont *s;

	/* free unused subfonts and compact */
	for(i=0; i<bit.nsubfont; i++){
		s = bit.subfont[i];
		if(s && s!=bdefont && s->ref==0){
			s->ref = 1;
			s->qid[0] = ~0;	/* force cleanup */
			subfontfree(s, i);
		}
	}
	bitcompact();
}

void*
bitmalloc(ulong n)
{
	void *p;

	p = malloc(n);
	if(p)
		return p;
	bitfreeup();
	return malloc(n);
}

void
bitdebug(void)
{
	int i;
	long l;
	Arena *a;

	l = 0;
	for(i=0; i<bit.narena; i++){
		a = &bit.arena[i];
		if(a->words){
			l += a->nwords;
			print("%d: %ld bytes used; %ld total\n", i,
				(a->wfree-a->words)*sizeof(ulong),
				a->nwords*sizeof(ulong));
		}
	}
	print("arena: %ld bytes\n", l*sizeof(ulong));
	l = 0;
	for(i=0; i<bit.nmap; i++)
		if(bit.map[i])
			l++;
	print("%d bitmaps ", l);
	l = 0;
	for(i=0; i<bit.nfont; i++)
		if(bit.font[i])
			l++;
	print("%d fonts ", l);
	l = 0;
	for(i=0; i<bit.nsubfont; i++)
		if(bit.subfont[i]){
			print("%d: %lux %lux ", i, bit.subfont[i]->qid[0], bit.subfont[i]->qid[1]);
			l++;
		}
	print("%d subfonts\n", l);
}

void
bitreset(void)
{
	int ws;
	ulong r;
	Arena *a;

	if(!conf.monitor)
		return;
	memmove(&bdefont0, defont, sizeof(*defont));
	bdefont = &bdefont0;
	bit.map = smalloc(DMAP*sizeof(GBitmap*));
	bit.nmap = DMAP;
	getcolor(0, &r, &r, &r);
	if(r == 0)
		flipping = 1;
	bit.bid = -1;
	bit.subfid = -1;
	bit.fid = -1;
	bit.cacheid = -1;
	bit.font = smalloc(DMAP*sizeof(GFont*));
	bit.nfont = DMAP;
	bit.subfont = smalloc(DMAP*sizeof(BSubfont*));
	bit.nsubfont = DMAP;
	bit.arena = smalloc(DMAP*sizeof(Arena));
	bit.narena = DMAP;
	a = &bit.arena[0];
	/*
	 * Somewhat of a heuristic: start with three screensful and
	 * allocate single screensful dynamically if needed.
	 */
	ws = BI2WD>>gscreen.ldepth;	/* pixels per word */
	a->nwords = 3*(HDR + gscreen.r.max.y*gscreen.r.max.x/ws);
	a->words = xalloc(a->nwords*sizeof(ulong));
	if(a->words == 0){
		/* try again */
		print("bitreset: allocating only 1 screenful\n");
		a->nwords /= 3;
		a->words = a->words = xalloc(a->nwords*sizeof(ulong));
		if(a->words == 0)
			panic("bitreset");
	}
	a->wfree = a->words;
	a->nbusy = 1;	/* keep 0th arena from being freed */
	Cursortocursor(&arrow);
}

/*
 *  screen bit depth changed, reset backup map for cursor
 */
void
bitdepth(void)
{
	cursoroff(1);
	if(gscreen.ldepth > 3)
		cursorback.ldepth = 0;
	else{
		cursorback.ldepth = gscreen.ldepth;
		cursorback.width = ((16 << gscreen.ldepth) + 31) >> 5;
	}
	cursoron(1);
}

void
bitinit(void)
{
	if(!conf.monitor)
		return;
	if(gscreen.ldepth > 3)
		cursorback.ldepth = 0;
	else{
		cursorback.ldepth = gscreen.ldepth;
		cursorback.width = ((16 << gscreen.ldepth) + 31) >> 5;
	}
	cursoron(1);
}

Chan*
bitattach(char *spec)
{
	if(!conf.monitor)
		error(Egreg);
	return devattach('b', spec);
}

Chan*
bitclone(Chan *c, Chan *nc)
{
	if(!conf.monitor)
		error(Egreg);
	nc = devclone(c, nc);
	if(c->qid.path != CHDIR)
		incref(&bit);
	return nc;
}

int
bitwalk(Chan *c, char *name)
{
	if(!conf.monitor)
		error(Egreg);
	return devwalk(c, name, bitdir, NBIT, devgen);
}

void
bitstat(Chan *c, char *db)
{
	if(!conf.monitor)
		error(Egreg);
	devstat(c, db, bitdir, NBIT, devgen);
}

Chan*
bitopen(Chan *c, int omode)
{
	GBitmap *b;

	if(!conf.monitor)
		error(Egreg);
	switch(c->qid.path){
	case CHDIR:
		if(omode != OREAD)
			error(Eperm);
		break;
	case Qmouse:
		lock(&bit);
		if(bit.mouseopen){
			unlock(&bit);
			error(Einuse);
		}
		bit.mouseopen = 1;
		bit.ref++;
		unlock(&bit);
		break;
	case Qbitblt:
		lock(&bit);
		if(bit.bitbltopen || bit.mouseopen){
			unlock(&bit);
			error(Einuse);
		}
		b = smalloc(sizeof(GBitmap));
		*b = gscreen;
		bit.map[0] = b;			/* bitmap 0 is screen */
		bit.subfont[0] = bdefont;	/* subfont 0 is default */
		bit.subfont[0]->ref = 1;
		bit.subfont[0]->qid[0] = 0;
		bit.subfont[0]->qid[1] = 0;
		bit.bid = -1;
		bit.fid = -1;
		bit.subfid = -1;
		bit.cacheid = -1;
		bit.rid = -1;
		bit.mid = -1;
		bit.init = 0;
		bit.bitbltopen = 1;
		Cursortocursor(&arrow);
		unlock(&bit);
		break;
	default:
		incref(&bit);
	}
	c->mode = openmode(omode);
	c->flag |= COPEN;
	c->offset = 0;
	return c;
}

void
bitcreate(Chan *c, char *name, int omode, ulong perm)
{
	if(!conf.monitor)
		error(Egreg);
	USED(c, name, omode, perm);
	error(Eperm);
}

void
bitremove(Chan *c)
{
	if(!conf.monitor)
		error(Egreg);
	USED(c);
	error(Eperm);
}

void
bitwstat(Chan *c, char *db)
{
	if(!conf.monitor)
		error(Egreg);
	USED(c, db);
	error(Eperm);
}

void
bitclose(Chan *c)
{
	GBitmap *b, **bp, **ebp;
	BSubfont *s, **sp, **esp;
	GFont *f, **fp, **efp;

	if(!conf.monitor)
		error(Egreg);
	if(c->qid.path!=CHDIR && (c->flag&COPEN)){
		lock(&bit);
		if(c->qid.path == Qmouse)
			bit.mouseopen = 0;
		if(c->qid.path == Qbitblt)
			bit.bitbltopen = 0;
		if(--bit.ref == 0){
			ebp = &bit.map[bit.nmap];
			for(bp = bit.map; bp<ebp; bp++){
				b = *bp;
				if(b){
					bitfree(b);
					*bp = 0;
				}
			}
			esp = &bit.subfont[bit.nsubfont];
			for(sp=bit.subfont; sp<esp; sp++){
				s = *sp;
				if(s)
					subfontfree(s, sp-bit.subfont);
				/* don't clear *sp: cached */
			}
			efp = &bit.font[bit.nfont];
			for(fp=bit.font; fp<efp; fp++){
				f = *fp;
				if(f){
					fontfree(f);
					*fp = 0;
				}
			}
		}
		unlock(&bit);
	}
}

long
bitread(Chan *c, void *va, long n, ulong offset)
{
	uchar *p, *q;
	long miny, maxy, t, x, y;
	ulong l, nw, ws, rv, gv, bv;
	int off, j;
	Fontchar *i;
	GBitmap *src;
	BSubfont *s;

	if(!conf.monitor)
		error(Egreg);
	if(c->qid.path & CHDIR)
		return devdirread(c, va, n, bitdir, NBIT, devgen);

	if(c->qid.path == Qmouse){
		/*
		 * mouse:
		 *	'm'		1
		 *	buttons		1
		 * 	point		8
		 * 	msec		4
		 */
		if(n < 14)
			error(Ebadblt);
	    Again:
		while(mouse.changed == 0)
			sleep(&mouse.r, mousechanged, 0);
		lock(&cursor);
		if(mouse.changed == 0){
			unlock(&cursor);
			goto Again;
		}
		p = va;
		p[0] = 'm';
		p[1] = mouse.buttons;
		BPLONG(p+2, mouse.xy.x);
		BPLONG(p+6, mouse.xy.y);
		BPLONG(p+10, TK2MS(MACHP(0)->ticks));
		mouse.changed = 0;
		unlock(&cursor);
		return 14;
	}
	if(c->qid.path == Qscreen){
		if(offset==0){
			if(n < 5*12)
				error(Eio);
			sprint(va, "%11d %11d %11d %11d %11d ",
				gscreen.ldepth, gscreen.r.min.x,
				gscreen.r.min.y, gscreen.r.max.x,
				gscreen.r.max.y);
			return 5*12;
		}
		ws = 1<<(3-gscreen.ldepth);	/* pixels per byte */
		l = (gscreen.r.max.x+ws-1)/ws - gscreen.r.min.x/ws;
		t = offset-5*12;
		miny = t/l;	/* unsigned computation */
		maxy = (t+n)/l;
		if(miny >= gscreen.r.max.y)
			return 0;
		if(maxy >= gscreen.r.max.y)
			maxy = gscreen.r.max.y;
		n = 0;
		p = va;
		for(y=miny; y<maxy; y++){
			q = (uchar*)gaddr(&gscreen, Pt(0, y));
			memmove(p, q, l);
			if(flipping)
				/* is screen, so must be word aligned */
				for(x=0; x<l; x+=sizeof(ulong),p+=sizeof(ulong))
					*(ulong*)p ^= ~0;
			else
				p += l;
			n += l;
		}
		return n;
	}
	if(c->qid.path != Qbitblt)
		error(Egreg);

	qlock(&bit);
	if(waserror()){
		qunlock(&bit);
		nexterror();
	}
	p = va;
	/*
	 * Fuss about and figure out what to say.
	 */
	if(bit.init){
		/*
		 * init:
		 *	'I'		1
		 *	ldepth		1
		 * 	rectangle	16
		 * 	clip rectangle	16
		 *	font info	3*12
		 *	fontchars	6*(bdefont->n+1)
		 */
		if(n < 34)
			error(Ebadblt);
		p[0] = 'I';
		p[1] = gscreen.ldepth;
		BPLONG(p+2, gscreen.r.min.x);
		BPLONG(p+6, gscreen.r.min.y);
		BPLONG(p+10, gscreen.r.max.x);
		BPLONG(p+14, gscreen.r.max.y);
		BPLONG(p+18, gscreen.clipr.min.x);
		BPLONG(p+22, gscreen.clipr.min.y);
		BPLONG(p+26, gscreen.clipr.max.x);
		BPLONG(p+30, gscreen.clipr.max.y);
		if(n >= 34+3*12+6*(bdefont->n+1)){
			p += 34;
			sprint((char*)p, "%11d %11d %11d ", bdefont->n,
				bdefont->height, bdefont->ascent);
			p += 3*12;
			for(i=bdefont->info,j=0; j<=bdefont->n; j++,i++,p+=6){
				BPSHORT(p, i->x);
				p[2] = i->top;
				p[3] = i->bottom;
				p[4] = i->left;
				p[5] = i->width;
			}
			n = 34+3*12+6*(bdefont->n+1);
		}else
			n = 34;
		bit.init = 0;
	}else if(bit.bid > 0){
		/*
		 * allocate:
		 *	'A'		1
		 *	bitmap id	2
		 */
		if(n < 3)
			error(Ebadblt);
		if(bit.bid<0 || bit.map[bit.bid]==0)
			error(Ebadbitmap);
		p[0] = 'A';
		BPSHORT(p+1, bit.bid);
		bit.bid = -1;
		n = 3;
	}else if(bit.subfid > 0){
		/*
		 * allocate subfont:
		 *	'K'		1
		 *	subfont id	2
		 */
		if(n<3 || bit.subfid<0)
			error(Ebadblt);
		s = bit.subfont[bit.subfid];
		if(s==0 || s->ref==0)
			error(Ebadfont);
		p[0] = 'K';
		BPSHORT(p+1, bit.subfid);
		bit.subfid = -1;
		n = 3;
	}else if(bit.cacheid >= 0){
		/*
		 * check cache for subfont:
		 *	'J'		1
		 *	subfont id	2
		 *	font info	3*12
		 *	fontchars	6*(subfont->n+1)
		 */
		p[0] = 'J';
		if(bit.cacheid < 0)
			error(Ebadfont);
		s = bit.subfont[bit.cacheid];
		if(s==0 || s->ref==0)
			error(Ebadfont);
		if(n < 3+3*12+6*(s->n+1))
			error(Ebadblt);
		BPSHORT(p+1, bit.cacheid);
		p += 3;
		sprint((char*)p, "%11d %11d %11d ", s->n, s->height, s->ascent);
		p += 3*12;
		for(i=s->info,j=0; j<=s->n; j++,i++,p+=6){
			BPSHORT(p, i->x);
			p[2] = i->top;
			p[3] = i->bottom;
			p[4] = i->left;
			p[5] = i->width;
		}
		n = 3+3*12+6*(s->n+1);
		bit.cacheid = -1;
	}else if(bit.fid >= 0){
		/*
		 * allocate font:
		 *	'N'		1
		 *	font id		2
		 */
		if(n < 3)
			error(Ebadblt);
		if(bit.fid<0 || bit.font[bit.fid]==0)
			error(Ebadfont);
		p[0] = 'N';
		BPSHORT(p+1, bit.fid);
		bit.fid = -1;
		n = 3;
	}else if(bit.mid >= 0){
		/*
		 * read colormap:
		 *	data		12*(2**bitmapdepth)
		 */
		src = bit.map[bit.mid];
		if(src == 0)
			error(Ebadbitmap);
		l = (1<<src->ldepth);
		nw = 1 << l;
		if(n < 12*nw)
			error(Ebadblt);
		for(j = 0; j < nw; j++){
			if(bit.mid == 0){
				getcolor(flipping? ~j : j, &rv, &gv, &bv);
			}else{
				rv = j;
				for(off = 32-l; off > 0; off -= l)
					rv = (rv << l) | j;
				gv = bv = rv;
			}
			BPLONG(p, rv);
			BPLONG(p+4, gv);
			BPLONG(p+8, bv);
			p += 12;
		}
		bit.mid = -1;
		n = 12*nw;
	}else if(bit.rid >= 0){
		/*
		 * read bitmap:
		 *	data		bytewidth*(maxy-miny)
		 */
		src = bit.map[bit.rid];
		if(src == 0)
			error(Ebadbitmap);
		off = 0;
		if(bit.rid == 0)
			off = 1;
		miny = bit.rminy;
		maxy = bit.rmaxy;
		if(miny>maxy || miny<src->r.min.y || maxy>src->r.max.y)
			error(Ebadblt);
		ws = 1<<(3-src->ldepth);	/* pixels per byte */
		/* set l to number of bytes of incoming data per scan line */
		if(src->r.min.x >= 0)
			l = (src->r.max.x+ws-1)/ws - src->r.min.x/ws;
		else{	/* make positive before divide */
			t = (-src->r.min.x)+ws-1;
			t = (t/ws)*ws;
			l = (t+src->r.max.x+ws-1)/ws;
		}
		if(n < l*(maxy-miny))
			error(Ebadblt);
		if(off)
			cursoroff(1);
		n = 0;
		p = va;
		for(y=miny; y<maxy; y++){
			q = (uchar*)gaddr(src, Pt(src->r.min.x, y));
			q += (src->r.min.x&((sizeof(ulong))*ws-1))/ws;
			memmove(p, q, l);
			if(islittle)
				pixreverse(p, l, src->ldepth);
			if(bit.rid==0 && flipping)
				/* is screen, so must be word aligned */
				for(x=0; x<l; x+=sizeof(ulong),p+=sizeof(ulong))
					*(ulong*)p ^= ~0;
			else
				p += l;
			n += l;
		}
		if(off)
			cursoron(1);
		bit.rid = -1;
	}

	poperror();
	qunlock(&bit);
	return n;
}

Point
bitstrsize(GFont *f, uchar *p, int l)
{
	ushort r;
	Point s = {0,0};
	GCacheinfo *c;

	while(l > 0){
		r = BGSHORT(p);
		p += 2;
		l -= 2;
		if(r >= f->ncache)
			continue;
		c = &f->cache[r];
		if(c->bottom > s.y)
			s.y = c->bottom;
		s.x += c->width;
	}
	return s;
}

long
bitwrite(Chan *c, void *va, long n, ulong offset)
{
	uchar *p, *q, *oq;
	long m, v, miny, maxy, t, x, y;
	ulong l, nw, ws, rv, q0, q1;
	ulong *lp;
	int off, isoff, i, j, ok;
 	ulong *endscreen = gaddr(&gscreen, Pt(0, gscreen.r.max.y));
	Point pt, pt1, pt2;
	Rectangle rect;
	Cursor curs;
	Fcode fc;
	Fontchar *fcp;
	GBitmap *src, *dst;
	BSubfont *f, *tf, **fp;
	GFont *ff, **ffp;
	GCacheinfo *gc;
	char buf[64];

	if(!conf.monitor)
		error(Egreg);
	USED(offset);

	if(c->qid.path == CHDIR)
		error(Eisdir);

	if(c->qid.path == Qmousectl){
		if(n >= sizeof(buf))
			n = sizeof(buf)-1;
		strncpy(buf, va, n);
		buf[n] = 0;
		mousectl(buf);
		return n;
	}

	if(c->qid.path != Qbitblt)
		error(Egreg);

	isoff = 0;
	qlock(&bit);
	if(waserror()){
		qunlock(&bit);
		if(isoff)
			cursoron(1);
		nexterror();
	}
	p = va;
	m = n;
	SET(src, dst, f, ff);
	while(m > 0)
		switch(*p){
		default:
			pprint("bitblt request 0x%x\n", *p);
			error(Ebadblt);

		case 'a':
			/*
			 * allocate:
			 *	'a'		1
			 *	ldepth		1
			 *	Rectangle	16
			 * next read returns allocated bitmap id
			 */
			if(m < 18)
				error(Ebadblt);
			v = *(p+1);
			if(v > 3)	/* BUG */
				error(Ebadblt);
			rect.min.x = BGLONG(p+2);
			rect.min.y = BGLONG(p+6);
			rect.max.x = BGLONG(p+10);
			rect.max.y = BGLONG(p+14);
			if(Dx(rect) < 0 || Dy(rect) < 0)
				error(Ebadblt);
			bit.bid = bitalloc(rect, v);
			m -= 18;
			p += 18;
			break;

		case 'b':
			/*
			 * bitblt
			 *	'b'		1
			 *	dst id		2
			 *	dst Point	8
			 *	src id		2
			 *	src Rectangle	16
			 *	code		2
			 */
			if(m < 31)
				error(Ebadblt);
			fc = BGSHORT(p+29) & 0xF;
			v = BGSHORT(p+11);
			if(v<0 || v>=bit.nmap || (src=bit.map[v])==0)
				error(Ebadbitmap);
			off = 0;
			if(v == 0){
				if(flipping)
					fc = flipS[fc];
				off = 1;
			}
			v = BGSHORT(p+1);
			if(v<0 || v>=bit.nmap || (dst=bit.map[v])==0)
				error(Ebadbitmap);
			if(v == 0){
				if(flipping)
					fc = flipD[fc];
				off = 1;
			}
			pt.x = BGLONG(p+3);
			pt.y = BGLONG(p+7);
			rect.min.x = BGLONG(p+13);
			rect.min.y = BGLONG(p+17);
			rect.max.x = BGLONG(p+21);
			rect.max.y = BGLONG(p+25);
			if(off && !isoff){
				cursoroff(1);
				isoff = 1;
			}
			gbitblt(dst, pt, src, rect, fc);
			if(dst->base < endscreen)
				mbbrect(Rpt(pt, add(pt, sub(rect.max, rect.min))));
			m -= 31;
			p += 31;
			break;

		case 'c':
			/*
			 * cursorswitch
			 *	'c'		1
			 * if one more byte, says whether to disable
			 * because of stupid lcd's (thank you bart)
			 * else
			 * nothing more: return to arrow; else
			 * 	Point		8
			 *	clr		32
			 *	set		32
			 */
			if(m == 1){
				if(!isoff){
					cursoroff(1);
					isoff = 1;
				}
				Cursortocursor(&arrow);
				m -= 1;
				p += 1;
				break;
			}
			if(m == 2){
				if(p[1]){	/* make damn sure */
					cursor.disable = 0;
					isoff = 1;
				}else{
					cursoroff(1);
					cursor.disable = 1;
				}
				m -= 2;
				p += 2;
				break;
			}
			if(m < 73)
				error(Ebadblt);
			curs.offset.x = BGLONG(p+1);
			curs.offset.y = BGLONG(p+5);
			memmove(curs.clr, p+9, 2*16);
			memmove(curs.set, p+41, 2*16);
			if(islittle){
				pixreverse(curs.clr, 2*16, 0);
				pixreverse(curs.set, 2*16, 0);
			}
			if(!isoff){
				cursoroff(1);
				isoff = 1;
			}
			Cursortocursor(&curs);
			m -= 73;
			p += 73;
			break;

		case 'e':
			/*
			 * polysegment
			 *
			 *	'e'		1
			 *	id		2
			 *	pt		8
			 *	value		1
			 *	code		2
			 *	n		2
			 *	pts		2*n
			 */
			if(m < 16)
				error(Ebadblt);
			l = BGSHORT(p+14);
			if(m < 16+2*l)
				error(Ebadblt);
			v = BGSHORT(p+1);
			if(v<0 || v>=bit.nmap || (dst=bit.map[v])==0)
				error(Ebadbitmap);
			off = 0;
			fc = BGSHORT(p+12) & 0xF;
			if(v == 0){
				if(flipping)
					fc = flipD[fc];
				off = 1;
			}
			pt1.x = BGLONG(p+3);
			pt1.y = BGLONG(p+7);
			t = p[11];
			if(off && !isoff){
				cursoroff(1);
				isoff = 1;
			}
			p += 16;
			m -= 16;
			while(l > 0){
				pt2.x = pt1.x + (schar)p[0];
				pt2.y = pt1.y + (schar)p[1];
				gsegment(dst, pt1, pt2, t, fc);
				if(dst->base < endscreen){
					mbbpt(pt1);
					mbbpt(pt2);
				}
				pt1 = pt2;
				p += 2;
				m -= 2;
				l--;
			}
			break;

		case 'f':
			/*
			 * free
			 *	'f'		1
			 *	id		2
			 */
			if(m < 3)
				error(Ebadblt);
			v = BGSHORT(p+1);
			if(v<0 || v>=bit.nmap || (dst=bit.map[v])==0)
				error(Ebadbitmap);
			bitfree(dst);
			bit.map[v] = 0;
			m -= 3;
			p += 3;
			break;

		case 'g':
			/*
			 * free subfont
			 *	'g'		1
			 *	id		2
			 */
			if(m < 3)
				error(Ebadblt);
			v = BGSHORT(p+1);
			if(v<0 || v>=bit.nsubfont || (f=bit.subfont[v])==0 || f->ref==0)
				error(Ebadfont);
			subfontfree(f, v);
			m -= 3;
			p += 3;
			break;

		case 'h':
			/*
			 * free font
			 *	'h'		1
			 *	id		2
			 */
			if(m < 3)
				error(Ebadblt);
			v = BGSHORT(p+1);
			if(v<0 || v>=bit.nfont || (ff=bit.font[v])==0)
				error(Ebadfont);
			fontfree(ff);
			bit.font[v] = 0;
			m -= 3;
			p += 3;
			break;

		case 'i':
			/*
			 * init
			 *
			 *	'i'		1
			 */
			bit.init = 1;
			m -= 1;
			p += 1;
			break;

		case 'j':
			/*
			 * subfont cache check
			 *
			 *	'j'		1
			 *	qid		8
			 */
			if(m < 9)
				error(Ebadblt);
			q0 = BGLONG(p+1);
			q1 = BGLONG(p+5);
			i = 0;
			if(q0 != ~0)
				for(; i<bit.nsubfont; i++){
					f = bit.subfont[i];
					if(f && f->qid[0]==q0 && f->qid[1]==q1)
						goto sfcachefound;
				}
			error(Esfnotcached);

		sfcachefound:
			f->ref++;
			bit.cacheid = i;
			m -= 9;
			p += 9;
			break;

		case 'k':
			/*
			 * allocate subfont
			 *	'k'		1
			 *	n		2
			 *	height		1
			 *	ascent		1
			 *	bitmap id	2
			 *	qid		8
			 *	fontchars	6*(n+1)
			 * next read returns allocated font id
			 */
			if(m < 15)
				error(Ebadblt);
			v = BGSHORT(p+1);
			if(v<0 || v>NINFO || m<15+6*(v+1))
				error(Ebadblt);
			for(i=1; i<bit.nsubfont; i++)
				if(bit.subfont[i] == 0)
					goto subfontfound;
			fp = bitmalloc((bit.nsubfont+DMAP)*sizeof(BSubfont*));
			if(fp == 0)
				error(Enomem);
			memmove(fp, bit.subfont, bit.nsubfont*sizeof(BSubfont*));
			free(bit.subfont);
			bit.subfont = fp;
			bit.nsubfont += DMAP;
		subfontfound:
			f = bitmalloc(sizeof(BSubfont));
			if(f == 0)
				error(Enomem);
			f->info = bitmalloc((v+1)*sizeof(Fontchar));
			if(f->info == 0){
				free(f);
				error(Enomem);
			}
			bit.subfont[i] = f;
			f->n = v;
			f->height = p[3];
			f->ascent = p[4];
			f->qid[0] = BGLONG(p+7);
			f->qid[1] = BGLONG(p+11);
			/* check to see if already there, uncache if so */
			for(j=0; j<bit.nsubfont; j++){
				if(j == i)
					continue;
				tf = bit.subfont[j];
				if(tf && tf->qid[0]==f->qid[0] && tf->qid[1]==f->qid[1]){
					f->qid[0] = ~0;	/* uncached */
					break;
				}
			}
			f->ref = 1;
			v = BGSHORT(p+5);
			if(v<0 || v>=bit.nmap || (dst=bit.map[v])==0)
				error(Ebadbitmap);
			f->bits = dst;
			bit.map[v] = 0;	/* subfont now owns bitmap */
			m -= 15;
			p += 15;
			fcp = f->info;
			for(j=0; j<=f->n; j++,fcp++){
				fcp->x = BGSHORT(p);
				fcp->top = p[2];
				fcp->bottom = p[3];
				fcp->left = p[4];
				fcp->width = p[5];
				fcp->top = p[2];
				p += 6;
				m -= 6;
			}
			bit.subfid = i;
			break;

		case 'l':
			/*
			 * line segment
			 *
			 *	'l'		1
			 *	id		2
			 *	pt1		8
			 *	pt2		8
			 *	value		1
			 *	code		2
			 */
			if(m < 22)
				error(Ebadblt);
			v = BGSHORT(p+1);
			if(v<0 || v>=bit.nmap || (dst=bit.map[v])==0)
				error(Ebadbitmap);
			off = 0;
			fc = BGSHORT(p+20) & 0xF;
			if(v == 0){
				if(flipping)
					fc = flipD[fc];
				off = 1;
			}
			pt1.x = BGLONG(p+3);
			pt1.y = BGLONG(p+7);
			pt2.x = BGLONG(p+11);
			pt2.y = BGLONG(p+15);
			t = p[19];
			if(off && !isoff){
				cursoroff(1);
				isoff = 1;
			}
			gsegment(dst, pt1, pt2, t, fc);
			if(dst->base < endscreen){
				mbbpt(pt1);
				mbbpt(pt2);
			}
			m -= 22;
			p += 22;
			break;

		case 'm':
			/*
			 * read colormap
			 *
			 *	'm'		1
			 *	id		2
			 */
			if(m < 3)
				error(Ebadblt);
			v = BGSHORT(p+1);
			if(v<0 || v>=bit.nmap || (dst=bit.map[v])==0)
				error(Ebadbitmap);
			bit.mid = v;
			m -= 3;
			p += 3;
			break;

		case 'n':
			/*
			 * allocate font
			 *	'n'		1
			 *	height		1
			 *	ascent		1
			 *	ldepth		2
			 *	ncache		2
			 * next read returns allocated font id
			 */
			if(m < 7)
				error(Ebadblt);
			v = BGSHORT(p+3);
			t = BGSHORT(p+5);
			if(v<0 || t<0)
				error(Ebadblt);
			for(i=0; i<bit.nfont; i++)
				if(bit.font[i] == 0)
					goto fontfound;
			ffp = bitmalloc((bit.nfont+DMAP)*sizeof(GFont*));
			if(ffp == 0)
				error(Enomem);
			memmove(ffp, bit.font, bit.nfont*sizeof(GFont*));
			free(bit.font);
			bit.font = ffp;
			bit.nfont += DMAP;
		fontfound:
			ff = bitmalloc(sizeof(GFont));
			if(ff == 0)
				error(Enomem);
			ff->ncache = t;
			ff->cache = bitmalloc(t*sizeof(GCacheinfo));
			if(ff->cache == 0){
				free(ff);
				error(Enomem);
			}
			bit.font[i] = ff;
			ff = bit.font[i];
			ff->height = p[1];
			ff->ascent = p[2];
			ff->ldepth = v;
			ff->width = 0;
			ff->b = 0;
			m -= 7;
			p += 7;
			bit.fid = i;
			break;

		case 'p':
			/*
			 * point
			 *
			 *	'p'		1
			 *	id		2
			 *	pt		8
			 *	value		1
			 *	code		2
			 */
			if(m < 14)
				error(Ebadblt);
			v = BGSHORT(p+1);
			if(v<0 || v>=bit.nmap || (dst=bit.map[v])==0)
				error(Ebadbitmap);
			off = 0;
			fc = BGSHORT(p+12) & 0xF;
			if(v == 0){
				if(flipping)
					fc = flipD[fc];
				off = 1;
			}
			pt1.x = BGLONG(p+3);
			pt1.y = BGLONG(p+7);
			t = p[11];
			if(off && !isoff){
				cursoroff(1);
				isoff = 1;
			}
			gpoint(dst, pt1, t, fc);
			if(dst->base < endscreen)
				mbbpt(pt1);
			m -= 14;
			p += 14;
			break;

		case 'q':
			/*
			 * clip rectangle
			 *	'q'		1
			 *	id		2
			 *	rect		16
			 */
			if(m < 19)
				error(Ebadblt);
			v = BGSHORT(p+1);
			if(v<0 || v>=bit.nmap || (dst=bit.map[v])==0)
				error(Ebadbitmap);
			rect.min.x = BGLONG(p+3);
			rect.min.y = BGLONG(p+7);
			rect.max.x = BGLONG(p+11);
			rect.max.y = BGLONG(p+15);
			if(rectclip(&rect, dst->r))
				dst->clipr = rect;
			else
				dst->clipr = dst->r;
			m -= 19;
			p += 19;
			break;

		case 'r':
			/*
			 * read
			 *	'r'		1
			 *	src id		2
			 *	miny		4
			 *	maxy		4
			 */
			if(m < 11)
				error(Ebadblt);
			v = BGSHORT(p+1);
			if(v<0 || v>=bit.nmap || (src=bit.map[v])==0)
				error(Ebadbitmap);
			miny = BGLONG(p+3);
			maxy = BGLONG(p+7);
			if(miny>maxy || miny<src->r.min.y || maxy>src->r.max.y)
				error(Ebadblt);
			bit.rid = v;
			bit.rminy = miny;
			bit.rmaxy = maxy;
			p += 11;
			m -= 11;
			break;

		case 's':
			/*
			 * string
			 *	's'		1
			 *	id		2
			 *	pt		8
			 *	font id		2
			 *	code		2
			 *	n		2
			 * 	cache indices	2*n (not null terminated)
			 */
			if(m < 17)
				error(Ebadblt);
			v = BGSHORT(p+1);
			if(v<0 || v>=bit.nmap || (dst=bit.map[v])==0)
				error(Ebadbitmap);
			off = 0;
			fc = BGSHORT(p+13) & 0xF;
			if(v == 0){
				if(flipping)
					fc = flipD[fc];
				off = 1;
			}
			pt.x = BGLONG(p+3);
			pt.y = BGLONG(p+7);
			v = BGSHORT(p+11);
			if(v<0 || v>=bit.nfont || (ff=bit.font[v])==0)
				error(Ebadfont);
			l = BGSHORT(p+15)*2;
			p += 17;
			m -= 17;
			if(l > m)
				error(Ebadblt);
			if(off && !isoff){
				cursoroff(1);
				isoff = 1;
			}
			bitstring(dst, pt, ff, p, l, fc);
			if(dst->base < endscreen)
				mbbrect(Rpt(pt, add(pt, bitstrsize(ff, p, l))));
			m -= l;
			p += l;
			break;

		case 't':
			/*
			 * texture
			 *	't'		1
			 *	dst id		2
			 *	rect		16
			 *	src id		2
			 *	fcode		2
			 */
			if(m < 23)
				error(Ebadblt);
			v = BGSHORT(p+1);
			if(v<0 || v>=bit.nmap || (dst=bit.map[v])==0)
				error(Ebadbitmap);
			off = 0;
			fc = BGSHORT(p+21) & 0xF;
			if(v == 0){
				if(flipping)
					fc = flipD[fc];
				off = 1;
			}
			rect.min.x = BGLONG(p+3);
			rect.min.y = BGLONG(p+7);
			rect.max.x = BGLONG(p+11);
			rect.max.y = BGLONG(p+15);
			v = BGSHORT(p+19);
			if(v<0 || v>=bit.nmap || (src=bit.map[v])==0)
				error(Ebadbitmap);
			if(off && !isoff){
				cursoroff(1);
				isoff = 1;
			}
			gtexture(dst, rect, src, fc);
			if(dst->base < endscreen)
				mbbrect(rect);
			m -= 23;
			p += 23;
			break;

		case 'v':
			/*
			 * clear font cache and bitmap.
			 * if error, font is unchanged.
			 *	'v'		1
			 *	id		2
			 *	ncache		2
			 *	width		2
			 */
			if(m < 7)
				error(Ebadblt);
			v = BGSHORT(p+1);
			t = BGSHORT(p+3);
			if(t<0 || v<0 || v>=bit.nfont || (ff=bit.font[v])==0)
				error(Ebadblt);
			x = BGSHORT(p+5);
			i = bitalloc(Rect(0, 0, t*x, ff->height), ff->ldepth);
			if(t != ff->ncache){
				gc = bitmalloc(t*sizeof(ff->cache[0]));
				if(gc == 0){
					bitfree(bit.map[i]);
					bit.map[i] = 0;
					error(Enomem);
				}
				free(ff->cache);
				ff->cache = gc;
				ff->ncache = t;
			}else{
				/*
				 * memset not necessary but helps avoid
				 * confusion if the cache is mishandled by the
				 * user.
				 */
				memset(ff->cache, 0, t*sizeof(ff->cache[0]));
			}
			if(ff->b)
				bitfree(ff->b);
			ff->b = bit.map[i];
			bit.map[i] = 0;	/* disconnect it from GBitmap space */
			ff->width = x;
			p += 7;
			m -= 7;
			break;

		case 'w':
			/*
			 * write
			 *	'w'		1
			 *	dst id		2
			 *	miny		4
			 *	maxy		4
			 *	data		bytewidth*(maxy-miny)
			 */
			if(m < 11)
				error(Ebadblt);
			v = BGSHORT(p+1);
			if(v<0 || v>=bit.nmap || (dst=bit.map[v])==0)
				error(Ebadbitmap);
			off = 0;
			if(v == 0)
				off = 1;
			miny = BGLONG(p+3);
			maxy = BGLONG(p+7);
			if(miny>maxy || miny<dst->r.min.y || maxy>dst->r.max.y)
				error(Ebadblt);
			ws = 1<<(3-dst->ldepth);	/* pixels per byte */
			/* set l to number of bytes of incoming data per scan line */
			if(dst->r.min.x >= 0)
				l = (dst->r.max.x+ws-1)/ws - dst->r.min.x/ws;
			else{	/* make positive before divide */
				t = (-dst->r.min.x)+ws-1;
				t = (t/ws)*ws;
				l = (t+dst->r.max.x+ws-1)/ws;
			}
			if(dst->base < endscreen)
				mbbrect(Rect(dst->r.min.x, miny, dst->r.max.x, maxy));
			p += 11;
			m -= 11;
			if(m < l*(maxy-miny))
				error(Ebadblt);
			if(off && !isoff){
				cursoroff(1);
				isoff = 1;
			}
			for(y=miny; y<maxy; y++){
				oq = (uchar*)gaddr(dst, Pt(dst->r.min.x, y));
				q = oq + (dst->r.min.x&((sizeof(ulong))*ws-1))/ws;
				memmove(q, p, l);
				if(islittle)
					pixreverse(q, l, dst->ldepth);
				if(v==0 && flipping){	/* flip bits */
					/* we know it's all word aligned */
					lp = (ulong*)oq;
					for(x=0; x<l; x+=sizeof(ulong))
						*lp++ ^= ~0;
				}
				p += l;
				m -= l;
			}
			if(v == 0)
				hwscreenwrite(miny, maxy);
			break;

		case 'x':
			/*
			 * cursorset
			 *
			 *	'x'		1
			 *	pt		8
			 */
			if(m < 9)
				error(Ebadblt);
			pt1.x = BGLONG(p+1);
			pt1.y = BGLONG(p+5);
			if(ptinrect(pt1, gscreen.r)){
				mouse.xy = pt1;
				mouse.track = 1;
				mouseclock();
			}
			m -= 9;
			p += 9;
			break;

		case 'y':
			/*
			 * load font from subfont
			 *	'y'		1
			 *	id		2
			 *	cache index	2
			 *	subfont id	2
			 *	subfont index	2
			 */
			if(m < 9)
				error(Ebadblt);
			v = BGSHORT(p+1);
			if(v<0 || v>=bit.nfont || (ff=bit.font[v])==0)
				error(Ebadblt);
			if(ff->b == 0)
				error(Ebadbitmap);
			l = BGSHORT(p+3);
			if(l >= ff->ncache)
				error(Ebadblt);
			v = BGSHORT(p+5);
			if(v<0 || v>=bit.nsubfont || (f=bit.subfont[v])==0 || f->ref==0)
				error(Ebadfont);
			nw = BGSHORT(p+7);
			if(nw >= f->n)
				error(Ebadblt);
			bitloadchar(ff, l, f, nw);
			p += 9;
			m -= 9;
			break;

		case 'z':
			/*
			 * write the colormap
			 *
			 *	'z'		1
			 *	id		2
			 *	map		12*(2**bitmapdepth)
			 */
			if(m < 3)
				error(Ebadblt);
			v = BGSHORT(p+1);
			if(v != 0)
				error(Ebadbitmap);
			m -= 3;
			p += 3;
			nw = 1 << (1 << gscreen.ldepth);
			if(m < 12*nw)
				error(Ebadblt);
			ok = 1;
			for(i = 0; i < nw; i++){
				ok &= setcolor(flipping ? ~i : i, BGLONG(p), BGLONG(p+4), BGLONG(p+8));
				p += 12;
				m -= 12;
			}
			if(!ok){
				/* assume monochrome: possibly change flipping */
				l = BGLONG(p-12);
				getcolor(nw-1, &rv, &rv, &rv);
				flipping = (l != rv);
			}
			break;
		}

	poperror();
	if(isoff)
		cursoron(1);
	screenupdate();
	qunlock(&bit);
	return n;
}

int
bitalloc(Rectangle rect, int ld)
{
	Arena *a, *ea, *aa;
	GBitmap *b, **bp, **ep;
	ulong l, ws, nw;
	long t;

	ws = BI2WD>>ld;	/* pixels per word */
	if(rect.min.x >= 0){
		l = (rect.max.x+ws-1)/ws;
		l -= rect.min.x/ws;
	}else{	/* make positive before divide */
		t = (-rect.min.x)+ws-1;
		t = (t/ws)*ws;
		l = (t+rect.max.x+ws-1)/ws;
	}
	nw = l*Dy(rect);
	ea = &bit.arena[bit.narena];

	/* first try easy fit */
	for(a=bit.arena; a<ea; a++){
		if(a->words == 0)
			continue;
		if(a->wfree+HDR+nw <= a->words+a->nwords)
			goto found;
	}

	/* compact and try again */
	bitcompact();
	aa = 0;
	for(a=bit.arena; a<ea; a++){
		if(a->words == 0){
			if(aa == 0)
				aa = a;
			continue;
		}
		if(a->wfree+HDR+nw <= a->words+a->nwords)
			goto found;
	}

	/* need new arena */
	if(aa){
		a = aa;
		a->nwords = HDR + (gscreen.r.max.y*gscreen.r.max.x)/ws;
		if(a->nwords < HDR+nw)
			a->nwords = HDR+nw;
		a->words = xalloc(a->nwords*sizeof(ulong));
		if(a->words){
			a->wfree = a->words;
			a->nbusy = 0;
			goto found;
		}
	}
	/* else can't grow list: bitmaps have backpointers */

	bitfreeup();

	for(a=bit.arena; a<ea; a++){
		if(a->words == 0)
			continue;
		if(a->wfree+HDR+nw <= a->words+a->nwords)
			goto found;
	}
	if(a == ea)
		error(Enobitstore);
	
    found:
	b = bitmalloc(sizeof(GBitmap));
	if(b == 0)
		error(Enomem);
	*a->wfree++ = nw;
	*a->wfree++ = (ulong)a;
	*a->wfree++ = (ulong)b;
	memset(a->wfree, 0, nw*sizeof(ulong));
	b->base = a->wfree;
	a->wfree += nw;
	a->nbusy++;
	b->zero = l*rect.min.y;
	if(rect.min.x >= 0)
		b->zero += rect.min.x/ws;
	else
		b->zero -= (-rect.min.x+ws-1)/ws;
	b->zero = -b->zero;
	b->width = l;
	b->ldepth = ld;
	b->r = rect;
	b->clipr = rect;
	b->cache = 0;
	/* worth doing better than linear lookup? */
	ep = bit.map+bit.nmap;
	for(bp=bit.map; bp<ep; bp++)
		if(*bp == 0)
			break;
	if(bp == ep){
		bp = bitmalloc((bit.nmap+DMAP)*sizeof(GBitmap*));
		if(bp == 0){
			bitfree(b);
			error(Enomem);
		}
		memmove(bp, bit.map, bit.nmap*sizeof(GBitmap*));
		free(bit.map);
		bit.map = bp;
		bp += bit.nmap;
		bit.nmap += DMAP;
	}
	*bp = b;
	return bp-bit.map;
}

void
bitfree(GBitmap *b)
{
	Arena *a;

	if(b->base != gscreen.base){	/* can't free screen memory */
		a = (Arena*)(b->base[-2]);
		a->nbusy--;
		if(a->nbusy == 0)
			arenafree(a);
		b->base[-1] = 0;
	}
	free(b);
}

void
fontfree(GFont *f)
{
	if(f->b)
		bitfree(f->b);
	free(f->cache);
	free(f);
}

void
subfontfree(BSubfont *s, int i)
{
	if(s!=bdefont && s->ref>0){	/* don't free subfont 0, bdefont */
		s->ref--;
		if(s->ref==0 && s->qid[0]==~0){	/* uncached */
			bitfree(s->bits);
			free(s->info);
			free(s);
			bit.subfont[i] = 0;
		}
	}
	return;
}

void
arenafree(Arena *a)
{
	xfree(a->words);
	a->words = 0;
}

void
bitstring(GBitmap *bp, Point pt, GFont *f, uchar *p, long l, Fcode fc)
{
	int full;
	Rectangle rect;
	ushort r;
	GCacheinfo *c;
	int x;
	Fcode clr;

	clr = 0;
	full = (fc&~S)^(D&~S);	/* result involves source */
	if(full){
		rect.min.y = 0;
		rect.max.y = f->height;
		/* set clr to result under fc if source pixel is zero */
		/* hard to do without knowing layout of bits, so we cheat */
		clr = (fc&3);	/* fc&3 is result if source is zero */
		clr |= clr<<2;	/* fc&(3<<2) is result if source is one */
	}

	while(l > 0){
		r = BGSHORT(p);
		p += 2;
		l -= 2;
		if(r >= f->ncache)
			continue;
		c = &f->cache[r];
		if(!full){
			rect.min.y = c->top;
			rect.max.y = c->bottom;
		}else{
			if(c->left > 0)
				gbitblt(bp, pt, bp,
					Rect(pt.x, pt.y, pt.x+c->left, pt.y+f->height),
					clr);
			x = c->left+(c->xright-c->x);
			if(x < c->width)
				gbitblt(bp, Pt(pt.x+x, pt.y), bp,
					Rect(pt.x+x, pt.y, pt.x+c->width, pt.y+f->height),
					clr);
		}
		rect.min.x = c->x;
		rect.max.x = c->xright;
		gbitblt(bp, Pt(pt.x+c->left, pt.y+rect.min.y), f->b, rect, fc);
		pt.x += c->width;
	}
}

void
bitloadchar(GFont *f, int ci, GSubfont *subf, int si)
{
	GCacheinfo *c;
	Rectangle rect;
	Fontchar *fi;
	int y;

	c = &f->cache[ci];
	fi = &subf->info[si];
	/* careful about sign extension: top and bottom are uchars */
	y = fi->top + (f->ascent-subf->ascent);
	if(y < 0)
		y = 0;
	c->top = y;
	y = fi->bottom + (f->ascent-subf->ascent);
	if(y < 0)
		y = 0;
	c->bottom = y;
	c->width = fi->width;
	c->left = fi->left;
	c->x = ci*f->width;
	c->xright = c->x + ((fi+1)->x - fi->x);
	rect.min.y = 0;
	rect.max.y = f->height;
	rect.min.x = c->x;
	rect.max.x = c->x+f->width;
	gbitblt(f->b, rect.min, f->b, rect, 0);
	rect.min.x = fi->x;
	rect.max.x = (fi+1)->x;
	rect.max.y = subf->height;
	gbitblt(f->b, Pt(c->x, f->ascent-subf->ascent), subf->bits, rect, S);
}

QLock	bitlock;

GBitmap*
id2bit(int v)
{
	GBitmap *bp=0;

	if(v<0 || v>=bit.nmap || (bp=bit.map[v])==0)
		error(Ebadbitmap);
	return bp;
}

void
bitcompact(void)
{
	Arena *a, *ea, *na;
	ulong *p1, *p2, n;

	qlock(&bitlock);
	ea = &bit.arena[bit.narena];
	for(a=bit.arena; a<ea; a++){
		if(a->words == 0)
			continue;
		/* first compact what's here */
		p1 = p2 = a->words;
		while(p2 < a->wfree){
			n = HDR+p2[0];
			if(p2[2] == 0){
				p2 += n;
				continue;
			}
			if(p1 != p2){
				memmove(p1, p2, n*sizeof(ulong));
				((GBitmap*)p1[2])->base = p1+HDR;
			}
			p2 += n;
			p1 += n;
		}
		/* now pull stuff from later arena to fill this one */
		na = a+1;
		while(na<ea && p1<a->words+a->nwords){
			p2 = na->words;
			if(p2 == 0){
				na++;
				continue;
			}
			while(p2 < na->wfree){
				n = HDR+p2[0];
				if(p2[2] == 0){
					p2 += n;
					continue;
				}
				if(p1+n < a->words+a->nwords){
					memmove(p1, p2, n*sizeof(ulong));
					((GBitmap*)p1[2])->base = p1+HDR;
					/* block now in new arena... */
					p1[1] = (ulong)a;
					a->nbusy++;
					/* ... not in old arena */
					na->nbusy--;
					p2[2] = 0;
					p1 += n;
				}
				p2 += n;
			}
			na++;
		}
		a->wfree = p1;
	}
	for(a=bit.arena; a<ea; a++)
		if(a->words && a->nbusy==0)
			arenafree(a);
	qunlock(&bitlock);
}

void
Cursortocursor(Cursor *c)
{
	int i;
	uchar *p;

	lock(&cursor);
	memmove(&cursor, c, sizeof(Cursor));
	for(i=0; i<16; i++){
		p = (uchar*)&setbits[i];
		*p = c->set[2*i];
		*(p+1) = c->set[2*i+1];
		p = (uchar*)&clrbits[i];
		*p = c->clr[2*i];
		*(p+1) = c->clr[2*i+1];
	}
	unlock(&cursor);
}

void
cursoron(int dolock)
{
	if(cursor.disable)
		return;
	if(dolock)
		lock(&cursor);
	if(cursor.visible++ == 0){
		cursor.r.min = mouse.xy;
		cursor.r.max = add(mouse.xy, Pt(16, 16));
		cursor.r = raddp(cursor.r, cursor.offset);
		gbitblt(&cursorback, Pt(0, 0), &gscreen, cursor.r, S);
		gbitblt(&gscreen, cursor.r.min,
			&clr, Rect(0, 0, 16, 16), flipping? flipD[D&~S] : D&~S);
		gbitblt(&gscreen, cursor.r.min,
			&set, Rect(0, 0, 16, 16), flipping? flipD[S|D] : S|D);
		mbbrect(cursor.r);
	}
	if(dolock)
		unlock(&cursor);
}

void
cursoroff(int dolock)
{
	if(cursor.disable)
		return;
	if(dolock)
		lock(&cursor);
	if(--cursor.visible == 0) {
		gbitblt(&gscreen, cursor.r.min, &cursorback, Rect(0, 0, 16, 16), S);
		mbbrect(cursor.r);
		mousescreenupdate();
	}
	if(dolock)
		unlock(&cursor);
}

void
mousedelta(int b, int dx, int dy)	/* called at higher priority */
{
	mouse.dx += dx;
	mouse.dy += dy;
	mouse.newbuttons = b;
	mouse.track = 1;
}

void
mousebuttons(int b)	/* called at higher priority */
{
	/*
	 * It is possible if you click very fast and get bad luck
	 * you could miss a button click (down up).  Doesn't seem
	 * likely or important enough to worry about.
	 */
	mouse.newbuttons = b;
	mouse.track = 1;		/* aggressive but o.k. */
	mouseclock();
}

void
mouseupdate(int dolock)
{
	int x, y;

	if(!mouse.track || (dolock && !canlock(&cursor)))
		return;

	x = mouse.xy.x + mouse.dx;
	if(x < gscreen.r.min.x)
		x = gscreen.r.min.x;
	if(x >= gscreen.r.max.x)
		x = gscreen.r.max.x;
	y = mouse.xy.y + mouse.dy;
	if(y < gscreen.r.min.y)
		y = gscreen.r.min.y;
	if(y >= gscreen.r.max.y)
		y = gscreen.r.max.y;
	cursoroff(0);
	mouse.xy = Pt(x, y);
	cursoron(0);
	mousescreenupdate();
	mouse.dx = 0;
	mouse.dy = 0;
	mouse.clock = 0;
	mouse.track = 0;
	mouse.buttons = mouse.newbuttons;
	mouse.changed = 1;

	if(dolock){
		unlock(&cursor);
		wakeup(&mouse.r);
	}
}

/*
 *  microsoft 3 button, 7 bit bytes
 *
 *	byte 0 -	1  L  R Y7 Y6 X7 X6
 *	byte 1 -	0 X5 X4 X3 X2 X1 X0
 *	byte 2 -	0 Y5 Y4 Y3 Y2 Y1 Y0
 *	byte 3 -	0  M  x  x  x  x  x	(optional)
 *
 *  shift & right button is the same as middle button (for 2 button mice)
 */
int
m3mouseputc(IOQ *q, int c)
{
	static uchar msg[3];
	static int nb;
	static int middle;
	static uchar b[] = { 0, 4, 1, 5, 0, 2, 1, 5 };
	short x;

	USED(q);
	/* 
	 *  check bit 6 for consistency
	 */
	if(nb==0){
		if((c&0x40) == 0){
			/* an extra byte gets sent for the middle button */
			middle = (c&0x20) ? 2 : 0;
			mousebuttons((mouse.buttons & ~2) | middle);
			return 0;
		}
	}
	msg[nb] = c;
	if(++nb == 3){
		nb = 0;
		mouse.newbuttons = middle | b[(msg[0]>>4)&3 | (mouseshifted ? 4 : 0)];
		x = (msg[0]&0x3)<<14;
		mouse.dx = (x>>8) | msg[1];
		x = (msg[0]&0xc)<<12;
		mouse.dy = (x>>8) | msg[2];
		mouse.track = 1;
		mouseclock();
	}
	return 0;
}

/*
 *  Logitech 5 byte packed binary mouse format, 8 bit bytes
 *
 *  shift & right button is the same as middle button (for 2 button mice)
 */
int
mouseputc(IOQ *q, int c)
{
	static short msg[5];
	static int nb;
	static uchar b[] = {0, 4, 2, 6, 1, 5, 3, 7, 0, 2, 2, 6, 1, 5, 3, 7};

	USED(q);
	if((c&0xF0) == 0x80)
		nb=0;
	msg[nb] = c;
	if(c & 0x80)
		msg[nb] |= ~0xFF;	/* sign extend */
	if(++nb == 5){
		mouse.newbuttons = b[((msg[0]&7)^7) | (mouseshifted ? 8 : 0)];
		mouse.dx = msg[1]+msg[3];
		mouse.dy = -(msg[2]+msg[4]);
		mouse.track = 1;
		mouseclock();
		nb = 0;
	}
	return 0;
}

int
mousechanged(void *m)
{
	USED(m);
	return mouse.changed;
}

/*
 *  reverse pixels into little endian order
 */
uchar revtab0[] = {
 0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
 0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
 0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
 0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
 0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
 0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
 0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
 0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
 0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
 0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
 0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
 0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
 0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
 0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
 0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
 0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
 0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
 0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
 0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
 0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
 0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
 0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
 0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
 0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
 0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
 0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
 0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
 0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
 0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
 0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
 0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
 0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff,
};
uchar revtab1[] = {
 0x00, 0x40, 0x80, 0xc0, 0x10, 0x50, 0x90, 0xd0,
 0x20, 0x60, 0xa0, 0xe0, 0x30, 0x70, 0xb0, 0xf0,
 0x04, 0x44, 0x84, 0xc4, 0x14, 0x54, 0x94, 0xd4,
 0x24, 0x64, 0xa4, 0xe4, 0x34, 0x74, 0xb4, 0xf4,
 0x08, 0x48, 0x88, 0xc8, 0x18, 0x58, 0x98, 0xd8,
 0x28, 0x68, 0xa8, 0xe8, 0x38, 0x78, 0xb8, 0xf8,
 0x0c, 0x4c, 0x8c, 0xcc, 0x1c, 0x5c, 0x9c, 0xdc,
 0x2c, 0x6c, 0xac, 0xec, 0x3c, 0x7c, 0xbc, 0xfc,
 0x01, 0x41, 0x81, 0xc1, 0x11, 0x51, 0x91, 0xd1,
 0x21, 0x61, 0xa1, 0xe1, 0x31, 0x71, 0xb1, 0xf1,
 0x05, 0x45, 0x85, 0xc5, 0x15, 0x55, 0x95, 0xd5,
 0x25, 0x65, 0xa5, 0xe5, 0x35, 0x75, 0xb5, 0xf5,
 0x09, 0x49, 0x89, 0xc9, 0x19, 0x59, 0x99, 0xd9,
 0x29, 0x69, 0xa9, 0xe9, 0x39, 0x79, 0xb9, 0xf9,
 0x0d, 0x4d, 0x8d, 0xcd, 0x1d, 0x5d, 0x9d, 0xdd,
 0x2d, 0x6d, 0xad, 0xed, 0x3d, 0x7d, 0xbd, 0xfd,
 0x02, 0x42, 0x82, 0xc2, 0x12, 0x52, 0x92, 0xd2,
 0x22, 0x62, 0xa2, 0xe2, 0x32, 0x72, 0xb2, 0xf2,
 0x06, 0x46, 0x86, 0xc6, 0x16, 0x56, 0x96, 0xd6,
 0x26, 0x66, 0xa6, 0xe6, 0x36, 0x76, 0xb6, 0xf6,
 0x0a, 0x4a, 0x8a, 0xca, 0x1a, 0x5a, 0x9a, 0xda,
 0x2a, 0x6a, 0xaa, 0xea, 0x3a, 0x7a, 0xba, 0xfa,
 0x0e, 0x4e, 0x8e, 0xce, 0x1e, 0x5e, 0x9e, 0xde,
 0x2e, 0x6e, 0xae, 0xee, 0x3e, 0x7e, 0xbe, 0xfe,
 0x03, 0x43, 0x83, 0xc3, 0x13, 0x53, 0x93, 0xd3,
 0x23, 0x63, 0xa3, 0xe3, 0x33, 0x73, 0xb3, 0xf3,
 0x07, 0x47, 0x87, 0xc7, 0x17, 0x57, 0x97, 0xd7,
 0x27, 0x67, 0xa7, 0xe7, 0x37, 0x77, 0xb7, 0xf7,
 0x0b, 0x4b, 0x8b, 0xcb, 0x1b, 0x5b, 0x9b, 0xdb,
 0x2b, 0x6b, 0xab, 0xeb, 0x3b, 0x7b, 0xbb, 0xfb,
 0x0f, 0x4f, 0x8f, 0xcf, 0x1f, 0x5f, 0x9f, 0xdf,
 0x2f, 0x6f, 0xaf, 0xef, 0x3f, 0x7f, 0xbf, 0xff,
};
uchar revtab2[] = {
 0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70,
 0x80, 0x90, 0xa0, 0xb0, 0xc0, 0xd0, 0xe0, 0xf0,
 0x01, 0x11, 0x21, 0x31, 0x41, 0x51, 0x61, 0x71,
 0x81, 0x91, 0xa1, 0xb1, 0xc1, 0xd1, 0xe1, 0xf1,
 0x02, 0x12, 0x22, 0x32, 0x42, 0x52, 0x62, 0x72,
 0x82, 0x92, 0xa2, 0xb2, 0xc2, 0xd2, 0xe2, 0xf2,
 0x03, 0x13, 0x23, 0x33, 0x43, 0x53, 0x63, 0x73,
 0x83, 0x93, 0xa3, 0xb3, 0xc3, 0xd3, 0xe3, 0xf3,
 0x04, 0x14, 0x24, 0x34, 0x44, 0x54, 0x64, 0x74,
 0x84, 0x94, 0xa4, 0xb4, 0xc4, 0xd4, 0xe4, 0xf4,
 0x05, 0x15, 0x25, 0x35, 0x45, 0x55, 0x65, 0x75,
 0x85, 0x95, 0xa5, 0xb5, 0xc5, 0xd5, 0xe5, 0xf5,
 0x06, 0x16, 0x26, 0x36, 0x46, 0x56, 0x66, 0x76,
 0x86, 0x96, 0xa6, 0xb6, 0xc6, 0xd6, 0xe6, 0xf6,
 0x07, 0x17, 0x27, 0x37, 0x47, 0x57, 0x67, 0x77,
 0x87, 0x97, 0xa7, 0xb7, 0xc7, 0xd7, 0xe7, 0xf7,
 0x08, 0x18, 0x28, 0x38, 0x48, 0x58, 0x68, 0x78,
 0x88, 0x98, 0xa8, 0xb8, 0xc8, 0xd8, 0xe8, 0xf8,
 0x09, 0x19, 0x29, 0x39, 0x49, 0x59, 0x69, 0x79,
 0x89, 0x99, 0xa9, 0xb9, 0xc9, 0xd9, 0xe9, 0xf9,
 0x0a, 0x1a, 0x2a, 0x3a, 0x4a, 0x5a, 0x6a, 0x7a,
 0x8a, 0x9a, 0xaa, 0xba, 0xca, 0xda, 0xea, 0xfa,
 0x0b, 0x1b, 0x2b, 0x3b, 0x4b, 0x5b, 0x6b, 0x7b,
 0x8b, 0x9b, 0xab, 0xbb, 0xcb, 0xdb, 0xeb, 0xfb,
 0x0c, 0x1c, 0x2c, 0x3c, 0x4c, 0x5c, 0x6c, 0x7c,
 0x8c, 0x9c, 0xac, 0xbc, 0xcc, 0xdc, 0xec, 0xfc,
 0x0d, 0x1d, 0x2d, 0x3d, 0x4d, 0x5d, 0x6d, 0x7d,
 0x8d, 0x9d, 0xad, 0xbd, 0xcd, 0xdd, 0xed, 0xfd,
 0x0e, 0x1e, 0x2e, 0x3e, 0x4e, 0x5e, 0x6e, 0x7e,
 0x8e, 0x9e, 0xae, 0xbe, 0xce, 0xde, 0xee, 0xfe,
 0x0f, 0x1f, 0x2f, 0x3f, 0x4f, 0x5f, 0x6f, 0x7f,
 0x8f, 0x9f, 0xaf, 0xbf, 0xcf, 0xdf, 0xef, 0xff,
};

void
pixreverse(uchar *p, int len, int ldepth)
{
	uchar *e;
	uchar *tab;

	switch(ldepth){
	case 0:
		tab = revtab0;
		break;
	case 1:
		tab = revtab1;
		break;
	case 2:
		tab = revtab2;
		break;
	default:
		return;
	}

	for(e = p + len; p < e; p++)
		*p = tab[*p];
}
