#include <u.h>
#include <libc.h>
#include <libg.h>
#include <frame.h>
#include "dat.h"
#include "fns.h"

#define	CLICKTIME	500	/* milliseconds for double click */

int	whichbut[] = { 0, 1, 2, 2, 3, 1, 2, 3 };

long		clicktime;
Text		snuf;
struct mouse	mouse;
Text		*curt;
Text		*prevt;

void
Strdelete(String *s, ulong n0, ulong n1)
{
	if(s->s == 0)
		return;
	memmove(s->s+n0, s->s+n1, (s->n-n1)*sizeof(Rune));
	s->n -= n1 - n0;
	s->s[s->n] = 0;
}

void
Strinsert(String *s, Rune *buf, int n, ulong posn)
{
	ulong a;

	if(s->n+n >= s->nmax){	/* need room for null */
		a = 3*(s->n+n)/2 + 1;
		s->s = erealloc(s->s, (a+1)*sizeof(Rune));	/* room for null */
		s->nmax = a;
	}
	memmove(s->s+posn+n, s->s+posn, (s->n-posn)*sizeof(Rune));
	memmove(s->s+posn, buf, n*sizeof(Rune));
	s->n += n;
	s->s[s->n] = 0;
}

void
mod(Text *t)
{
	Page *p;
	Rune *s;
	ulong q0;

	p = t->page;
	if(p->mod==0 && t==&p->body){
		t = &p->tag;
		s = t->s;
		if(s)
			s = rstrchr(s, '\t');
		if(s)
			q0 = s-t->s;
		else
			q0 = t->n;
		puttag(p, "Put!", q0);
		p->mod = 1;
	}
}

void
highlight(Text *t)
{
	if(t->q0<0 || t->q1<0 || t->q0>t->n || t->q1>t->n)
		error("highlight");
	if(t->org+t->p0!=t->q0 || t->org+t->p1!=t->q1){
		frselectp(t, F&~D);
		t->p0 = max(0, t->q0-t->org);
		t->p1 = max(0, t->q1-t->org);
		frselectp(t, F&~D);
	}
	scrdraw(t);
}

void
fill(Text *t)
{
	long q, n, otn;
	Rune *r;

	if(t->s == 0)
		error("fill");
	/* do a few hundred at a time to avoid painting huge invisible parts */
	otn = -1;
	while((q=t->org+t->nchars)<t->n && t->nchars!=otn){
		r = t->s+q;
		n = t->n - q;
		if(n > 300)
			n = 300;
		otn = t->nchars;
		frinsert(t, r, r+n, t->nchars);
	}
	highlight(t);
/*	show(t, t->q0);*/
/*	run(w->p);		/* in case any i/o is completable */
}

void
snarf(Text *t)
{
	if(t->q0 == t->q1)
		return;
	Strdelete(&snuf, 0, snuf.n);
	Strinsert(&snuf, t->s+t->q0, t->q1-t->q0, 0);
}

void
cut(Text *t, int save)
{
	if(t->q1 == t->q0)
		return;
	if(save)
		snarf(t);
	Strdelete(t, t->q0, t->q1);
	frdelete(t, max(0, t->q0-t->org), max(0, t->q1-t->org));
	if(t->org >= t->q1)
		t->org -= t->q1-t->q0;
	else if(t->org > t->q0)
		t->org = t->q0;
	t->q1 = t->q0;
	mod(t);
	fill(t);
}

void
paste(Text *t)
{
	if(snuf.n == 0)
		return;
	cut(t, 0);
	textinsert(t, snuf.s, t->q0, 1, 1);
	mod(t);
	fill(t);
}

long
backnl(Text *t, long p, long n)
{
	int i, j;

	i = n;
	while(i-->0 && p>0){
		--p;	/* it's at a newline now; back over it */
		/* at 128 chars, call it a line anyway */
		for(j=128; --j>0 && --p>0;)
			if(t->s[p]=='\n'){
				p++;
				break;
			}
	}
	return p;
}

