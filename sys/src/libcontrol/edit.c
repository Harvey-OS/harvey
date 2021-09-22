#include <u.h>
#include <libc.h>
#include <thread.h>
#include <draw.h>
#include <mouse.h>
#include <keyboard.h>
#include <control.h>
#include <editcontrol.h>
#include "string.h"

#define	FRTICKW	3
#define	FRTICKH	24

int debug;
enum{
	EAppend,
	EAddrune,
	EBorder,
	EBordercolor,
	EClear,
	EContext,
	ECut,
	EDelete,
	EDelrune,
EDump,
	EFocus,
	EHide,
	EImage,
	EInsert,
	EJoin,
	EPaste,
	ERect,
	ERedo,
	ERedraw,
	EReveal,
	EScroll,
	ESelect,
	ESelectcolor,
	EShow,
	ESize,
	ESnarf,
	ESpacing,
	ESplit,
	EStringset,
	ETextcolor,
	ETop,
	EUndo,
	EValue,
};

static char *cmds[] = {
	[EAppend] =		"append",	//	runes
	[EAddrune] =		"addrune",	//	runes
	[EBorder] =		"border",
	[EBordercolor] =	"bordercolor",
	[EClear] =		"clear",
	[EContext] =		"context",	//	name attr=value ...
	[ECut] =		"cut",		//	selection #
	[EDelete] =		"delete",	//	string#
	[EDelrune] =		"delrune",	//	# of runes
	[EDump] =		"dump",		//	debugging
	[EFocus] = 		"focus",
	[EHide] =		"hide",
	[EImage] =		"image",
	[EInsert] =		"insert",	//	string#, { runes }
	[EJoin] =		"join",		//	position
	[EPaste] =		"paste",	//	selection #
	[ERect] =		"rect",
	[ERedo] =		"redo",
	[ERedraw] =		"redraw",
	[EReveal] =		"reveal",
	[EScroll] =		"scroll",
	[ESelect] =		"select",
	[ESelectcolor] =	"selectcolor",	//	color, color, color (empty names allowed)
	[EShow] =		"show",
	[ESize] =		"size",
	[ESnarf] =		"snarf",	//	selection #
	[ESpacing] =		"spacing",	//	[ font | npixels ]
	[ESplit] =		"split",	//	position
	[EStringset] =		"stringset",	//	0x%p, address of stringset
	[ETextcolor] =		"textcolor",
	[ETop] =		"top",		//	string#, rune#
	[EUndo] =		"undo",
	[EValue] =		"value",	//	string#, fontname, runes
	nil
};

/*	The edit control manages the display of a series of Strings.  Strings are
	numbered.  The value of a String is a Font, and a series of zero or more
	Runes.  A Position is a String number and a Rune number within the String.
	Top is the Position of the first Rune visible in the edit control.

	The edit control need not know the value of each String.  When a String
	whose value is not known becomes visible in the edit control, the control
	will generate an event requesting the value.

	Keystrokes generate events; they do not directly affect the display, nor the
	contents of any String.

	The mouse can be used to make selections and generate events.  Each button
	event is reported in an event which reports which buttons are down after the
	event.  Position is reported in terms of String/Rune number.
	In addition, text selection colors can be associated with mouse buttons
	1, 2 and 3.  If a color is associated with button n, then pressing that button
	by itself will initiate a selection in the associated color.  Releasing the button
	or pressing any other button will freeze the selection (and, as mentioned, will
	report the button event).  The three selections may also be set via a select
	command.
	Creating a selection will remove the previous selection for the same button.

	If line-spacing is defined, it determines the height given to each line.  If it
	is not defined, each line is given the highest font height occurring on the line.
	Lines are broken either at white space, or after the last rune that fits, depending
	on the line-break setting.  A line break always occurs at a newline character.

	The Addrune (Delrune) commands replace (delete) the given selection,
	or they insert characters after (delete the character _posbefore) the typing tick;
	the selection is replaced by a  null selection (typing tick) after the character(s)
	inserted (_posbefore the character deleted).  Addrune adds its characters to
	the String containing the beginning of the Selection.
	Delrune at the beginning of the first String is a no-op.
	When Scroll is on, an Addrune or Delrune command will automatically set
	Top to make the cursor visible.
*/

Context* context;

static void scroll(Edit *e, Position top);

static void
editclean(Edit *e)
{
	Selection *s;
	Line *l, *ll;

	debugprint(DBGCLEAN, "editclean\n");
	for (s = e->sel; s < e->sel + nelem(e->sel); s++){
		s->mpl.f = nil;
		s->mpr.f = nil;
		s->mpl.p = Pt(-1, -1);
		s->mpr.p = Pt(-1, -1);
	}
	for (l = e->l; l; l = ll){
		freefrag(l->f);
		ll = l->next;
		freeline(l);
	}
	e->l = nil;
}

static void
editclear(Edit *e)
{
	editclean(e);
	freestringset(e->ss);
	e->ss = newstringset("default");
}

static void
editfree(Control *c)
{
	int i;
	Edit *e;

	e = (Edit*)c;
	_putctlimage(e->image);
	_putctlimage(e->textcolor);
	_putctlimage(e->bordercolor);
	for(i=0; i<nelem(e->sel); i++)
		if (e->sel[i].color)
			_putctlimage(e->sel[i].color);
	editclear(e);
	free(e->ss->string);
}

