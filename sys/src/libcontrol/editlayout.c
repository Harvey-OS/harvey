#include <u.h>
#include <libc.h>
#include <thread.h>
#include <draw.h>
#include <mouse.h>
#include <keyboard.h>
#include <control.h>
#include <editcontrol.h>
#include "string.h"
	
Frag *frags;
Line *lines;
int nfrags, nlines;
int nfalloc, nlalloc;

static Frag *
allocfrag(void)
{
	Frag *f;

	nfalloc++;
	if (f = frags)
		frags = f->next;
	else{
		nfrags++;
		f = ctlmalloc(sizeof(Frag));
	}
	f->next = nil;
	return f;
}

Line *
allocline(void)
{
	Line *l;

	nlalloc++;
	if (l = lines)
		lines = l->next;
	else{
		nlines++;
		l = ctlmalloc(sizeof(Line));
	}
	l->f = nil;
	l->next = nil;
	return l;
}

void
freefrag(Frag *frag)
{
	Frag *f;

	if (frag == nil)
		return;
	for (f = frag; f->next; f = f->next)
		;
	f->next = frags;
	frags = frag;
}

void
freeline(Line *l)
{
	if (l == nil)
		return;
	l->next = lines;
	lines = l;
}

static int
insel(Edit *e, Position pos)
{
	int i;

	for (i = 0; i < nelem(e->sel); i++)
		if ((e->sel[i].state & (Dragging|Set))
		&& !_posbefore(pos, e->sel[i].mpl.pos)
		&& _posbefore(pos, e->sel[i].mpr.pos))
			return i;
	return -1;
}

static int
nextsel(Edit *e, Position spos)
{
	int i, sel;
	Position pos = {0x7fffffff, 0x7fffffff};

	sel = -1;
	for (i = 0; i < nelem(e->sel); i++)
		if ((e->sel[i].state & (Dragging|Set))
		&& _posbefore(spos, e->sel[i].mpl.pos)
		&& _posbefore(e->sel[i].mpl.pos, pos))
			pos = e->sel[sel = i].mpl.pos;
	return sel;
}

Mpos*
endsel(Edit *e, Position pos, int sel)
{
	Mpos *mp;
	int i;

	/* Find start of next selection; if sel is set, look for end */
	mp = &e->sel[sel].mpr;	// end of selection

	// look for higher priority selections starting earlier than epos
	for (i = 0; i < sel; i++)
		if (_posbefore(pos, e->sel[i].mpl.pos) && _posbefore(e->sel[i].mpl.pos, mp->pos))
			return &e->sel[i].mpl;
	return mp;
}

Frag *
findfrag(Edit *e, Position p)
{
	Line *l;
	Frag *f;

	for (l = e->l; l; l = l->next)
		for (f = l->f; f; f = f->next){
			assert(f->l == l);
			if (f->spos.str == p.str && f->spos.rn <= p.rn && p.rn <= f->epos.rn)
				return f;
		}
	return nil;
}

static Frag *
addfrag(Edit *e, Line *l, Position pos, int nr, int minx, int maxx)
{
	Selection *sel;
	String *s;
	Context *c;
	int w;
	Frag *f;

	f = allocfrag();
	f->minx = minx;
	f->maxx = maxx;
	f->spos = pos;
	f->epos = pos;
	f->epos.rn += nr;
	f->l = l;

	// check for selection end points
	for (sel = e->sel; sel < e->sel + nelem(e->sel); sel++){
		if ((sel->state & (Set|Dragging)) == 0)
			continue;
		if (sel->mpl.pos.str == pos.str
		&& sel->mpl.pos.rn >= pos.rn
		&& sel->mpl.pos.rn < pos.rn +nr){
			sel->mpl.f = f;
			s = stringof(e->ss, pos);
			c = _contextnamed(s->context);
			w = runestringnwidth(c->f->font, s->r + pos.rn, sel->mpl.pos.rn - pos.rn);
			sel->mpl.p = Pt(minx + w, l->r.min.y);
		}
		if (sel->mpr.pos.str == pos.str
		&& sel->mpr.pos.rn > pos.rn
		&& sel->mpr.pos.rn <= f->epos.rn){
			sel->mpr.f = f;
			s = stringof(e->ss, pos);
			c = _contextnamed(s->context);
			w = runestringnwidth(c->f->font, s->r + pos.rn, sel->mpr.pos.rn - pos.rn);
			sel->mpr.p = Pt(minx + w, l->r.min.y);
		}
	}
	return f;
}

