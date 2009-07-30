#include <u.h>
#include <libc.h>
#include <draw.h>
#include <memdraw.h>
#include <thread.h>
#include <cursor.h>
#include <mouse.h>
#include <keyboard.h>
#include <frame.h>
#include <plumb.h>
#include <html.h>
#include "dat.h"
#include "fns.h"

static void pageload1(Page *, Url *, int);

static
void
addchild(Page *p, Page *c)
{
	Page *t;

	c->parent = p;
	c->w = p->w;
	c->b = p->b;
	c->col = p->col;
	c->row = p->row;
	if(p->child == nil)
		p->child = c;
	else{
		for(t=p->child; t->next!=nil; t=t->next)
			;
		t->next = c;
	}
}

static
void
loadchilds(Page *p, Kidinfo *k)
{
	Runestr rs;
	Kidinfo *t;
	Page *c;

	addrefresh(p, "loading frames...");
	p->kidinfo = k;
	for(t=k->kidinfos; t!=nil; t=t->next){
		c = emalloc(sizeof(Page));
		addchild(p, c);
		if(t->isframeset){
			c->url = urldup(p->url);
			loadchilds(c, t);
		}else{
			c->kidinfo = t;
			/* this check shouldn't be necessary, but... */
			if(t->src){
				rs.r = urlcombine(p->url->act.r, t->src);
				rs.nr = runestrlen(rs.r);
				pageload1(c, urlalloc(&rs, nil, HGet), FALSE);
				closerunestr(&rs);
			}
		}
	}
}

static struct {
	char *mime;
	char *filter;
}filtertab[] = {
	"image/gif",	"gif -t9",
	"image/jpeg",	"jpg -t9",
	"image/jpg",	"jpg -t9",
	"image/pjpeg",	"jpg -t9",
	"image/png",	"png -t9",
	"image/ppm",	"ppm -t9",
	nil,	nil,
};

char *
getfilter(Rune *r, int x, int y)
{
	char buf[128];
	int i;

	snprint(buf, sizeof(buf), "%S", r);
	for(i=0; filtertab[i].mime!=nil; i++)
		if(cistrncmp(buf, filtertab[i].mime, strlen(filtertab[i].mime)) == 0)
			break;

	if(filtertab[i].filter == nil)
		return nil;

	if(x==0 && y==0)
		return smprint("%s", filtertab[i].filter);
	if(x!=0 && y!=0)
		return smprint("%s | resample -x %d -y %d", filtertab[i].filter, x, y);
	if(x != 0)
		return smprint("%s | resample -x %d", filtertab[i].filter, x);
	/* y != 0 */
	return smprint("%s | resample -y %d", filtertab[i].filter, y);
}

static Cimage *cimages = nil;
static QLock cimagelock;

static
void
freecimage(Cimage *ci)
{
	Cimage *ci1;

	qlock(&cimagelock);
	if(decref(ci) == 0){
		if(ci->i)
			freeimage(ci->i);
		else if(ci->mi)
			freememimage(ci->mi);
		urlfree(ci->url);
		ci1 = cimages;
		if(ci1 == ci)
			cimages = ci->next;
		else{
			while(ci1){
				if(ci1->next == ci){
					ci1->next = ci->next;
					break;
				}
				ci1 = ci1->next;
			}
		}
		free(ci);
	}
	qunlock(&cimagelock);
}

static
void
closeimages(Page *p)
{
	int i;

	for(i=0; i<p->ncimage; i++)
		freecimage(p->cimage[i]);
	free(p->cimage);
	p->cimage =nil;
	p->ncimage = 0;
}

