#include <u.h>
#include <libc.h>
#include <libg.h>
#include <frame.h>
#include <layer.h>
#include <auth.h>
#include <fcall.h>
#include "dat.h"
#include "fns.h"

#define	HIWAT	30000
#define	LOWAT	20000
#define	WATBUF	5000
#define	ZIP	(-100000)
#define	CLICKTIME	500	/* milliseconds for double click */

Window	window[NSLOT];
Window	*snarf = &window[SNARF];
long		clicktime;

int	slot;

void	termctl(void);

int
newterm(Rectangle r, int dospawn, int pid)
{
	int i;
	Window *w;
	char buf[128];

	/* slot 0 is snarf buffer only */
	if(snarf->text.s == 0)
		textinsert(snarf, &snarf->text, (Rune*)0, 0, 0, 0);

	for(i=1; window[i].busy; i++)
		if(i == NSLOT-1)
			return -1;
	w = &window[i];
	memset(w, 0, sizeof(Window));
	w->l = lalloc(&cover, r);
	if(w->l == 0)
		return -1;
	w->busy = 1;
	w->slot = i;
	w->l->user = w->slot;
	termrects(w, r);
	textinsert(w, &w->text, (Rune*)0, 0, 0, 0);
	frinit(&w->f, w->r, font, w->l);
	if(dospawn){
		pid = spawn(w->slot);
		if(pid < 0){
			lfree(w->l);
			w->slot = 0;
			w->busy = 0;
			return -1;
		}
		sprint(w->label, "%d /bin/rc", pid);
	}
	sprint(buf, "/proc/%d/notepg", pid);
	w->pgrpfd = open(buf, OWRITE|OCEXEC);	/* if error, we'll notice later */
	w->p = newproc(termctl, w);
	w->rfid = -1;
	w->mfid = -1;
	w->scroll = defscroll;
	w->reshape.min.x = ZIP;
	sprint(w->wqid, "%d", ++wqid);
	if(dospawn)
		current(w);
	else
		termborder(w);
	scrdraw(w);
	bflush();
	return w->slot;
}

void
termrects(Window *w, Rectangle r)
{
	w->clipr = inset(r, 4);
	w->l->clipr = w->clipr;
	w->r = inset(r, 5);
	w->scrollr = w->r;
	w->scrollr.max.x = w->scrollr.min.x+SCROLLWID;
	w->r.min.x = w->scrollr.max.x+SCROLLGAP;
	w->lastbar = Rect(0, 0, 0, 0);
}

void
termborder(Window *w)
{
	Rectangle c;

	if(w->l == 0)	/* deleted window */
		return;
	c = w->l->clipr;
	w->l->clipr = w->l->r;
	if(w == input){
		border(w->l, w->l->r, 4, 0xF);
		if(w->hold)
			border(w->l, inset(w->l->r, 2), 1, 0x0);
	}else{
		border(w->l, w->l->r, 4, 0x0);
		border(w->l, w->l->r, 1, 0xF);
	}
	w->l->clipr = c;
}

Window*
termwhich(Point pt)
{
	Layer *l;

	l = cover.front;
	for(;;){
		if(l == 0)
			return 0;
		if(ptinrect(pt, l->r))
			break;
		l = l->next;
	}
	return &window[l->user];
}

Window*
termtop(Point pt)
{
	Window *w;

	w = termwhich(pt);
	if(w && w!=input)
		ltofront(w->l);
	return w;
}

long
backnl(Window *w, long p, long n)
{
	int i, j;

	i = n;
	while(i-->0 && p>0){
		--p;	/* it's at a newline now; back over it */
		if(p == 0)
			break;
		/* at 128 chars, call it a line anyway */
		for(j=128; --j>0 && --p>0;)
			if(w->text.s[p]=='\n'){
				p++;
				break;
			}
	}
	return p;
}

