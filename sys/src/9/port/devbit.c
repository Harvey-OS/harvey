#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

#include	"devtab.h"

#include	<libg.h>
#include	<gnot.h>
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
void	cursoron(int);
void	cursoroff(int);
int	mousechanged(void*);

enum{
	Qdir,
	Qbitblt,
	Qmouse,
	Qscreen,
};

Dirtab bitdir[]={
	"bitblt",	{Qbitblt},	0,			0666,
	"mouse",	{Qmouse},	0,			0666,
	"screen",	{Qscreen},	0,			0444,
};

#define	NBIT	(sizeof bitdir/sizeof(Dirtab))
#define	NINFO	8192	/* max chars per subfont; sanity check only */
#define	HDR	3

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

long
bitwrite(Chan *c, void *va, long n, ulong offset)
{
	uchar *p, *q, *oq;
	long m, v, miny, maxy, t, x, y;
	ulong l, nw, ws, rv, q0, q1;
	ulong *lp;
	int off, isoff, i, j, ok;
	Point pt, pt1, pt2;
	Rectangle rect;
	Cursor curs;
	Fcode fc;
	Fontchar *fcp;
	GBitmap *src, *dst;
	BSubfont *f, *tf, **fp;
	GFont *ff, **ffp;
	GCacheinfo *gc;

	if(!conf.monitor)
		error(Egreg);
	USED(offset);

	if(c->qid.path == CHDIR)
		error(Eisdir);

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
			m -= 31;
			p += 31;
			break;

		case 'c':
			/*
			 * cursorswitch
			 *	'c'		1
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
			if(m < 73)
				error(Ebadblt);
			curs.offset.x = BGLONG(p+1);
			curs.offset.y = BGLONG(p+5);
			memmove(curs.clr, p+9, 2*16);
			memmove(curs.set, p+41, 2*16);
			if(!isoff){
				cursoroff(1);
				isoff = 1;
			}
			Cursortocursor(&curs);
			m -= 73;
			p += 73;
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
			bit.subfont[i] = f;
			f->info = bitmalloc((v+1)*sizeof(Fontchar));
			if(f->info == 0){
				free(f);
				error(Enomem);
			}
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
				if(v==0 && flipping){	/* flip bits */
					/* we know it's all word aligned */
					lp = (ulong*)oq;
					for(x=0; x<l; x+=sizeof(ulong))
						*lp++ ^= ~0;
				}
				p += l;
				m -= l;
			}
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
				ok &= setcolor(i, BGLONG(p), BGLONG(p+4), BGLONG(p+8));
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
	if(dolock)
		lock(&cursor);
	if(cursor.visible++ == 0){
		cursor.r.min = mouse.xy;
		cursor.r.max = add(mouse.xy, Pt(16, 16));
		cursor.r = raddp(cursor.r, cursor.offset);
		gbitblt(&cursorback, Pt(0, 0), &gscreen, cursor.r, S);
		gbitblt(&gscreen, add(mouse.xy, cursor.offset),
			&clr, Rect(0, 0, 16, 16), flipping? flipD[D&~S] : D&~S);
		gbitblt(&gscreen, add(mouse.xy, cursor.offset),
			&set, Rect(0, 0, 16, 16), flipping? flipD[S|D] : S|D);
	}
	if(dolock)
		unlock(&cursor);
}

void
cursoroff(int dolock)
{
	if(dolock)
		lock(&cursor);
	if(--cursor.visible == 0)
		gbitblt(&gscreen, cursor.r.min, &cursorback, Rect(0, 0, 16, 16), S);
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

int
mouseputc(IOQ *q, int c)
{
	static short msg[5];
	static int nb;
	static uchar b[] = {0, 4, 2, 6, 1, 5, 3, 7};

	USED(q);
	if((c&0xF0) == 0x80)
		nb=0;
	msg[nb] = c;
	if(c & 0x80)
		msg[nb] |= ~0xFF;	/* sign extend */
	if(++nb == 5){
		mouse.newbuttons = b[(msg[0]&7)^7];
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