void
setorigin(Text *t, long n)
{
	long a;

	a = n-t->org;
	if(a>=0 && a<t->nchars)
		frdelete(t, 0, a);
	else if(a<0 && -a<t->nchars)
		frinsert(t, t->s+n, t->s+t->org, 0);
	else
		frdelete(t, 0, t->nchars);
	t->org = n;
	fill(t);
}

void
scrorigin(Text *t, int but, long n)
{
	int i;

	switch(but){
	case 1:
		/* n is the number of lines of text to back up */
		n = backnl(t, t->org, n);
		break;
	case 2:
		/* n is an estimate of the char posn; find a newline */
		if(n == 0)
			break;
		/* don't try harder than 256 chars */
		for(i=0; i<256 && n<t->n; i++, n++)
			if(t->s[n] == '\n'){
				n++;
				break;
			}
		break;
	case 3:
		/* n is already the right spot */
		break;
	}
	/* n is now the correct origin to move to */
	setorigin(t, n);
}

void
jump(Text *t, int nl)
{
	ulong n;

	n = t->org+frcharofpt(t, Pt(t->r.min.x, t->r.min.y+nl*font->height));
	setorigin(t, n);
}


void
show(Text *t, ulong n)
{
	ulong p;

	if(n<t->org || t->org+t->nchars<n){
		p = backnl(t, n, min(3, t->maxlines));
		/* don't scroll back if we're just gonna go forward again */
		if(!(p<t->org && n>=t->org))
			setorigin(t, p);
		/* make sure we see at least the first char */
		if(n < t->n)
			n++;
		while(t->org+t->nchars < n)
			jump(t, 1);
	}
/*	if(w->org+w->f.nchars == w->text.n)
		termunblock(w);/**/
}


void
newsel(Text *t)
{

	if(curt == t)
		return;
	if(curt){
		prevt = curt;
		setoutline(curt, 1);
	}
	curt = t;
	setoutline(curt, 0);
}

void
type(int kbdc)
{
	Rune buf[2];
	Text *t;
	ulong q0;
	int c;

	clicktime = 0;
	if((page=curpage()) && (t=page->text)){	/* assign = */
		newsel(t);
		q0 = t->q0;
		if(kbdc == '\b'){
			if(t->q0 > 0)
				t->q0--;
			cut(t, 0);
		}else if(kbdc == 0x15){		/* ^U */
			if(q0 > t->org){
				while(q0>0 && q0>t->org){
					if(t->s[q0-1] == '\n')
						break;
					q0--;
				}
				goto Cut;
			}
		}else if(kbdc == 0x17){		/* ^W */
			if(q0 > t->org){
				c = t->s[--q0];
				if(c == '\n')
					goto Cut;
				while(q0>0 && q0>t->org){
					if(alnum(c))
						break;
					if(c == '\n'){
						q0++;
						goto Cut;
					}
					c = t->s[--q0];
				}
				while(q0>0 && q0>t->org){
					if(!alnum(t->s[q0-1]))
						break;
					q0--;
				}
		Cut:
				t->q0 = q0;
				cut(t, 0);
			}
		}else{
			cut(t, 1);
			buf[0] = kbdc;
			buf[1] = 0;
			textinsert(t, buf, t->q0, 0, 1);
			mod(t);
		}
		scrdraw(t);
	}
}

/*
 * Wait for all buttons up
 */
void
button0(void)
{
	while(mouse.buttons)
		frgetmouse();
}