void
setorigin(Window *w, long n)
{
	long a;

	a = n-w->org;
	if(a>=0 && a<w->f.nchars)
		frdelete(&w->f, 0, a);
	else if(a<0 && -a<w->f.nchars)
		frinsert(&w->f, &w->text.s[n], &w->text.s[w->org], 0);
	else
		frdelete(&w->f, 0, w->f.nchars);
	w->org = n;
	termfill(w);
	scrdraw(w);
}

void
scrorigin(Window *w, int but, long n)
{
	int i;

	switch(but){
	case 1:
		/* n is the number of lines of text to back up */
		n = backnl(w, w->org, n);
		break;
	case 2:
		/* n is an estimate of the char posn; find a newline */
		if(n == 0)
			break;
		/* don't try harder than 256 chars */
		for(i=0; i<256 && n<w->text.n; i++, n++)
			if(w->text.s[n] == '\n'){
				n++;
				break;
			}
		break;
	case 3:
		/* n is already the right spot */
		break;
	}
	/* n is now the correct origin to move to */
	setorigin(w, n);
}

void
termclear(Window *w)
{
	if(w->deleted)
		return;
	if(w == input)
		current(0);
	if(w->pgrpfd >= 0){
		close(w->pgrpfd);
		w->pgrpfd = -1;
	}
	frclear(&w->f);
	if(w->l){
		lfree(w->l);
		w->l = 0;
	}
	free(w->text.s);
	free(w->rawbuf.s);
	w->text.s = 0;	/* force early crash if we reuse */
	w->rawbuf.s = 0;
	w->deleted = 1;
}

void
termdelete(Window *w)
{
	char **ptr;

	/* Delete from hidden list */
	for(ptr = menu3str; *ptr; ptr++)
		if(*ptr == w->label) {
			for(; *ptr; ptr++)
				*ptr = ptr[1];
			break;
		}

	if(w == input)
		current(0);
	termclear(w);
	if(w->p)
		w->p->dead = 1;
	w->busy = 0;
}

void
termwrite(Window *w, int tag, int fid, char *buf, int cnt)
{
	w->wtag = tag;
	w->wbuf = buf;
	w->wfid = fid;
	w->wcnt = cnt;
	w->woff = 0;
	w->wbuf[cnt] = 0;	/* there's room */
	if(w==&window[SNARF])
		return;
	run(w->p);
}

void
termread(Window *w, int tag, int fid, int cnt)
{
	w->rtag = tag;
	w->rcnt = cnt;
	w->rfid = fid;
	run(w->p);
}

void
termmouseread(Window *w, int tag, int fid, int block)
{
	w->mtag = tag;
	w->MFID = fid;	/*BUG*/
	w->mfid = fid;
	if(!block)
		w->mousechanged = 1;
	run(w->p);
}

void
textinsert(Window *w, Text *t, Rune *buf, int n, ulong posn, int trunc)
{
	long a;

	if(t->n+n >= t->nalloc){	/* need room for null */
		a = t->n+n + WATBUF;
		t->s = erealloc(t->s, (a+1)*sizeof(Rune));	/* room for null */
		t->nalloc = a;
	}
	if(trunc && t->n+n > HIWAT){
		a = t->n+n - LOWAT;
		if(a > w->org)
			a = w->org;
		if(a > posn)
			a = posn;
		if(a > 0){
			memmove(t->s, t->s+a, (t->n+1-a)*sizeof(Rune));
			t->n -= a;
			w->org -= a;
			posn -= a;
			w->q0 = max(0, w->q0-a);
			w->q1 = max(0, w->q1-a);
			w->qh = max(0, w->qh-a);
		}
	}
	memmove(t->s+posn+n, t->s+posn, (t->n-posn)*sizeof(Rune));
	memmove(t->s+posn, buf, n*sizeof(Rune));
	t->n += n;
	t->s[t->n] = 0;
}

void
textdelete(Text *t, long n0, long n1)
{
	if(t->s == 0)
		return;
	memmove(t->s+n0, t->s+n1, (t->n-n1)*sizeof(Rune));
	t->n -= n1 - n0;
	t->s[t->n] = 0;
}

