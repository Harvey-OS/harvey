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

static void sizeitem(Lay *, Item *);

static
void
sizetext(Lay *lay, Itext *i)
{
	lay->font = getfont(i->fnt);
	i->height = lay->font->height + 2*Space;
	i->width = runestringwidth(lay->font, i->s);
	i->width += runestringnwidth(lay->font, L" ", 1);
}

static
void
sizerule(Lay *lay, Irule *i)
{
	i->width = lay->width;
	i->height = Space + i->size + Space;
}

static
void
sizeimage(Lay *, Iimage *i)
{
	Cimage *ci;

	ci = (Cimage *)i->aux;

	if(ci==nil)
		return;

	if(ci->i == nil)
		getimage(ci, i->altrep);
	if(ci->i == nil)
		return;
	i->width = Dx(ci->i->r) + i->border + i->hspace;
	i->height = Dy(ci->i->r) + i->border + i->vspace;
}

static
void
sizetextfield(Lay *, Iformfield *i)
{
	Formfield *ff;
	Font *f;
	int w, h;

	ff = i->formfield;
	if(ff->ftype == Ftextarea){
		w = ff->cols;
		h = ff->rows;
	}else{
		w = ff->size;
		h = 1;
	}
	f = getfont(WFont);
	i->width = runestringnwidth(f, L"0", 1)*w + 2*(Space+Border+Margin);
	i->width += Scrollsize+Scrollgap;
	i->height = f->height*h + 2*(Space+Border+Margin);
}

static
void
sizecheck(Lay *, Iformfield *i)
{
	i->width = Boxsize + Space;
	i->height = Boxsize;
}

static
void
sizebutton(Lay *, Iformfield *i)
{
	Font *f;
	int x;

	x = Margin + Border + Space;
	f = getfont(WFont);
	i->width = runestringwidth(f, i->formfield->value) + 2*x + Space;
	i->height = f->height + 2*x;
}

static
void
sizefimage(Lay *lay, Iformfield *i)
{
	Iimage *ii;

	ii = (Iimage *)i->formfield->image;
	sizeimage(lay, ii);
	i->width = ii->width;
	i->height = ii->height;
}

static
void
sizeselect(Lay *, Iformfield *i)
{
	Option *o;
	Font *f;
	int x;

	f = getfont(WFont);
	i->width = 0;
	for(o=i->formfield->options; o!=nil; o=o->next)
		i->width = max(i->width, runestringwidth(f, o->display));
	x = Margin + Border + Space;
	i->width += 2*x;
	i->height = f->height+2*x;
}

static
void
sizeformfield(Lay *lay, Iformfield *i)
{
	int type;

	type = i->formfield->ftype;

	if(type==Ftext || type==Ftextarea || type==Fpassword)
		sizetextfield(lay, i);
	else if(type==Fcheckbox || type==Fradio)
		sizecheck(lay, i);
	else if(type==Fbutton || type==Freset || type==Fsubmit)
		sizebutton(lay, i);
	else if(type == Fimage)
		sizefimage(lay, i);
	else if(type == Fselect)
		sizeselect(lay, i);
}

static
void
sizetable(Lay *lay, Itable *i)
{
	tablesize(i->table, lay->width);
	i->width = i->table->totw;
	i->height = i->table->toth;
}

static
void
sizefloat(Lay *lay, Ifloat *i)
{
	sizeitem(lay, i->item);
	i->width = i->item->width;
	i->height = i->item->height;
}

static
void
sizespacer(Lay *lay, Ispacer *i)
{
	if(i->spkind != ISPnull){
		if(i->spkind == ISPhspace)
			i->width = stringnwidth(lay->font, " ", 1);
		i->height = lay->font->height + 2*Space;
	}
}

