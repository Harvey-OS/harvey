#include <u.h>
#include <libc.h>
#include <libg.h>
#include <frame.h>
#include "flayer.h"
#include "samterm.h"

#define	DELTA	10

static Flayer	**llist;	/* front to back */
static int	nllist;
static int	nlalloc;
static Rectangle lDrect;

extern Bitmap	screen;
extern Mouse	mouse;

Vis		visibility(Flayer *);
void		newvisibilities(int);
void		llinsert(Flayer*);
void		lldelete(Flayer*);

void
flstart(Rectangle r)
{
	lDrect = r;
}

void
flnew(Flayer *l, Rune *(*fn)(Flayer*, long, ulong*), int u0, void *u1)
{
	if(nllist == nlalloc){
		nlalloc += DELTA;
		llist = realloc(llist, nlalloc*sizeof(Flayer**));
		if(llist == 0)
			panic("flnew");
	}
	l->textfn = fn;
	l->user0 = u0;
	l->user1 = u1;
	llinsert(l);
}

Rectangle
flrect(Flayer *l, Rectangle r)
{
	rectclip(&r, lDrect);
	l->entire = r;
	l->scroll = inset(r, FLMARGIN);
	r.min.x =
	 l->scroll.max.x = r.min.x+FLMARGIN+FLSCROLLWID+(FLGAP-FLMARGIN);
	return r;
}

void
flinit(Flayer *l, Rectangle r, Font *ft)
{
	lldelete(l);
	llinsert(l);
	l->visible = All;
	l->origin = l->p0 = l->p1 = 0;
	frinit(&l->f, inset(flrect(l, r), FLMARGIN), ft, &screen);
	newvisibilities(1);
	bitblt(&screen, l->entire.min, &screen, l->entire, 0);
	scrdraw(l, 0L);
	flborder(l, 0);
}

void
flclose(Flayer *l)
{
	if(l->visible == All)
		bitblt(&screen, l->entire.min, &screen, l->entire, 0);
	else if(l->visible == Some){
		if(l->f.b == 0)
			l->f.b = balloc(l->entire, screen.ldepth);
		if(l->f.b){
			bitblt(l->f.b, l->entire.min, l->f.b, l->entire, 0);
			flrefresh(l, l->entire, 0);
		}
	}
	frclear(&l->f);
	lldelete(l);
	if(l->f.b && l->visible!=All)
		bfree(l->f.b);
	l->textfn = 0;
	newvisibilities(1);
}

void
flborder(Flayer *l, int wide)
{
	if(flprepare(l)){
		border(l->f.b, l->entire, FLMARGIN, 0);
		border(l->f.b, l->entire, wide? FLMARGIN : 1, F&~D);
		if(l->visible==Some)
			flrefresh(l, l->entire, 0);
	}
}

Flayer *
flwhich(Point p)
{
	int i;

	if(p.x==0 && p.y==0)
		return nllist? llist[0] : 0;
	for(i=0; i<nllist; i++)
		if(ptinrect(p, llist[i]->entire))
			return llist[i];
	return 0;
}

void
flupfront(Flayer *l)
{
	int v = l->visible;

	lldelete(l);
	llinsert(l);
	if(v!=All)
		newvisibilities(0);
}

void
newvisibilities(int redraw)
	/* if redraw false, we know it's a flupfront, and needn't
	 * redraw anyone becoming partially covered */
{
	int i;
	Vis ov;
	Flayer *l;

	for(i = 0; i<nllist; i++){
		l = llist[i];
		ov = l->visible;
		l->visible = visibility(l);
#define	V(a, b)	(((a)<<2)|((b)))
		switch(V(ov, l->visible)){
		case V(Some, None):
			if(l->f.b)
				bfree(l->f.b);
		case V(All, None):
		case V(All, Some):
			l->f.b = 0;
			frclear(&l->f);
			break;

		case V(Some, Some):
			if(l->f.b==0 && redraw)
		case V(None, Some):
				flprepare(l);
			if(l->f.b && redraw){
				flrefresh(l, l->entire, 0);
				bfree(l->f.b);
				l->f.b = 0;
				frclear(&l->f);
			}
		case V(None, None):
		case V(All, All):
			break;

		case V(Some, All):
			if(l->f.b){
				bitblt(&screen, l->entire.min, l->f.b, l->entire, S);
				bfree(l->f.b);
				l->f.b = &screen;
				break;
			}
		case V(None, All):
			flprepare(l);
			break;
		}
		if(ov==None && l->visible!=None)
			flnewlyvisible(l);
	}
}