static
Cimage *
loadimg(Rune *src, int x , int y)
{
	Channel *sync;
	Cimage *ci;
	Runestr rs;
	Exec *e;
	char *filter;
	int fd, p[2], q[2];

	ci = emalloc(sizeof(Cimage));
	rs. r = src;
	rs.nr = runestrlen(rs.r);
	ci->url = urlalloc(&rs, nil, HGet);
	fd = urlopen(ci->url);
	if(fd < 0){
    Err1:
		return ci;
	}
	filter = getfilter(ci->url->ctype.r, x, y);
	if(filter == nil){
		werrstr("%S unsupported: %S", ci->url->ctype.r, ci->url->act.r);
    Err2:
		close(fd);
		goto Err1;
	}

	if(pipe(p)<0 || pipe(q)<0)
		error("can't create pipe");
	close(p[0]);
	p[0] = fd;
	sync = chancreate(sizeof(ulong), 0);
	if(sync == nil)
		error("can't create channel");
	e = emalloc(sizeof(Exec));
	e->p[0] = p[0];
	e->p[1] = p[1];
	e->q[0] = q[0];
	e->q[1] = q[1];
	e->cmd = filter;
	e->sync = sync;
	proccreate(execproc, e, STACK);
	recvul(sync);
	chanfree(sync);
	close(p[0]);
	close(p[1]);
	close(q[1]);

	ci->mi = readmemimage(q[0]);
	close(q[0]);
	if(ci->mi == nil){
		werrstr("can't read image");
		goto Err2;
	}
	free(filter);
	return ci;
}

static
Cimage *
findimg(Rune *s)
{
	Cimage *ci;

	qlock(&cimagelock);
	for(ci=cimages; ci!=nil; ci=ci->next)
		if(runestrcmp(ci->url->src.r, s) == 0)
			break;

	qunlock(&cimagelock);
	return ci;
}

void
loadimages(Page *p)
{
	Cimage *ci;
	Iimage *i;
	Rune *src;

	addrefresh(p, "loading images...");
	reverseimages(&p->doc->images);
	for(i=p->doc->images; i!=nil; i=i->nextimage){
		if(p->aborting)
			break;
		src = urlcombine(getbase(p), i->imsrc);
		ci = findimg(src);
		if(ci == nil){
			ci = loadimg(src, i->imwidth, i->imheight);
			qlock(&cimagelock);
			ci->next = cimages;
			cimages = ci;
			qunlock(&cimagelock);
		}
		free(src);
		incref(ci);
		i->aux = ci;
		p->cimage = erealloc(p->cimage, ++p->ncimage*sizeof(Cimage *));
		p->cimage[p->ncimage-1] = ci;
		p->changed = TRUE;
		addrefresh(p, "");
	}
}

static char *mimetab[] = {
	"text/html",
	"application/xhtml",
	nil,
};

static
void
pageloadproc(void *v)
{
	Page *p;
	char buf[BUFSIZE], *s;
	long n, l;
	int fd, i, ctype;

	threadsetname("pageloadproc");
	rfork(RFFDG);

	p = v;
	addrefresh(p, "opening: %S...", p->url->src.r);
	fd = urlopen(p->url);
	if(fd < 0){
		addrefresh(p, "%S: %r", p->url->src.r);
    Err:
		p->loading = FALSE;
		return;
	}
	if(runestrlen(p->url->ctype.r) == 0) /* assume .html when headers don't say anyting */
		goto Html;

	snprint(buf, sizeof(buf), "%S", p->url->ctype.r);
	for(i=0; mimetab[i]!=nil; i++)
		if(cistrncmp(buf, mimetab[i], strlen(mimetab[i])) == 0)
			break;

	if(mimetab[i]){
    Html:
		ctype = TextHtml;
	}else if(cistrncmp(buf, "text/", 5) == 0)
		ctype = TextPlain;
	else{
		close(fd);
		addrefresh(p, "%S: unsupported mime type: '%S'", p->url->act.r, p->url->ctype.r);
		goto Err;
	}
	addrefresh(p, "loading: %S...", p->url->src.r);
	s = nil;
	l = 0;
	while((n=read(fd, buf, sizeof(buf))) > 0){
		if(p->aborting){
			if(s){
				free(s);
				s = nil;
			}
			break;
		}
		s = erealloc(s, l+n+1);
		memmove(s+l, buf, n);
		l += n;
		s[l] = '\0';
	}
	close(fd);
	n = l;
	if(s){
		s = convert(p->url->ctype, s, &n);
		p->items = parsehtml((uchar *)s, n, p->url->act.r, ctype, UTF_8, &p->doc);
		free(s);
		fixtext(p);
		if(ctype==TextHtml && p->aborting==FALSE){
			p->changed = TRUE;
			addrefresh(p, "");
			if(p->doc->doctitle){
				p->title.r = erunestrdup(p->doc->doctitle);
				p->title.nr = runestrlen(p->title.r);
			}
			p->loading = XXX;
			if(p->doc->kidinfo)
				loadchilds(p, p->doc->kidinfo);
			else if(p->doc->images)
				loadimages(p);
		}
	}
	p->changed = TRUE;
	p->loading = FALSE;
	addrefresh(p, "");
}