static
void
sizeitem(Lay *lay, Item *i)
{

	switch(i->tag){
	case Itexttag:
		sizetext(lay, (Itext *)i);
		break;
	case Iruletag:
		sizerule(lay, (Irule *)i);
		break;
	case Iimagetag:
		sizeimage(lay, (Iimage *)i);
		break;
	case Iformfieldtag:
		sizeformfield(lay, (Iformfield *)i);
		break;
	case Itabletag:
		sizetable(lay, (Itable *)i);
		break;
	case Ifloattag:
		sizefloat(lay, (Ifloat *)i);
		break;
	case Ispacertag:
		sizespacer(lay, (Ispacer *)i);
		break;
	default:
		error("can't happen");
	}
}

static
void
drawtext(Box *b, Page *p, Image *im)
{
	Rectangle r, r1;
	Image *c;
	Point pt;
	Font *f;
	Itext *i;
	int q0, q1;

	r = rectsubpt(b->r, p->pos);
	i = (Itext *)b->i;
	f = getfont(i->fnt);
	if(istextsel(p, b->r, &q0, &q1, i->s, f)){
		r1 = r;
		if(q0 > 0)
			r1.min.x += runestringnwidth(f, i->s, q0);
		if(q1 > 0)
			r1.max.x = r1.min.x + runestringnwidth(f, i->s+q0, q1-q0);
		draw(im, r1, textcols[HIGH], nil, ZP);
	}
	c = getcolor(i->fg);
	runestringbg(im, r.min, c, ZP, f, i->s, im, addpt(r.min, im->r.min));

	if(i->ul == ULnone)
		return;

	if(i->ul == ULmid)
		r.min.y += f->height/2;
	else
		r.min.y +=f->height-1;
	pt = r.min;
	pt.x +=  runestringwidth(f, i->s);
	line(im, r.min, pt, Enddisc, Enddisc, 0, c, ZP);
}

static
void
drawrule(Box *b, Page *p, Image *im)
{
	Rectangle r;
	Irule *i;

	i = ((Irule *)b->i);
	r = rectsubpt(b->r, p->pos);
	r.min.y += Space;
	r.max.y -=Space;
	draw(im, r, getcolor(i->color), nil, ZP);
}

static
void
drawimage(Box *b, Page *p, Image *im)
{
	Rectangle r;
	Cimage *ci;
	Iimage *i;
	Image *c;

	if(b->i->tag==Iimagetag)
		i = (Iimage *)b->i;
	else
		i = (Iimage *)((Iformfield *)b->i)->formfield->image;

	ci = (Cimage *)i->aux;
	if(ci==nil || ci->i==nil)
		return;

	r = rectsubpt(b->r, p->pos);
	r.min.x += i->border + i->hspace;
	r.min.y += i->border + i->vspace;
	r.max.x -= i->border + i->hspace;
	r.max.y -= i->border + i->vspace;

	draw(im, r,  ci->i, nil, ci->i->r.min);

	if(i->border){
		if(i->anchorid >= 0)
			c = getcolor(p->doc->link);
		else
			c = display->black;

		border(im, r, i->border, c, ZP);
	}
}

static
void
drawtextfield(Image *im, Rectangle r, Iformfield *i)
{
	Formfield *ff;
	Image *c[3];
	Text *t;
	Font *f;

	r = insetrect(r, Space);
	colarray(c, getcolor(Dark), getcolor(Light), display->white, 1);
	rect3d(im, r, Border, c, ZP);
	r = insetrect(r, Border+Margin);

	if(i->aux == nil){
		ff = i->formfield;
		t = emalloc(sizeof(Text));
		if(ff->ftype == Ftextarea)
			t->what = Textarea;
		else
			t->what = Entry;
		if(ff->ftype == Fpassword)
			f = passfont;
		else
			f = getfont(WFont);
		textinit(t, im, r, f, textcols);
		if(ff->value!=nil){
			textinsert(t, 0, ff->value, runestrlen(ff->value));
			textsetselect(t, t->rs.nr, t->rs.nr);
		}
		if(t->what == Textarea)
			textscrdraw(t);
		i->aux = t;
	}else
		textresize(i->aux, im, r);
}

