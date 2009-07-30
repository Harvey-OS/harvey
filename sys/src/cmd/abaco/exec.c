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

void	del(Text *, Text *, int, int, Rune *, int);
void	delcol(Text *, Text *, int, int, Rune *, int);
void	cut(Text *, Text *, int, int, Rune *, int);
void	exit(Text *, Text *, int, int, Rune *, int);
void	get(Text *, Text *, int, int, Rune *, int);
void	go(Text *,Text *,  int, int, Rune *, int);
void	google(Text *,Text *,  int, int, Rune *, int);
void	new(Text*, Text *, int, int, Rune *, int);
void	newcol(Text*, Text *, int, int, Rune *, int);
void	paste(Text *, Text *, int, int, Rune *, int);
void	sort(Text *, Text *, int, int, Rune *, int);
void	stop(Text *, Text *, int, int, Rune *, int);
void	debug(Text *, Text *, int, int, Rune *, int);

typedef struct Exectab Exectab;
struct Exectab
{
	Rune	*name;
	void	(*fn)(Text *, Text *, int, int, Rune *, int);
	int		flag1;
	int		flag2;
};

Exectab exectab[] = {
	{ L"Back",		go,		FALSE,	XXX		},
	{ L"Cut",		cut,		TRUE,	TRUE	},
	{ L"Debug",	debug,	XXX,		XXX		},
	{ L"Del",		del,		XXX,		XXX		},
	{ L"Delcol",	delcol,	FALSE,	TRUE	},
	{ L"Exit",		exit,		XXX,		XXX		},
	{ L"Get",		get,		XXX,		XXX		},
	{ L"Google",	google,	XXX,		XXX		},
	{ L"New",		new,		XXX,		XXX		},
	{ L"Newcol",	newcol,	XXX,		XXX		},
	{ L"Next",		go,		TRUE,	XXX		},
	{ L"Paste",		paste,	TRUE,	XXX		},
	{ L"Snarf",		cut,		TRUE,	FALSE	},
	{ L"Stop",		stop,		XXX,		XXX		},
	{ L"Sort",		sort,		XXX,		XXX		},
	{ nil, 			nil,		0,		0		},
};

static
Exectab*
lookup(Rune *r, int n)
{
	Exectab *e;
	int nr;

	r = skipbl(r, n, &n);
	if(n == 0)
		return nil;
	findbl(r, n, &nr);
	nr = n-nr;
	for(e=exectab; e->name; e++)
		if(runeeq(r, nr, e->name, runestrlen(e->name)) == TRUE)
			return e;
	return nil;
}

int
isexecc(int c)
{
	if(isalnum(c))
		return 1;
	return c=='<' || c=='|' || c=='>';
}

void
execute(Text *t, uint aq0, uint aq1, Text *)
{
	uint q0, q1;
	Rune *r, *s;
	Exectab *e;
	int c, n;

	q0 = aq0;
	q1 = aq1;
	if(q1 == q0){	/* expand to find word (actually file name) */
		/* if in selection, choose selection */
		if(t->q1>t->q0 && t->q0<=q0 && q0<=t->q1){
			q0 = t->q0;
			q1 = t->q1;
		}else{
			while(q1<t->rs.nr && isexecc(c=t->rs.r[q1]) && c!=':')
				q1++;
			while(q0>0 && isexecc(c=t->rs.r[q0-1]) && c!=':')
				q0--;
			if(q1 == q0)
				return;
		}
	}
	r = runemalloc(q1-q0);
	runemove(r, t->rs.r+q0, q1-q0);
	e = lookup(r, q1-q0);
	if(e){
		s = skipbl(r, q1-q0, &n);
		s = findbl(s, n, &n);
		s = skipbl(s, n, &n);
		(*e->fn)(t, seltext, e->flag1, e->flag2, s, n);
	}
	free(r);
}

void
newcol(Text *et, Text *, int, int, Rune *, int)
{
	Column *c;

	c = rowadd(et->row, nil, -1);
	if(c)
		winsettag(coladd(c, nil, nil, -1));
}

void
delcol(Text *t, Text *, int, int, Rune *, int)
{
	Column *c;

	c = t->col;
	if(c==nil || colclean(c)==0)
		return;

	rowclose(c->row, c, TRUE);
}

void
del(Text *et, Text *, int flag1, int, Rune *, int)
{
	if(et->w==nil)
		return;

	if(flag1 || winclean(et->w, FALSE))
		colclose(et->col, et->w, TRUE);
}

void
sort(Text *et, Text *, int, int, Rune *, int)
{
	if(et->col)
		colsort(et->col);
}

void
exit(Text *, Text *, int, int, Rune *, int)
{
	sendul(cexit, 0);
	threadexits(nil);
}

