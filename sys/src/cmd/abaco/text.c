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

Image	*tagcols[NCOL];
Image	*textcols[NCOL];


void
textinit(Text *t, Image *b, Rectangle r, Font *f, Image *cols[NCOL])
{
	t->all = r;
	t->scrollr = r;
	t->scrollr.max.x = r.min.x+Scrollsize;
	t->lastsr = ZR;
	r.min.x += Scrollsize+Scrollgap;
	t->rs.nr = 0;
	memmove(t->Frame.cols, cols, sizeof t->Frame.cols);
	textredraw(t, r, f, b);
}

void
textredraw(Text *t, Rectangle r, Font *f, Image *b)
{
	Rectangle r1;

	frinit(t, r, f, b, t->Frame.cols);
	r1 = t->r;
	r1.min.x -= Scrollsize+Scrollgap;	/* back fill to scroll bar */
	draw(t->b, r1, t->cols[BACK], nil, ZP);
	t->maxtab = Maxtab*stringwidth(f, "0");
	textfill(t);
	textsetselect(t, t->q0, t->q1);
}

int
textresize(Text *t, Image *b, Rectangle r)
{
	if(Dy(r) > 0)
		r.max.y -= Dy(r)%t->font->height;
	else
		r.max.y = r.min.y;

	t->all = r;
	t->scrollr = r;
	t->scrollr.max.x = r.min.x+Scrollsize;
	t->lastsr = ZR;
	r.min.x += Scrollsize+Scrollgap;
	frclear(t, 0);
	textredraw(t, r, t->font, b);
	if(t->what == Textarea)
		textscrdraw(t);
	return r.max.y;
}

void
textclose(Text *t)
{
	closerunestr(&t->rs);
	frclear(t, 1);
}

void
textinsert(Text *t, uint q0, Rune *r, uint n)
{
	if(n == 0)
		return;

	t->rs.r = runerealloc(t->rs.r, t->rs.nr+n);
	runemove(t->rs.r+q0+n, t->rs.r+q0, t->rs.nr-q0);
	runemove(t->rs.r+q0, r, n);
	t->rs.nr += n;
	if(q0 < t->q1)
		t->q1 += n;
	if(q0 < t->q0)
		t->q0 += n;
	if(q0 < t->org)
		t->org += n;
	else if(q0 <= t->org+t->nchars)
		frinsert(t, r, r+n, q0-t->org);
}

void
textfill(Text *t)
{
	Rune *rp;
	int i, n, m, nl;

	if(t->lastlinefull)
		return;
	rp = runemalloc(BUFSIZE*8);
	do{
		n = t->rs.nr-(t->org+t->nchars);
		if(n == 0)
			break;
		if(n > 2000)	/* educated guess at reasonable amount */
			n = 2000;
		runemove(rp, t->rs.r+(t->org+t->nchars), n);
		/*
		 * it's expensive to frinsert more than we need, so
		 * count newlines.
		 */
		nl = t->maxlines-t->nlines;
		m = 0;
		for(i=0; i<n; ){
			if(rp[i++] == '\n'){
				m++;
				if(m >= nl)
					break;
			}
		}
		frinsert(t, rp, rp+i, t->nchars);
	}while(t->lastlinefull == FALSE);
	free(rp);
}

void
textdelete(Text *t, uint q0, uint q1)
{
	uint n, p0, p1;

	n = q1-q0;
	if(n == 0)
		return;

	runemove(t->rs.r+q0, t->rs.r+q1, t->rs.nr-q1);
	t->rs.nr -= n;
	if(q0 < t->q0)
		t->q0 -= min(n, t->q0-q0);
	if(q0 < t->q1)
		t->q1 -= min(n, t->q1-q0);
	if(q1 <= t->org)
		t->org -= n;
	else if(q0 < t->org+t->nchars){
		p1 = q1 - t->org;
		if(p1 > t->nchars)
			p1 = t->nchars;
		if(q0 < t->org){
			t->org = q0;
			p0 = 0;
		}else
			p0 = q0 - t->org;
		frdelete(t, p0, p1);
		textfill(t);
	}
	t->rs.r[t->rs.nr] = L'\0';
}

