#include	"u.h"
#include	"lib.h"
#include	"dat.h"
#include	"fns.h"
#include	"error.h"

#define	Image	IMAGE
#include	<draw.h>
#include	<memdraw.h>
#include	<memlayer.h>
#include	<cursor.h>
#include	"screen.h"

enum
{
	Qtopdir		= 0,
	Qnew,
	Q3rd,
	Q2nd,
	Qcolormap,
	Qctl,
	Qdata,
	Qrefresh,
};

/*
 * Qid path is:
 *	 4 bits of file type (qids above)
 *	24 bits of mux slot number +1; 0 means not attached to client
 */
#define	QSHIFT	4	/* location in qid of client # */

#define	QID(q)		((((ulong)(q).path)&0x0000000F)>>0)
#define	CLIENTPATH(q)	((((ulong)q)&0x7FFFFFF0)>>QSHIFT)
#define	CLIENT(q)	CLIENTPATH((q).path)

#define	NHASH		(1<<5)
#define	HASHMASK	(NHASH-1)
#define	IOUNIT	(64*1024)

typedef struct Client Client;
typedef struct Draw Draw;
typedef struct DImage DImage;
typedef struct DScreen DScreen;
typedef struct CScreen CScreen;
typedef struct FChar FChar;
typedef struct Refresh Refresh;
typedef struct Refx Refx;
typedef struct DName DName;

ulong blanktime = 30;	/* in minutes; a half hour */

struct Draw
{
	QLock	lk;
	int		clientid;
	int		nclient;
	Client**	client;
	int		nname;
	DName*	name;
	int		vers;
	int		softscreen;
	int		blanked;	/* screen turned off */
	ulong		blanktime;	/* time of last operation */
	ulong		savemap[3*256];
};

struct Client
{
	Ref		r;
	DImage*		dimage[NHASH];
	CScreen*	cscreen;
	Refresh*	refresh;
	Rendez		refrend;
	uchar*		readdata;
	int		nreaddata;
	int		busy;
	int		clientid;
	int		slot;
	int		refreshme;
	int		infoid;
	int		op;
};

struct Refresh
{
	DImage*		dimage;
	Rectangle	r;
	Refresh*	next;
};

struct Refx
{
	Client*		client;
	DImage*		dimage;
};

struct DName
{
	char			*name;
	Client	*client;
	DImage*		dimage;
	int			vers;
};

struct FChar
{
	int		minx;	/* left edge of bits */
	int		maxx;	/* right edge of bits */
	uchar		miny;	/* first non-zero scan-line */
	uchar		maxy;	/* last non-zero scan-line + 1 */
	schar		left;	/* offset of baseline */
	uchar		width;	/* width of baseline */
};

/*
 * Reference counts in DImages:
 *	one per open by original client
 *	one per screen image or fill
 * 	one per image derived from this one by name
 */
struct DImage
{
	int		id;
	int		ref;
	char		*name;
	int		vers;
	Memimage*	image;
	int		ascent;
	int		nfchar;
	FChar*		fchar;
	DScreen*	dscreen;	/* 0 if not a window */
	DImage*	fromname;	/* image this one is derived from, by name */
	DImage*		next;
};

struct CScreen
{
	DScreen*	dscreen;
	CScreen*	next;
};

struct DScreen
{
	int		id;
	int		public;
	int		ref;
	DImage	*dimage;
	DImage	*dfill;
	Memscreen*	screen;
	Client*		owner;
	DScreen*	next;
};

static	Draw		sdraw;
static	Memimage	*screenimage;
static	Memdata	screendata;
static	Rectangle	flushrect;
static	int		waste;
static	DScreen*	dscreen;
extern	void		flushmemscreen(Rectangle);
	void		drawmesg(Client*, void*, int);
	void		drawuninstall(Client*, int);
	void		drawfreedimage(DImage*);
	Client*		drawclientofpath(ulong);

static	char Enodrawimage[] =	"unknown id for draw image";
static	char Enodrawscreen[] =	"unknown id for draw screen";
static	char Eshortdraw[] =	"short draw message";
static	char Eshortread[] =	"draw read too short";
static	char Eimageexists[] =	"image id in use";
static	char Escreenexists[] =	"screen id in use";
static	char Edrawmem[] =	"image memory allocation failed";
static	char Ereadoutside[] =	"readimage outside image";
static	char Ewriteoutside[] =	"writeimage outside image";
static	char Enotfont[] =	"image not a font";
static	char Eindex[] =		"character index out of range";
static	char Enoclient[] =	"no such draw client";
/* static	char Edepth[] =	"image has bad depth"; */
static	char Enameused[] =	"image name in use";
static	char Enoname[] =	"no image with that name";
static	char Eoldname[] =	"named image no longer valid";
static	char Enamed[] = 	"image already has name";
static	char Ewrongname[] = 	"wrong name for image";

int
drawcanqlock(void)
{
	return canqlock(&sdraw.lk);
}

void
drawqlock(void)
{
	qlock(&sdraw.lk);
}

void
drawqunlock(void)
{
	qunlock(&sdraw.lk);
}

static int
drawgen(Chan *c, char *name, Dirtab *dt, int ndt, int s, Dir *dp)
{
	int t;
	Qid q;
	ulong path;
	Client *cl;

	USED(name);
	USED(dt);
	USED(ndt);

	q.vers = 0;

	if(s == DEVDOTDOT){
		switch(QID(c->qid)){
		case Qtopdir:
		case Q2nd:
			mkqid(&q, Qtopdir, 0, QTDIR);
			devdir(c, q, "#i", 0, eve, 0500, dp);
			break;
		case Q3rd:
			cl = drawclientofpath(c->qid.path);
			if(cl == nil)
				strcpy(up->genbuf, "??");
			else
				sprint(up->genbuf, "%d", cl->clientid);
			mkqid(&q, Q2nd, 0, QTDIR);
			devdir(c, q, up->genbuf, 0, eve, 0500, dp);
			break;
		default:
			panic("drawwalk %llux", c->qid.path);
		}
		return 1;
	}

	/*
	 * Top level directory contains the name of the device.
	 */
	t = QID(c->qid);
	if(t == Qtopdir){
		switch(s){
		case 0:
			mkqid(&q, Q2nd, 0, QTDIR);
			devdir(c, q, "draw", 0, eve, 0555, dp);
			break;
		default:
			return -1;
		}
		return 1;
	}

	/*
	 * Second level contains "new" plus all the clients.
	 */
	if(t == Q2nd || t == Qnew){
		if(s == 0){
			mkqid(&q, Qnew, 0, QTFILE);
			devdir(c, q, "new", 0, eve, 0666, dp);
		}
		else if(s <= sdraw.nclient){
			cl = sdraw.client[s-1];
			if(cl == 0)
				return 0;
			sprint(up->genbuf, "%d", cl->clientid);
			mkqid(&q, (s<<QSHIFT)|Q3rd, 0, QTDIR);
			devdir(c, q, up->genbuf, 0, eve, 0555, dp);
			return 1;
		}
		else
			return -1;
		return 1;
	}

	/*
	 * Third level.
	 */
	path = c->qid.path&~((1<<QSHIFT)-1);	/* slot component */
	q.vers = c->qid.vers;
	q.type = QTFILE;
	switch(s){
	case 0:
		q.path = path|Qcolormap;
		devdir(c, q, "colormap", 0, eve, 0600, dp);
		break;
	case 1:
		q.path = path|Qctl;
		devdir(c, q, "ctl", 0, eve, 0600, dp);
		break;
	case 2:
		q.path = path|Qdata;
		devdir(c, q, "data", 0, eve, 0600, dp);
		break;
	case 3:
		q.path = path|Qrefresh;
		devdir(c, q, "refresh", 0, eve, 0400, dp);
		break;
	default:
		return -1;
	}
	return 1;
}