void
drawcheck(Image *im, Rectangle r, Formfield *f)
{
	Image *c[3];
	Point pt;
	int n;

	if(f->flags & FFchecked)
		colarray(c, getcolor(Dark), getcolor(Light), getcolor(Red), TRUE);
	else
		colarray(c, getcolor(Dark), getcolor(Light), getcolor(Back), FALSE);

	if(f->ftype == Fradio){
		n = Boxsize/2-1;
		pt = addpt(r.min, Pt(n,n));
		ellipse3d(im, pt, n, Border, c, ZP);
	}else
		rect3d(im, r, Border, c, ZP);
}

void
drawbutton(Image *im, Rectangle r, Formfield *f, int checked)
{
	Image *c[3];

	r = insetrect(r, Space);
	colarray(c, getcolor(Dark), getcolor(Light), getcolor(Back), checked);
	rect3d(im, r, Border, c, ZP);
	r.min.x += Border + Margin;
	r.min.y += Border + Margin;
	runestringbg(im, r.min, display->black, ZP,  getfont(WFont), f->value, c[2], ZP);
}

void
drawselect(Image *im, Rectangle r,	Iformfield *i)
{
	Formfield *f;
	Image *c[3];

	f = i->formfield;
	if(f->options == nil)
		return;
	r = insetrect(r, Space);
	colarray(c, getcolor(Dark), getcolor(Light), display->white, 1);
	rect3d(im, r, Border, c, ZP);
	r = insetrect(r, Border+Margin);
	draw(im, r, textcols[HIGH], nil, ZP);
	if(i->aux==nil){
		i->aux = f->options->display;
		i->formfield->value = erunestrdup(f->options->value);
	}
	runestring(im, r.min, display->black, ZP,  getfont(WFont), i->aux);
}

/* Formfields are a special case */
static
void
drawformfield(Box *b, Page *p, Image *im)
{
	Formfield *f;
	int type;

	f = ((Iformfield *)b->i)->formfield;
	type =f->ftype;
	if(istextfield(b->i))
		drawtextfield(im, rectsubpt(b->r, p->pos), (Iformfield *)b->i);
	else if(type==Fcheckbox || type==Fradio)
		drawcheck(im, rectsubpt(b->r, p->pos), f);
	else if(type==Fbutton || type==Freset || type==Fsubmit)
		drawbutton(im, rectsubpt(b->r, p->pos), f, FALSE);
	else if(type == Fimage)
		drawimage(b, p, im);
	else if(type == Fselect)
		drawselect(im, rectsubpt(b->r, p->pos), (Iformfield *)b->i);
}

static
void
drawnull(Box *, Page *, Image *)
{
}

static
Page *
whichtarget1(Page *p, Rune *r)
{
	Kidinfo *k;
	Page *c, *ret;

	k = p->kidinfo;
	if(k && k->name && runestrcmp(k->name, r)==0)
		return p;
	for(c=p->child; c; c=c->next){
		ret = whichtarget1(c, r);
		if(ret)
			return ret;
	}
	return nil;
}

static
Page *
whichtarget(Page *p, int t)
{
	Page *r;

	switch(t){
	case FTblank:
	case FTtop:
		r = &p->w->page;
		break;
	case FTself:
		r = p;
		break;
	case FTparent:
		r = p->parent;
		break;
	default:
		if(targetname(t) == L"?")
			error("targetname");
		r = whichtarget1(&p->w->page, targetname(t));
	}

	return r ? r: &p->w->page;
}