int
textbswidth(Text *t, Rune c)
{
	uint q, eq;
	Rune r;
	int skipping;

	/* there is known to be at least one character to erase */
	if(c == 0x08)	/* ^H: erase character */
		return 1;
	q = t->q0;
	skipping = TRUE;
	while(q > 0){
		r = t->rs.r[q-1];
		if(r == '\n'){		/* eat at most one more character */
			if(q == t->q0)	/* eat the newline */
				--q;
			break;
		}
		if(c == 0x17){
			eq = isalnum(r);
			if(eq && skipping)	/* found one; stop skipping */
				skipping = FALSE;
			else if(!eq && !skipping)
				break;
		}
		--q;
	}
	return t->q0-q;
}

void
texttype(Text *t, Rune r)
{
	uint q0, q1;
	int nb, n;
	int nr;
	Rune *rp;

	if(t->what!=Textarea && r=='\n'){
		if(t->what==Urltag)
			get(t, t, XXX, XXX, nil, 0);
		return;
	}
	if(t->what==Tag && (r==Kscrollonedown || r==Kscrolloneup))
		return;

	nr = 1;
	rp = &r;
	switch(r){
	case Kleft:
		if(t->q0 > 0)
			textshow(t, t->q0-1, t->q0-1, TRUE);
		return;
	case Kright:
		if(t->q1 < t->rs.nr)
			textshow(t, t->q1+1, t->q1+1, TRUE);
		return;
	case Kdown:
		n = t->maxlines/3;
		goto case_Down;
	case Kscrollonedown:
		n = mousescrollsize(t->maxlines);
		if(n <= 0)
			n = 1;
		goto case_Down;
	case Kpgdown:
		n = 2*t->maxlines/3;
	case_Down:
		q0 = t->org+frcharofpt(t, Pt(t->r.min.x, t->r.min.y+n*t->font->height));
		textsetorigin(t, q0, TRUE);
		return;
	case Kup:
		n = t->maxlines/3;
		goto case_Up;
	case Kscrolloneup:
		n = mousescrollsize(t->maxlines);
		goto case_Up;
	case Kpgup:
		n = 2*t->maxlines/3;
	case_Up:
		q0 = textbacknl(t, t->org, n);
		textsetorigin(t, q0, TRUE);
		return;
	case Khome:
		textshow(t, 0, 0, FALSE);
		return;
	case Kend:
		textshow(t, t->rs.nr, t->rs.nr, FALSE);
		return;
	case 0x01:	/* ^A: beginning of line */
		/* go to where ^U would erase, if not already at BOL */
		nb = 0;
		if(t->q0>0 && t->rs.r[t->q0-1]!='\n')
			nb = textbswidth(t, 0x15);
		textshow(t, t->q0-nb, t->q0-nb, TRUE);
		return;
	case 0x05:	/* ^E: end of line */
		q0 = t->q0;
		while(q0<t->rs.nr && t->rs.r[q0]!='\n')
			q0++;
		textshow(t, q0, q0, TRUE);
		return;
	}
	if(t->q1 > t->q0)
		cut(t, t, TRUE, TRUE, nil, 0);

	textshow(t, t->q0, t->q0, TRUE);
	switch(r){
	case 0x08:	/* ^H: erase character */
	case 0x15:	/* ^U: erase line */
	case 0x17:	/* ^W: erase word */
		if(t->q0 == 0)	/* nothing to erase */
			return;
		nb = textbswidth(t, r);
		q1 = t->q0;
		q0 = q1-nb;
		/* if selection is at beginning of window, avoid deleting invisible text */
		if(q0 < t->org){
			q0 = t->org;
			nb = q1-q0;
		}
		if(nb > 0){
			textdelete(t, q0, q0+nb);
			textsetselect(t, q0, q0);
		}
		return;
	}
	/* otherwise ordinary character; just insert */
	textinsert(t, t->q0, &r, 1);
	if(rp != &r)
		free(rp);
	textsetselect(t, t->q0+nr, t->q0+nr);
	if(t->what == Textarea)
		textscrdraw(t);
}

static	Text	*clicktext;
static	uint	clickmsec;
static	Text	*selecttext;
static	uint	selectq;