void
adjustlines(Edit *e, Undo *c)
{
	Line *l;
	Frag *f;

	for (l = e->l; l; l = l->next){
		for (f = l->f; f; f = f->next){
			_posadjust(c, &f->spos);
			_posadjust(c, &f->epos);
			assert(!_posbefore(f->epos, f->spos));
			if (f->epos.str > f->spos.str){
				f->epos.str = f->spos.str;
				f->epos.rn = e->ss->string[f->epos.str]->n;
			}
		}
	}
}

void
adjustselections(Edit *e, Undo *c)
{
	Selection *sel;

	for (sel = e->sel; sel < e->sel + nelem(e->sel); sel++){
		if ((sel->state & (Dragging|Set)) == 0)
			continue;
		_posadjust(c, &sel->mpl.pos);
		_posadjust(c, &sel->mpr.pos);
	}
}

static void
showselect(Edit *e, Mpos *p1, Mpos *p2, int which, int sel)
{
	Frag *f;
	Rune *rp, *ep;
	Stringset *ss;
	String *s;
	Context *c;
	Position lpos, rpos;
	Image *color;
	Point p;
	Rectangle r;
	int i;
	Line *l;

	debugprint(DBGSELECT, "select: %d/%d %d/%d %d\n",
			p1->pos.str, p1->pos.rn, p2->pos.str, p2->pos.rn, sel);
	ss = e->ss;
	if (_posequal(p1->pos, p2->pos))
		return;
	assert(_posbefore(p1->pos, p2->pos));
	if (!_posbefore(e->top, p2->pos) || !_posbefore(p2->pos, e->bot))
		return;	// that was easy
	if (_posbefore(p1->pos, e->top))
		p1->pos = e->top;
	if (_posbefore(e->bot, p2->pos))
		p2->pos = e->bot;
	if (sel)
		color = e->sel[which].color->image;
	else{
		// deselecting, check for clashes with other selections
		for (i = 0; i < nelem(e->sel); i++) {
			if (i == which || (e->sel[i].state & Set) == 0)
				continue;
			if (!_posbefore(p1->pos, e->sel[i].mpr.pos) || !_posbefore(e->sel[i].mpl.pos, p2->pos))
				continue;
			debugprint(DBGSELECT, "select: clash with selection %d\n", i);
			if (!_posbefore(p1->pos, e->sel[i].mpl.pos)){
				if (_posbefore(e->sel[i].mpr.pos, p2->pos)){
					debugprint(DBGSELECT, "select: left clash\n");
					showselect(e, p1, &e->sel[i].mpr, i, 1);
					showselect(e, &e->sel[i].mpr, p2, which, 0);
				}else{
					debugprint(DBGSELECT, "select: complete clash\n");
					showselect(e, p1, p2, i, 1);
				}
			}else{
				if (_posbefore(e->sel[i].mpr.pos, p2->pos)){
					debugprint(DBGSELECT, "select: internal clash\n");
					showselect(e, p1, &e->sel[i].mpl, which, 0);
					showselect(e, &e->sel[i].mpl, &e->sel[i].mpr, i, 1);
					showselect(e, &e->sel[i].mpr, p2, which, 0);
				}else{
					debugprint(DBGSELECT, "select: right clash\n");
					showselect(e, p1, &e->sel[i].mpl, which, 0);
					showselect(e, &e->sel[i].mpl, p2, i, 1);
				}
			}
			return;
		}
		color = e->image->image;
	}
	f = findfrag(e, p1->pos);
	assert(f);
	for(;;) {
		debugprint(DBGSELECT, "select: new frag %d/%d-%d/%d %R\n", f->spos.str, f->spos.rn, f->epos.str, f->epos.rn, fragr(f));
		s = stringof(ss, f->spos);
		if (s == nil){
			SET(s);
			abort();
		}
		c = _contextnamed(s->context);
		r = fragr(f);
		if (_posbefore(f->spos, p1->pos)){
			debugprint(DBGSELECT, "select: start halfway\n");
			// start half way
			assert(f->spos.str == p1->pos.str);
			lpos = p1->pos;
			p = p1->p;
			r.min.x = p1->p.x;
		}else{
			lpos = f->spos;
			p = Pt(f->minx, f->l->r.min.y);
		}
		rp = s->r + lpos.rn;
		rpos = f->epos;
		if (_posbefore(p2->pos, rpos)){
			rpos = p2->pos;
			r.max.x = p2->p.x;
		}
		ep = s->r + rpos.rn;
		// selection is from rune ptr rp to ep, and from position lpos to rpos
		debugprint(DBGSELECT, "select: %P %R %ld\n", p, r, ep-rp);
		assert(ep >= rp);
		if (rp != ep){
			if (*rp < ' '){
				assert(ep - rp == 1);
				draw(e->screen, fragr(f), color, nil, ZP);
			}else
				_string(e->screen, p, e->textcolor->image, ZP, c->f->font, nil, rp, ep-rp, r, color, ZP);
		}
		if (!_posbefore(rpos, p2->pos)){
			debugprint(DBGSELECT, "select: finished\n");
			break;
		}
		if (f->next){
			debugprint(DBGSELECT, "select: next frag in line\n");
			f = f->next;
		}else {
			debugprint(DBGSELECT, "select: frag in line after 0x%p\n", f->l);
			l = f->l;
			f = nil;
			if (l = l->next)
				while (l){
					if (f = l->f)
						break;
					debugprint(DBGSELECT, "select: no frag in line 0x%p\n", l);
					l = l->next;
				}
			if (f == nil){
				debugprint(DBGSELECT, "select: no more frags\n");
				break;
			}
		}
	}
}