void
llinsert(Flayer *l)
{
	int i;
	for(i=nllist; i>0; --i)
		llist[i]=llist[i-1];
	llist[0]=l;
	nllist++;
}

void
lldelete(Flayer *l)
{
	int i;

	for(i=0; i<nllist; i++)
		if(llist[i]==l){
			--nllist;
			for(; i<nllist; i++)
				llist[i] = llist[i+1];
			return;
		}
	panic("lldelete");
}

void
flinsert(Flayer *l, Rune *sp, Rune *ep, long p0)
{
	if(flprepare(l)){
		frinsert(&l->f, sp, ep, p0-l->origin);
		scrdraw(l, scrtotal(l));
		if(l->visible==Some)
			flrefresh(l, l->entire, 0);
	}
}

void
fldelete(Flayer *l, long p0, long p1)
{
	if(flprepare(l)){
		p0 -= l->origin;
		if(p0 < 0)
			p0 = 0;
		p1 -= l->origin;
		if(p1<0)
			p1 = 0;
		frdelete(&l->f, p0, p1);
		scrdraw(l, scrtotal(l));
		if(l->visible==Some)
			flrefresh(l, l->entire, 0);
	}
}

int
flselect(Flayer *l)
{
	int ret = 0;
	if(l->visible!=All)
		flupfront(l);
	frselect(&l->f, &mouse);
	if(l->f.p0==l->f.p1){
		if(mouse.msec-l->click<Clicktime && l->f.p0+l->origin==l->p0){
			ret = 1;
			l->click = 0;
		}else
			l->click = mouse.msec;
	}else
		l->click = 0;
	l->p0 = l->f.p0+l->origin, l->p1 = l->f.p1+l->origin;
	return ret;
}

void
flsetselect(Flayer *l, long p0, long p1)
{
	ulong fp0, fp1;

	l->click = 0;
	if(l->visible==None || !flprepare(l)){
		l->p0 = p0, l->p1 = p1;
		return;
	}
	l->p0 = p0, l->p1 = p1;
	flfp0p1(l, &fp0, &fp1);
	if(fp0==l->f.p0 && fp1==l->f.p1)
		return;
	frselectp(&l->f, F&~D);
	l->f.p0 = fp0, l->f.p1 = fp1;
	frselectp(&l->f, F&~D);
	if(l->visible==Some)
		flrefresh(l, l->entire, 0);
}

void
flfp0p1(Flayer *l, ulong *pp0, ulong *pp1)
{
	long p0 = l->p0-l->origin, p1 = l->p1-l->origin;

	if(p0 < 0)
		p0 = 0;
	if(p1 < 0)
		p1 = 0;
	if(p0 > l->f.nchars)
		p0 = l->f.nchars;
	if(p1 > l->f.nchars)
		p1 = l->f.nchars;
	*pp0 = p0;
	*pp1 = p1;
}

Rectangle
rscale(Rectangle r, Point old, Point new)
{
	r.min.x = r.min.x*new.x/old.x;
	r.min.y = r.min.y*new.y/old.y;
	r.max.x = r.max.x*new.x/old.x;
	r.max.y = r.max.y*new.y/old.y;
	return r;
}