static
int
drawrefactive(void *a)
{
	Client *c;

	c = a;
	return c->refreshme || c->refresh!=0;
}

static
void
drawrefreshscreen(DImage *l, Client *client)
{
	while(l != nil && l->dscreen == nil)
		l = l->fromname;
	if(l != nil && l->dscreen->owner != client)
		l->dscreen->owner->refreshme = 1;
}

static
void
drawrefresh(Memimage *m, Rectangle r, void *v)
{
	Refx *x;
	DImage *d;
	Client *c;
	Refresh *ref;

	USED(m);

	if(v == 0)
		return;
	x = v;
	c = x->client;
	d = x->dimage;
	for(ref=c->refresh; ref; ref=ref->next)
		if(ref->dimage == d){
			combinerect(&ref->r, r);
			return;
		}
	ref = malloc(sizeof(Refresh));
	if(ref){
		ref->dimage = d;
		ref->r = r;
		ref->next = c->refresh;
		c->refresh = ref;
	}
}

static void
addflush(Rectangle r)
{
	int abb, ar, anbb;
	Rectangle nbb;

	if(sdraw.softscreen==0 || !rectclip(&r, screenimage->r))
		return;

	if(flushrect.min.x >= flushrect.max.x){
		flushrect = r;
		waste = 0;
		return;
	}
	nbb = flushrect;
	combinerect(&nbb, r);
	ar = Dx(r)*Dy(r);
	abb = Dx(flushrect)*Dy(flushrect);
	anbb = Dx(nbb)*Dy(nbb);
	/*
	 * Area of new waste is area of new bb minus area of old bb,
	 * less the area of the new segment, which we assume is not waste.
	 * This could be negative, but that's OK.
	 */
	waste += anbb-abb - ar;
	if(waste < 0)
		waste = 0;
	/*
	 * absorb if:
	 *	total area is small
	 *	waste is less than half total area
	 * 	rectangles touch
	 */
	if(anbb<=1024 || waste*2<anbb || rectXrect(flushrect, r)){
		flushrect = nbb;
		return;
	}
	/* emit current state */
	if(flushrect.min.x < flushrect.max.x)
		flushmemscreen(flushrect);
	flushrect = r;
	waste = 0;
}

static
void
dstflush(int dstid, Memimage *dst, Rectangle r)
{
	Memlayer *l;

	if(dstid == 0){
		combinerect(&flushrect, r);
		return;
	}
	/* how can this happen? -rsc, dec 12 2002 */
	if(dst == 0){
		print("nil dstflush\n");
		return;
	}
	l = dst->layer;
	if(l == nil)
		return;
	do{
		if(l->screen->image->data != screenimage->data)
			return;
		r = rectaddpt(r, l->delta);
		l = l->screen->image->layer;
	}while(l);
	addflush(r);
}

void
drawflush(void)
{
	if(flushrect.min.x < flushrect.max.x)
		flushmemscreen(flushrect);
	flushrect = Rect(10000, 10000, -10000, -10000);
}

void
drawflushr(Rectangle r)
{
	qlock(&sdraw.lk);
	flushmemscreen(r);
	qunlock(&sdraw.lk);
}

static
int
drawcmp(char *a, char *b, int n)
{
	if(strlen(a) != n)
		return 1;
	return memcmp(a, b, n);
}

DName*
drawlookupname(int n, char *str)
{
	DName *name, *ename;

	name = sdraw.name;
	ename = &name[sdraw.nname];
	for(; name<ename; name++)
		if(drawcmp(name->name, str, n) == 0)
			return name;
	return 0;
}

int
drawgoodname(DImage *d)
{
	DName *n;

	/* if window, validate the screen's own images */
	if(d->dscreen)
		if(drawgoodname(d->dscreen->dimage) == 0
		|| drawgoodname(d->dscreen->dfill) == 0)
			return 0;
	if(d->name == nil)
		return 1;
	n = drawlookupname(strlen(d->name), d->name);
	if(n==nil || n->vers!=d->vers)
		return 0;
	return 1;
}

DImage*
drawlookup(Client *client, int id, int checkname)
{
	DImage *d;

	d = client->dimage[id&HASHMASK];
	while(d){
		if(d->id == id){
			if(checkname && !drawgoodname(d))
				error(Eoldname);
			return d;
		}
		d = d->next;
	}
	return 0;
}

DScreen*
drawlookupdscreen(int id)
{
	DScreen *s;

	s = dscreen;
	while(s){
		if(s->id == id)
			return s;
		s = s->next;
	}
	return 0;
}

DScreen*
drawlookupscreen(Client *client, int id, CScreen **cs)
{
	CScreen *s;

	s = client->cscreen;
	while(s){
		if(s->dscreen->id == id){
			*cs = s;
			return s->dscreen;
		}
		s = s->next;
	}
	error(Enodrawscreen);
	return 0;
}

Memimage*
drawinstall(Client *client, int id, Memimage *i, DScreen *dscreen)
{
	DImage *d;

	d = malloc(sizeof(DImage));
	if(d == 0)
		return 0;
	d->id = id;
	d->ref = 1;
	d->name = 0;
	d->vers = 0;
	d->image = i;
	d->nfchar = 0;
	d->fchar = 0;
	d->fromname = 0;
	d->dscreen = dscreen;
	d->next = client->dimage[id&HASHMASK];
	client->dimage[id&HASHMASK] = d;
	return i;
}

Memscreen*
drawinstallscreen(Client *client, DScreen *d, int id, DImage *dimage, DImage *dfill, int public)
{
	Memscreen *s;
	CScreen *c;

	c = malloc(sizeof(CScreen));
	if(dimage && dimage->image && dimage->image->chan == 0)
		panic("bad image %p in drawinstallscreen", dimage->image);

	if(c == 0)
		return 0;
	if(d == 0){
		d = malloc(sizeof(DScreen));
		if(d == 0){
			free(c);
			return 0;
		}
		s = malloc(sizeof(Memscreen));
		if(s == 0){
			free(c);
			free(d);
			return 0;
		}
		s->frontmost = 0;
		s->rearmost = 0;
		d->dimage = dimage;
		if(dimage){
			s->image = dimage->image;
			dimage->ref++;
		}
		d->dfill = dfill;
		if(dfill){
			s->fill = dfill->image;
			dfill->ref++;
		}
		d->ref = 0;
		d->id = id;
		d->screen = s;
		d->public = public;
		d->next = dscreen;
		d->owner = client;
		dscreen = d;
	}
	c->dscreen = d;
	d->ref++;
	c->next = client->cscreen;
	client->cscreen = c;
	return d->screen;
}

void
drawdelname(DName *name)
{
	int i;

	i = name-sdraw.name;
	memmove(name, name+1, (sdraw.nname-(i+1))*sizeof(DName));
	sdraw.nname--;
}