void
layoutprep(Edit *e, Position p1, Position p2)
{
	/* Delete frags _posbefore changing screen content
	 * layoutrect will restore afterwards
	 */
	Frag *f, *ef, **fp;
	Line *l, *ll, *ls, *le, **lp;
	int newlredraw;

	f = nil;
	fp = nil;
	newlredraw = e->lredraw == nil;
	debugprint(DBGPREP, "layoutprep %d/%d - %d/%d, new %d\n", p1.str, p1.rn, p2.str, p2.rn, newlredraw);
	if (debug & DBGPREP) dumplines(e);
	for (lp = &e->l; ls = *lp; lp = &ls->next){
		if (ls == e->lredraw){
			debugprint(DBGPREP, "layoutprep current _posbefore new\n");
			newlredraw = 0;	// redraw point may be _posbefore current
		}
		for (fp = &ls->f; f = *fp; fp = &f->next){
			if (p1.str == f->spos.str && p1.rn < f->epos.rn)
				break;
			if (f->spos.str > p1.str)
				break;
		}
		if (f)
			break;
	}
	if (ls == nil){
		debugprint(DBGPREP, "layoutprep: nothing on screen\n");
		return;
	}
	if (e->lredraw && _posbefore(f->spos, e->lredraw->pos))
		newlredraw++;
	if (ef = findfrag(e, p2))
		le = ef->l;
	else
		le = nil;
	debugprint(DBGPREP, "layoutprep: ls = 0x%p, le = 0x%p\n", ls, le);
	if (newlredraw){
		e->lredraw = ls;
		ls->pos = f->spos;
		ls->rplay.min.x = f->minx;
		debugprint(DBGPREP, "layoutprep: new redraw point %d/%d, 0x%p, rect %R\n",
			ls->pos.str, ls->pos.rn, ls, ls->rplay);
		assert(*fp == f);
		freefrag(f);
		*fp = nil;
		lp = & ls->next;
		ls = *lp;
	}

	for(l = ls; l; l = ll) {
		ll = l->next;
		debugprint(DBGPREP, "layoutprep: freeline 0x%p\n", l);
		assert(*lp == l);
		freefrag(l->f);
		freeline(l);
		*lp = ll;
		if (l == le)
			break;
	}
	if (debug & DBGPREP) dumplines(e);
}

static Position
layoutline(Edit *e, Line *l)
{
	String *s;
	Context *c;
	Frag **fp, *f;
	int wordbreak, charbreak;
	int wordx, charx;
	Position pos;
	Rune *rp;
	int x, w, nr, dy, i, linebreak, fragbreak;

	debugprint(DBGLAYOUT, "layoutline\n");
	dy = 0;	// height of this line;
	for (fp = &l->f; f = *fp; fp = &(*fp)->next){
		s = stringof(e->ss, f->spos);
		c = _contextnamed(s->context);
		if (dy < c->f->font->height) dy = c->f->font->height;
	}
	wordbreak = -1;
	charbreak = -1;
	wordx = 0;	// x at last workbreak;
	charx = 0;		// x at last charbreak;
	s = nil;
	c = nil;
	x = 0;	// width of characters processed so far
	nr = 0;	// number of runes processed but not yet put in frag
	rp = nil;	// pointer to current rune (x, nr and rp are updated together)
	pos = l->pos;
	linebreak = 0;
	fragbreak = 0;
	for(;;){
		if (s == nil){
			if ((s = stringof(e->ss, pos)) == nil){
				debugprint(DBGLAYOUT, "layoutline: no more strings at string %d\n", pos.str);
				break;
			}
			c = _contextnamed(s->context);
			assert(c);
			rp = s->r + pos.rn;
			nr = 0;
		}
		if (fragbreak || rp >= s->r + s->n || *rp == '\n' || *rp == '\t' || x > Dx(l->rplay)){
			// all the reasons for dumping a fragment (and starting a new or finishing)

			if (x > Dx(l->rplay) && charbreak >= 0){
				// we've got too many characters, backup
				if (c->wordbreak && wordbreak >= 0){
					x = wordx;
					nr = wordbreak;
				}else{
					x = charx;
					nr = charbreak;
				}
				linebreak++;
			}

			if (nr){
				// add fragment
				f =addfrag(e, l, pos, nr, l->rplay.min.x, l->rplay.min.x+x);
				l->rplay.min.x += x;
				*fp = f;
				fp = &f->next;
				if (dy < c->f->font->height) dy = c->f->font->height;
				pos.rn += nr;
				x = 0;
				nr = 0;
				wordbreak = 0;
				charbreak = 0;
				wordx = 0;
				charx = 0;
			}
			if (linebreak) {
				debugprint(DBGLAYOUT, "layoutline: line break at %d:%d\n",
					pos.str, pos.rn);
				break;
			}
			if (rp >= s->r + s->n){
				// goto next string
				pos.rn = 0;
				pos.str++;
				s = nil;
				c = nil;
				continue;
			}
			fragbreak = 0;
		}
		charbreak = nr;
		charx = x;
		if (isspacerune(*rp)){
			wordbreak = nr;
			wordx = x;
		}
		if (*rp == '\t'){
			debugprint(DBGLAYOUT, "layoutline: tab at %d:%d\n",
				pos.str, pos.rn);

			w = Dx(l->r) - Dx(l->rplay);
			x = stringwidth(c->f->font, " ");
			for (i = 0; i < nelem(c->tabs); i++){
				if (c->tabs[i] - w >= x) {
					debugprint(DBGLAYOUT, "layoutline: tab %d at %d\n",
						i, c->tabs[i]);
					x = c->tabs[i] - w;
					break;
				}
			}
			if (i == nelem(c->tabs)){
				i = stringwidth(c->f->font, "0000");
				x = i - w % i;
				debugprint(DBGLAYOUT, "layoutline: insufficient tab stops\n",
					pos.str, pos.rn);
			}
			debugprint(DBGLAYOUT, "layoutline: tab width %d\n", x);
			fragbreak++;	// force a one-character frag
		}else if (*rp == '\n'){
			x = Dx(l->rplay);
			debugprint(DBGLAYOUT, "layoutline: newline at %d:%d\n",
				pos.str, pos.rn);
			fragbreak++;	// force a one-character frag
			linebreak++;
		} else {
			x += runestringnwidth(c->f->font, rp, 1);
		}
		rp++;
		nr++;
	}
	l->r.max.y = l->r.min.y + (e->spacing ? e->spacing : dy);
	l->rplay.max.y = l->r.max.y;
	l->pos = pos;
	if (s && pos.rn >= s->n){
		pos.str++;
		pos.rn = 0;
	}
	debugprint(DBGLAYOUT, "layoutline: done at %d/%d, play is %d\n", pos.str, pos.rn, Dx(l->rplay));
	return pos;
}