/*
 * called from frame library
 */
void
framescroll(Frame *f, int dl)
{
	if(f != &selecttext->Frame)
		error("frameselect not right frame");
	textframescroll(selecttext, dl);
}

void
textframescroll(Text *t, int dl)
{
	uint q0;

	if(dl == 0){
		scrsleep(100);
		return;
	}
	if(dl < 0){
		q0 = textbacknl(t, t->org, -dl);
		if(selectq > t->org+t->p0)
			textsetselect(t, t->org+t->p0, selectq);
		else
			textsetselect(t, selectq, t->org+t->p0);
	}else{
		if(t->org+t->nchars == t->rs.nr)
			return;
		q0 = t->org+frcharofpt(t, Pt(t->r.min.x, t->r.min.y+dl*t->font->height));
		if(selectq > t->org+t->p1)
			textsetselect(t, t->org+t->p1, selectq);
		else
			textsetselect(t, selectq, t->org+t->p1);
	}
	textsetorigin(t, q0, TRUE);
}

void
textselect(Text *t)
{
	uint q0, q1;
	int b, x, y;
	int state;

	selecttext = t;
	/*
	 * To have double-clicking and chording, we double-click
	 * immediately if it might make sense.
	 */
	b = mouse->buttons;
	q0 = t->q0;
	q1 = t->q1;
	selectq = t->org+frcharofpt(t, mouse->xy);
	if(clicktext==t && mouse->msec-clickmsec<500)
	if(q0==q1 && selectq==q0){
		textdoubleclick(t, &q0, &q1);
		textsetselect(t, q0, q1);
		flushimage(display, 1);
		x = mouse->xy.x;
		y = mouse->xy.y;
		/* stay here until something interesting happens */
		do
			readmouse(mousectl);
		while(mouse->buttons==b && abs(mouse->xy.x-x)<3 && abs(mouse->xy.y-y)<3);
		mouse->xy.x = x;	/* in case we're calling frselect */
		mouse->xy.y = y;
		q0 = t->q0;	/* may have changed */
		q1 = t->q1;
		selectq = q0;
	}
	if(mouse->buttons == b){
		t->Frame.scroll = framescroll;
		frselect(t, mousectl);
		/* horrible botch: while asleep, may have lost selection altogether */
		if(selectq > t->rs.nr)
			selectq = t->org + t->p0;
		t->Frame.scroll = nil;
		if(selectq < t->org)
			q0 = selectq;
		else
			q0 = t->org + t->p0;
		if(selectq > t->org+t->nchars)
			q1 = selectq;
		else
			q1 = t->org+t->p1;
	}
	if(q0 == q1){
		if(q0==t->q0 && clicktext==t && mouse->msec-clickmsec<500){
			textdoubleclick(t, &q0, &q1);
			clicktext = nil;
		}else{
			clicktext = t;
			clickmsec = mouse->msec;
		}
	}else
		clicktext = nil;
	textsetselect(t, q0, q1);
	flushimage(display, 1);
	state = 0;	/* undo when possible; +1 for cut, -1 for paste */
	while(mouse->buttons){
		mouse->msec = 0;
		b = mouse->buttons;
		if((b&1) && (b&6)){
			if(b & 2){
				if(state==-1 && t->what==Textarea){
					textsetselect(t, q0, t->q0);
					state = 0;
				}else if(state != 1){
					cut(t, t, TRUE, TRUE, nil, 0);
					state = 1;
				}
			}else{
				if(state==1 && t->what==Textarea){
					textsetselect(t, q0, t->q1);
					state = 0;
				}else if(state != -1){
					paste(t, t, TRUE, FALSE, nil, 0);
					state = -1;
				}
			}
			textscrdraw(t);
		}
		flushimage(display, 1);
		while(mouse->buttons == b)
			readmouse(mousectl);
		clicktext = nil;
	}
}