void
drawfreedscreen(DScreen *this)
{
	DScreen *ds, *next;

	this->ref--;
	if(this->ref < 0)
		print("negative ref in drawfreedscreen\n");
	if(this->ref > 0)
		return;
	ds = dscreen;
	if(ds == this){
		dscreen = this->next;
		goto Found;
	}
	while((next = ds->next)){	/* assign = */
		if(next == this){
			ds->next = this->next;
			goto Found;
		}
		ds = next;
	}
	error(Enodrawimage);

    Found:
	if(this->dimage)
		drawfreedimage(this->dimage);
	if(this->dfill)
		drawfreedimage(this->dfill);
	free(this->screen);
	free(this);
}

void
drawfreedimage(DImage *dimage)
{
	int i;
	Memimage *l;
	DScreen *ds;

	dimage->ref--;
	if(dimage->ref < 0)
		print("negative ref in drawfreedimage\n");
	if(dimage->ref > 0)
		return;

	/* any names? */
	for(i=0; i<sdraw.nname; )
		if(sdraw.name[i].dimage == dimage)
			drawdelname(sdraw.name+i);
		else
			i++;
	if(dimage->fromname){	/* acquired by name; owned by someone else*/
		drawfreedimage(dimage->fromname);
		goto Return;
	}
	if(dimage->image == screenimage)	/* don't free the display */
		goto Return;
	ds = dimage->dscreen;
	if(ds){
		l = dimage->image;
		if(l->data == screenimage->data)
			addflush(l->layer->screenr);
		if(l->layer->refreshfn == drawrefresh)	/* else true owner will clean up */
			free(l->layer->refreshptr);
		l->layer->refreshptr = nil;
		if(drawgoodname(dimage))
			memldelete(l);
		else
			memlfree(l);
		drawfreedscreen(ds);
	}else
		freememimage(dimage->image);
    Return:
	free(dimage->fchar);
	free(dimage);
}

void
drawuninstallscreen(Client *client, CScreen *this)
{
	CScreen *cs, *next;

	cs = client->cscreen;
	if(cs == this){
		client->cscreen = this->next;
		drawfreedscreen(this->dscreen);
		free(this);
		return;
	}
	while((next = cs->next)){	/* assign = */
		if(next == this){
			cs->next = this->next;
			drawfreedscreen(this->dscreen);
			free(this);
			return;
		}
		cs = next;
	}
}

void
drawuninstall(Client *client, int id)
{
	DImage *d, *next;

	d = client->dimage[id&HASHMASK];
	if(d == 0)
		error(Enodrawimage);
	if(d->id == id){
		client->dimage[id&HASHMASK] = d->next;
		drawfreedimage(d);
		return;
	}
	while((next = d->next)){	/* assign = */
		if(next->id == id){
			d->next = next->next;
			drawfreedimage(next);
			return;
		}
		d = next;
	}
	error(Enodrawimage);
}

void
drawaddname(Client *client, DImage *di, int n, char *str)
{
	DName *name, *ename, *new, *t;

	name = sdraw.name;
	ename = &name[sdraw.nname];
	for(; name<ename; name++)
		if(drawcmp(name->name, str, n) == 0)
			error(Enameused);
	t = smalloc((sdraw.nname+1)*sizeof(DName));
	memmove(t, sdraw.name, sdraw.nname*sizeof(DName));
	free(sdraw.name);
	sdraw.name = t;
	new = &sdraw.name[sdraw.nname++];
	new->name = smalloc(n+1);
	memmove(new->name, str, n);
	new->name[n] = 0;
	new->dimage = di;
	new->client = client;
	new->vers = ++sdraw.vers;
}

Client*
drawnewclient(void)
{
	Client *cl, **cp;
	int i;

	for(i=0; i<sdraw.nclient; i++){
		cl = sdraw.client[i];
		if(cl == 0)
			break;
	}
	if(i == sdraw.nclient){
		cp = malloc((sdraw.nclient+1)*sizeof(Client*));
		if(cp == 0)
			return 0;
		memmove(cp, sdraw.client, sdraw.nclient*sizeof(Client*));
		free(sdraw.client);
		sdraw.client = cp;
		sdraw.nclient++;
		cp[i] = 0;
	}
	cl = malloc(sizeof(Client));
	if(cl == 0)
		return 0;
	memset(cl, 0, sizeof(Client));
	cl->slot = i;
	cl->clientid = ++sdraw.clientid;
	cl->op = SoverD;
	sdraw.client[i] = cl;
	return cl;
}

static int
drawclientop(Client *cl)
{
	int op;

	op = cl->op;
	cl->op = SoverD;
	return op;
}

int
drawhasclients(void)
{
	/*
	 * if draw has ever been used, we can't resize the frame buffer,
	 * even if all clients have exited (nclients is cumulative); it's too
	 * hard to make work.
	 */
	return sdraw.nclient != 0;
}

Client*
drawclientofpath(ulong path)
{
	Client *cl;
	int slot;

	slot = CLIENTPATH(path);
	if(slot == 0)
		return nil;
	cl = sdraw.client[slot-1];
	if(cl==0 || cl->clientid==0)
		return nil;
	return cl;
}


Client*
drawclient(Chan *c)
{
	Client *client;

	client = drawclientofpath(c->qid.path);
	if(client == nil)
		error(Enoclient);
	return client;
}

Memimage*
drawimage(Client *client, uchar *a)
{
	DImage *d;

	d = drawlookup(client, BGLONG(a), 1);
	if(d == nil)
		error(Enodrawimage);
	return d->image;
}

void
drawrectangle(Rectangle *r, uchar *a)
{
	r->min.x = BGLONG(a+0*4);
	r->min.y = BGLONG(a+1*4);
	r->max.x = BGLONG(a+2*4);
	r->max.y = BGLONG(a+3*4);
}

void
drawpoint(Point *p, uchar *a)
{
	p->x = BGLONG(a+0*4);
	p->y = BGLONG(a+1*4);
}

#define isvgascreen(dst) 1


Point
drawchar(Memimage *dst, Memimage *rdst, Point p, 
	Memimage *src, Point *sp, DImage *font, int index, int op)
{
	FChar *fc;
	Rectangle r;
	Point sp1;
	static Memimage *tmp;

	fc = &font->fchar[index];
	r.min.x = p.x+fc->left;
	r.min.y = p.y-(font->ascent-fc->miny);
	r.max.x = r.min.x+(fc->maxx-fc->minx);
	r.max.y = r.min.y+(fc->maxy-fc->miny);
	sp1.x = sp->x+fc->left;
	sp1.y = sp->y+fc->miny;
	
	/*
	 * If we're drawing greyscale fonts onto a VGA screen,
	 * it's very costly to read the screen memory to do the
	 * alpha blending inside memdraw.  If this is really a stringbg,
	 * then rdst is the bg image (in main memory) which we can
	 * refer to for the underlying dst pixels instead of reading dst
	 * directly.
	 */
	if(1 || (isvgascreen(dst) && !isvgascreen(rdst) /*&& font->image->depth > 1*/)){
		if(tmp == nil || tmp->chan != dst->chan || Dx(tmp->r) < Dx(r) || Dy(tmp->r) < Dy(r)){
			if(tmp)
				freememimage(tmp);
			tmp = allocmemimage(Rect(0,0,Dx(r),Dy(r)), dst->chan);
			if(tmp == nil)
				goto fallback;
		}
		memdraw(tmp, Rect(0,0,Dx(r),Dy(r)), rdst, r.min, memopaque, ZP, S);
		memdraw(tmp, Rect(0,0,Dx(r),Dy(r)), src, sp1, font->image, Pt(fc->minx, fc->miny), op);
		memdraw(dst, r, tmp, ZP, memopaque, ZP, S);
	}else{
	fallback:
		memdraw(dst, r, src, sp1, font->image, Pt(fc->minx, fc->miny), op);
	}

	p.x += fc->width;
	sp->x += fc->width;
	return p;
}