static void
adjustrects(Line *l, int dy)
{
	while (l) {
		l->r.min.y += dy;
		l->r.max.y += dy;
		l->rplay.min.y += dy;
		l->rplay.max.y += dy;
		l = l->next;
	}
}

void
showfrag(Edit *e, Frag *f)
{
	String *s;
	Context *c;
	Rune *rp, *ep;
	Position spos;
	Mpos *mp;
	int sel;
	Image *bg;
	Point p;

	debugprint(DBGSHOW, "showfrag\n");
	s = stringof(e->ss, f->spos);
	if (s == nil){
		debugprint(DBGSHOW, "showfrag: no more strings\n");
		abort();
	}
	c = _contextnamed(s->context);
	spos = f->spos;
	p = Pt(f->minx, f->l->r.min.y);
	rp = s->r + f->spos.rn;
	for(;;){
		debugprint(DBGSHOW, "showfrag: at pos %d/%d, rp 0x%p\n", spos.str, spos.rn, rp);
		if ((sel = insel(e, spos)) >= 0){
			debugprint(DBGSHOW, "showfrag: insel\n");
			bg = e->sel[sel].color->image;
			mp = endsel(e, spos, sel);
			if (!_posbefore(mp->pos, f->epos))
				ep = s->r + f->epos.rn;
			else{
				ep = s->r + mp->pos.rn;
			}
		}else{
			debugprint(DBGSHOW, "showfrag: regular\n");
			bg = e->image->image;
			sel = nextsel(e, spos);
			SET(mp);
			if (sel >= 0 && _posbefore((mp = &e->sel[sel].mpl)->pos, f->epos)){
				ep = s->r + mp->pos.rn;
			}else
				ep = s->r + f->epos.rn;
		}
		if (debug & DBGSHOW){
			int w;
			fprint(2, "showfrag: _string '");
			for (w = 0; w < ep-rp; w++)
				fprint(2, "%C", rp[w]);
			w = runestringnwidth(c->f->font, rp, ep-rp);
			fprint(2, "' in %R\n", Rect(p.x, p.y, p.x+w, p.y+c->f->font->height));
		}
		assert(ep >= rp);
		if (*rp < ' ')
			draw(e->screen, fragr(f), bg, nil, ZP);
		else
			p = _string(e->screen, p, e->textcolor->image, ZP, c->f->font, nil, rp, ep-rp, fragr(f), bg, ZP);
		if (debug & DBGSHOW)
			border(e->screen, insetrect(fragr(f), 2), 1, e->bordercolor->image, e->bordercolor->image->r.min);
		if (ep >= s->r + f->epos.rn)
			break;
		spos = mp->pos;
		rp = ep;
	}
}