static
void
mouselink(Box *b, Page *p, int but)
{
	Runestr rs;
	Anchor *a;

	/* eat mouse */
	while(mousectl->buttons)
		readmouse(mousectl);

	if(b->i->anchorid < 0)
		return;

	/* binary search would be better */
	for(a=p->doc->anchors; a!=nil; a=a->next)
		if(a->index == b->i->anchorid)
			break;

	if(a==nil || a->href==nil)
		return;

	p = whichtarget(p, a->target);
	rs.r = urlcombine(getbase(p), a->href);
	if(rs.r == nil)
		return;
	rs.nr = runestrlen(rs.r);

	if(but == 1)
		pageget(p, &rs, nil, HGet, p==&p->w->page);
	else if(but == 2)
		textset(&p->w->status, rs.r, rs.nr);
	else if(but == 3)
		plumbrunestr(&rs, nil);
	closerunestr(&rs);
}

static
void
submit(Page *p, Formfield *formfield, int subfl)
{
	Formfield *f;
	Form *form;
	Runestr src, post;
	Rune *x, *sep, *y, *z;

	form = formfield->form;
	x = erunestrdup(L"");
	sep = L"";
	for(f=form->fields; f!=nil; f=f->next){
		if(f->ftype == Freset)
			continue;
		if((f->ftype==Fradio || f->ftype==Fcheckbox) && !(f->flags&FFchecked))
			continue;
		if(f->ftype==Fsubmit && (f!=formfield || !subfl))
			continue;
		if(f->value==nil || f->name==nil || runestrcmp(f->name, L"_no_name_submit_")==0)
			continue;

		z = ucvt(f->value);
		y = runesmprint("%S%S%S=%S", x, sep, f->name, z);
		free(z);
		sep = L"&";
		free(x);
		x = y;
	}
	p = whichtarget(p, form->target);
	y = urlcombine(getbase(p), form->action);

	memset(&src, 0, sizeof(Runestr));
	memset(&post, 0, sizeof(Runestr));
	if(form->method == HGet){
		if(y[runestrlen(y)-1] == L'?')
			sep = L"";
		else
			sep = L"?";
		src.r = runesmprint("%S%S%S",y, sep,  x);
		free(x);
		free(y);
	}else{
		src.r = y;
		post.r = x;
		post.nr = runestrlen(x);
		if(post.nr == 0){
			free(post.r);
			post.r = nil;
		}
	}
	src.nr = runestrlen(src.r);
	pageget(p, &src, &post, form->method, p==&p->w->page);
	closerunestr(&src);
	closerunestr(&post);
}

static
void
setradios(Formfield *formfield)
{
	Formfield *f;

	for(f=formfield->form->fields; f!=nil; f=f->next)
		if(f->ftype==Fradio && f!=formfield && runestrcmp(f->name, formfield->name)==0)
			f->flags &=~FFchecked;
}

static
void
selectmouse(Box *b, Page *p, int but)
{
	Formfield *f;
	Option *o;
	Menu m;
	char **item;
	int i, n;

	f = ((Iformfield *)b->i)->formfield;
	n = 0;
	item = nil;
	for(o=f->options; o!=nil; o=o->next){
		item = erealloc(item, ++n*sizeof(char *));
		if(o->display)
			item[n-1] = smprint("%S", o->display);
		else
			item[n-1] = estrdup("--");
	}
	if(item == nil)
		return;

	item[n] = 0;
	m.item = item;
	i = menuhit(but, mousectl, &m, nil);
	if(i >= 0){
		for(o=f->options; o!=nil; o=o->next, i--){
			if(i == 0)
				break;
		}
		((Iformfield *)b->i)->aux = o->display;
		drawselect(p->b, rectaddpt(rectsubpt(b->r, p->pos), p->r.min), (Iformfield *)b->i);
		if(f->value != nil)
			free(f->value);
		f->value = erunestrdup(o->value);
	}
	for(i=0; i< n; i++)
		free(item[i]);
//	free(item);
}