static int
initscreenimage(void)
{
	int width, depth;
	ulong chan;
	void *X;
	Rectangle r;

	if(screenimage != nil)
		return 1;

	screendata.base = nil;
	screendata.bdata = attachscreen(&r, &chan, &depth, &width, &sdraw.softscreen, &X);
	if(screendata.bdata == nil && X == nil)
		return 0;
	screendata.ref = 1;

	screenimage = allocmemimaged(r, chan, &screendata, X);
	if(screenimage == nil){
		/* RSC: BUG: detach screen */
		return 0;
	}

	screenimage->width = width;
	screenimage->clipr = r;
	return 1;
}

void
deletescreenimage(void)
{
	qlock(&sdraw.lk);
	/* RSC: BUG: detach screen */
	if(screenimage)
		freememimage(screenimage);
	screenimage = nil;
	qunlock(&sdraw.lk);
}

static Chan*
drawattach(char *spec)
{
	qlock(&sdraw.lk);
	if(!initscreenimage()){
		qunlock(&sdraw.lk);
		error("no frame buffer");
	}
	qunlock(&sdraw.lk);
	return devattach('i', spec);
}

static Walkqid*
drawwalk(Chan *c, Chan *nc, char **name, int nname)
{
	if(screendata.bdata == nil)
		error("no frame buffer");
	return devwalk(c, nc, name, nname, 0, 0, drawgen);
}

static int
drawstat(Chan *c, uchar *db, int n)
{
	return devstat(c, db, n, 0, 0, drawgen);
}

static Chan*
drawopen(Chan *c, int omode)
{
	Client *cl;

	if(c->qid.type & QTDIR){
		c = devopen(c, omode, 0, 0, drawgen);
		c->iounit = IOUNIT;
	}

	qlock(&sdraw.lk);
	if(waserror()){
		qunlock(&sdraw.lk);
		nexterror();
	}

	if(QID(c->qid) == Qnew){
		cl = drawnewclient();
		if(cl == 0)
			error(Enodev);
		c->qid.path = Qctl|((cl->slot+1)<<QSHIFT);
	}

	switch(QID(c->qid)){
	case Qnew:
		break;

	case Qctl:
		cl = drawclient(c);
		if(cl->busy)
			error(Einuse);
		cl->busy = 1;
		flushrect = Rect(10000, 10000, -10000, -10000);
		drawinstall(cl, 0, screenimage, 0);
		incref(&cl->r);
		break;
	case Qcolormap:
	case Qdata:
	case Qrefresh:
		cl = drawclient(c);
		incref(&cl->r);
		break;
	}
	qunlock(&sdraw.lk);
	poperror();
	c->mode = openmode(omode);
	c->flag |= COPEN;
	c->offset = 0;
	c->iounit = IOUNIT;
	return c;
}

static void
drawclose(Chan *c)
{
	int i;
	DImage *d, **dp;
	Client *cl;
	Refresh *r;

	if(QID(c->qid) < Qcolormap)	/* Qtopdir, Qnew, Q3rd, Q2nd have no client */
		return;
	qlock(&sdraw.lk);
	if(waserror()){
		qunlock(&sdraw.lk);
		nexterror();
	}

	cl = drawclient(c);
	if(QID(c->qid) == Qctl)
		cl->busy = 0;
	if((c->flag&COPEN) && (decref(&cl->r)==0)){
		while((r = cl->refresh)){	/* assign = */
			cl->refresh = r->next;
			free(r);
		}
		/* free names */
		for(i=0; i<sdraw.nname; )
			if(sdraw.name[i].client == cl)
				drawdelname(sdraw.name+i);
			else
				i++;
		while(cl->cscreen)
			drawuninstallscreen(cl, cl->cscreen);
		/* all screens are freed, so now we can free images */
		dp = cl->dimage;
		for(i=0; i<NHASH; i++){
			while((d = *dp) != nil){
				*dp = d->next;
				drawfreedimage(d);
			}
			dp++;
		}
		sdraw.client[cl->slot] = 0;
		drawflush();	/* to erase visible, now dead windows */
		free(cl);
	}
	qunlock(&sdraw.lk);
	poperror();
}

long
drawread(Chan *c, void *a, long n, vlong off)
{
	int index, m;
	ulong red, green, blue;
	Client *cl;
	uchar *p;
	Refresh *r;
	DImage *di;
	Memimage *i;
	ulong offset = off;
	char buf[16];

	if(c->qid.type & QTDIR)
		return devdirread(c, a, n, 0, 0, drawgen);
	cl = drawclient(c);
	qlock(&sdraw.lk);
	if(waserror()){
		qunlock(&sdraw.lk);
		nexterror();
	}
	switch(QID(c->qid)){
	case Qctl:
		if(n < 12*12)
			error(Eshortread);
		if(cl->infoid < 0)
			error(Enodrawimage);
		if(cl->infoid == 0){
			i = screenimage;
			if(i == nil)
				error(Enodrawimage);
		}else{
			di = drawlookup(cl, cl->infoid, 1);
			if(di == nil)
				error(Enodrawimage);
			i = di->image;
		}
		n = sprint(a, "%11d %11d %11s %11d %11d %11d %11d %11d %11d %11d %11d %11d ",
			cl->clientid, cl->infoid, chantostr(buf, i->chan), (i->flags&Frepl)==Frepl,
			i->r.min.x, i->r.min.y, i->r.max.x, i->r.max.y,
			i->clipr.min.x, i->clipr.min.y, i->clipr.max.x, i->clipr.max.y);
		cl->infoid = -1;
		break;

	case Qcolormap:
		drawactive(1);	/* to restore map from backup */
		p = malloc(4*12*256+1);
		if(p == 0)
			error(Enomem);
		m = 0;
		for(index = 0; index < 256; index++){
			getcolor(index, &red, &green, &blue);
			m += sprint((char*)p+m, "%11d %11lud %11lud %11lud\n", index, red>>24, green>>24, blue>>24);
		}
		n = readstr(offset, a, n, (char*)p);
		free(p);
		break;

	case Qdata:
		if(cl->readdata == nil)
			error("no draw data");
		if(n < cl->nreaddata)
			error(Eshortread);
		n = cl->nreaddata;
		memmove(a, cl->readdata, cl->nreaddata);
		free(cl->readdata);
		cl->readdata = nil;
		break;

	case Qrefresh:
		if(n < 5*4)
			error(Ebadarg);
		for(;;){
			if(cl->refreshme || cl->refresh)
				break;
			qunlock(&sdraw.lk);
			if(waserror()){
				qlock(&sdraw.lk);	/* restore lock for waserror() above */
				nexterror();
			}
			sleep(&cl->refrend, drawrefactive, cl);
			poperror();
			qlock(&sdraw.lk);
		}
		p = a;
		while(cl->refresh && n>=5*4){
			r = cl->refresh;
			BPLONG(p+0*4, r->dimage->id);
			BPLONG(p+1*4, r->r.min.x);
			BPLONG(p+2*4, r->r.min.y);
			BPLONG(p+3*4, r->r.max.x);
			BPLONG(p+4*4, r->r.max.y);
			cl->refresh = r->next;
			free(r);
			p += 5*4;
			n -= 5*4;
		}
		cl->refreshme = 0;
		n = p-(uchar*)a;
	}
	qunlock(&sdraw.lk);
	poperror();
	return n;
}