void
debug(Text *, Text *, int, int, Rune *, int)
{
	Column *c;
	int i, j;

	for(j=0; j<row.ncol; j++){
		c = row.col[j];
		for(i=0; i<c->nw; i++){
			fprint(2, "Col: %d; Win: %d\n", j, i);
			windebug(c->w[i]);
		}
	}
}

void
stop(Text *t, Text *, int, int, Rune *, int)
{
	if(t==nil || t->w==nil)
		return;

	pageabort(&t->w->page);
}

void
get(Text *t, Text *, int, int, Rune *, int)
{
	Window *w;
	int dohist;

	if(t==nil || t->w==nil)
		return;
	w = t->w;
	if(w->url.rs.nr == 0)
		return;

	dohist = FALSE;
	if(w->page.url==nil || runestreq(w->page.url->act, w->url.rs)==FALSE)
		dohist = TRUE;

	pageget(&w->page, &w->url.rs, nil, HGet, dohist);
}

void
go(Text *et, Text *t, int isnext, int, Rune *, int)
{
	if(et!=nil && et->w!=nil)
		t = et;
	if(t==nil || t->w==nil)
		return;

	wingohist(t->w, isnext);
}

void
cut(Text *, Text *t, int dosnarf, int docut, Rune *, int)
{
	Runestr rs;
	uint u;

	if(selpage){
		if(dosnarf && !docut && !eqpt(selpage->top, selpage->bot))
			pagesnarf(selpage);
		return;
	}
	if(t==nil){
		/* can only happen if seltext == nil */
		return;
	}
	if(t->q0 > t->q1){
		u = t->q0;
		t->q0 = t->q1;
		t->q1 =u;
	}

	if(t->q0 == t->q1)
		return;

	if(dosnarf){
		rs.nr = t->q1-t->q0;
		rs.r = runemalloc(rs.nr);
		runemove(rs.r, t->rs.r+t->q0, rs.nr);
		putsnarf(&rs);
		closerunestr(&rs);
	}
	if(docut){
		textdelete(t, t->q0, t->q1);
		textsetselect(t, t->q0, t->q0);
		if(t->w)
			textscrdraw(t);
	}else if(dosnarf)	/* Snarf command */
		argtext = t;
}

void
paste(Text *, Text *t, int selectall, int, Rune *, int)
{
	Runestr rs;
	uint q1;

	if(t == nil)
		return;

	getsnarf(&rs);
	if(rs.nr == 0)
		return;

	cut(t, t, FALSE, TRUE, nil, 0);
	textinsert(t, t->q0, rs.r, rs.nr);
	q1 = t->q0+rs.nr;
	if(selectall)
		textsetselect(t, t->q0, q1);
	else
		textsetselect(t, q1, q1);
	if(t->w)
		textscrdraw(t);

	closerunestr(&rs);
}

typedef	struct	Expand Expand;

struct Expand
{
	uint	q0;
	uint	q1;
	Rune	*name;
	int	nname;
	int	jump;
	union{
		Text	*at;
		Rune	*ar;
	};
	int	(*agetc)(void*, uint);
	int	a0;
	int	a1;
};

int
expand(Text *t, uint q0, uint q1, Expand *e)
{
	memset(e, 0, sizeof *e);

	/* if in selection, choose selection */
	e->jump = TRUE;
	if(q1==q0 && t->q1>t->q0 && t->q0<=q0 && q0<=t->q1){
		q0 = t->q0;
		q1 = t->q1;
		if(t->what == Tag)
			e->jump = FALSE;
	}
	if(q0 == q1){
		while(q1<t->rs.nr && isalnum(t->rs.r[q1]))
			q1++;
		while(q0>0 && isalnum(t->rs.r[q0-1]))
			q0--;
	}
	e->q0 = q0;
	e->q1 = q1;
	return q1 > q0;
}