static Mpos
p2pos(Edit *e, Point p)
{
	Mpos m;
	Line *l, *ll;
	Frag *f;
	int dx, w;
	Rune *rp;
	Stringset *ss;
	String *s;
	Context *c;
	Rectangle r;

	ss = e->ss;
	r = insetrect(e->rect, e->border);
	m.p = r.min;
	m.pos.str = -1;
	m.f = nil;
	m.flags = 0;
	if (p.x >= r.max.x) p.x = r.max.x - 1;
	if (p.x < r.min.x) p.x = r.min.x;
	if (p.y >= r.max.y) p.y = r.max.y - 1;
	if (p.y < r.min.y) p.y = r.min.y;
	ll = nil;
	for (l = e->l; l; l = l->next){
		ll = l;
		if (ptinrect(p, l->r)){
			m.p = l->r.min;
			for (f = l->f; f; f = f->next){
				m.pos = f->spos;
				m.f = f;
				if (ptinrect(p, fragr(f))){
					if ((s = stringof(ss, m.pos)) == nil)
						abort();
					c = _contextnamed(s->context);
					rp = s->r + m.pos.rn;
					dx= 0;
					if (*rp >= ' ')
						for( ; rp < s->r + f->epos.rn; rp++){
							w = runestringnwidth(c->f->font, rp, 1);
							if (f->minx + dx + w >= p.x)
								break;
							dx += w;
						}
					m.pos.rn = rp - s->r;
					m.p.x = f->minx + dx;
					return m;
				}
				m.p.x = f->maxx;
				m.pos = f->epos;
			}
			m.flags = Eol;
			return m;
		}
		m.f = nil;
	}
	if (ll){
		m.p = ll->rplay.min;
		for (f = ll->f; f; f = f->next)
			m.f = f;
	}
	m.pos = e->bot;
	m.flags = Bof;
	return m;
}

static int
pos2mpos(Edit *e, Position pos, Mpos *mp)
{
	String *s;
	Stringset *ss;
	Context *c;

	ss = e->ss;
	mp->pos = pos;
	mp->p = Pt(-1, -1);
	mp->f = nil;
	if ((mp->f = findfrag(e, pos)) == nil){
		mp->flags = Outside;
		return 0;
	}
	mp->flags = 0;
	assert(mp->f->spos.str == pos.str);
	s = stringof(ss, pos);
	assert(s);
	c = _contextnamed(s->context);
	mp->p = Pt(mp->f->minx, mp->f->l->r.min.y);
	mp->p.x += runestringnwidth(c->f->font, s->r + mp->f->spos.rn, pos.rn - mp->f->spos.rn);
	return 1;
}

void
layoutselections(Edit *e)
{
	Selection *sel;

	if (e->ss == nil)
		return;
	for (sel = e->sel; sel < e->sel + nelem(e->sel); sel++){
		if ((sel->state & (Dragging|Set)) == 0)
			continue;
		pos2mpos(e, sel->mpl.pos, &sel->mpl);
		pos2mpos(e, sel->mpr.pos, &sel->mpr);
	}
}

static void
frinittick(Edit *e)
{
	/* Shamelessly stolen from libframe */
	Image *b;
	Selection *s;

	b = display->image;
	for (s = e->sel; s < e->sel + nelem(e->sel); s++){
		if(s->tick)
			free(s->tick);
		s->tick = allocimage(display, Rect(0, 0, FRTICKW, FRTICKH), b->chan, 0, DWhite);
		if(s->tick == nil)
			return;
		s->tickback = allocimage(display, s->tick->r, b->chan, 0, DWhite);
		if(s->tickback == 0){
			freeimage(s->tick);
			s->tick = 0;
			return;
		}
		s->tickh = 0;
		s->state &= ~Ticked;
	}
}

static void
frtick(Edit *e, Selection *sel, int ticked)
{
	Rectangle r;
	Stringset *ss;
	String *s;
	Context *c;
	int h;
	Mpos *mp;

	if ((ss = e->ss) == nil)
		return;
	/* Shamelessly stolen from libframe */
	mp = &sel->mpl;
	debugprint(DBGTICK, "frtick: %P %d\n", mp->p, ticked);
	if((sel->state & Ticked) == ticked || sel->tick==0)
		return;
	if (pos2mpos(e, mp->pos, mp) == 0)
		return;
	r = Rect(mp->p.x-1, mp->p.y, mp->p.x+FRTICKW-1, mp->p.y + FRTICKH);
	if(ticked){
		if(_posbefore(mp->pos, e->top) || _posbefore(e->bot, mp->pos))
			return;
		draw(sel->tickback, sel->tickback->r, screen, nil, r.min);
		s = stringof(ss, mp->f->spos);
		assert(s);
		c = _contextnamed(s->context);
		h = c->f->font->height;
		if (h > FRTICKH) h = FRTICKH;
		if (h != sel->tickh){
			debugprint(DBGTICK, "frtick: new cursor ht %d\n", h);
			/* background color */
			draw(sel->tick, sel->tick->r, e->image->image, nil, ZP);
			/* vertical line */
			draw(sel->tick, Rect(FRTICKW/2, 0, FRTICKW/2+1, h), display->black, nil, ZP);
			/* box on each end */
			draw(sel->tick, Rect(0, 0, FRTICKW, FRTICKW), e->textcolor->image, nil, ZP);
			draw(sel->tick, Rect(0, h-FRTICKW, FRTICKW, h), e->textcolor->image, nil, ZP);
			sel->tickh = h;
		}
		draw(e->screen, r, sel->tick, nil, ZP);
		sel->state |= ticked;
	}else{
		draw(e->screen, r, sel->tickback, nil, ZP);
		sel->state &= ~Ticked;
	}
}