void
drawwakeall(void)
{
	Client *cl;
	int i;

	for(i=0; i<sdraw.nclient; i++){
		cl = sdraw.client[i];
		if(cl && (cl->refreshme || cl->refresh))
			wakeup(&cl->refrend);
	}
}

static long
drawwrite(Chan *c, void *a, long n, vlong offset)
{
	char buf[128], *fields[4], *q;
	Client *cl;
	int i, m, red, green, blue, x;

	USED(offset);

	if(c->qid.type & QTDIR)
		error(Eisdir);
	cl = drawclient(c);
	qlock(&sdraw.lk);
	if(waserror()){
		drawwakeall();
		qunlock(&sdraw.lk);
		nexterror();
	}
	switch(QID(c->qid)){
	case Qctl:
		if(n != 4)
			error("unknown draw control request");
		cl->infoid = BGLONG((uchar*)a);
		break;

	case Qcolormap:
		drawactive(1);	/* to restore map from backup */
		m = n;
		n = 0;
		while(m > 0){
			x = m;
			if(x > sizeof(buf)-1)
				x = sizeof(buf)-1;
			q = memccpy(buf, a, '\n', x);
			if(q == 0)
				break;
			i = q-buf;
			n += i;
			a = (char*)a + i;
			m -= i;
			*q = 0;
			if(tokenize(buf, fields, nelem(fields)) != 4)
				error(Ebadarg);
			i = strtoul(fields[0], 0, 0);
			red = strtoul(fields[1], 0, 0);
			green = strtoul(fields[2], 0, 0);
			blue = strtoul(fields[3], &q, 0);
			if(fields[3] == q)
				error(Ebadarg);
			if(red>255 || green>255 || blue>255 || i<0 || i>255)
				error(Ebadarg);
			red |= red<<8;
			red |= red<<16;
			green |= green<<8;
			green |= green<<16;
			blue |= blue<<8;
			blue |= blue<<16;
			setcolor(i, red, green, blue);
		}
		break;

	case Qdata:
		drawmesg(cl, a, n);
		drawwakeall();
		break;

	default:
		error(Ebadusefd);
	}
	qunlock(&sdraw.lk);
	poperror();
	return n;
}

uchar*
drawcoord(uchar *p, uchar *maxp, int oldx, int *newx)
{
	int b, x;

	if(p >= maxp)
		error(Eshortdraw);
	b = *p++;
	x = b & 0x7F;
	if(b & 0x80){
		if(p+1 >= maxp)
			error(Eshortdraw);
		x |= *p++ << 7;
		x |= *p++ << 15;
		if(x & (1<<22))
			x |= ~0<<23;
	}else{
		if(b & 0x40)
			x |= ~0<<7;
		x += oldx;
	}
	*newx = x;
	return p;
}

static void
printmesg(char *fmt, uchar *a, int plsprnt)
{
	char buf[256];
	char *p, *q;
	int s;

	if(1|| plsprnt==0){
		SET(s);
		SET(q);
		SET(p);
		USED(fmt);
		USED(a);
		p = buf;
		USED(p);
		USED(q);
		USED(s);
		return;
	}
	q = buf;
	*q++ = *a++;
	for(p=fmt; *p; p++){
		switch(*p){
		case 'l':
			q += sprint(q, " %ld", (long)BGLONG(a));
			a += 4;
			break;
		case 'L':
			q += sprint(q, " %.8lux", (ulong)BGLONG(a));
			a += 4;
			break;
		case 'R':
			q += sprint(q, " [%d %d %d %d]", BGLONG(a), BGLONG(a+4), BGLONG(a+8), BGLONG(a+12));
			a += 16;
			break;
		case 'P':
			q += sprint(q, " [%d %d]", BGLONG(a), BGLONG(a+4));
			a += 8;
			break;
		case 'b':
			q += sprint(q, " %d", *a++);
			break;
		case 's':
			q += sprint(q, " %d", BGSHORT(a));
			a += 2;
			break;
		case 'S':
			q += sprint(q, " %.4ux", BGSHORT(a));
			a += 2;
			break;
		}
	}
	*q++ = '\n';
	*q = 0;
	iprint("%.*s", (int)(q-buf), buf);
}