void
control(void)
{
	int but, obut, n, dclick;
	ulong p0, p1, cc;
	Page *p, *op, *parent;
	Text *t;
	Point pt;

	Strinsert(&snuf, 0, 0, 0);

Button0:
	button0();
	while(mouse.buttons == 0)
		frgetmouse();

	page = curpage();
	if(page == 0){
		clicktime = 0;
		goto Button0;
	}

	if(page->down && ptinrect(mouse.xy, page->tabsr))
		goto Tabs;

	t = page->text;
	if(ptinrect(mouse.xy, t->scrollr)){
		if(t->n)
			goto Scrollbar;
		goto Button0;
	}

	if(mouse.buttons & 1)
		goto Button1;
	if(mouse.buttons & 2)
		goto Button2;
	else
		goto Button3;

Scrollbar:
	clicktime = 0;
	scroll(t, whichbut[mouse.buttons]);
	goto Button0;

Tabs:
	clicktime = 0;
	op = 0;
Tabs1:
	if(page->ver)
		n = (mouse.xy.y-page->tabsr.min.y)/TABSZ;
	else
		n = (mouse.xy.x-page->tabsr.min.x)/TABSZ;
	p = 0;
	if(ptinrect(mouse.xy, page->tabsr))
		p = whichpage(page, n);
	if(p!=op && (op = p))	/* assign = */
		pagetop(op, op->r.min, 0);
	frgetmouse();
	if(mouse.buttons&1)
		goto Tabs1;
	goto Button0;

Button1:
	dclick = curt==t && t->q0==t->q1;
	p0 = t->q0;
	newsel(t);
	obut = 1;
	pick(t, &mouse, 1);
	t->q0 = t->p0 + t->org;
	t->q1 = t->p1 + t->org;
	if(t->q0 == t->q1){
		cc = gettime();
		if(dclick && t->q0==p0 && cc-clicktime<CLICKTIME){
			doubleclick(t, t->q0);
			highlight(t);
			clicktime = 0;
		}else
			clicktime = cc;
	}else
		clicktime = 0;

Button1picked:
	but = mouse.buttons;
	if(but==7 || (but&1)==0)
		goto Button0;
	if(((but^obut)&2) && (but&2))
		cut(t, 1);
	if(((but^obut)&4) && (but&4))
		paste(t);
	obut = but;
	while(mouse.buttons == but)
		frgetmouse();
	goto Button1picked;

Button2:
	clicktime = 0;
	if(page->text == &page->tag)
		newsel(&page->body);
	else if(curt==0)
		if(prevt && prevt->q0!=prevt->q1)
			newsel(prevt);
	pick2(t, &mouse, 2, &p0, &p1);
	p0 += t->org;
	p1 += t->org;
	if(mouse.buttons == 0)
		execute(t, p0, p1);
	goto Button0;

Button3:
	clicktime = 0;
	if(page->text != &page->tag)
		goto Button0;
	cursorswitch(&box);
	while(mouse.buttons == 4)
		frgetmouse();
	parent = page->parent;
	pt = mouse.xy;
	if(parent && mouse.buttons==0){
		if(rectXrect(parent->r, Rect(pt.x-50, pt.y-50, pt.x+50, pt.y+50))){
			/* maybe outside, but not too far */
			/* leave in current parent */
			pagetop(page, pt, 0);
		}else{
			parent = curpage();
			if(parent)
			if(parent = parent->parent)	/* assign = */
			if(ptinrect(pt, inset(parent->r, 20))){
				/* well inside */
				p = pageadd(parent, 0);
				Strinsert(&p->body, page->body.s, page->body.n, 0);
				Strinsert(&p->tag, page->tag.s, page->tag.n, 0);
				p->tag.q0 = page->tag.q0;
				p->tag.q1 = page->tag.q1;
				p->body.q0 = page->body.q0;
				p->body.q1 = page->body.q1;
				p->body.org = page->body.org;
				p->id = page->id;
				p->mod = page->mod;
				highlight(&p->body);
				highlight(&p->tag);
				setoutline(&p->tag, page->tag.outline);
				setoutline(&p->body, page->body.outline);
				if(curt == &page->body)
					curt = &p->body;
				else if(curt == &page->tag)
					curt = &p->tag;
				/* delete old */
				pagetop(page, pt, 1);
				/* create new */
				pagetop(p, pt, 0);
				page = p;
			}
		}
	}
	cursorswitch(0);
	goto Button0;
}