void
layoutrect(Edit *e)
{
	Line *l, *ll;
	Frag *f;
	Rectangle r, rr;
	Point p;
	Position pos, npos;

	r = insetrect(e->rect, e->border);
	if (e->ss == nil){
		draw(e->screen, r, e->image->image, nil, e->image->image->r.min);
		return;
	}
	if ((l = e->lredraw) == nil){
		debugprint(DBGLAYOUT, "layoutrect: nothing to do\n");
		return;
	}
	pos = l->pos;
	debugprint(DBGLAYOUT, "layoutrect: %R start at line 0x%p pos %d/%d Point %P\n",
		r, l, pos.str, pos.rn, l->rplay.min);
	if (debug & DBGLAYOUT) dumplines(e);

	for(;;) {
		debugprint(DBGLAYOUT, "layoutrect: next line\n");
		npos = layoutline(e, l);
		r.min.y = l->r.max.y;
		if (r.min.y >= r.max.y) {
			debugprint(DBGLAYOUT, "layoutrect: new line doens't fit, delete\n");
			l->pos = pos;
			freefrag(l->f);
			l->f = nil;
			l->r.max.y = r.max.y;
			l->rplay.min.x = r.min.x;
			l->rplay.max.y = r.max.y;
			ll = l->next;
			break;
		}
		pos = npos;
		while ((ll = l->next) && ll->f){
			debugprint(DBGLAYOUT, "layoutrect: !_posbefore(%d/%d, %d/%d)\n",
				ll->f->spos.str, ll->f->spos.rn, pos.str, pos.rn);
			if (!_posbefore(ll->f->spos, pos)){
				if (ll->r.min.y >= r.min.y){
					debugprint(DBGLAYOUT|DBGSHOW, "layoutrect: next line not in the way\n");
					break;
				}
				debugprint(DBGLAYOUT|DBGSHOW, "layoutrect: move lines out of the way");
				debugprint(DBGLAYOUT|DBGSHOW, ", rect %R dst rect %R, src pt %P\n", r, rr, p);
				draw(e->screen, r, e->screen, nil, ll->r.min);
				adjustrects(ll, r.min.y - ll->r.min.y);
				break;
			}
			// this one's no use
			debugprint(DBGLAYOUT|DBGSHOW, "layoutrect: delete early line %R\n", ll->r);
			l->next = ll->next;
			freefrag(ll->f);
			freeline(ll);
		}
		// now draw this line
		debugprint(DBGLAYOUT, "layoutrect: draw line\n");
		rr = l->r;
		for(f = l->f; f; f = f->next){
			if (f->minx > rr.min.x){
				rr.max.x = f->minx;
				draw(e->screen, rr, e->image->image, nil, e->image->image->r.min);
				if (debug & DBGSHOW)
					border(e->screen, insetrect(rr, 1), 1, display->black, ZP);
			}
			showfrag(e, f);
			rr.min.x = f->maxx;
		}
		draw(e->screen, l->rplay, e->image->image, nil, e->image->image->r.min);
		if (debug & DBGSHOW)
			border(e->screen, insetrect(l->rplay, 1), 1, display->black, ZP);
		r.min.y = l->r.max.y;
		if (r.min.y >= r.max.y){
			debugprint(DBGLAYOUT, "layoutrect: bottom of screen at %d/%d\n",
				pos.str, pos.rn);
			break;
		}
		if (pos.str >= e->ss->nstring){
			debugprint(DBGLAYOUT, "layoutrect: end of input %d/%d\n",
				pos.str, pos.rn);
			draw(e->screen, r, e->image->image, nil, e->image->image->r.min);
			ll = l->next;
			break;
		}
		while (ll && ll->f && _posequal(ll->f->spos, pos)){
			assert(ll == l->next);
			debugprint(DBGLAYOUT, "layoutrect: using old line at %d/%d\n",
				pos.str, pos.rn);
			l = ll;
			ll = l->next;
			if (l->r.min.y != r.min.y){
				debugprint(DBGLAYOUT, "layoutrect: move up by %d\n",
					l->r.min.y - r.min.y);
				draw(e->screen, r, e->screen, nil, l->r.min);
				adjustrects(l, r.min.y - l->r.min.y);
			}
			pos = l->pos;
			r.min.y = l->r.max.y;
			if (r.min.y >= r.max.y)
				break;
		}
		if (r.min.y >= r.max.y){
			debugprint(DBGLAYOUT, "layoutrect: bottom of screen while processing used lines\n");
			break;
		}
		debugprint(DBGLAYOUT, "layoutrect: start a new line\n");
		l->next = allocline();
		l = l->next;
		l->next = ll;
		l->r = r;
		l->rplay = r;
		l->pos = pos;
	}
	debugprint(DBGLAYOUT, "layoutrect: done\n");
	if (r.min.y < r.max.x){
		debugprint(DBGLAYOUT, "layoutrect: blank rest of screen\n");
		draw(e->screen, r, e->image->image, nil, e->image->image->r.min);
	}
	assert(ll == l->next);
	l->next = nil;
	for (l = ll; l; l = ll){
		ll = l->next;
		freefrag(l->f);
		freeline(l);
	}
	e->lredraw = nil;
	e->bot = pos;
}