void
textshow(Text *t, uint q0, uint q1, int doselect)
{
	int qe;
	int nl;
	uint q;

	if(t->what != Textarea){
		if(doselect)
			textsetselect(t, q0, q1);
		return;
	}
	if(doselect)
		textsetselect(t, q0, q1);
	qe = t->org+t->nchars;
	if(t->org<=q0 && (q0<qe || (q0==qe && qe==t->rs.nr)))
		textscrdraw(t);
	else{
		nl = t->maxlines/4;
		q = textbacknl(t, q0, nl);
		/* avoid going backwards if trying to go forwards - long lines! */
		if(!(q0>t->org && q<t->org))
			textsetorigin(t, q, TRUE);
		while(q0 > t->org+t->nchars)
			textsetorigin(t, t->org+1, FALSE);
	}
}

static
int
region(int a, int b)
{
	if(a < b)
		return -1;
	if(a == b)
		return 0;
	return 1;
}

void
selrestore(Frame *f, Point pt0, uint p0, uint p1)
{
	if(p1<=f->p0 || p0>=f->p1){
		/* no overlap */
		frdrawsel0(f, pt0, p0, p1, f->cols[BACK], f->cols[TEXT]);
		return;
	}
	if(p0>=f->p0 && p1<=f->p1){
		/* entirely inside */
		frdrawsel0(f, pt0, p0, p1, f->cols[HIGH], f->cols[HTEXT]);
		return;
	}

	/* they now are known to overlap */

	/* before selection */
	if(p0 < f->p0){
		frdrawsel0(f, pt0, p0, f->p0, f->cols[BACK], f->cols[TEXT]);
		p0 = f->p0;
		pt0 = frptofchar(f, p0);
	}
	/* after selection */
	if(p1 > f->p1){
		frdrawsel0(f, frptofchar(f, f->p1), f->p1, p1, f->cols[BACK], f->cols[TEXT]);
		p1 = f->p1;
	}
	/* inside selection */
	frdrawsel0(f, pt0, p0, p1, f->cols[HIGH], f->cols[HTEXT]);
}

void
textsetselect(Text *t, uint q0, uint q1)
{
	int p0, p1;

	/* t->p0 and t->p1 are always right; t->q0 and t->q1 may be off */
	t->q0 = q0;
	t->q1 = q1;
	/* compute desired p0,p1 from q0,q1 */
	p0 = q0-t->org;
	p1 = q1-t->org;
	if(p0 < 0)
		p0 = 0;
	if(p1 < 0)
		p1 = 0;
	if(p0 > t->nchars)
		p0 = t->nchars;
	if(p1 > t->nchars)
		p1 = t->nchars;
	if(p0==t->p0 && p1==t->p1)
		return;
	/* screen disagrees with desired selection */
	if(t->p1<=p0 || p1<=t->p0 || p0==p1 || t->p1==t->p0){
		/* no overlap or too easy to bother trying */
		frdrawsel(t, frptofchar(t, t->p0), t->p0, t->p1, 0);
		frdrawsel(t, frptofchar(t, p0), p0, p1, 1);
		goto Return;
	}
	/* overlap; avoid unnecessary painting */
	if(p0 < t->p0){
		/* extend selection backwards */
		frdrawsel(t, frptofchar(t, p0), p0, t->p0, 1);
	}else if(p0 > t->p0){
		/* trim first part of selection */
		frdrawsel(t, frptofchar(t, t->p0), t->p0, p0, 0);
	}
	if(p1 > t->p1){
		/* extend selection forwards */
		frdrawsel(t, frptofchar(t, t->p1), t->p1, p1, 1);
	}else if(p1 < t->p1){
		/* trim last part of selection */
		frdrawsel(t, frptofchar(t, p1), p1, t->p1, 0);
	}

    Return:
	t->p0 = p0;
	t->p1 = p1;
}


/*
 * Release the button in less than DELAY ms and it's considered a null selection
 * if the mouse hardly moved, regardless of whether it crossed a char boundary.
 */
enum {
	DELAY = 2,
	MINMOVE = 4,
};