void
termhighlight(Window *w, long q0, long q1)
{
	if(q0<0 || q1<0 || q0>w->text.n || q1>w->text.n)
		berror("termhighlight");
	if(w->org+w->f.p0!=q0 || w->org+w->f.p1!=q1){
		frselectp(&w->f, F&~D);
		w->f.p0 = max(0, q0-w->org);
		w->f.p1 = max(0, q1-w->org);
		frselectp(&w->f, F&~D);
	}
	w->q0 = q0;
	w->q1 = q1;
}

void
termselect(Window *w, Mouse *m)
{
	long cc;
	long q0, q1, selectq;
	int b, x, y;

	b = m->buttons;
	cc = m->msec;
	/* button1 is down */
	q0 = w->q0;
	q1 = w->q1;
	selectq = w->org+frcharofpt(&w->f, m->xy);
	if(cc-clicktime < 500)
	if(q0==q1 && selectq==q0){
		doubleclick(w, &q0, &q1);
		w->q0 = q0;
		w->q1 = q1;
		termhighlight(w, w->q0, w->q1);
		bflush();
		x = m->xy.x;
		y = m->xy.y;
		/* stay here until something interesting happens */
		do
			frgetmouse();
		while(m->buttons==b && abs(m->xy.x-x)<3 && abs(m->xy.y-y)<3);
		m->xy.x = x;	/* in case we're calling frselect */
		m->xy.y = y;
		q0 = w->q0;	/* may have changed */
		q1 = w->q1;
		selectq = q0;
	}
	if(m->buttons == b){
		frselect(&w->f, m);
		/* horrible botch: while asleep, may have lost selection altogether */
		if(selectq > w->text.n)
			selectq = w->org + w->f.p0;
		if(selectq < w->org)
			q0 = selectq;
		else
			q0 = w->org + w->f.p0;
		if(selectq > w->org+w->f.nchars)
			q1 = selectq;
		else
			q1 = w->org+w->f.p1;
	}
	if(q0 == q1){
		if(q0==w->q0 && m->msec-clicktime<500){
			doubleclick(w, &q0, &q1);
			clicktime = 0;
		}else
			clicktime = m->msec;
	}else
		clicktime = 0;
	w->q0 = q0;
	w->q1 = q1;
	termhighlight(w, w->q0, w->q1);
	bflush();
	while(m->buttons){
		m->msec = 0;
		b = m->buttons;
		if(b & 6){
			if(b & 2)
				termcut(w, 1);
			else
				termpaste(w);
		}
		while(m->buttons == b)
			frgetmouse();
		clicktime = 0;
	}
}

void
termfill(Window *w)
{
	long p;

	if(w->text.s == 0)
		return;
	p = w->org+w->f.nchars;
	if(p < w->text.n)
		frinsert(&w->f, w->text.s+p, w->text.s+w->text.n, w->f.nchars);
	termhighlight(w, w->q0, w->q1);
	termshow(w, w->text.n, 0, 0);
	run(w->p);		/* in case any i/o is completable */
}

void
termcut(Window *w, int save)
{
	long q0, q1;

	q0 = w->q0;
	q1 = w->q1;
	if(q1 > q0){
		if(save)
			termsnarf(w);
		textdelete(&w->text, q0, q1);
		frdelete(&w->f, max(0, q0-w->org), max(0, q1-w->org));
		if(w->qh >= q1)
			w->qh -= q1-q0;
		else if(w->qh > q0)
			w->qh = q0;
		if(w->org >= q1)
			w->org -= q1-q0;
		else if(w->org > q0)
			w->org = q0;
		w->q1 = w->q0;
		termfill(w);
	}else
		w->f.modified = 1;	/* caller may change w->q0, w->q1 */
}