static
void
pageload1(Page *p, Url *u, int dohist)
{
	pageclose(p);
	p->loading = TRUE;
	p->url = u;
	if(dohist)
		winaddhist(p->w, p->url);
	proccreate(pageloadproc, p, STACK);
}

void
pageload(Page *p, Url *u, int dohist)
{
	if(p->parent == nil)
		textset(&p->w->url, u->src.r, u->src.nr);
	draw(p->b, p->all, display->white, nil, ZP);
	pageload1(p, u, dohist);
}

void
pageget(Page *p, Runestr *src, Runestr *post,  int m, int dohist)
{
	pageload(p, urlalloc(src, post, m), dohist);
}

void
pageclose(Page *p)
{
	Page *c, *nc;

	if(p == selpage)
		selpage = nil;
	pageabort(p);
	closeimages(p);
	urlfree(p->url);
	p->url = nil;
	if(p->doc){
		freedocinfo(p->doc);
		p->doc = nil;
	}
	layfree(p->lay);
	p->lay = nil;
	freeitems(p->items);
	p->items = nil;
	for(c=p->child; c!=nil; c=nc){
		nc = c->next;
		pageclose(c);
		free(c);
	}
	p->child = nil;
	closerunestr(&p->title);
	closerunestr(&p->refresh.rs);
	p->refresh.t = 0;
	p->pos = ZP;
	p->top = ZP;
	p->bot = ZP;
	p->loading = p->aborting = FALSE;
}

int
pageabort(Page *p)
{
	Page *c;

	for(c=p->child; c!=nil; c=c->next)
		pageabort(c);

	p->aborting = TRUE;
	while(p->loading)
		sleep(100);

	p->aborting = FALSE;
	return TRUE;
}


static Image *tmp;

void
tmpresize(void)
{
	if(tmp)
		freeimage(tmp);

	tmp = eallocimage(display, Rect(0,0,Dx(screen->r),Dy(screen->r)), screen->chan, 0, -1);
}

static
void
renderchilds(Page *p)
{
	Rectangle r;
	Kidinfo *k;
	Page *c;
	int i, j, x, y, *w, *h;

	draw(p->b, p->all, display->white, nil, ZP);
	r = p->all;
	y = r.min.y;
	c = p->child;
	k = p->kidinfo;
	frdims(k->rows, k->nrows, Dy(r), &h);
	frdims(k->cols, k->ncols, Dx(r), &w);
	for(i=0; i<k->nrows; i++){
		x = r.min.x;
		for(j=0; j<k->ncols; j++){
			if(c->aborting)
				return;
			c->b = p->b;
			c->all = Rect(x,y,x+w[j],y+h[i]);
			c->w = p->w;
			pagerender(c);
			c = c->next;
			x += w[j];
		}
		y += h[i];
	}
	free(w);
	free(h);
}

static
void
pagerender1(Page *p)
{
	Rectangle r;

	r = p->all;
	p->hscrollr = r;
	p->hscrollr.min.y = r.max.y-Scrollsize;
	p->vscrollr = r;
	p->vscrollr.max.x = r.min.x+Scrollsize;
	r.max.y -= Scrollsize;
	r.min.x += Scrollsize;
	p->r = r;
	p->vscrollr.max.y = r.max.y;
	p->hscrollr.min.x = r.min.x;
	laypage(p);
	pageredraw(p);
}

void
pagerender(Page *p)
{
	if(p->child && p->loading==FALSE)
		renderchilds(p);
	else if(p->doc)
		pagerender1(p);
}