uint
xselect(Frame *f, Mousectl *mc, Image *col, uint *p1p)	/* when called, button is down */
{
	uint p0, p1, q, tmp;
	ulong msec;
	Point mp, pt0, pt1, qt;
	int reg, b;

	mp = mc->xy;
	b = mc->buttons;
	msec = mc->msec;

	/* remove tick */
	if(f->p0 == f->p1)
		frtick(f, frptofchar(f, f->p0), 0);
	p0 = p1 = frcharofpt(f, mp);
	pt0 = frptofchar(f, p0);
	pt1 = frptofchar(f, p1);
	reg = 0;
	frtick(f, pt0, 1);
	do{
		q = frcharofpt(f, mc->xy);
		if(p1 != q){
			if(p0 == p1)
				frtick(f, pt0, 0);
			if(reg != region(q, p0)){	/* crossed starting point; reset */
				if(reg > 0)
					selrestore(f, pt0, p0, p1);
				else if(reg < 0)
					selrestore(f, pt1, p1, p0);
				p1 = p0;
				pt1 = pt0;
				reg = region(q, p0);
				if(reg == 0)
					frdrawsel0(f, pt0, p0, p1, col, display->white);
			}
			qt = frptofchar(f, q);
			if(reg > 0){
				if(q > p1)
					frdrawsel0(f, pt1, p1, q, col, display->white);

				else if(q < p1)
					selrestore(f, qt, q, p1);
			}else if(reg < 0){
				if(q > p1)
					selrestore(f, pt1, p1, q);
				else
					frdrawsel0(f, qt, q, p1, col, display->white);
			}
			p1 = q;
			pt1 = qt;
		}
		if(p0 == p1)
			frtick(f, pt0, 1);
		flushimage(f->display, 1);
		readmouse(mc);
	}while(mc->buttons == b);
	if(mc->msec-msec < DELAY && p0!=p1
	&& abs(mp.x-mc->xy.x)<MINMOVE
	&& abs(mp.y-mc->xy.y)<MINMOVE) {
		if(reg > 0)
			selrestore(f, pt0, p0, p1);
		else if(reg < 0)
			selrestore(f, pt1, p1, p0);
		p1 = p0;
	}
	if(p1 < p0){
		tmp = p0;
		p0 = p1;
		p1 = tmp;
	}
	pt0 = frptofchar(f, p0);
	if(p0 == p1)
		frtick(f, pt0, 0);
	selrestore(f, pt0, p0, p1);
	/* restore tick */
	if(f->p0 == f->p1)
		frtick(f, frptofchar(f, f->p0), 1);
	flushimage(f->display, 1);
	*p1p = p1;
	return p0;
}

int
textselect23(Text *t, uint *q0, uint *q1, Image *high, int mask)
{
	uint p0, p1;
	int buts;

	p0 = xselect(t, mousectl, high, &p1);
	buts = mousectl->buttons;
	if((buts & mask) == 0){
		*q0 = p0+t->org;
		*q1 = p1+t->org;
	}

	while(mousectl->buttons)
		readmouse(mousectl);
	return buts;
}

int
textselect2(Text *t, uint *q0, uint *q1, Text **tp)
{
	int buts;

	*tp = nil;
	buts = textselect23(t, q0, q1, but2col, 4);
	if(buts & 4)
		return 0;
	if(buts & 1){	/* pick up argument */
		*tp = argtext;
		return 1;
	}
	return 1;
}

int
textselect3(Text *t, uint *q0, uint *q1)
{
	int h;

	h = (textselect23(t, q0, q1, but3col, 1|2) == 0);
	return h;
}

static Rune left1[] =  { L'{', L'[', L'(', L'<', L'«', 0 };
static Rune right1[] = { L'}', L']', L')', L'>', L'»', 0 };
static Rune left2[] =  { L'\n', 0 };
static Rune left3[] =  { L'\'', L'"', L'`', 0 };

static
Rune *left[] = {
	left1,
	left2,
	left3,
	nil
};
static
Rune *right[] = {
	right1,
	left2,
	left3,
	nil
};