static
void
mouseform(Box *b, Page *p, int but)
{
	Rectangle r, cr;
	Formfield *f;
	Text *t;

	f = ((Iformfield *)b->i)->formfield;
	r = rectaddpt(rectsubpt(b->r, p->pos), p->r.min);
	if(istextfield(b->i)){
		cr = p->b->clipr;
		replclipr(p->b, 0, p->r);
		t = ((Iformfield *)b->i)->aux;
		if(p->b != t->b)
			drawtextfield(p->b, r, (Iformfield *)b->i);
		textmouse(t, mouse->xy, but);
		if(f->value)
			free(f->value);
		f->value = runesmprint("%.*S", t->rs.nr, t->rs.r);
		replclipr(p->b, 0, cr);
		return;
	}

	if(but != 1)
		return;

	if(f->ftype==Fselect){
		selectmouse(b, p, but);
		return;
	}
	if(f->ftype==Fsubmit || f->ftype==Fimage){
		if(f->ftype == Fsubmit)
			drawbutton(p->b, r, f, TRUE);
		while(mouse->buttons == but)
			readmouse(mousectl);
		if(f->ftype == Fsubmit)
			drawbutton(p->b, r, f, FALSE);
		if(mouse->buttons==0 && ptinrect(mouse->xy, r))
			submit(p, f, TRUE);
		return;
	}
	if(f->ftype==Fradio || f->ftype==Fcheckbox){
		if(f->flags&FFchecked){
			if(f->ftype==Fcheckbox)
				f->flags &=~FFchecked;
		}else{
			f->flags |= FFchecked;
		}
		if(f->ftype == Fradio)
			setradios(f);
		pageredraw(p);
	}
}

static
void
keyform(Box *b, Page *p, Rune r)
{
	Rectangle cr;
	Formfield *f;
	Text *t;

	f = ((Iformfield *)b->i)->formfield;
	if(r==L'\n' && f->ftype==Ftext){
		submit(p, f, FALSE);
		return;
	}
	t = ((Iformfield *)b->i)->aux;
	cr = p->b->clipr;
	replclipr(p->b, 0, p->r);
	if(t->b != p->b)
		drawtextfield(p->b, rectaddpt(rectsubpt(b->r, p->pos), p->r.min), (Iformfield *)b->i);
	texttype(t, r);
	if(f->value)
		free(f->value);
	f->value = runesmprint("%.*S", t->rs.nr, t->rs.r);
	replclipr(p->b, 0, cr);
}

void
boxinit(Box *b)
{
	if(b->i->anchorid)
		b->mouse = mouselink;
	/* override mouselink for forms */
	if(b->i->tag == Iformfieldtag){
		b->mouse = mouseform;
		if(istextfield(b->i))
			b->key = keyform;
	}
	switch(b->i->tag){
	case Itexttag:
		b->draw = drawtext;
		break;
	case Iruletag:
		b->draw = drawrule;
		break;
	case Iimagetag:
		b->draw = drawimage;
		break;
	case Iformfieldtag:
		b->draw = drawformfield;
		break;
	case Itabletag:
		b->draw = drawtable;
		break;
	case Ifloattag:
		b->draw = drawnull;
		break;
	case Ispacertag:
		b->draw = drawnull;
	}
}

Box *
boxalloc(Line *l, Item *i, Rectangle r)
{
	Box *b;

	b = emalloc(sizeof(Box));
	b->i = i;
	b->r = r;
	if(l->boxes == nil)
		l->boxes = b;
	else{
		b->prev = l->lastbox;
		l->lastbox->next = b;
	}
	l->lastbox = b;

	return b;
}

Box *
pttobox(Line *l, Point xy)
{
	Box *b;

	for(b=l->boxes; b!=nil; b=b->next)
		if(ptinrect(xy, b->r))
			return b;

	return nil;
}

static
Line *
tbtoline(Itable *i, Point xy)
{
	Tablecell *c;

	for(c=i->table->cells; c!=nil; c=c->next)
		if(ptinrect(xy, c->lay->r))
			return linewhich(c->lay, xy);

	return nil;
}