void
look3(Text *t, uint q0, uint q1)
{
	Expand e;
	Text *ct;
	Runestr rs;
	char buf[32];
	Rune *r, c;
	uint p;
	int n;

	ct = seltext;
	if(ct == nil)
		ct = seltext = t;
	if(expand(t, q0, q1, &e) == FALSE)
		return;
	if(plumbsendfd >= 0){
		/* send whitespace-delimited word to plumber */
		buf[0] = '\0';
		if(q1 == q0){
			if(t->q1>t->q0 && t->q0<=q0 && q0<=t->q1){
				q0 = t->q0;
				q1 = t->q1;
			}else{
				p = q0;
				while(q0>0 && (c=t->rs.r[q0-1])!=L' ' && c!=L'\t' && c!=L'\n')
					q0--;
				while(q1<t->rs.nr && (c=t->rs.r[q1])!=L' ' && c!=L'\t' && c!=L'\n')
					q1++;
				if(q1 == q0)
					return;
				sprint(buf, "click=%d", p-q0);
			}
		}
		rs.r = runemalloc(q1-q0);
		runemove(rs.r, t->rs.r+q0, q1-q0);
		rs.nr = q1-q0;
		if(plumbrunestr(&rs, buf) >= 0){
			closerunestr(&rs);
			return;
		}
		/* plumber failed to match; fall through */
	}
	if(t == ct)
		textsetselect(ct, e.q1, e.q1);
	n = e.q1 - e.q0;
	r = runemalloc(n);
	runemove(r, t->rs.r+e.q0, n);
	if(search(ct, r, n) && e.jump)
		moveto(mousectl, addpt(frptofchar(ct, ct->p0), Pt(4, ct->font->height-4)));

	free(r);
}

int
search(Text *ct, Rune *r, uint n)
{
	uint q, nb, maxn;
	int around;
	Rune *s, *b, *c;

	if(n==0 || n>ct->rs.nr || 2*n>RBUFSIZE)
		return FALSE;

	maxn = max(n*2, RBUFSIZE);
	s = runemalloc(RBUFSIZE);
	b = s;
	nb = 0;
	b[nb] = 0;
	around = 0;
	q = ct->q1;
	for(;;){
		if(q >= ct->rs.nr){
			q = 0;
			around = 1;
			nb = 0;
			b[nb] = 0;
		}
		if(nb > 0){
			c = runestrchr(b, r[0]);
			if(c == nil){
				q += nb;
				nb = 0;
				b[nb] = 0;
				if(around && q>=ct->q1)
					break;
				continue;
			}
			q += (c-b);
			nb -= (c-b);
			b = c;
		}
		/* reload if buffer covers neither string nor rest of file */
		if(nb<n && nb!=ct->rs.nr-q){
			nb = ct->rs.nr-q;
			if(nb >= maxn)
				nb = maxn-1;
			runemove(s, ct->rs.r+q, nb);
			b = s;
			b[nb] = '\0';
		}
		/* this runeeq is fishy but the null at b[nb] makes it safe */
		if(runeeq(b, n, r, n) == TRUE){
			if(ct->w)
				textshow(ct, q, q+n, 1);
			else{
				ct->q0 = q;
				ct->q1 = q+n;
			}
			seltext = ct;
			free(s);
			return TRUE;
		}
		if(around && q>=ct->q1)
			break;
		--nb;
		b++;
		q++;
	}
	free(s);
	return FALSE;
}

Window*
lookpage(Rune *s, int n)
{
	int i, j;
	Window *w;
	Column *c;
	Page *p;

	/* avoid terminal slash on directories */
	if(n>1 && s[n-1] == '/')
		--n;
	for(j=0; j<row.ncol; j++){
		c = row.col[j];
		for(i=0; i<c->nw; i++){
			w = c->w[i];
			p = &w->page;
			if(p->url && runeeq(p->url->src.r, p->url->src.nr, s, n))
				if(w->col != nil)
					return w;
		}
	}
	return nil;
}

Window *
openpage(Page *p, Runestr *rs)
{
	Window *w;

	if(!validurl(rs->r))
		return nil;

	w = lookpage(rs->r, rs->nr);
	if(w){
		p = &w->page;
		if(!p->col->safe && Dy(p->r)==0) /* window is obscured by full-column window */
			colgrow(p->col, p->col->w[0], 1);
	}else{
		w = makenewwindow(p);
		winsettag(w);
		pageget(&w->page, rs, nil, HGet, TRUE);
	}
	return w;
}

void
plumblook(Plumbmsg *m)
{
	Runestr rs;

	if(m->ndata >= BUFSIZE){
		fprint(2, "insanely long file name (%d bytes) in plumb message (%.32s...)\n", m->ndata, m->data);
		return;
	}
	if(m->data[0] == '\0')
		return;

	bytetorunestr(m->data, &rs);
	openpage(nil, &rs);
	closerunestr(&rs);
}

void
new(Text *et, Text *, int, int, Rune *, int)
{
	if(et->col != nil)
		winsettag(coladd(et->col, nil, nil, -1));
}

void
google(Text *, Text *, int, int, Rune *arg, int narg)
{
	Runestr rs;
	Rune *s;

	s = ucvt(arg);
	rs.r = runesmprint("http://www.google.com/search?hl=en&ie=UTF-8&q=%.*S", narg, s);
	rs.nr = runestrlen(rs.r);
	openpage(nil, &rs);
	free(s);
	closerunestr(&rs);
}