static void
showticks(Edit *e)
{
	Selection *s;

	for (s = e->sel; s < e->sel + nelem(e->sel); s++)
		if ((s->state & (Dragging|Set)) && _posequal(s->mpl.pos, s->mpr.pos)){
			s->state &= ~Ticked;
			frtick(e, s, Ticked);
		}
}

static void
hideticks(Edit *e)
{
	Selection *s;

	for (s = e->sel; s < e->sel + nelem(e->sel); s++)
		if (s->state & Ticked)
			frtick(e, s, 0);
}

static void
drag(Edit *e, Mouse *m, int cursel){
	Selection *s;
	Mpos mp;
	Point p;
	Rectangle r;
	Position pos;

	if (debug) fprint(2, "drag\n");
	r = insetrect(e->rect, e->border);
	s = e->sel + cursel;
	assert(s->state & Dragging);
	p = m->xy;
	if (p.y >= r.max.y) {
		p.y = r.max.y - 1;
		pos = e->top;
		if (_posinc(e->ss, &pos) && _stringfindrune(e->ss, &pos, '\n')){
			if (debug) fprint(2, "scroll down from %d/%d to %d/%d\n",
				e->top.str, e->top.rn, pos.str, pos.rn);
			scroll(e, pos);
		}
	}
	if (p.y < r.min.y)  {
		pos = e->top;
		p.y = r.min.y;
		if (!_posdec(e->ss, &pos) || !_stringfindrrune(e->ss, &pos, '\n')){
			pos.str = 0;
			pos.rn = 0;
		}
		if (debug) fprint(2, "scroll up from %d/%d to %d/%d\n",
			e->top.str, e->top.rn, pos.str, pos.rn);
		scroll(e, pos);
	}
	mp = p2pos(e, m->xy);
	if (mp.flags & Outside)
		return;
	if (s->state & Drag1){
		if (_posequal(s->mpr.pos,  mp.pos))
			return;	// nothing changed
		if (_posbefore(s->mpr.pos, mp.pos)){
			// selection became bigger
			showselect(e, &s->mpr, &mp, cursel, 1);
			s->mpr = mp;
		}else if (_posbefore(mp.pos, s->mpl.pos)){
			// went past beginning of selection
			showselect(e, &s->mpl, &s->mpr, cursel, 0);
			showselect(e, &mp, &s->mpl, cursel, 1);
			s->mpr = s->mpl;
			s->mpl = mp;
			s->state &= ~Drag1;
		} else {
			// selection became smaller
			showselect(e, &mp, &s->mpr, cursel, 0);
			s->mpr = mp;
		}
	}else{
		if (_posequal(s->mpl.pos, mp.pos))
			return;	// nothing changed
		if (_posbefore(mp.pos, s->mpl.pos)){
			// selection became bigger
			showselect(e, &mp, &s->mpl, cursel, 1);
			s->mpl = mp;
		}else if (_posbefore(s->mpr.pos, mp.pos)){
			// went past beginning of selection
			showselect(e, &s->mpl, &s->mpr, cursel, 0);
			showselect(e, &s->mpr, &mp, cursel, 1);
			s->mpl = s->mpr;
			s->mpr = mp;
			s->state |= Drag1;
		} else {
			// selection became smaller
			showselect(e, &s->mpl, &mp, cursel, 0);
			s->mpl = mp;
		}
	}
}

static void
editkey(Control *c, Rune *r)
{
	while (*r){
		switch(*r){
		case 0x000d:	//end
		case 0x0080:	//down arrow
		case 0xf00d:	//home
		case 0xf00e:	//up arrow
		case 0xf00f:	//page up
		case 0xf011:	//left arrow
		case 0xf012:	//right arrow
		case 0xf013:	//page down
			break;
		default:
			chanprint(c->event, "%q: key %d", c->name, *r);
		}
		r++;
	}
}