void
pageredraw(Page *p)
{
	Rectangle r;

	r = p->lay->r;
	if(Dx(r)==0 || Dy(r)==0){
		draw(p->b, p->r, display->white, nil, ZP);
		return;
	}
	if(tmp == nil)
		tmpresize();

	p->selecting = FALSE;
	draw(tmp, tmp->r, getbg(p), nil, ZP);
	laydraw(p, tmp, p->lay);
	draw(p->b, p->r, tmp, nil, tmp->r.min);
	r = p->vscrollr;
	r.min.y = r.max.y;
	r.max.y += Scrollsize;
	draw(p->b, r, tagcols[HIGH], nil, ZP);
	draw(p->b, insetrect(r, 1), tagcols[BACK], nil, ZP);
	pagescrldraw(p);
}

static
void
pageselect1(Page *p)	/* when called, button 1 is down */
{
	Point mp, npos, opos;
	int b, scrled, x, y;

	b = mouse->buttons;
	mp = mousectl->xy;
	opos = getpt(p, mp);
	do{
		x = y = 0;
		if(mp.x < p->r.min.x)
			x -= p->r.min.x-mp.x;
		else if(mp.x > p->r.max.x)
			x += mp.x-p->r.max.x;
		if(mp.y < p->r.min.y)
			y -= (p->r.min.y-mp.y)*Panspeed;
		else if(mp.y > p->r.max.y)
			y += (mp.y-p->r.max.y)*Panspeed;

		scrled = pagescrollxy(p, x, y);
		npos = getpt(p, mp);
		if(opos.y <  npos.y){
			p->top = opos;
			p->bot = npos;
		}else{
			p->top = npos;
			p->bot = opos;
		}
		pageredraw(p);
		if(scrled == TRUE)
			scrsleep(100);
		else
			readmouse(mousectl);

		mp = mousectl->xy;
	}while(mousectl->buttons == b);
}

static Rune left1[] =  { L'{', L'[', L'(', L'<', L'«', 0 };
static Rune right1[] = { L'}', L']', L')', L'>', L'»', 0 };
static Rune left2[] =  { L'\'', L'"', L'`', 0 };

static
Rune *left[] = {
	left1,
	left2,
	nil
};
static
Rune *right[] = {
	right1,
	left2,
	nil
};

void
pagedoubleclick(Page *p)
{
	Point xy;
	Line *l;
	Box *b;

	xy = getpt(p, mouse->xy);
	l = linewhich(p->lay, xy);
	if(l==nil || l->hastext==FALSE)
		return;

	if(xy.x<l->boxes->r.min.x && hasbrk(l->state)){	/* beginning of line? */
		p->top = l->boxes->r.min;
		if(l->next && !hasbrk(l->next->state)){
			for(l=l->next; l->next!=nil; l=l->next)
				if(hasbrk(l->next->state))
					break;
		}
		p->bot = l->lastbox->r.max;;
	}else if(xy.x>l->lastbox->r.max.x && hasbrk(l->next->state)){	/* end of line? */
		p->bot = l->lastbox->r.max;
		if(!hasbrk(l->state) && l->prev!=nil){
			for(l=l->prev; l->prev!=nil; l=l->prev)
				if(hasbrk(l->state))
					break;
		}
		p->top = l->boxes->r.min;
	}else{
		b = pttobox(l, xy);
		if(b!=nil && b->i->tag==Itexttag){
			p->top = b->r.min;
			p->bot = b->r.max;
		}
	}
	p->top.y += 2;
	p->bot.y -= 2;
	pageredraw(p);
}

static uint clickmsec;

void
pageselect(Page *p)
{
	int b, x, y;


	selpage = p;
	/*
	 * To have double-clicking and chording, we double-click
	 * immediately if it might make sense.
	 */
	b = mouse->buttons;
	if(mouse->msec-clickmsec<500){
		pagedoubleclick(p);
		x = mouse->xy.x;
		y = mouse->xy.y;
		/* stay here until something interesting happens */
		do
			readmouse(mousectl);
		while(mouse->buttons==b && abs(mouse->xy.x-x)<3 && abs(mouse->xy.y-y)<3);
		mouse->xy.x = x;	/* in case we're calling pageselect1 */
		mouse->xy.y = y;
	}
	if(mousectl->buttons == b)
		pageselect1(p);

	if(eqpt(p->top, p->bot)){
		if(mouse->msec-clickmsec<500)
			pagedoubleclick(p);
		else
			clickmsec = mouse->msec;
	}
	while(mouse->buttons){
		mouse->msec = 0;
		b = mouse->buttons;
		if(b & 2)	/* snarf only */
			cut(nil, nil, TRUE, FALSE, nil, 0);
		while(mouse->buttons == b)
			readmouse(mousectl);
	}
}