void
drawmesg(Client *client, void *av, int n)
{
	int c, repl, m, y, dstid, scrnid, ni, ci, j, nw, e0, e1, op, ox, oy, oesize, esize, doflush;
	uchar *u, *a, refresh;
	char *fmt;
	ulong value, chan;
	Rectangle r, clipr;
	Point p, q, *pp, sp;
	Memimage *i, *dst, *src, *mask;
	Memimage *l, **lp;
	Memscreen *scrn;
	DImage *font, *ll, *di, *ddst, *dsrc;
	DName *dn;
	DScreen *dscrn;
	FChar *fc;
	Refx *refx;
	CScreen *cs;
	Refreshfn reffn;

	a = av;
	m = 0;
	fmt = nil;
	if(waserror()){
		if(fmt) printmesg(fmt, a, 1);
	/*	iprint("error: %s\n", up->errstr);	*/
		nexterror();
	}
	while((n-=m) > 0){
		USED(fmt);
		a += m;
		switch(*a){
		default:
			error("bad draw command");
		/* new allocate: 'b' id[4] screenid[4] refresh[1] chan[4] repl[1] R[4*4] clipR[4*4] rrggbbaa[4] */
		case 'b':
			printmesg(fmt="LLbLbRRL", a, 0);
			m = 1+4+4+1+4+1+4*4+4*4+4;
			if(n < m)
				error(Eshortdraw);
			dstid = BGLONG(a+1);
			scrnid = BGSHORT(a+5);
			refresh = a[9];
			chan = BGLONG(a+10);
			repl = a[14];
			drawrectangle(&r, a+15);
			drawrectangle(&clipr, a+31);
			value = BGLONG(a+47);
			if(drawlookup(client, dstid, 0))
				error(Eimageexists);
			if(scrnid){
				dscrn = drawlookupscreen(client, scrnid, &cs);
				scrn = dscrn->screen;
				if(repl || chan!=scrn->image->chan)
					error("image parameters incompatible with screen");
				reffn = 0;
				switch(refresh){
				case Refbackup:
					break;
				case Refnone:
					reffn = memlnorefresh;
					break;
				case Refmesg:
					reffn = drawrefresh;
					break;
				default:
					error("unknown refresh method");
				}
				l = memlalloc(scrn, r, reffn, 0, value);
				if(l == 0)
					error(Edrawmem);
				addflush(l->layer->screenr);
				l->clipr = clipr;
				rectclip(&l->clipr, r);
				if(drawinstall(client, dstid, l, dscrn) == 0){
					memldelete(l);
					error(Edrawmem);
				}
				dscrn->ref++;
				if(reffn){
					refx = nil;
					if(reffn == drawrefresh){
						refx = malloc(sizeof(Refx));
						if(refx == 0){
							drawuninstall(client, dstid);
							error(Edrawmem);
						}
						refx->client = client;
						refx->dimage = drawlookup(client, dstid, 1);
					}
					memlsetrefresh(l, reffn, refx);
				}
				continue;
			}
			i = allocmemimage(r, chan);
			if(i == 0)
				error(Edrawmem);
			if(repl)
				i->flags |= Frepl;
			i->clipr = clipr;
			if(!repl)
				rectclip(&i->clipr, r);
			if(drawinstall(client, dstid, i, 0) == 0){
				freememimage(i);
				error(Edrawmem);
			}
			memfillcolor(i, value);
			continue;

		/* allocate screen: 'A' id[4] imageid[4] fillid[4] public[1] */
		case 'A':
			printmesg(fmt="LLLb", a, 1);
			m = 1+4+4+4+1;
			if(n < m)
				error(Eshortdraw);
			dstid = BGLONG(a+1);
			if(dstid == 0)
				error(Ebadarg);
			if(drawlookupdscreen(dstid))
				error(Escreenexists);
			ddst = drawlookup(client, BGLONG(a+5), 1);
			dsrc = drawlookup(client, BGLONG(a+9), 1);
			if(ddst==0 || dsrc==0)
				error(Enodrawimage);
			if(drawinstallscreen(client, 0, dstid, ddst, dsrc, a[13]) == 0)
				error(Edrawmem);
			continue;

		/* set repl and clip: 'c' dstid[4] repl[1] clipR[4*4] */
		case 'c':
			printmesg(fmt="LbR", a, 0);
			m = 1+4+1+4*4;
			if(n < m)
				error(Eshortdraw);
			ddst = drawlookup(client, BGLONG(a+1), 1);
			if(ddst == nil)
				error(Enodrawimage);
			if(ddst->name)
				error("can't change repl/clipr of shared image");
			dst = ddst->image;
			if(a[5])
				dst->flags |= Frepl;
			drawrectangle(&dst->clipr, a+6);
			continue;

		/* draw: 'd' dstid[4] srcid[4] maskid[4] R[4*4] P[2*4] P[2*4] */
		case 'd':
			printmesg(fmt="LLLRPP", a, 0);
			m = 1+4+4+4+4*4+2*4+2*4;
			if(n < m)
				error(Eshortdraw);
			dst = drawimage(client, a+1);
			dstid = BGLONG(a+1);
			src = drawimage(client, a+5);
			mask = drawimage(client, a+9);
			drawrectangle(&r, a+13);
			drawpoint(&p, a+29);
			drawpoint(&q, a+37);
			op = drawclientop(client);
			memdraw(dst, r, src, p, mask, q, op);
			dstflush(dstid, dst, r);
			continue;

		/* toggle debugging: 'D' val[1] */
		case 'D':
			printmesg(fmt="b", a, 0);
			m = 1+1;
			if(n < m)
				error(Eshortdraw);
			drawdebug = a[1];
			continue;

		/* ellipse: 'e' dstid[4] srcid[4] center[2*4] a[4] b[4] thick[4] sp[2*4] alpha[4] phi[4]*/
		case 'e':
		case 'E':
			printmesg(fmt="LLPlllPll", a, 0);
			m = 1+4+4+2*4+4+4+4+2*4+2*4;
			if(n < m)
				error(Eshortdraw);
			dst = drawimage(client, a+1);
			dstid = BGLONG(a+1);
			src = drawimage(client, a+5);
			drawpoint(&p, a+9);
			e0 = BGLONG(a+17);
			e1 = BGLONG(a+21);
			if(e0<0 || e1<0)
				error("invalid ellipse semidiameter");
			j = BGLONG(a+25);
			if(j < 0)
				error("negative ellipse thickness");
			drawpoint(&sp, a+29);
			c = j;
			if(*a == 'E')
				c = -1;
			ox = BGLONG(a+37);
			oy = BGLONG(a+41);
			op = drawclientop(client);
			/* high bit indicates arc angles are present */
			if(ox & (1U<<31)){
				if((ox & (1<<30)) == 0)
					ox &= ~(1U<<31);
				memarc(dst, p, e0, e1, c, src, sp, ox, oy, op);
			}else
				memellipse(dst, p, e0, e1, c, src, sp, op);
			dstflush(dstid, dst, Rect(p.x-e0-j, p.y-e1-j, p.x+e0+j+1, p.y+e1+j+1));
			continue;

		/* free: 'f' id[4] */
		case 'f':
			printmesg(fmt="L", a, 1);
			m = 1+4;
			if(n < m)
				error(Eshortdraw);
			ll = drawlookup(client, BGLONG(a+1), 0);
			if(ll && ll->dscreen && ll->dscreen->owner != client)
				ll->dscreen->owner->refreshme = 1;
			drawuninstall(client, BGLONG(a+1));
			continue;

		/* free screen: 'F' id[4] */
		case 'F':
			printmesg(fmt="L", a, 1);
			m = 1+4;
			if(n < m)
				error(Eshortdraw);
			drawlookupscreen(client, BGLONG(a+1), &cs);
			drawuninstallscreen(client, cs);
			continue;

		/* initialize font: 'i' fontid[4] nchars[4] ascent[1] */
		case 'i':
			printmesg(fmt="Llb", a, 1);
			m = 1+4+4+1;
			if(n < m)
				error(Eshortdraw);
			dstid = BGLONG(a+1);
			if(dstid == 0)
				error("can't use display as font");
			font = drawlookup(client, dstid, 1);
			if(font == 0)
				error(Enodrawimage);
			if(font->image->layer)
				error("can't use window as font");
			ni = BGLONG(a+5);
			if(ni<=0 || ni>4096)
				error("bad font size (4096 chars max)");
			free(font->fchar);	/* should we complain if non-zero? */
			font->fchar = malloc(ni*sizeof(FChar));
			if(font->fchar == 0)
				error("no memory for font");
			memset(font->fchar, 0, ni*sizeof(FChar));
			font->nfchar = ni;
			font->ascent = a[9];
			continue;

		/* load character: 'l' fontid[4] srcid[4] index[2] R[4*4] P[2*4] left[1] width[1] */
		case 'l':
			printmesg(fmt="LLSRPbb", a, 0);
			m = 1+4+4+2+4*4+2*4+1+1;
			if(n < m)
				error(Eshortdraw);
			font = drawlookup(client, BGLONG(a+1), 1);
			if(font == 0)
				error(Enodrawimage);
			if(font->nfchar == 0)
				error(Enotfont);
			src = drawimage(client, a+5);
			ci = BGSHORT(a+9);
			if(ci >= font->nfchar)
				error(Eindex);
			drawrectangle(&r, a+11);
			drawpoint(&p, a+27);
			memdraw(font->image, r, src, p, memopaque, p, S);
			fc = &font->fchar[ci];
			fc->minx = r.min.x;
			fc->maxx = r.max.x;
			fc->miny = r.min.y;
			fc->maxy = r.max.y;
			fc->left = a[35];
			fc->width = a[36];
			continue;

		/* draw line: 'L' dstid[4] p0[2*4] p1[2*4] end0[4] end1[4] radius[4] srcid[4] sp[2*4] */
		case 'L':
			printmesg(fmt="LPPlllLP", a, 0);
			m = 1+4+2*4+2*4+4+4+4+4+2*4;
			if(n < m)
				error(Eshortdraw);
			dst = drawimage(client, a+1);
			dstid = BGLONG(a+1);
			drawpoint(&p, a+5);
			drawpoint(&q, a+13);
			e0 = BGLONG(a+21);
			e1 = BGLONG(a+25);
			j = BGLONG(a+29);
			if(j < 0)
				error("negative line width");
			src = drawimage(client, a+33);
			drawpoint(&sp, a+37);
			op = drawclientop(client);
			memline(dst, p, q, e0, e1, j, src, sp, op);
			/* avoid memlinebbox if possible */
			if(dstid==0 || dst->layer!=nil){
				/* BUG: this is terribly inefficient: update maximal containing rect*/
				r = memlinebbox(p, q, e0, e1, j);
				dstflush(dstid, dst, insetrect(r, -(1+1+j)));
			}
			continue;

		/* create image mask: 'm' newid[4] id[4] */
/*
 *
		case 'm':
			printmesg("LL", a, 0);
			m = 4+4;
			if(n < m)
				error(Eshortdraw);
			break;
 *
 */

		/* attach to a named image: 'n' dstid[4] j[1] name[j] */
		case 'n':
			printmesg(fmt="Lz", a, 0);
			m = 1+4+1;
			if(n < m)
				error(Eshortdraw);
			j = a[5];
			if(j == 0)	/* give me a non-empty name please */
				error(Eshortdraw);
			m += j;
			if(n < m)
				error(Eshortdraw);
			dstid = BGLONG(a+1);
			if(drawlookup(client, dstid, 0))
				error(Eimageexists);
			dn = drawlookupname(j, (char*)a+6);
			if(dn == nil)
				error(Enoname);
			if(drawinstall(client, dstid, dn->dimage->image, 0) == 0)
				error(Edrawmem);
			di = drawlookup(client, dstid, 0);
			if(di == 0)
				error("draw: can't happen");
			di->vers = dn->vers;
			di->name = smalloc(j+1);
			di->fromname = dn->dimage;
			di->fromname->ref++;
			memmove(di->name, a+6, j);
			di->name[j] = 0;
			client->infoid = dstid;
			continue;

		/* name an image: 'N' dstid[4] in[1] j[1] name[j] */
		case 'N':
			printmesg(fmt="Lbz", a, 0);
			m = 1+4+1+1;
			if(n < m)
				error(Eshortdraw);
			c = a[5];
			j = a[6];
			if(j == 0)	/* give me a non-empty name please */
				error(Eshortdraw);
			m += j;
			if(n < m)
				error(Eshortdraw);
			di = drawlookup(client, BGLONG(a+1), 0);
			if(di == 0)
				error(Enodrawimage);
			if(di->name)
				error(Enamed);
			if(c)
				drawaddname(client, di, j, (char*)a+7);
			else{
				dn = drawlookupname(j, (char*)a+7);
				if(dn == nil)
					error(Enoname);
				if(dn->dimage != di)
					error(Ewrongname);
				drawdelname(dn);
			}
			continue;

		/* position window: 'o' id[4] r.min [2*4] screenr.min [2*4] */
		case 'o':
			printmesg(fmt="LPP", a, 0);
			m = 1+4+2*4+2*4;
			if(n < m)
				error(Eshortdraw);
			dst = drawimage(client, a+1);
			if(dst->layer){
				drawpoint(&p, a+5);
				drawpoint(&q, a+13);
				r = dst->layer->screenr;
				ni = memlorigin(dst, p, q);
				if(ni < 0)
					error("image origin failed");
				if(ni > 0){
					addflush(r);
					addflush(dst->layer->screenr);
					ll = drawlookup(client, BGLONG(a+1), 1);
					drawrefreshscreen(ll, client);
				}
			}
			continue;

		/* set compositing operator for next draw operation: 'O' op */
		case 'O':
			printmesg(fmt="b", a, 0);
			m = 1+1;
			if(n < m)
				error(Eshortdraw);
			client->op = a[1];
			continue;

		/* filled polygon: 'P' dstid[4] n[2] wind[4] ignore[2*4] srcid[4] sp[2*4] p0[2*4] dp[2*2*n] */
		/* polygon: 'p' dstid[4] n[2] end0[4] end1[4] radius[4] srcid[4] sp[2*4] p0[2*4] dp[2*2*n] */
		case 'p':
		case 'P':
			printmesg(fmt="LslllLPP", a, 0);
			m = 1+4+2+4+4+4+4+2*4;
			if(n < m)
				error(Eshortdraw);
			dstid = BGLONG(a+1);
			dst = drawimage(client, a+1);
			ni = BGSHORT(a+5);
			if(ni < 0)
				error("negative count in polygon");
			e0 = BGLONG(a+7);
			e1 = BGLONG(a+11);
			j = 0;
			if(*a == 'p'){
				j = BGLONG(a+15);
				if(j < 0)
					error("negative polygon line width");
			}
			src = drawimage(client, a+19);
			drawpoint(&sp, a+23);
			drawpoint(&p, a+31);
			ni++;
			pp = malloc(ni*sizeof(Point));
			if(pp == nil)
				error(Enomem);
			doflush = 0;
			if(dstid==0 || (dst->layer && dst->layer->screen->image->data == screenimage->data))
				doflush = 1;	/* simplify test in loop */
			ox = oy = 0;
			esize = 0;
			u = a+m;
			for(y=0; y<ni; y++){
				q = p;
				oesize = esize;
				u = drawcoord(u, a+n, ox, &p.x);
				u = drawcoord(u, a+n, oy, &p.y);
				ox = p.x;
				oy = p.y;
				if(doflush){
					esize = j;
					if(*a == 'p'){
						if(y == 0){
							c = memlineendsize(e0);
							if(c > esize)
								esize = c;
						}
						if(y == ni-1){
							c = memlineendsize(e1);
							if(c > esize)
								esize = c;
						}
					}
					if(*a=='P' && e0!=1 && e0 !=~0)
						r = dst->clipr;
					else if(y > 0){
						r = Rect(q.x-oesize, q.y-oesize, q.x+oesize+1, q.y+oesize+1);
						combinerect(&r, Rect(p.x-esize, p.y-esize, p.x+esize+1, p.y+esize+1));
					}
					if(rectclip(&r, dst->clipr))		/* should perhaps be an arg to dstflush */
						dstflush(dstid, dst, r);
				}
				pp[y] = p;
			}
			if(y == 1)
				dstflush(dstid, dst, Rect(p.x-esize, p.y-esize, p.x+esize+1, p.y+esize+1));
			op = drawclientop(client);
			if(*a == 'p')
				mempoly(dst, pp, ni, e0, e1, j, src, sp, op);
			else
				memfillpoly(dst, pp, ni, e0, src, sp, op);
			free(pp);
			m = u-a;
			continue;

		/* read: 'r' id[4] R[4*4] */
		case 'r':
			printmesg(fmt="LR", a, 0);
			m = 1+4+4*4;
			if(n < m)
				error(Eshortdraw);
			i = drawimage(client, a+1);
			drawrectangle(&r, a+5);
			if(!rectinrect(r, i->r))
				error(Ereadoutside);
			c = bytesperline(r, i->depth);
			c *= Dy(r);
			free(client->readdata);
			client->readdata = mallocz(c, 0);
			if(client->readdata == nil)
				error("readimage malloc failed");
			client->nreaddata = memunload(i, r, client->readdata, c);
			if(client->nreaddata < 0){
				free(client->readdata);
				client->readdata = nil;
				error("bad readimage call");
			}
			continue;

		/* string: 's' dstid[4] srcid[4] fontid[4] P[2*4] clipr[4*4] sp[2*4] ni[2] ni*(index[2]) */
		/* stringbg: 'x' dstid[4] srcid[4] fontid[4] P[2*4] clipr[4*4] sp[2*4] ni[2] bgid[4] bgpt[2*4] ni*(index[2]) */
		case 's':
		case 'x':
			printmesg(fmt="LLLPRPs", a, 0);
			m = 1+4+4+4+2*4+4*4+2*4+2;
			if(*a == 'x')
				m += 4+2*4;
			if(n < m)
				error(Eshortdraw);

			dst = drawimage(client, a+1);
			dstid = BGLONG(a+1);
			src = drawimage(client, a+5);
			font = drawlookup(client, BGLONG(a+9), 1);
			if(font == 0)
				error(Enodrawimage);
			if(font->nfchar == 0)
				error(Enotfont);
			drawpoint(&p, a+13);
			drawrectangle(&r, a+21);
			drawpoint(&sp, a+37);
			ni = BGSHORT(a+45);
			u = a+m;
			m += ni*2;
			if(n < m)
				error(Eshortdraw);
			clipr = dst->clipr;
			dst->clipr = r;
			op = drawclientop(client);
			l = dst;
			if(*a == 'x'){
				/* paint background */
				l = drawimage(client, a+47);
				drawpoint(&q, a+51);
				r.min.x = p.x;
				r.min.y = p.y-font->ascent;
				r.max.x = p.x;
				r.max.y = r.min.y+Dy(font->image->r);
				j = ni;
				while(--j >= 0){
					ci = BGSHORT(u);
					if(ci<0 || ci>=font->nfchar){
						dst->clipr = clipr;
						error(Eindex);
					}
					r.max.x += font->fchar[ci].width;
					u += 2;
				}
				memdraw(dst, r, l, q, memopaque, ZP, op);
				u -= 2*ni;
			}
			q = p;
			while(--ni >= 0){
				ci = BGSHORT(u);
				if(ci<0 || ci>=font->nfchar){
					dst->clipr = clipr;
					error(Eindex);
				}
				q = drawchar(dst, l, q, src, &sp, font, ci, op);
				u += 2;
			}
			dst->clipr = clipr;
			p.y -= font->ascent;
			dstflush(dstid, dst, Rect(p.x, p.y, q.x, p.y+Dy(font->image->r)));
			continue;

		/* use public screen: 'S' id[4] chan[4] */
		case 'S':
			printmesg(fmt="Ll", a, 0);
			m = 1+4+4;
			if(n < m)
				error(Eshortdraw);
			dstid = BGLONG(a+1);
			if(dstid == 0)
				error(Ebadarg);
			dscrn = drawlookupdscreen(dstid);
			if(dscrn==0 || (dscrn->public==0 && dscrn->owner!=client))
				error(Enodrawscreen);
			if(dscrn->screen->image->chan != BGLONG(a+5))
				error("inconsistent chan");
			if(drawinstallscreen(client, dscrn, 0, 0, 0, 0) == 0)
				error(Edrawmem);
			continue;

		/* top or bottom windows: 't' top[1] nw[2] n*id[4] */
		case 't':
			printmesg(fmt="bsL", a, 0);
			m = 1+1+2;
			if(n < m)
				error(Eshortdraw);
			nw = BGSHORT(a+2);
			if(nw < 0)
				error(Ebadarg);
			if(nw == 0)
				continue;
			m += nw*4;
			if(n < m)
				error(Eshortdraw);
			lp = malloc(nw*sizeof(Memimage*));
			if(lp == 0)
				error(Enomem);
			if(waserror()){
				free(lp);
				nexterror();
			}
			for(j=0; j<nw; j++)
				lp[j] = drawimage(client, a+1+1+2+j*4);
			if(lp[0]->layer == 0)
				error("images are not windows");
			for(j=1; j<nw; j++)
				if(lp[j]->layer->screen != lp[0]->layer->screen)
					error("images not on same screen");
			if(a[1])
				memltofrontn(lp, nw);
			else
				memltorearn(lp, nw);
			if(lp[0]->layer->screen->image->data == screenimage->data)
				for(j=0; j<nw; j++)
					addflush(lp[j]->layer->screenr);
			ll = drawlookup(client, BGLONG(a+1+1+2), 1);
			drawrefreshscreen(ll, client);
			poperror();
			free(lp);
			continue;

		/* visible: 'v' */
		case 'v':
			printmesg(fmt="", a, 0);
			m = 1;
			drawflush();
			continue;

		/* write: 'y' id[4] R[4*4] data[x*1] */
		/* write from compressed data: 'Y' id[4] R[4*4] data[x*1] */
		case 'y':
		case 'Y':
			printmesg(fmt="LR", a, 0);
		//	iprint("load %c\n", *a);
			m = 1+4+4*4;
			if(n < m)
				error(Eshortdraw);
			dstid = BGLONG(a+1);
			dst = drawimage(client, a+1);
			drawrectangle(&r, a+5);
			if(!rectinrect(r, dst->r))
				error(Ewriteoutside);
			y = memload(dst, r, a+m, n-m, *a=='Y');
			if(y < 0)
				error("bad writeimage call");
			dstflush(dstid, dst, r);
			m += y;
			continue;
		}
	}
	poperror();
}