void
flreshape(Rectangle dr)
{
	int i;
	Flayer *l;
	Frame *f;
	Rectangle r, olDrect;
	int move;

	olDrect = lDrect;
	lDrect = dr;
	move = 0;
	if(Dx(dr)==Dx(olDrect) && Dy(dr)==Dy(olDrect))
		move = 1;
	else
		bitblt(&screen, lDrect.min, &screen, lDrect, 0);
	for(i=0; i<nllist; i++){
		l = llist[i];
		f = &l->f;
		if(move)
			r = raddp(rsubp(l->entire, olDrect.min), dr.min);
		else{
			r = raddp(rscale(rsubp(l->entire, olDrect.min),
				sub(olDrect.max, olDrect.min),
				sub(dr.max, dr.min)), dr.min);
			if(l->visible==Some && f->b){
				bfree(f->b);
				frclear(f);
			}
			f->b = 0;
			if(l->visible!=None)
				frclear(f);
		}
		if(!rectclip(&r, dr))
			panic("flreshape");
		if(r.max.x-r.min.x<100)
			r.min.x = dr.min.x;
		if(r.max.x-r.min.x<100)
			r.max.x = dr.max.x;
		if(r.max.y-r.min.y<2*FLMARGIN+f->font->height)
			r.min.y = dr.min.y;
		if(r.max.y-r.min.y<2*FLMARGIN+f->font->height)
			r.max.y = dr.max.y;
		if(!move)
			l->visible = None;
		frsetrects(f, inset(flrect(l, r), FLMARGIN), f->b);
		if(!move && f->b)
			scrdraw(l, scrtotal(l));
	}
	newvisibilities(1);
}

int
flprepare(Flayer *l)
{
	Frame *f;
	ulong n;
	Rune *r;

	if(l->visible == None)
		return 0;
	f = &l->f;
	if(f->b == 0){
		if(l->visible == All)
			f->b = &screen;
		else if((f->b = balloc(l->entire, screen.ldepth))==0)
			return 0;
		bitblt(f->b, l->entire.min, f->b, l->entire, 0);
		border(f->b, l->entire, l==llist[0]? FLMARGIN : 1, F&~D);
		n = f->nchars;
		frinit(f, f->entire, f->font, f->b);
		r = (*l->textfn)(l, n, &n);
		frinsert(f, r, r+n, (ulong)0);
		frselectp(f, F&~D);
		flfp0p1(l, &l->f.p0, &l->f.p1);
		frselectp(f, F&~D);
		scrdraw(l, scrtotal(l));
	}
	return 1;
}

static	int	somevis, someinvis, justvis;

Vis
visibility(Flayer *l)
{
	somevis = someinvis = 0;
	justvis = 1;
	flrefresh(l, l->entire, 0);
	justvis = 0;
	if(somevis==0)
		return None;
	if(someinvis==0)
		return All;
	return Some;
}

void
flrefresh(Flayer *l, Rectangle r, int i)
{
	Flayer *t;
	Rectangle s;

    Top:
	if((t=llist[i++]) == l){
		if(!justvis)
			bitblt(&screen, r.min, l->f.b, r, S);
		somevis = 1;
	}else{
		if(!rectXrect(t->entire, r))
			goto Top;	/* avoid stacking unnecessarily */
		if(t->entire.min.x>r.min.x){
			s = r;
			s.max.x = t->entire.min.x;
			flrefresh(l, s, i);
			r.min.x = t->entire.min.x;
		}
		if(t->entire.min.y>r.min.y){
			s = r;
			s.max.y = t->entire.min.y;
			flrefresh(l, s, i);
			r.min.y = t->entire.min.y;
		}
		if(t->entire.max.x<r.max.x){
			s = r;
			s.min.x = t->entire.max.x;
			flrefresh(l, s, i);
			r.max.x = t->entire.max.x;
		}
		if(t->entire.max.y<r.max.y){
			s = r;
			s.min.y = t->entire.max.y;
			flrefresh(l, s, i);
			r.max.y = t->entire.max.y;
		}
		/* remaining piece of r is blocked by t; forget about it */
		someinvis = 1;
	}
}