Line *
linewhich(Lay *lay, Point xy)
{
	Line *l, *t;
	Box *b;

	t = nil;
	for(l=lay->lines; l!=nil; l=l->next)
		if(ptinrect(xy, l->r))
			break;

	if(l!=nil && l->hastable){
		b = pttobox(l, xy);
		if(b!=nil && b->i->tag==Itabletag)
			t = tbtoline((Itable *)b->i, xy);
	}
	return t? t: l;
}

Box *
boxwhich(Lay *lay, Point xy)
{
	Line *l;

	l = linewhich(lay, xy);
	if(l)
		return pttobox(l, xy);

	return nil;
}

static void justline1(Line *, int);

static
void
justlay(Lay *lay, int x)
{
	Line *l;

	lay->r.min.x += x;
	lay->r.max.x += x;

	for(l=lay->lines; l!=nil; l=l->next)
		justline1(l, x);
}

static
void
justtable(Itable *i, int x)
{
	Tablecell *c;

	for(c=i->table->cells; c!=nil; c=c->next)
		justlay(c->lay, x);
}

static
void
justline1(Line *l, int x)
{
	Box *b;

	l->r.min.x += x;
	l->r.max.x += x;
	for(b=l->boxes; b!=nil; b=b->next){
		if(b->i->tag == Itabletag)
			justtable((Itable *)b->i, x);
		b->r.min.x += x;
		b->r.max.x += x;
	}
}

static
void
justline(Lay *lay, Line *l)
{

	int w, x;

	w = Dx(l->r);
	if(w>0 && w<lay->width){
		x = 0;
		if(l->state & IFrjust)
			x = lay->width - w;
		else if(l->state & IFcjust)
			x = lay->width/2 - w/2;
		if(x > 0)
			justline1(l, x);
	}
}

static
void
newline(Lay *lay, int state)
{
	Line *l, *last;
	int indent, nl;

	last = lay->lastline;
	if(lay->laying == TRUE)
		justline(lay, last);

	lay->r.max.x = max(lay->r.max.x, last->r.max.x);
	lay->r.max.y = last->r.max.y;

	indent = ((state&IFindentmask)>>IFindentshift) * Tabspace;
	nl = (state & IFbrksp) ? 1 : 0;

	l = emalloc(sizeof(Line));
	l->state = state;
	l->hastext = FALSE;
	l->hastable = FALSE;
	l->r.min.x = lay->r.min.x + indent;
	l->r.min.y = last->r.max.y + font->height*nl;
	l->r.max = l->r.min;
	l->prev = last;
	last->next = l;
	lay->lastline = l;
}


static
void
layitem(Lay *lay, Item *i)
{
	Rectangle r;
	Line *l;
	Box *b;

	if(i->state&IFbrk || i->state&IFbrksp)
		newline(lay, i->state);
	else	if(lay->lastline->r.max.x+i->width>lay->xwall && forceitem(i)==FALSE)
		newline(lay, i->state);

	l = lay->lastline;
	r = Rect(l->r.max.x, l->r.min.y, l->r.max.x+i->width, l->r.min.y+i->height);
	l->r.max.x = r.max.x;
	if(l->r.max.y < r.max.y)
		l->r.max.y = r.max.y;

	if(i->tag == Ifloattag)
		i = ((Ifloat *)i)->item;
	if(i->tag == Itexttag)
		l->hastext = TRUE;
	else if(i->tag == Itabletag && lay->laying==TRUE){
		laytable((Itable *)i, r);
		l->hastable = TRUE;
	}
	b = boxalloc(l, i, r);
	if(lay->laying)
		boxinit(b);
}

static
void
linefix(Lay *lay)
{
	Line *l;

	for(l=lay->lines; l!=nil; l=l->next){
		l->r.min.x = lay->r.min.x;
		l->r.max.x = lay->r.max.x;
	}
}