Dev drawdevtab = {
	'i',
	"draw",

	devreset,
	devinit,
	devshutdown,
	drawattach,
	drawwalk,
	drawstat,
	drawopen,
	devcreate,
	drawclose,
	drawread,
	devbread,
	drawwrite,
	devbwrite,
	devremove,
	devwstat,
};

/*
 * On 8 bit displays, load the default color map
 */
void
drawcmap(void)
{
	int r, g, b, cr, cg, cb, v;
	int num, den;
	int i, j;

	drawactive(1);	/* to restore map from backup */
	for(r=0,i=0; r!=4; r++)
	    for(v=0; v!=4; v++,i+=16){
		for(g=0,j=v-r; g!=4; g++)
		    for(b=0;b!=4;b++,j++){
			den = r;
			if(g > den)
				den = g;
			if(b > den)
				den = b;
			if(den == 0)	/* divide check -- pick grey shades */
				cr = cg = cb = v*17;
			else{
				num = 17*(4*den+v);
				cr = r*num/den;
				cg = g*num/den;
				cb = b*num/den;
			}
			setcolor(i+(j&15),
				cr*0x01010101, cg*0x01010101, cb*0x01010101);
		    }
	}
}

void
drawblankscreen(int blank)
{
	int i, nc;
	ulong *p;

	if(blank == sdraw.blanked)
		return;
	if(!canqlock(&sdraw.lk))
		return;
	if(!initscreenimage()){
		qunlock(&sdraw.lk);
		return;
	}
	p = sdraw.savemap;
	nc = screenimage->depth > 8 ? 256 : 1<<screenimage->depth;

	/*
	 * blankscreen uses the hardware to blank the screen
	 * when possible.  to help in cases when it is not possible,
	 * we set the color map to be all black.
	 */
	if(blank == 0){	/* turn screen on */
		for(i=0; i<nc; i++, p+=3)
			setcolor(i, p[0], p[1], p[2]);
	//	blankscreen(0);
	}else{	/* turn screen off */
	//	blankscreen(1);
		for(i=0; i<nc; i++, p+=3){
			getcolor(i, &p[0], &p[1], &p[2]);
			setcolor(i, 0, 0, 0);
		}
	}
	sdraw.blanked = blank;
	qunlock(&sdraw.lk);
}

/*
 * record activity on screen, changing blanking as appropriate
 */
void
drawactive(int active)
{
/*
	if(active){
		drawblankscreen(0);
		sdraw.blanktime = MACHP(0)->ticks;
	}else{
		if(blanktime && sdraw.blanktime && TK2SEC(MACHP(0)->ticks - sdraw.blanktime)/60 >= blanktime)
			drawblankscreen(1);
	}
*/
}

int
drawidletime(void)
{
	return 0;
/*	return TK2SEC(MACHP(0)->ticks - sdraw.blanktime)/60; */
}