static void
editmouse(Control *c, Mouse *m)
{
	Edit *e;
	Mpos mp;
	static int cursel = -1;
	int sel;
	Selection *s;

	e = (Edit*)c;
	if (e->lastbut == 0 && (m->buttons & 7) == 0)
		return;
	if (cursel >= 0){
		s = e->sel + cursel;
		if (_posequal(s->mpl.pos, s->mpr.pos))
			frtick(e, s, 0);
		drag(e, m, cursel);
		if (_posequal(s->mpl.pos, s->mpr.pos))
			frtick(e, s, Ticked);
		flushimage(display, 1);
		if (e->lastbut != (m->buttons & 0x7)){
			// end selection, buttons changed
			s->state &= ~Dragging;
			s->state |= Set;
			chanprint(e->event, "%q: selection %d %d %d %d %d %d", e->name, cursel,
				s->mpl.pos.str, s->mpl.pos.rn, s->mpr.pos.str, s->mpr.pos.rn,
				m->buttons & 7);
			cursel = -1;
		}
		e->lastbut = m->buttons & 7;
		return;
	}
	sel = 0;
	mp = p2pos(e, m->xy);
	if (mp.flags & Outside)
		return;
	if (e->lastbut == 0)
		switch (m->buttons&7){
		case 4:
			sel++;
		case 2:
			sel++;
		case 1:
			s = e->sel + sel;
			if(s->state & Enabled){
				debugprint(DBGMOUSE, "start select\n");
				assert((s->state & Dragging) == 0);
				if (s->state & Set){
					if (_posequal(s->mpl.pos, s->mpr.pos))
						frtick(e, s, 0);
					else
						showselect(e, &s->mpl, &s->mpr, sel, 0);
				}
				s->state &= ~(Set|Drag1);
				s->state |= Dragging;
				s->mpl = mp;
				s->mpr = mp;
				cursel = sel;
				e->lastbut = m->buttons & 7;
				frtick(e, s, Ticked);
				flushimage(display, 1);
				return;
			}
		}
	if (e->lastbut != (m->buttons & 7)){
		chanprint(e->event, "%q: button %d %d %d", e->name,
				mp.pos.str, mp.pos.rn, m->buttons & 7);
		e->lastbut = m->buttons & 7;
	}
}

static void
editlayout(Edit *e)
{
	Line *l;

	debugprint(DBGLAYOUT, "editlayout\n");
	editclean(e);
	assert(e->l == nil);
	l = allocline();
	l->r = insetrect(e->rect, e->border);
	l->rplay = insetrect(e->rect, e->border);
	l->pos = e->top;
	e->l = l;
	e->lredraw = e->l;
	layoutrect(e);
	layoutselections(e);
	e->dirty = 0;
}

static void
scroll(Edit *e, Position top)
{
	Line *l;

	debugprint(DBGLAYOUT, "scroll: from %d/%d to %d/%d\n",
		e->top.str, e->top.rn, top.str, top.rn);
	hideticks(e);
	if (_posequal(e->top, top))
		return;
	e->top = top;
	l = allocline();
	l->r = insetrect(e->rect, e->border);
	l->rplay = insetrect(e->rect, e->border);
	l->pos = e->top;
	l->next = e->l;
	e->l = l;
	e->lredraw = e->l;
	layoutrect(e);
	layoutselections(e);
	showticks(e);
	flushimage(display, 1);
	e->dirty = 0;
}

static void
editshow(Edit *e)
{
	Rectangle r;
	Position pos;
	Frag *f;
	int lineno;
	Line *l;

	if (e->hidden){
		debugprint(DBGSHOW, "editshow: returns: hidden");
		return;
	}
	r = e->rect;
	debugprint(DBGSHOW, "editshow: clear rect %R", r);
	draw(e->screen, r, e->image->image, nil, e->image->image->r.min);
	if(e->border > 0){
		border(e->screen, r, e->border, e->bordercolor->image, e->bordercolor->image->r.min);
		r = insetrect(r, e->border);
		debugprint(DBGSHOW, "editshow: after border rect %R", r);
	}
	if (e->dirty){
		debugprint(DBGSHOW, ": dirty, using editlayout\n");
		editlayout(e);
	}else{
		debugprint(DBGSHOW, ": clean\n");
		if (pos.str < 0){
			debugprint(DBGSHOW, "editshow: early return\n");
			return;
		}
		lineno = 0;
		for (l = e->l; l; l = l->next) {
			debugprint(DBGSHOW, "editshow: line %d\n", lineno++);
			for (f = l->f; f; f = f->next)
				showfrag(e, f);
		}
	}
	showticks(e);
	flushimage(display, 1);
}