void
termsnarf(Window *w)
{
	long q0, q1;

	q0 = w->q0;
	q1 = w->q1;
	if(q0 == q1)
		return;
	textdelete(&snarf->text, 0, snarf->text.n);
	textinsert(snarf, &snarf->text, w->text.s+q0, q1-q0, 0, 0);
}

void
termpaste(Window *w)
{
	termcut(w, 0);
	textinsert(w, &w->text, snarf->text.s, snarf->text.n, w->q0, 0);
	if(w->q0 >= w->org)
		frinsert(&w->f, snarf->text.s, snarf->text.s+snarf->text.n,
			w->q0-w->org);
	if(w->qh > w->q0)
		w->qh += snarf->text.n;
	if(w->org > w->q0)
		w->org += snarf->text.n;
	w->q1 = w->q0+snarf->text.n;
	termfill(w);
}

void
termsend(Window *w)
{
	if(w->q1 > w->q0)
		termsnarf(w);
	if(snarf->text.n > 0){
		w->send = 1;
		run(w->p);
	}
}

void
termjump(Window *w, int nl)
{
	long n;

	n = w->org+frcharofpt(&w->f, Pt(w->f.r.min.x, w->f.r.min.y+nl*w->f.font->height));
	setorigin(w, n);
}

void
termunblock(Window *w)
{
	IOQ *wq;

	while(wq = w->wq){	/* assign = */
		sendwrite(0, wq->tag, wq->fid, wq->cnt);
		w->wq = wq->next;
		free(wq);
	}
}

void
termshow(Window *w, ulong n, int scroll, int unblock)
{
	long p;

	if(scroll && (n<w->org || w->org+w->f.nchars<n)){
		p = backnl(w, n, max(0, w->f.maxlines-3));
		/* don't scroll back if we're just gonna go forward again */
		if(!(p<w->org && n>=w->org))
			setorigin(w, p);
		while(w->org+w->f.nchars < n)
			termjump(w, 3);
	}
	if(w->org+w->f.nchars==w->text.n
	|| (unblock && w->org<=n && n<=w->org+w->f.nchars))
		termunblock(w);
}

void
termscroll(Window *w)
{
	if(w->scroll ^= 1)
		termshow(w, w->text.n, 1, 0);
}