Lay *
layitems(Item *items, Rectangle r, int laying)
{
	Lay *lay;
	Line *l;
	Item *i;

	lay = emalloc(sizeof(Lay));
	lay->r.min = r.min;
	lay->r.max = r.min;
	lay->xwall = r.max.x;
	lay->width = Dx(r);
	lay->laying = laying;
	l = emalloc(sizeof(Line));
	l->r.min = lay->r.min;
	l->r.max = lay->r.min;
	l->state = IFbrk;
	l->boxes = nil;
	lay->lines = l;
	lay->lastline = l;
	lay->font = font;

	for(i=items; i; i=i->next){
		sizeitem(lay, i);
		layitem(lay, i);
	}
	newline(lay, IFbrk);
	if(laying)
		linefix(lay);

	return lay;
}

void
laypage(Page *p)
{
	settables(p);
	layfree(p->lay);
	p->lay = layitems(p->items, Rect(0,0,Dx(p->r),Dy(p->r)), TRUE);
	p->lay->r.max.y = max(p->lay->r.max.y, Dy(p->r));
}

static
void
drawline(Page *p, Image *im, Line *l)
{
	Box *b;

	for(b=l->boxes; b!=nil; b=b->next)
		b->draw(b, p, im);
}

void
laydraw(Page *p, Image *im, Lay *lay)
{
	Rectangle r;
	Line *l;

	r = rectaddpt(p->lay->r, p->pos);
	for(l=lay->lines; l!=nil; l=l->next){
		if(rectXrect(r, l->r))
			drawline(p, im, l);
	}
}

static
void
laytablefree(Table *t)
{
	Tablecell *c;

	for(c=t->cells; c!=nil; c=c->next){
		layfree(c->lay);
		c->lay = nil;
	}
}

void
layfree(Lay *lay)
{
	Line *l, *nextline;
	Box *b, *nextbox;
	void **aux;

	if(lay == nil)
		return;

	for(l=lay->lines; l!=nil; l=nextline){
		for(b=l->boxes; b!=nil; b=nextbox){
			nextbox = b->next;
			if(b->i->tag==Iformfieldtag && istextfield(b->i)){
				aux = &((Iformfield *)b->i)->aux;
				if(*aux){
					textclose(*aux);
					free(*aux);
				}
				*aux = nil;
			}else if(b->i->tag == Itabletag)
				laytablefree(((Itable *)b->i)->table);

			free(b);
		}
		nextline = l->next;
		free(l);
	}
	free(lay);
}

void
laysnarf(Page *p, Lay *lay, Runestr *rs)
{
	Tablecell *c;
	Itext *i;
	Font *f;
	Line *l;
	Box *b;
	int q0, q1, n;

	for(l=lay->lines; l!=nil; l=l->next) for(b=l->boxes; b!=nil; b=b->next){
		if(p->selecting && hasbrk(b->i->state)){
			rs->r = runerealloc(rs->r, rs->nr+2);
			rs->r[rs->nr++] = L'\n';
			rs->r[rs->nr] = L'\0';
		}
		if(b->i->tag==Itexttag){
			i = (Itext *)b->i;
			f = getfont(i->fnt);
			if(istextsel(p, b->r, &q0, &q1, i->s, f)){
				if(q1 == 0)
					q1 = runestrlen(i->s);
				n = q1-q0;
				if(n == 0)
					n = runestrlen(i->s);
				rs->r = runerealloc(rs->r, rs->nr+n+2);
				runemove(rs->r+rs->nr, i->s+q0, n);
				rs->nr += n;
				rs->r[rs->nr++] = L' ';
				rs->r[rs->nr] = L'\0';
			}
		}else if(b->i->tag == Itabletag)
			for(c=((Itable *)b->i)->table->cells; c!=nil; c=c->next)
				if(c->lay)
					laysnarf(p, c->lay, rs);
	}
}