static void
editctl(Control *ctl, CParse *cp)
{
	int cmd, i, n, thistab, lasttab;
	Rectangle r;
	Position p1, p2;
	Stringset *ss;
	String *s;
	Edit *e;
	Selection *sel;
	Rune rune;
	Context *c;

	e = (Edit*)ctl;
	if (ss = e->ss)
		qlock(ss);
	cmd = _ctllookup(cp->args[0], cmds, nelem(cmds));
	switch(cmd){
	case EAddrune:
		_ctlargcount(e, cp, 2);
		sel = e->sel;
		if ((sel->state & Set) == 0)
			ctlerror("%q: inactive selection: %d", e->name, 0);
		rune = cp->iargs[1];
		if (ss == nil){
			ss = newstringset("default");
			ss->c = (Control*)e;
			qlock(ss);
			e->ss = ss;
		}
		hideticks(e);
		cut(ss, sel->mpl.pos, sel->mpr.pos);
		if (ss->nstring == 0){
			char buf[UTFmax+1];

			snprint(buf, sizeof buf, "%C", rune);
			s = allocstring("default", buf);
			_stringadd(ss, s, 0);
			freestring(s);
			sel->mpr.pos.rn++;
			sel->mpl.pos.rn++;
		}else{
			sel->mpr.pos = _addonerune(ss, sel->mpl.pos, rune);
			sel->mpl.pos = sel->mpr.pos;
		}
		e->dirty = 1;
		layoutrect(e);
		layoutselections(e);
		showticks(e);
		flushimage(display, 1);
		break;
	case EAppend:
		_ctlargcount(e, cp, 2);
		s = ctlmalloc(sizeof(String));
		s->context = strdup(e->curcontext->name);
		s->r = _ctlrunestr(cp->args[1]);
		s->n = runestrlen(s->r);
		s->ref = 1;
		debugprint(DBGSTRING, "allocate string: 0x%p %s [%d]", s, e->curcontext->name, s->n);
		if (ss == nil){
			ss = newstringset("default");
			ss->c = (Control*)e;
			qlock(ss);
			e->ss = ss;
		}
		_stringspace(ss, ss->nstring+1);
		ss->string[ss->nstring] = s;
		ss->nstring++;
		break;
	case EBorder:
		_ctlargcount(e, cp, 2);
		if(cp->iargs[1] < 0)
			ctlerror("%q: bad border: %c", e->name, cp->str);
		e->border = cp->iargs[1];
		e->dirty = 1;
		break;
	case EBordercolor:
		_ctlargcount(e, cp, 2);
		_setctlimage(e, &e->bordercolor, cp->args[1]);
		break;
	case EClear:
		_ctlargcount(e, cp, 1);
		editclear(e);
		e->dirty = 1;
		editshow(e);
		break;
	case EContext:
		if (cp->nargs < 2)
			ctlerror("%q: context: argcount", e->name);
		for (c = context; c; c = c->next)
			if (strcmp(c->name, cp->args[1]) == 0)
				break;
		if (c == nil){
			if (cp->nargs == 2)
				ctlerror("%q: context: non-existent context %q",
					e->name, cp->args[1]);
			c = _newcontext(cp->args[1]);
		}
		context = c;
		if (cp->nargs == 2)
			break;
		_setctlfont(e, &c->f, cp->args[2]);
		if (cp->nargs == 3)
			break;
		c->wordbreak = cp->iargs[3];
		if (cp->nargs == 4 && c->tabs[0] > 0)
			break;
		if (cp->nargs-4 >= nelem(c->tabs))
			ctlerror("%q: context: maximum number of tab settings is %d",
				e->name, nelem(c->tabs));
		lasttab = 0;
		for (i = 4; i < cp->nargs; i++){
			thistab = cp->iargs[i];
			if (thistab <= lasttab)
				ctlerror("%q: context: tab settings must monotonically increase",
					e->name);
			c->tabs[i-4] = thistab;
			lasttab = thistab;
		}
		// set remaining tabs
		for (i = i-4; i < nelem(c->tabs); i++){
			thistab = lasttab + stringwidth(c->f->font, "0000");
			c->tabs[i] = thistab;
			lasttab = thistab;
		}
		c->next = context;
		context = c;
		break;
	case ECut:
		_ctlargcount(e, cp, 2);
		i = cp->iargs[1];
		if (i < 0 || i >= nelem(e->sel))
			ctlerror("%q: nonexistent selection: %d", e->name, i);
		sel = e->sel + i;
		if ((sel->state & Set) == 0)
			ctlerror("%q: inactive selection: %d", e->name, i);
		if (ss == nil)
			ctlerror("%q: no stringset to cut from");
		hideticks(e);
		cut(ss, sel->mpl.pos, sel->mpr.pos);
		layoutrect(e);
		layoutselections(e);
		showticks(e);
		flushimage(display, 1);
		break;
	case EDelete:
		_ctlargcount(e, cp, 2);
		n = cp->iargs[1];
		if (n < 0 || n >= ss->nstring)
			ctlerror("%q: no such string: %d", e->name, n);
		hideticks(e);
		delete(ss, n);
		layoutrect(e);
		layoutselections(e);
		showticks(e);
		flushimage(display, 1);
		break;
	case EDelrune:
		_ctlargcount(e, cp, 1);
		if (ss == nil)
			break;
		sel = e->sel;
		hideticks(e);
		if (_posequal(sel->mpl.pos, sel->mpr.pos)){
			sel->mpr.pos = _delonerune(ss, sel->mpl.pos);
			sel->mpl.pos = sel->mpr.pos;
		}else
			cut(ss, sel->mpl.pos, sel->mpr.pos);
		layoutrect(e);
		layoutselections(e);
		showticks(e);
		flushimage(display, 1);
		break;
	case EDump:
		if (ss){
			dumpstrings(ss);
			dumpundo(ss);
		}
		dumplines(e);
		dumpselections(e);
		break;
	case EFocus:
		break;
	case EHide:
		_ctlargcount(e, cp, 1);
		e->hidden = 1;
		break;
	case EImage:
		_ctlargcount(e, cp, 2);
		_setctlimage(e, &e->image, cp->args[1]);
		break;
	case EInsert:
		_ctlargcount(e, cp, 3);
		n = cp->iargs[1];
		s = ctlmalloc(sizeof(String));
		s->context = strdup(e->curcontext->name);
		s->r = _ctlrunestr(cp->args[2]);
		s->n = runestrlen(ss->string[n]->r);
		s->ref = 1;
		if (ss == nil){
			ss = newstringset("default");
			ss->c = (Control*)e;
			qlock(ss);
			e->ss = ss;
		}
		insert(ss, n, s);
		e->dirty = 1;
		layoutrect(e);
		layoutselections(e);
		showticks(e);
		flushimage(display, 1);
		break;
	case EJoin:
		_ctlargcount(e, cp, 3);
		p1.str = cp->iargs[1];
		p1.rn = cp->iargs[2];
		if (p1.str < 0 || p1.str >= ss->nstring-1)
			ctlerror("%q: no such string: %d", e->name, p1.str);
		hideticks(e);
		join(ss, p1);
		layoutrect(e);
		layoutselections(e);
		showticks(e);
		flushimage(display, 1);
		break;
	case EPaste:
		_ctlargcount(e, cp, 2);
		i = cp->iargs[1];
		if (i < 0 || i >= nelem(e->sel))
			ctlerror("%q: nonexistent selection: %d", e->name, i);
		sel = e->sel + i;
		if ((sel->state & Set) == 0)
			ctlerror("%q: inactive selection: %d", e->name, i);
		if (ss == nil)
			ctlerror("%q: no stringset to paste from");
		p1 = sel->mpl.pos;
		hideticks(e);
		cut(ss, sel->mpl.pos, sel->mpr.pos);
		paste(ss, sel->mpl.pos, ss->snarf, ss->nsnarf);
		sel->mpl.pos = p1;
		layoutrect(e);
		layoutselections(e);
		showticks(e);
		flushimage(display, 1);
		break;
	case ERect:
		_ctlargcount(e, cp, 5);
		r.min.x = cp->iargs[1];
		r.min.y = cp->iargs[2];
		r.max.x = cp->iargs[3];
		r.max.y = cp->iargs[4];
		if(Dx(r)<=0 || Dy(r)<=0)
			ctlerror("%q: bad rectangle: %s", e->name, cp->str);
		e->rect = r;
		e->dirty =1;
		break;
	case ERedo:
		_ctlargcount(e, cp, 1);
		if (ss == nil)
			break;
		hideticks(e);
		redo(ss);
		layoutrect(e);
		layoutselections(e);
		showticks(e);
		flushimage(display, 1);
		break;
	case ERedraw:
		_ctlargcount(e, cp, 1);
		editshow(e);
		break;
	case EReveal:
		_ctlargcount(e, cp, 1);
		e->hidden = 0;
		editshow(e);
		break;
	case EScroll:
		break;
	case ESelect:
		_ctlargcount(e, cp, 6);
		i = cp->iargs[1];
		if (i < 0 || i >= nelem(e->sel))
			ctlerror("%q: select index: %s", e->name, cp->str);
		sel = e->sel + i;
		if ((sel->state & Enabled) == 0)
			ctlerror("%q: select selection %d not enabled", e->name, i);
		hideticks(e);
		p1.str = cp->iargs[2];
		p1.rn = cp->iargs[3];
		p2.str = cp->iargs[4];
		p2.rn = cp->iargs[5];
		sel = e->sel + i;
		if ((sel->state & (Set|Dragging)) && ! _posequal(sel->mpl.pos, sel->mpr.pos))
			layoutprep(e, sel->mpl.pos, sel->mpr.pos);
		if (_posbefore(p2, p1)){
			sel->mpl.pos = p2;
			sel->mpr.pos = p1;
		}else{
			sel->mpl.pos = p1;
			sel->mpr.pos = p2;
		}
		sel->state = Enabled | Set;
		if (!_posequal(sel->mpl.pos, sel->mpr.pos))
			layoutprep(e, sel->mpl.pos, sel->mpr.pos);
		layoutrect(e);
		pos2mpos(e, sel->mpl.pos, &sel->mpl);
		pos2mpos(e, sel->mpr.pos, &sel->mpr);
		showticks(e);
		flushimage(display, 1);
		break;
	case ESelectcolor:
		_ctlargcount(e, cp, 3);
		i = cp->iargs[1];
		if (i < 0 || i >= nelem(e->sel))
			ctlerror("%q: selectcolor index: %s", e->name, cp->str);
		_setctlimage(e, &e->sel[i].color, cp->args[2]);
		e->sel[i].state |= Enabled;
		break;
	case EShow:
		_ctlargcount(e, cp, 1);
		if (e->dirty && e->lredraw == nil)
			editshow(e);
		else{
			layoutrect(e);
			layoutselections(e);
			showticks(e);
			flushimage(display, 1);
		}
		break;
	case ESize:
		if (cp->nargs == 3)
			r.max = Pt(10000, 10000);
		else{
			_ctlargcount(e, cp, 5);
			r.max.x = cp->iargs[3];
			r.max.y = cp->iargs[4];
		}
		r.min.x = cp->iargs[1];
		r.min.y = cp->iargs[2];
		if(r.min.x<=0 || r.min.y<=0 || r.max.x<=0 || r.max.y<=0 || r.max.x < r.min.x || r.max.y < r.min.y)
			ctlerror("%q: bad sizes: %s", e->name, cp->str);
		e->size.min = r.min;
		e->size.max = r.max;
		break;
	case ESnarf:
		_ctlargcount(e, cp, 2);
		i = cp->iargs[1];
		if (i < 0 || i >= nelem(e->sel))
			ctlerror("%q: nonexistent selection: %d", e->name, i);
		sel = e->sel + i;
		if ((sel->state & Set) == 0)
			ctlerror("%q: inactive selection: %d", e->name, i);
		if (ss == nil)
			ctlerror("%q: no stringset to paste into", e->name);
		snarf(ss, sel->mpl.pos, sel->mpr.pos);
		break;
	case ESpacing:
		_ctlargcount(e, cp, 2);
		if(cp->iargs[1] < 0)
			ctlerror("%q: bad spacing: %c", e->name, cp->str);
		e->spacing = cp->iargs[1];
		break;
	case ESplit:
		_ctlargcount(e, cp, 3);
		p1.str = cp->iargs[1];
		p1.rn = cp->iargs[2];
		if ((s = stringof(ss, p1)) == nil)
			ctlerror("%q: no such string: %d", e->name, p1.str);
		if (p1.rn <= 0 || p1.rn >= s->n)
			ctlerror("%q: split outside string: %d/%d", e->name, p1.str, p1.rn);
		hideticks(e);
		split(ss, p1);
		layoutrect(e);
		layoutselections(e);
		showticks(e);
		flushimage(display, 1);
		break;
	case EStringset:
		_ctlargcount(e, cp, 2);
		ss = stringsetnamed(cp->args[1]);
		if (ss == nil)
			ctlerror("%q: no such stringset: %s", e->name, cp->args[1]);
		e->ss = ss;
		ss->c = (Control*)e;
		qlock(ss);
		e->dirty = 1;
		break;
	case ETextcolor:
		_ctlargcount(e, cp, 2);
		_setctlimage(e, &e->textcolor, cp->args[1]);
		break;
	case ETop:
		_ctlargcount(e, cp, 3);
		if (ss == nil)
			break;
		p1.str = cp->iargs[1];
		p1.rn = cp->iargs[2];
		if (_stringfindrrune(ss, &p1, '\n'))
			p1.rn++;
		debugprint(DBGLAYOUT, "top command %d/%d\n", p1.str, p1.rn);
		scroll(e, p1);
		break;
	case EUndo:
		_ctlargcount(e, cp, 1);
		if (ss == nil)
			break;
		hideticks(e);
		undo(ss);
		layoutrect(e);
		layoutselections(e);
		showticks(e);
		flushimage(display, 1);
		break;
	case EValue:
		_ctlargcount(e, cp, 3);
		n = cp->iargs[1];
		s = ctlmalloc(sizeof(String));
		s->context = strdup(e->curcontext->name);
		s->r = _ctlrunestr(cp->args[2]);
		s->n = runestrlen(ss->string[n]->r);
		s->ref = 1;
		debugprint(DBGSTRING, "allocate string: 0x%p %s [%d]", ss->string[n], e->curcontext->name, ss->string[n]->n);
		if (ss == nil){
			ss = newstringset("default");
			ss->c = (Control*)e;
			qlock(ss);
			e->ss = ss;
		}
		_stringspace(ss, n+1);
		if (ss->string[n]){
			p1.str = p2.str = n;
			p1.rn = 0;
			p2.rn = ss->string[n]->n;
			layoutprep(e, p1, p2);
			freestring(ss->string[n]);
		}
		ss->string[n] = s;
		if (ss->nstring <= n)
			ss->nstring = n+1;
		e->dirty = 1;
		layoutrect(e);
		layoutselections(e);
		showticks(e);
		flushimage(display, 1);
		break;
	}
	if (e->ss)
		qunlock(e->ss);
}