int
termreshape(Window *w, Rectangle r, int dofree, int redraw)
{
	Layer *l;
	int move;

	l = lalloc(&cover, r);
	if(l == 0)
		return 0;
	move = 0;
	if(Dx(r)==Dx(w->l->r) && Dy(r)==Dy(w->l->r)){
		move = 1;
		bitblt(l, l->r.min, w->l, w->l->r, S);
	}
	if(dofree)
		lfree(w->l);
	w->l = l;
	w->l->user = w->slot;
	termrects(w, r);
	if(move && !redraw)
		frsetrects(&w->f, w->r, w->l);
	else{
		if(dofree)
			frclear(&w->f);
		frinit(&w->f, w->r, font, w->l);
		frinsert(&w->f, w->text.s+w->org, w->text.s+w->text.n, 0);
		termhighlight(w, w->q0, w->q1);
		termshow(w, 0, 0, 0);
		scrdraw(w);
	}
	current(w);
	w->reshaped = 1;
	return 1;
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
termreshapeall(void)
{
	Rectangle osr, r;
	Layer *l;
	int i, n;
	static Window *wa[NSLOT];
	Window *w;

	input = 0;
	n = 0;
	for(l=cover.front; l; l=l->next){
		w = &window[l->user];
		frclear(&w->f);
		w->r = w->l->r;	/* remember for later */
		wa[n++] = w;
	}
	while(cover.front)
		lfree(cover.front);

	osr = screen.r;
	screen.r = bscreenrect(&screen.clipr);
	texture(&screen, screen.clipr, lightgrey, S);

	for(i=n-1; i>=0; --i){
		w = wa[i];
		r = raddp(rscale(rsubp(w->r, osr.min),
			sub(osr.max, osr.min),
			sub(screen.r.max, screen.r.min)), screen.r.min);
		if(Dx(r) < 100)
			r.max.x = r.min.x+100;
		if(Dy(r) < 50)
			r.max.y = r.min.y+50;
		if(!termreshape(w, r, 0, 1))
			w->dodelete = 1;		/* sorry.... */
		run(w->p);
	}
}

Rune*
strrune(Rune *s, Rune c)
{
	Rune c1;

	if(c == 0) {
		while(*s++)
			;
		return s-1;
	}

	while(c1 = *s++)
		if(c1 == c)
			return s-1;
	return 0;
}

Rune*
termbrkc(Window *w)
{
	Rune *pnl, *peot;

	pnl = strrune(w->text.s+w->qh, '\n');
	peot = strrune(w->text.s+w->qh, 0x04);
	if(pnl==0 || peot==0){
		if(pnl==0)
			return peot;
		return pnl;
	}
	if(pnl > peot)
		return peot;
	return pnl;
}

void
termhold(Window *w, int h)
{
	w->hold = h;
	w->cursorp = h? &whitearrow : 0;
	checkcursor(0);
	termborder(w);
}

void
termnote(Window *w, char *s)
{
	int n;

	n = strlen(s);
	if(w->pgrpfd>=0 && write(w->pgrpfd, s, n)!=n){
		close(w->pgrpfd);
		w->pgrpfd = -1;
	}
}

void
termkbd(Window *w)
{
	int c;
	Rune ich[2];
	long q0;

	while(w->rawbuf.n>0){
		ich[1] = 0;
		ich[0] = w->rawbuf.s[0];
		if(!w->mouseopen && ich[0]==0x80){	/* VIEW */
			termjump(w, w->f.maxlines/2);
			textdelete(&w->rawbuf, 0, 1);
			continue;
		}
		if(w->raw && (w->q0>=w->qh || w->mouseopen))
			break;
		textdelete(&w->rawbuf, 0, 1);
		if(ich[0] == 0x1b){	/* ESC */
			termhold(w, w->hold ^ 1);
			continue;
		}
		if(ich[0] == 0x7F){
			termnote(w, "interrupt");
			w->qh = w->text.n;
			continue;
		}
		termcut(w, 1);
		q0 = w->q0;
		if(ich[0] == '\b'){
			if(q0>w->org && q0!=w->qh){
				w->q0--;
				termcut(w, 0);
				q0--;
			}
		}else if(ich[0] == 0x15){		/* ^U */
			if(q0>w->org && q0!=w->qh){
				while(q0>0 && q0!=w->qh){
					if(w->text.s[q0-1]==0x04 || w->text.s[q0-1]=='\n')
						break;
					q0--;
				}
				w->q0 = q0;
				termcut(w, 0);
			}
		}else if(ich[0] == 0x17){		/* ^W */
			if(q0>w->org && q0!=w->qh){
				c = w->text.s[--q0];
				if(c=='\n')
					goto wcut;
				while(q0>0 && q0!=w->qh){
					if(alnum(c))
						break;
					if(c=='\n'){
						q0++;
						goto wcut;
					}
					c = w->text.s[--q0];
				}
				while(q0>0 && q0!=w->qh){
					if(!alnum(w->text.s[q0-1]))
						break;
					q0--;
				}
		wcut:
				w->q0 = q0;
				termcut(w, 0);
			}
		}else{		/* BUG: old also checked kbdc<font->n */
			textinsert(w, &w->text, ich, 1, q0, 0);
			q0 = w->q0;	/* may have changed */
			if(q0 >= w->org)
				frinsert(&w->f, ich, ich+1, q0-w->org);
			else
				w->org++;
			if(w->qh > q0)
				w->qh++;
			q0++;
		}
		termhighlight(w, q0, q0);
		termshow(w, q0, 1, 0);
	}
	if(kbdcnt)
		run(pkbd);
}

void
termraw(Window *w)
{
	w->raw = 1;
	termhold(w, 0);		/* can't unhold if raw */
	run(w->p);		/* typed characters may now be readable */
}

void
termunraw(Window *w)
{
	w->raw = 0;
}

void
termflush(int tag)
{
	IOQ *oq, *q;
	Window *w;
	int i;

	/*
	 * Could have a by-tag index but it seems hardly worth it
	 */
	w = &window[1];	/* skip slot 0 == slaves */
	for(i=1; i<NSLOT; i++,w++){
		for(oq=0,q=w->rq; q; oq=q,q=q->next){
			if(q->tag == tag){
				if(oq)
					oq->next = q->next;
				else
					w->rq = q->next;
				free(q);
				return;
			}
		}
		for(oq=0,q=w->wq; q; oq=q,q=q->next){
			if(q->tag == tag){
				if(oq)
					oq->next = q->next;
				else
					w->wq = q->next;
				free(q);
				return;
			}
		}
		if(w->mfid>=0 && w->mtag==tag){
			w->mfid = -1;
			w->mousechanged = 0;
			return;
		}
	}
}

int
termwrune(Window *w)
{
	Rune r;
	int wid;

	if(w->nwpart){
		/* known to be first part of message; woff is zero */
		memmove(w->wbuf+w->nwpart, w->wbuf, w->wcnt);
		memmove(w->wbuf, w->wpart, w->nwpart);
		w->wcnt += w->nwpart;
		w->nwpart = 0;
	}
    loop:
	if(w->wcnt == 0)
		return -1;
	r = ((uchar*)w->wbuf)[w->woff];
	if(r < Runeself){	/* easy fast case */
		w->woff++;
		w->wcnt--;
		if(r == 0)		/* BUG: old also checked r>=font->n */
			goto loop;
		return r;
	}
	if(fullrune(w->wbuf+w->woff, w->wcnt)){
		wid = chartorune(&r, w->wbuf+w->woff);
		w->woff += wid;
		w->wcnt -= wid;
		if(r == 0)		/* BUG: old also checked r>=font->n */
			goto loop;
		return r;
	}
	/* incomplete rune */
	memmove(w->wpart, w->wbuf+w->woff, w->wcnt);
	w->nwpart = w->wcnt;
	w->wcnt = 0;
	return -1;
}

int
termrrune(Window *w, int tag, int fid, Rune *buf, int rcnt, int maxcnt)
{
	int n, nr, cnt, wid;
	char *tmp;

	tmp = emalloc(maxcnt+UTFmax);
	n = 0;
	nr = 0;
	if(w->nrpart){
		n = w->nrpart;
		if(n > maxcnt)
			n = maxcnt;
		memmove(tmp, w->rpart, n);
		memmove(w->rpart, w->rpart+n, w->nrpart-n);
		w->nrpart -= n;
		if(n == maxcnt)
			goto send;
	}
	for(cnt=0; cnt<rcnt; cnt++){
		wid = runetochar(tmp+n, buf+cnt);
		nr++;
		n += wid;
		if(n < maxcnt)
			continue;
		n -= maxcnt;
		memmove(w->rpart, tmp+maxcnt, n);
		w->nrpart = n;
		n = maxcnt;
		break;
	}
    send:
	sendread(0, tag, fid, tmp, n);
	free(tmp);
	return nr;
}

void
termctl(void)
{
	Window *w;
	uchar k[14], *up;
	Rune *p, *p1, *p2, *pe;
	long q0, q1;
	ulong n, nr, eot, nmax;
	int c, doscr, cnt;
	IOQ *rq, *nrq, *wq;
	static Rune *nl = L"\n";
	Mouse *m;
	Rectangle clipr;

	m = &proc.p->mouse;
	doscr = 0;
	w = proc.p->arg;
	for(;;){
		if(w->reshape.min.x != ZIP){
			if(termreshape(w, w->reshape, 1, 0) && w->hidemenu){
				hidemenu(w, w->hidemenu);
				if(w->hidemenu > 0)
					current(0);
				w->hidemenu = 0;
			}
			w->reshape.min.x = ZIP;
		}
		if(w->send){
			c = snarf->text.s[snarf->text.n-1];
			if(w->raw){
				textinsert(w, &w->rawbuf, snarf->text.s, snarf->text.n, w->rawbuf.n, 0);
				if(c!='\n' && c!=0x04)
					textinsert(w, &w->rawbuf, nl, 1, w->rawbuf.n, 0);
			}else{
				frinsert(&w->f, snarf->text.s,
					snarf->text.s+snarf->text.n, w->text.n-w->org);
				textinsert(w, &w->text, snarf->text.s, snarf->text.n, w->text.n, 1);
				if(c!='\n' && c!=0x04){
					frinsert(&w->f, nl, nl+1, w->text.n-w->org);
					textinsert(w, &w->text, nl, 1, w->text.n, 1);
				}
			}
			termhighlight(w, w->text.n, w->text.n);
			termshow(w, w->q1, 1, 1);
			w->send = 0;
			doscr = 1;
		}
		if(w->wbuf){
			if(w->bitopen){
				clipr = w->l->clipr;
				w->l->clipr = w->l->r;
			}
			/* queue the write; order isn't important */
			wq = emalloc(sizeof(IOQ));
			wq->next = w->wq;
			wq->tag = w->wtag;
			wq->fid = w->wfid;
			wq->cnt = w->wcnt;
			w->wq = wq;

			/*
			 * Convert to runes and clean the buffer of dreck.
			 */
			/* nwpart+ for pending partial rune(s), +1 for nul */
			p = emalloc((w->nwpart+w->wcnt+1)*sizeof(Rune));
			pe = p;
			for(;;){
				c = termwrune(w);
				if(c == -1)
					break;
				*pe++ = c;
			}
			*pe = 0;
			cnt = pe-p;
			p1 = p;
			while(p1 < pe){
				c = *p1++;
				if(c == '\b'){
					--p1;
					break;
				}
			}
			if(p1 < pe){
				n = 0;
				p2 = p1;
				while(p1 < pe){
					c = *p1++;
					if(c == '\b'){
						if(p2 > p)
							--p2;
						else
							n++;
					}else
						*p2++ = c;
				}
				cnt = p2-p;
				*p2 = 0;
				if(n > 0){	/* backspace n chars before qh */
					if(n > w->qh)
						n = w->qh;
					q1 = w->qh;
					q0 = q1-n;
					textdelete(&w->text, q0, q1);
					frdelete(&w->f, max(0, q0-w->org), max(0, q1-w->org));
					w->qh -= n;
					if(w->org >= q1)
						w->org -= n;
					else if(w->org > q0)
						w->org = q0;
					if(w->q0 >= q1)
						w->q0 -= n;
					else if(w->q0 > q0)
						w->q0 = q0;
					if(w->q1 >= q1)
						w->q1 -= n;
					else if(w->q1 > q0)
						w->q1 = q0;
					termfill(w);
				}
			}

			textinsert(w, &w->text, p, cnt, w->qh, 1);
			if(w->qh >= w->org)
				frinsert(&w->f, p, p+cnt, w->qh-w->org);
			else
				w->org += cnt;
			free(p);
			q0 = w->q0;
			q1 = w->q1;
			if(w->qh <= q0){
				q0 += cnt;
				q1 += cnt;
			}else if(w->qh < q1)
				q1 += cnt;
			termhighlight(w, q0, q1);
			w->qh += cnt;
			termshow(w, w->qh, w->scroll||w->mouseopen, 1);
			w->wbuf = 0;
			doscr = 1;
			if(w->bitopen)
				w->l->clipr = clipr;
		}
		if(w->rawbuf.n >= 0){
			if(w->bitopen){
				clipr = w->l->clipr;
				w->l->clipr = w->l->r;
			}
			termkbd(w);
			if(!w->raw)
				doscr = 1;
			if(w->bitopen)
				w->l->clipr = clipr;
		}
		rq = w->rq;
		if(w->rfid >= 0){
			nrq = emalloc(sizeof(IOQ));
			nrq->next = 0;
			nrq->tag = w->rtag;
			nrq->fid = w->rfid;
			nrq->cnt = w->rcnt;
			if(rq == 0)
				rq = w->rq = nrq;
			else{
				while(rq->next)
					rq = rq->next;
				rq->next = nrq;
			}
			w->rfid = -1;
		}
		if(w->mfid>0 && (w->mousechanged || w->reshaped)){
			up = k;
			up[0] = 'm';
			up[1] = m->buttons;
			if(w->reshaped){
				up[1] |= 0x80;
				w->reshaped = 0;
			}
			BPLONG(up+2, mouse.xy.x);
			BPLONG(up+6, mouse.xy.y);
			BPLONG(up+10, mouse.msec);
			sendread(0, w->mtag, w->mfid, (char*)k, 14);
			w->mfid = -1;
			w->mousechanged = 0;
		}
		if(rq && !w->hold){
			nmax = rq->cnt;
			eot = 0;
			if(!w->raw){
				p = termbrkc(w);
				n = rq->cnt;
				if(n == 0)
					goto Sendread;
				if(p){
					p++;
					if(rq->cnt > p-(w->text.s+w->qh))
						rq->cnt = p-(w->text.s+w->qh);
					else
						p = w->text.s+w->qh+rq->cnt;
					n = rq->cnt;
					if(n>0 && p[-1]==0x04){
						eot = 1;
						--n;
					}
			Sendread:
					nr = termrrune(w, rq->tag, rq->fid, w->text.s+w->qh, n, nmax);
					w->qh += nr;
					if(n == nr)
						w->qh += eot;
					w->rq = rq->next;
					free(rq);
				}
			}else if(w->qh < w->text.n){
				n = w->text.n - w->qh;
				if(rq->cnt > n)			
					rq->cnt = n;
				n = rq->cnt;
				goto Sendread;
			}else if(w->nrpart || w->rawbuf.n){
				n = w->rawbuf.n;
				if(n > rq->cnt)			
					n = rq->cnt;
				n = termrrune(w, rq->tag, rq->fid, w->rawbuf.s, n, nmax);
				textdelete(&w->rawbuf, 0, n);
				w->rq = rq->next;
				free(rq);
			}
		}
		while(w->kbdbuf.n && w->kq){		/* send stuff to kbd reader */
			int c=0, wid;
			up = (uchar *)emalloc(w->kq->cnt+UTFmax);
			wq = w->kq;
			while(c<wq->cnt && w->kbdbuf.n){
				wid = runetochar((char *)up+c, w->kbdbuf.s);
				c += wid;
				if (c<=w->kq->cnt)
					textdelete(&w->kbdbuf, 0, 1);
			}
			if(w->deleted)
				c = 0;
			sendread(0, w->kq->tag, w->kq->fid, (char *)up, c<wq->cnt? c:wq->cnt);
			w->kq = wq->next;
			free(wq);
			free(up);
		}
		if(doscr){
			scrdraw(w);
			doscr = 0;
		}
		/*
		 * do this last so everything's done that can be
		 */
		if(w->dodelete){
			termnote(w, "hangup");
			w->dodelete = 0;
			termclear(w);
			while(rq = w->rq){
				sendread(Eshutdown, rq->tag, rq->fid, 0, 0);
				w->rq = rq->next;
				free(rq);
			}
			while(wq = w->wq){
				sendwrite(Eshutdown, wq->tag, wq->fid, 0);
				w->wq = wq->next;
				free(wq);
			}
			if(w->mfid >= 0){
				sendread(Eshutdown, w->mtag, w->mfid, 0, 0);
				w->mfid = -1;
				w->mousechanged = 0;
			}
		}
		bflush();
		sched();
	}
}