void
textdoubleclick(Text *t, uint *q0, uint *q1)
{
	int c, i;
	Rune *r, *l, *p;
	uint q;

	if(t->rs.nr == 0)
		return;

	for(i=0; left[i]!=nil; i++){
		q = *q0;
		l = left[i];
		r = right[i];
		/* try matching character to left, looking right */
		if(q == 0)
			c = '\n';
		else
			c = t->rs.r[q-1];
		p = runestrchr(l, c);
		if(p != nil){
			if(textclickmatch(t, c, t->rs.r[p-l], 1, &q))
				*q1 = q-(c!='\n');
			return;
		}
		/* try matching character to right, looking left */
		if(q == t->rs.nr)
			c = '\n';
		else
			c = t->rs.r[q];
		p = runestrchr(r, c);
		if(p != nil){
			if(textclickmatch(t, c, l[p-r], -1, &q)){
				*q1 = *q0+(*q0<t->rs.nr && c=='\n');
				*q0 = q;
				if(c!='\n' || q!=0 || t->rs.r[0]=='\n')
					(*q0)++;
			}
			return;
		}
	}
	/* try filling out word to right */
	while(*q1<t->rs.nr && isalnum(t->rs.r[*q1]))
		(*q1)++;
	/* try filling out word to left */
	while(*q0>0 && isalnum(t->rs.r[*q0-1]))
		(*q0)--;
}

int
textclickmatch(Text *t, int cl, int cr, int dir, uint *q)
{
	Rune c;
	int nest;

	nest = 1;
	for(;;){
		if(dir > 0){
			if(*q == t->rs.nr)
				break;
			c = t->rs.r[*q];
			(*q)++;
		}else{
			if(*q == 0)
				break;
			(*q)--;
			c = t->rs.r[*q];
		}
		if(c == cr){
			if(--nest==0)
				return 1;
		}else if(c == cl)
			nest++;
	}
	return cl=='\n' && nest==1;
}

uint
textbacknl(Text *t, uint p, uint n)
{
	int i, j;

	/* look for start of this line if n==0 */
	if(n==0 && p>0 && t->rs.r[p-1]!='\n')
		n = 1;
	i = n;
	while(i-->0 && p>0){
		--p;	/* it's at a newline now; back over it */
		if(p == 0)
			break;
		/* at 128 chars, call it a line anyway */
		for(j=128; --j>0 && p>0; p--)
			if(t->rs.r[p-1]=='\n')
				break;
	}
	return p;
}

void
textsetorigin(Text *t, uint org, int exact)
{
	int i, a, fixup;
	Rune *r;
	uint n;

	if(org>0 && !exact){
		/* org is an estimate of the char posn; find a newline */
		/* don't try harder than 256 chars */
		for(i=0; i<256 && org<t->rs.nr; i++){
			if(t->rs.r[org] == '\n'){
				org++;
				break;
			}
			org++;
		}
	}
	a = org-t->org;
	fixup = 0;
	if(a>=0 && a<t->nchars){
		frdelete(t, 0, a);
		fixup = 1;	/* frdelete can leave end of last line in wrong selection mode; it doesn't know what follows */
	}else if(a<0 && -a<t->nchars){
		n = t->org - org;
		r = runemalloc(n);
		runemove(r, t->rs.r+org, n);
		frinsert(t, r, r+n, 0);
		free(r);
	}else
		frdelete(t, 0, t->nchars);
	t->org = org;
	textfill(t);
	textscrdraw(t);
	textsetselect(t, t->q0, t->q1);
	if(fixup && t->p1 > t->p0)
		frdrawsel(t, frptofchar(t, t->p1-1), t->p1-1, t->p1, 1);
}

void
textset(Text *t, Rune*r, int n)
{
	textdelete(t, 0, t->rs.nr);
	textinsert(t, 0, r, n);
	textsetselect(t, t->q0, t->q1);
}

void
textmouse(Text *t, Point xy, int but)
{
	Text *argt;
	uint q0, q1;

	if(ptinrect(xy, t->scrollr)){
		if(t->what == Columntag)
			rowdragcol(&row, t->col, but);
		else if(t->what == Tag)
			coldragwin(t->col, t->w, but);
		else if(t->what == Textarea)
			textscroll(t, but);
		if(t->col)
			activecol = t->col;
		return;
	}
	if(but == 1){
		selpage = nil;
		textselect(t);
		argtext = t;
		seltext = t;
	}else if(but == 2){
		if(textselect2(t, &q0, &q1, &argt))
			execute(t, q0, q1, argt);
	}else if(but == 3){
		if(textselect3(t, &q0, &q1))
			look3(t, q0, q1);
	}
}