Control*
createedit(Controlset *cs, char *name)
{
	Edit *e;
	int i, thistab;
	Selection *sel;
	Context *c;

	e = (Edit*)_createctl(cs, "edit", sizeof(Edit), name);
	e->border = 0;
	e->top.str = 0;
	e->top.rn = 0;
	e->bot.str = 0;
	e->bot.rn = 0;
	e->dirty = 0;
	e->scroll = 0;
	e->spacing = 0;
	e->lastbut = 0;
	e->image = _getctlimage("white");
	e->textcolor = _getctlimage("black");
	e->bordercolor = _getctlimage("black");
	for (i = 0; i < nelem(e->sel); i++){
		e->sel[i].state = 0;
		e->sel[i].color = nil;
	}
	sel = e->sel;
	sel->color = _getctlimage("paleyellow");
	sel->state = Enabled|Set;
	sel->mpl.pos.str = 0;
	sel->mpl.pos.rn = 0;
	sel->mpr.pos.str = 0;
	sel->mpr.pos.rn = 0;
	e->l = nil;
	e->ss = nil;
	e->mouse = editmouse;
	e->key = editkey;
	e->ctl = editctl;
	e->exit = editfree;
	if ((c = _contextnamed("default")) == nil){
		c = _newcontext("default");
		_setctlfont(e, &c->f, "font");
		c->wordbreak = 0;
		thistab = 0;
		for (i = 0; i < nelem(c->tabs); i++){
			thistab += stringwidth(c->f->font, "0000");
			c->tabs[i] = thistab;
		}
	}
	e->curcontext = c;
	frinittick(e);
	return (Control *)e;
}
