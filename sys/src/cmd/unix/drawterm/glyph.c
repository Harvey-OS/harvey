#include "lib9.h"
#include "bitblt.h"

enum
{
	/* starting values */
	LOG2NFCACHE =	6,
	NFLOOK =	5,				/* #chars to scan in cache */
	NFCACHE = 	((1<<LOG2NFCACHE)+NFLOOK),	/* #chars cached */
	/* max value */
	MAXFCACHE =	(1<<16)+NFLOOK,	/* generous upper limit */
};

typedef struct Runecache	Runecache;
typedef struct Centry		Centry;

struct Centry
{
	uint		age;
	uchar		height;
	uchar		ascent;	
	uint		id;	/* id of sub font */
	int		off;	/* offset within subfont */
	int		x;	/* offset in image */
	uchar		top;	/* first non-zero scan-line */
	uchar		bottom;	/* last non-zero scan-line + 1 */
	uchar		left;	/* first non-zero bit */
	uchar		right;	/* last non-zero bit + 1 */
};

struct Runecache {
	uint		age;
	int		width;		/* largest seen so far */
	int		nentry;
	Centry		*entry;
	Bitmap		*b;
};

static Centry	*loadchar(Runecache*, Glyph *, int, int);
static uint	agecache(Runecache*);
static void	resize(Runecache*, int, int);

/* one cache for each power of two for font hieght and each ldepth */
Runecache	cache[8][4];

void
glyph(Bitmap *b, Point pt, int height, int ascent, Glyph g, int code)
{
	Runecache *rc;
	int i, full;
	Centry *c;
	int width;
	Rectangle r;

	for(i = 0; i < 7; i++)
		if((1<<i) >= height)
			break;

	rc = &cache[i][b->ldepth];

	if(rc->b == 0){
		rc->b = balloc(Rect(0,0,1,1<<i), b->ldepth);
		resize(rc, g.f->info[g.off].width, NFCACHE);
	}
		
	full = (code&~S)^(D&~S);	/* result involves source */
	width = g.f->info[g.off].width;
	c = loadchar(rc, &g, height, ascent);
	if (full) {
		r = Rect(c->x, 0, c->x+width, height);
		bitblt(b, pt, rc->b, r, code);
	} else {
		r = Rect(c->x+c->left, c->top, c->x+c->right, c->bottom);
		bitblt(b, Pt(pt.x+c->left, pt.y+c->top), rc->b, r, code);
	}
}

static Centry*
loadchar(Runecache *rc, Glyph *g, int height, int ascent)
{
	uint sh, h, th;
	Centry *c, *ec, *tc;
	int width, off, nc;
	Fontchar *fc;
	uint a;
	Rectangle r;

	fc = &g->f->info[g->off];
	
	width = fc->width;

	if(width > rc->width)
		resize(rc, width, rc->nentry);

	sh = ((g->f->qid*17 + g->off)*17 + height+ascent) & (rc->nentry-NFLOOK-1);
	c = &rc->entry[sh];
	ec = c+NFLOOK;
	for(;c < ec; c++)
		if(c->height == height && c->ascent == ascent && c->id == g->f->qid &&
				c->off == g->off) {
			c->age = agecache(rc);
			return c;
		}

	/*
	 * Not found; toss out oldest entry
	 */
	a = ~0U;
	th = sh;
	h = 0;
	tc = &rc->entry[th];
	while(tc < ec){
		if(tc->age < a){
			a = tc->age;
			h = th;
			c = tc;
		}
		tc++;
		th++;
	}

	if(a>0 && (rc->age-a)<500){	/* kicking out too recent; resize */
		nc = 2*(rc->nentry-NFLOOK) + NFLOOK;
		if(nc <= MAXFCACHE) {
			resize(rc, rc->width, nc);
			return loadchar(rc, g, height, ascent);
		}
	}
	c->height = height;
	c->ascent = ascent;
	c->id = g->f->qid;
	c->off = g->off;
	c->x = h*rc->width;

	off = ascent - g->f->ascent;

	if(fc->left > 0)
		c->left = fc->left;
	else
		c->left = 0;

	c->right = fc[1].x-fc[0].x + fc->left;
	if(c->right > width)
		c->right = width;

	if(fc->top+off >= 0)
		c->top = c->top;
	else
		c->top = 0;

	if(fc->bottom+off <= height)
		c->bottom = fc->bottom+off;
	else
		c->bottom = g->f->height;

	bitblt(rc->b, Pt(c->x, 0), rc->b, Rect(0, 0, width, height), Zero);
	setclipr(rc->b, Rect(c->x, 0, c->x+width, height));
	r = Rect(fc->x, fc->top, fc[1].x, fc->bottom);
	bitblt(rc->b, Pt(c->x+fc->left, fc->top+off), g->f->bits, r, S);
	setclipr(rc->b, rc->b->r);

	c->age = agecache(rc);
	return c;
}

static uint
agecache(Runecache *rc)
{
	Centry *c, *ec;

	rc->age++;
	if(rc->age < (1<<31))
		return rc->age;

	/*
	 * Renormalize ages
	 */
	c = rc->entry;
	ec = c+rc->nentry;
	for(;c < ec; c++)
		if(c->age > 1)
			c->age >>= 2;
	return rc->age >>=2;
}

static void
resize(Runecache *rc, int wid, int nentry)
{
	int ldepth, height;

	ldepth = rc->b->ldepth;
	height = rc->b->r.max.y;

	bfree(rc->b);
	rc->b = balloc(Rect(0, 0, nentry*wid, height), ldepth);
	rc->width = wid;
	rc->entry = realloc(rc->entry, nentry*sizeof(Centry));
	rc->nentry = nentry;
	/* just wipe the cache clean and things will be ok */
	memset(rc->entry, 0, rc->nentry*sizeof(Centry));
}