Page *
pagewhich(Page *p, Point xy)
{
	Page *c;

	if(p->child == nil)
		return p;

	for(c=p->child; c!=nil; c=c->next)
		if(ptinrect(xy, c->all))
			return pagewhich(c, xy);

	return nil;
}

void
pagemouse(Page *p, Point xy, int but)
{
	Box *b;

	p = pagewhich(p, xy);
	if(p == nil)
		return;

	if(pagerefresh(p))
		return;

	if(p->lay == nil)
		return;

	if(ptinrect(xy, p->vscrollr)){
		pagescroll(p, but, FALSE);
		return;
	}
	if(ptinrect(xy, p->hscrollr)){
		pagescroll(p, but, TRUE);
		return;
	}
	xy = getpt(p, xy);
	b = boxwhich(p->lay, xy);
	if(b && b->mouse)
		b->mouse(b, p, but);
	else if(but == 1)
		pageselect(p);
}

void
pagetype(Page *p, Rune r, Point xy)
{
	Box *b;
	int x, y;

	p = pagewhich(p, xy);
	if(p == nil)
		return;

	if(pagerefresh(p))
		return;

	if(p->lay == nil)
		return;

	/* text field? */
	xy = getpt(p, xy);
	b = boxwhich(p->lay, xy);
	if(b && b->key){
		b->key(b, p, r);
		return;
	}
	/* ^H: same as 'Back' */
	if(r == 0x08){
		wingohist(p->w, FALSE);
		return;
	}

	x = 0;
	y = 0;
	switch(r){
	case Kleft:
		x -= Dx(p->r)/2;
		break;
	case Kright:
		x += Dx(p->r)/2;
		break;
	case Kdown:
	case Kscrollonedown:
		y += Dy(p->r)/2;
		break;
	case Kpgdown:
		y += Dy(p->r);
		break;
	case Kup:
	case Kscrolloneup:
		y -= Dy(p->r)/2;
		break;
	case Kpgup:
		y -= Dy(p->r);
		break;
	case Khome:
		y -= Dy(p->lay->r);	/* force p->pos.y = 0 */
		break;
	case Kend:
		y = Dy(p->lay->r) - Dy(p->r);
		break;
	default:
		return;
	}
	if(pagescrollxy(p, x, y))
		pageredraw(p);
}

void
pagesnarf(Page *p)
{
	Runestr rs;

	memset(&rs, 0, sizeof(Runestr));
	laysnarf(p, p->lay, &rs);
	putsnarf(&rs);
	closerunestr(&rs);
}

void
pagesetrefresh(Page *p)
{
	Runestr rs;
	Rune *s, *q, *t;
	char *v;
	int n;

	if(!p->doc || !p->doc->refresh)
		return;

	s = p->doc->refresh;
	q = runestrchr(s, L'=');
	if(q == nil)
		return;
	q++;
	if(!q)
		return;
	n = runestrlen(q);
	if(*q == L'''){
		q++;
		n -= 2;
	}
	if(n <= 0)
		return;
	t = runesmprint("%.*S", n, q);
	rs.r = urlcombine(getbase(p), t);
	rs.nr = runestrlen(rs.r);
	copyrunestr(&p->refresh.rs, &rs);
	closerunestr(&rs);
	free(t);

	/* now the time */
	q = runestrchr(s, L';');
	if(q){
		v = smprint("%.*S", (int)(q-s),  s);
		p->refresh.t = atoi(v);
		free(v);
	}else
		p->refresh.t = 1;

	p->refresh.t += time(0);
}

int
pagerefresh(Page *p)
{
	int t;

	if(!p->refresh.t)
		return 0;

	t = p->refresh.t - time(0);
	if(t > 0)
		return 0;

	pageget(p, &p->refresh.rs, nil, HGet, FALSE);
	return 1;
}
