#include <u.h>
#include <libc.h>
#include <thread.h>
#include <draw.h>
#include <mouse.h>
#include <keyboard.h>
#include <control.h>
#include "group.h"

static int debug = 0;
static int debugm = 0;
static int debugr = 0;

enum{
	EAdd,
	EBorder,
	EBordercolor,
	EFocus,
	EHide,
	EImage,
	ERect,
	ERemove,
	EReveal,
	ESeparation,
	EShow,
	ESize,
};

static char *cmds[] = {
	[EAdd] =			"add",
	[EBorder] =		"border",
	[EBordercolor] =	"bordercolor",
	[EFocus] = 		"focus",
	[EHide] =			"hide",
	[EImage] =		"image",
	[ERect] =			"rect",
	[ERemove] =		"remove",
	[EReveal] =		"reveal",
	[ESeparation] =		"separation",
	[EShow] =			"show",
	[ESize] =			"size",
};

static void		boxboxresize(Group*, Rectangle);
static void		columnresize(Group*, Rectangle);
static void		groupctl(Control *c, CParse *cp);
static void		groupfree(Control*);
static void		groupmouse(Control *, Mouse *);
static void		groupsize(Control *c);
static void		removegroup(Group*, int);
static void		rowresize(Group*, Rectangle);
static void		stackresize(Group*, Rectangle);

static void
groupinit(Group *g)
{
	g->bordercolor = _getctlimage("black");
	g->image = _getctlimage("white");
	g->border = 0;
	g->mansize = 0;
	g->separation = 0;
	g->selected = -1;
	g->lastkid = -1;
	g->kids = nil;
	g->separators = nil;
	g->nkids = 0;
	g->nseparators = 0;
	g->ctl = groupctl;
	g->mouse = groupmouse;
	g->exit = groupfree;
}

static void
groupctl(Control *c, CParse *cp)
{
	int cmd, i, n;
	
	Rectangle r;
	Group *g;

	g = (Group*)c;
	cmd = _ctllookup(cp->args[0], cmds, nelem(cmds));
	switch(cmd){
	case EAdd:
		for (i = 1; i < cp->nargs; i++){
			c = controlcalled(cp->args[i]);
			if (c == nil)
				ctlerror("%q: no such control: %s", g->name, cp->args[i]);
			_ctladdgroup(g, c);
		}
		if (g->setsize)
			g->setsize((Control*)g);
		break;
	case EBorder:
		_ctlargcount(g, cp, 2);
		if(cp->iargs[1] < 0)
			ctlerror("%q: bad border: %c", g->name, cp->str);
		g->border = cp->iargs[1];
		break;
	case EBordercolor:
		_ctlargcount(g, cp, 2);
		_setctlimage(g, &g->bordercolor, cp->args[1]);
		break;
	case EFocus:
		/* ignore focus change */
		break;
	case EHide:
		_ctlargcount(g, cp, 1);
		for (i = 0; i < g->nkids; i++)
			if (g->kids[i]->ctl)
				_ctlprint(g->kids[i], "hide");
		g->hidden = 1;
		break;
	case EImage:
		_ctlargcount(g, cp, 2);
		_setctlimage(g, &g->image, cp->args[1]);
		break;
	case ERect:
		_ctlargcount(g, cp, 5);
		r.min.x = cp->iargs[1];
		r.min.y = cp->iargs[2];
		r.max.x = cp->iargs[3];
		r.max.y = cp->iargs[4];
		if(Dx(r)<=0 || Dy(r)<=0)
			ctlerror("%q: bad rectangle: %s", g->name, cp->str);
		g->rect = r;
		r = insetrect(r, g->border);
		if (g->nkids == 0)
			return;
		switch(g->type){
		case Ctlboxbox:
			boxboxresize(g, r);
			break;
		case Ctlcolumn:
			columnresize(g, r);
			break;
		case Ctlrow:
			rowresize(g, r);
			break;
		case Ctlstack:
			stackresize(g, r);
			break;
		}
		break;
	case ERemove:
		_ctlargcount(g, cp, 2);
		for (n = 0; n < g->nkids; n++)
			if (strcmp(cp->args[1], g->kids[n]->name) == 0)
				break;
		if (n == g->nkids)
			ctlerror("%s: remove nonexistent control: %q", g->name, cp->args[1]);
		removegroup(g, n);
		if (g->setsize)
			g->setsize((Control*)g);
		break;
	case EReveal:
		g->hidden = 0;
		if (debugr) fprint(2, "reveal %s\n", g->name);
		if (g->type == Ctlstack){
			if (cp->nargs == 2){
				if (cp->iargs[1] < 0 || cp->iargs[1] >= g->nkids)
					ctlerror("%s: control out of range: %q", g->name, cp->str);
				g->selected = cp->iargs[1];
			}else
				_ctlargcount(g, cp, 1);
			for (i = 0; i < g->nkids; i++)
				if (g->kids[i]->ctl){
					if (g->selected == i){
						if (debugr) fprint(2, "reveal %s: reveal kid %s\n", g->name, g->kids[i]->name);
						_ctlprint(g->kids[i], "reveal");
					}else{
						if (debugr) fprint(2, "reveal %s: hide kid %s\n", g->name, g->kids[i]->name);
						_ctlprint(g->kids[i], "hide");
					}
				}
			break;
		}
		_ctlargcount(g, cp, 1);
		if (debug) fprint(2, "reveal %s: border %R/%d\n", g->name, g->rect, g->border);
		border(g->screen, g->rect, g->border, g->bordercolor->image, g->bordercolor->image->r.min);
		r = insetrect(g->rect, g->border);
		if (debug) fprint(2, "reveal %s: draw %R\n", g->name, r);
		draw(g->screen, r, g->image->image, nil, g->image->image->r.min);
		for (i = 0; i < g->nkids; i++)
			if (g->kids[i]->ctl)
				_ctlprint(g->kids[i], "reveal");
		break;
	case EShow:
		_ctlargcount(g, cp, 1);
		if (g->hidden)
			break;
		// pass it on to the kiddies
		if (debug) fprint(2, "show %s: border %R/%d\n", g->name, g->rect, g->border);
		border(g->screen, g->rect, g->border, g->bordercolor->image, g->bordercolor->image->r.min);
		r = insetrect(g->rect, g->border);
		if (debug) fprint(2, "show %s: draw %R\n", g->name, r);
		draw(g->screen, r, g->image->image, nil, g->image->image->r.min);
		for (i = 0; i < g->nkids; i++)
			if (g->kids[i]->ctl){
				if (debug) fprint(2, "show %s: kid %s: %q\n", g->name, g->kids[i]->name, cp->str);
				_ctlprint(g->kids[i], "show");
			}
		flushimage(display, 1);
		break;
	case ESize:
		r.max = Pt(_Ctlmaxsize, _Ctlmaxsize);
		if (g->type == Ctlboxbox)
			_ctlargcount(g, cp, 5);
		switch(cp->nargs){
		default:
			ctlerror("%s: args of %q", g->name, cp->str);
		case 1:
			/* recursively set size */
			g->mansize = 0;
			if (g->setsize)
				g->setsize((Control*)g);
			break;
		case 5:
			_ctlargcount(g, cp, 5);
			r.max.x = cp->iargs[3];
			r.max.y = cp->iargs[4];
			/* fall through */
		case 3:
			r.min.x = cp->iargs[1];
			r.min.y = cp->iargs[2];
			if(r.min.x<=0 || r.min.y<=0 || r.max.x<=0 || r.max.y<=0 || r.max.x < r.min.x || r.max.y < r.min.y)
			ctlerror("%q: bad sizes: %s", g->name, cp->str);
			g->size = r;
			g->mansize = 1;
			break;
		}
		break;
	case ESeparation:
		if (g->type != Ctlstack){
			_ctlargcount(g, cp, 2);
			if(cp->iargs[1] < 0)
				ctlerror("%q: illegal value: %c", g->name, cp->str);
			g->separation = cp->iargs[1];
			break;
		}
		// fall through for Ctlstack
	default:
		ctlerror("%q: unrecognized message '%s'", g->name, cp->str);
		break;
	}
}

static void
groupfree(Control *c)
{
	Group *g;

	g = (Group*)c;
	_putctlimage(g->bordercolor);
	free(g->kids);
}

static void
groupmouse(Control *c, Mouse *m)
{
	Group *g;
	int i, lastkid;

	g = (Group*)c;
	if (g->type == Ctlstack){
		i = g->selected;
		if (i >= 0 && g->kids[i]->mouse &&
                        ( ( ((m->buttons == 0) || (g->lastbut == 0)) &&
                           ptinrect(m->xy, g->kids[i]->rect) ) ||
                         ( ((m->buttons != 0) || (g->lastbut != 0)) &&
		         (g->lastkid == i) ) ) ) {
			if (debugm) fprint(2, "groupmouse %s mouse kid %s i=%d lastkid=%d buttons=%d lastbut=%d inrect=%d\n",
						g->name, g->kids[i]->name, i, g->lastkid, m->buttons, g->lastbut,
						ptinrect(m->xy, g->kids[i]->rect) ? 1 : 0);
			(g->kids[i]->mouse)(g->kids[i], m);
			g->lastkid = i;
			g->lastbut = m->buttons;
		} else {
			if (debugm) fprint(2, "groupmouse %s skip kid %s i=%d lastkid=%d buttons=%d lastbut=%d inrect=%d\n",
						g->name, g->kids[i]->name, i, g->lastkid, m->buttons, g->lastbut,
						ptinrect(m->xy, g->kids[i]->rect) ? 1 : 0);
		}
		return;
	}

	lastkid = -1;
	for(i=0; i<g->nkids; i++) {
		if(g->kids[i]->mouse &&
                      ( ( ((m->buttons == 0) || (g->lastbut == 0)) &&
                           ptinrect(m->xy, g->kids[i]->rect) ) ||
                        ( ((m->buttons != 0) || (g->lastbut != 0)) &&
		         (g->lastkid == i) ) ) ) {
			if (debugm) fprint(2, "groupmouse %s mouse kid %s i=%d lastkid=%d buttons=%d lastbut=%d inrect=%d\n",
						g->name, g->kids[i]->name, i, g->lastkid, m->buttons, g->lastbut,
						ptinrect(m->xy, g->kids[i]->rect) ? 1 : 0);
			(g->kids[i]->mouse)(g->kids[i], m);
			lastkid = i;
		} else {
			if (debugm) fprint(2, "groupmouse %s skip kid %s i=%d lastkid=%d buttons=%d lastbut=%d inrect=%d\n",
						g->name, g->kids[i]->name, i, g->lastkid, m->buttons, g->lastbut,
						ptinrect(m->xy, g->kids[i]->rect) ? 1 : 0);
		}
	}
	g->lastkid = lastkid;
	g->lastbut = m->buttons;

#ifdef notdef
	if(m->buttons == 0){
		/* buttons now up */
		g->lastbut = 0;
		return;
	}
	if(g->lastbut == 0 && m->buttons != 0){
		/* button went down, start tracking border */
		switch(g->stacking){
		default:
			return;
		case Vertical:
			p = Pt(m->xy.x, middle_of_border.y);
			p0 = Pt(g->r.min.x, m->xy.y);
			p1 = Pt(g->r.max.x, m->xy.y);
			break;
		case Horizontal:
			p = Pt(middle_of_border.x, m->xy.y);
			p0 = Pt(m->xy.x, g->r.min.y);
			p1 = Pt(m->xy.x, g->r.max.y);
			break;
		}
	//	setcursor();
		oi = nil;
	} else if (g->lastbut != 0 && s->m.buttons != 0){
		/* button is down, keep tracking border */
		if(!eqpt(s->m.xy, p)){
			p = onscreen(s->m.xy);
			r = canonrect(Rpt(p0, p));
			if(Dx(r)>5 && Dy(r)>5){
				i = allocwindow(wscreen, r, Refnone, 0xEEEEEEFF); /* grey */
				freeimage(oi);
				if(i == nil)
					goto Rescue;
				oi = i;
				border(i, r, Selborder, red, ZP);
				flushimage(display, 1);
			}
		}
	} else if (g->lastbut != 0 && s->m.buttons == 0){
		/* button went up, resize kiddies */
	}
	g->lastbut = s->m.buttons;
#endif
}

static void
activategroup(Control *c, int act)
{
	int i;
	Group *g;

	g = (Group*)c;
	for (i = 0; i < g->nkids; i++)
		if (act)
			activate(g->kids[i]);
		else
			deactivate(g->kids[i]);
}

Control *
createrow(Controlset *cs, char *name)
{
	Control *c;
	c = _createctl(cs, "row", sizeof(Group), name);
	groupinit((Group*)c);
	c->setsize = groupsize;
	c->activate = activategroup;
	return c;
}

Control *
createcolumn(Controlset *cs, char *name)
{
	Control *c;
	c = _createctl(cs, "column", sizeof(Group), name);
	groupinit((Group*)c);
	c->setsize = groupsize;
	c->activate = activategroup;
	return c;
}

Control *
createboxbox(Controlset *cs, char *name)
{
	Control *c;
	c = _createctl(cs, "boxbox", sizeof(Group), name);
	groupinit((Group*)c);
	c->activate = activategroup;
	return c;
}

Control *
createstack(Controlset *cs, char *name)
{
	Control *c;
	c = _createctl(cs, "stack", sizeof(Group), name);
	groupinit((Group*)c);
	c->setsize = groupsize;
	return c;
}

void
_ctladdgroup(Control *c, Control *q)
{
	Group *g = (Group*)c;

	g->kids = ctlrealloc(g->kids, sizeof(Group*)*(g->nkids+1));
	g->kids[g->nkids++] = q;
}

static void
removegroup(Group *g, int n)
{
	int i;

	if (g->selected == n)
		g->selected = -1;
	else if (g->selected > n)
		g->selected--;

	for (i = n+1; i < g->nkids; i++)
		g->kids[i-1] = g->kids[i];
	g->nkids--;
}

static void
groupsize(Control *c)
{
	Rectangle r;
	int i;
	Control *q;
	Group *g;

	g = (Group*)c;
	assert(g->type == Ctlcolumn || g->type == Ctlrow || g->type == Ctlstack);
	if (g->mansize) return;
	r = Rect(1, 1, 1, 1);
	if (debug) fprint(2, "groupsize %q\n", g->name);
	for (i = 0; i < g->nkids; i++){
		q = g->kids[i];
		if (q->setsize)
			q->setsize(q);
		if (q->size.min.x == 0 || q->size.min.y == 0 || q->size.max.x == 0 || q->size.max.y == 0)
			ctlerror("%q: bad size %R", q->name, q->size);
		if (debug) fprint(2, "groupsize %q: [%d %q]: %R\n", g->name, i, q->name, q->size);
		switch(g->type){
		case Ctlrow:
			if (i)
				r.min.x += q->size.min.x + g->border;
			else
				r.min.x = q->size.min.x;
			if (i)
				r.max.x += q->size.max.x + g->border;
			else
				r.max.x = q->size.max.x;
			if (r.min.y < q->size.min.y) r.min.y = q->size.min.y;
			if (r.max.y < q->size.max.y) r.max.y = q->size.max.y;
			break;
		case Ctlcolumn:
			if (r.min.x < q->size.min.x) r.min.x = q->size.min.x;
			if (r.max.x < q->size.max.x) r.max.x = q->size.max.x;
			if (i)
				r.min.y += q->size.min.y + g->border;
			else
				r.min.y = q->size.min.y;
			if (i)
				r.max.y += q->size.max.y + g->border;
			else
				r.max.y = q->size.max.y;
			break;
		case Ctlstack:
			if (r.min.x < q->size.min.x) r.min.x = q->size.min.x;
			if (r.max.x < q->size.max.x) r.max.x = q->size.max.x;
			if (r.min.y < q->size.min.y) r.min.y = q->size.min.y;
			if (r.max.y < q->size.max.y) r.max.y = q->size.max.y;
			break;
		}
	}
	g->size = rectaddpt(r, Pt(g->border, g->border));
	if (debug) fprint(2, "groupsize %q: %R\n", g->name, g->size);
}

static void
boxboxresize(Group *g, Rectangle r)
{
	int rows, cols, ht, wid, i, hpad, wpad;
	Rectangle rr;

	if(debug) fprint(2, "boxboxresize %q %R (%d×%d) min/max %R separation %d\n", g->name, r, Dx(r), Dy(r), g->size, g->separation);
	ht = 0;
	for(i=0; i<g->nkids; i++){
		if (g->kids[i]->size.min.y > ht)
			ht = g->kids[i]->size.min.y;
	}
	if (ht == 0)
		ctlerror("boxboxresize: height");
	rows = Dy(r) / (ht+g->separation);
	hpad = (Dy(r) % (ht+g->separation)) / g->nkids;
	cols = (g->nkids+rows-1)/rows;
	wid = Dx(r) / cols - g->separation;
	for(i=0; i<g->nkids; i++){
		if (g->kids[i]->size.max.x < wid)
			wid = g->kids[i]->size.max.x;
	}
	for(i=0; i<g->nkids; i++){
		if (g->kids[i]->size.min.x > wid)
			wid = g->kids[i]->size.min.x;
	}
	if (wid > Dx(r) / cols)
		ctlerror("can't fit controls in boxbox");
	wpad = (Dx(r) % (wid+g->separation)) / g->nkids;
	rr = rectaddpt(Rect(0,0,wid, ht), addpt(r.min, Pt(g->separation/2, g->separation/2)));
	if(debug) fprint(2, "boxboxresize rows %d, cols %d, wid %d, ht %d, wpad %d, hpad %d\n", rows, cols, wid, ht, wpad, hpad);
	for(i=0; i<g->nkids; i++){
		if(debug) fprint(2, "	%d %q: %R (%d×%d)\n", i, g->kids[i]->name, rr, Dx(rr), Dy(rr));
		_ctlprint(g->kids[i], "rect %R",
			rectaddpt(rr, Pt((wpad+wid+g->separation)*(i/rows), (hpad+ht+g->separation)*(i%rows))));
	}
	g->nseparators = rows + cols - 2;
	g->separators = realloc(g->separators, g->nseparators*sizeof(Rectangle));
	rr = r;
	rr.max.y = rr.min.y + g->separation+hpad;
	for (i = 1; i < rows; i++){
		g->separators[i-1] = rectaddpt(rr, Pt(0, (hpad+ht+g->separation)*i-g->separation-hpad));
		if(debug) fprint(2, "row separation %d [%d]: %R\n", i, i-1, rectaddpt(rr, Pt(0, (hpad+ht+g->separation)*i-g->separation)));
	}
	rr = r;
	rr.max.x = rr.min.x + g->separation+wpad;
	for (i = 1; i < cols; i++){
		g->separators[i+rows-2] = rectaddpt(rr, Pt((wpad+wid+g->separation)*i-g->separation-wpad, 0));
		if(debug) fprint(2, "col separation %d [%d]: %R\n", i, i+rows-2, rectaddpt(rr, Pt((wpad+wid+g->separation)*i-g->separation, 0)));
	}
}

static void
columnresize(Group *g, Rectangle r)
{
	int x, y, *d, *p, i, j, t;
	Rectangle rr;
	Control *q;

	x = Dx(r);
	y = Dy(r);
	if(debug) fprint(2, "columnresize %q %R (%d×%d) min/max %R separation %d\n", g->name, r, Dx(r), Dy(r), g->size, g->separation);
	if (x < g->size.min.x) {
		werrstr("resize %s: too narrow: need %d, have %d", g->name, g->size.min.x, x);
		r.max.x = r.min.x + g->size.min.x;
	}
	if (y < g->size.min.y) {
		werrstr("resize %s: too short: need %d, have %d", g->name, g->size.min.y, y);
		r.max.y = r.min.y + g->size.min.y;
		y = Dy(r);
	}
	d = ctlmalloc(g->nkids*sizeof(int));
	p = ctlmalloc(g->nkids*sizeof(int));
	if(debug) fprint(2, "kiddies: ");
	for (i = 0; i < g->nkids; i++) {
		q = g->kids[i];
		if(debug) fprint(2, "[%q]: %d⋯%d\t", q->name, q->size.min.y, q->size.max.y);
		d[i] = q->size.min.y;
		y -= d[i];
		p[i] = q->size.max.y - q->size.min.y;
	}
	if(debug) fprint(2, "\n");
	y -= (g->nkids-1) * g->separation;
	if(y < 0){
		if (debug) fprint(2, "columnresize: y == %d\n", y);
		y = 0;
	}
	if (y >= g->size.max.y - g->size.min.y) {
		// all rects can be maximum width
		for (i = 0; i < g->nkids; i++)
			d[i] += p[i];
		y -= g->size.max.y - g->size.min.y;
	} else {
		// rects can't be max width, divide up the rest
		j = y;
		for (i = 0; i < g->nkids; i++) {
			t = p[i] * y/(g->size.max.y - g->size.min.y);
			d[i] += t;
			j -= t;
		}
		d[0] += j;
		y = 0;
	}
	g->nseparators = g->nkids-1;
	g->separators = realloc(g->separators, g->nseparators*sizeof(Rectangle));
	j = 0;
	rr = r;
	for (i = 0; i < g->nkids; i++) {
		q = g->kids[i];
		if (i < g->nkids - 1){
			g->separators[i].min.x = r.min.x;
			g->separators[i].max.x = r.max.x;
		}
		t = y / (g->nkids - i);
		y -= t;
		j += t/2;
		rr.min.y = r.min.y + j;
		if (i)
			g->separators[i-1].max.y = rr.min.y;
		j += d[i];
		rr.max.y = r.min.y + j;
		if (i < g->nkids - 1)
			g->separators[i].min.y = rr.max.y;
		j += g->separation + t - t/2;
		_ctlprint(q, "rect %R", rr);
		if(debug) fprint(2, "	%d %q: %R (%d×%d)\n", i, q->name, rr, Dx(rr), Dy(rr));
	}
	free(d);
	free(p);
}

static void
rowresize(Group *g, Rectangle r)
{
	int x, y, *d, *p, i, j, t;
	Rectangle rr;
	Control *q;

	x = Dx(r);
	y = Dy(r);
	if(debug) fprint(2, "rowresize %q %R (%d×%d), separation %d\n", g->name, r, Dx(r), Dy(r), g->separation);
	if (x < g->size.min.x) {
		werrstr("resize %s: too narrow: need %d, have %d", g->name, g->size.min.x, x);
		r.max.x = r.min.x + g->size.min.x;
		x = Dx(r);
	}
	if (y < g->size.min.y) {
		werrstr("resize %s: too short: need %d, have %d", g->name, g->size.min.y, y);
		r.max.y = r.min.y + g->size.min.y;
	}
	d = ctlmalloc(g->nkids*sizeof(int));
	p = ctlmalloc(g->nkids*sizeof(int));
	if(debug) fprint(2, "kiddies: ");
	for (i = 0; i < g->nkids; i++) {
		q = g->kids[i];
		if(debug) fprint(2, "[%q]: %d⋯%d\t", q->name, q->size.min.x, q->size.max.x);
		d[i] = q->size.min.x;
		x -= d[i];
		p[i] = q->size.max.x - q->size.min.x;
	}
	if(debug) fprint(2, "\n");
	x -= (g->nkids-1) * g->separation;
	if(x < 0){
		if (debug) fprint(2, "rowresize: x == %d\n", x);
		x = 0;
	}
	if (x >= g->size.max.x - g->size.min.x) {
		if (debug) fprint(2, "max: %d > %d - %d", x, g->size.max.x, g->size.min.x);
		// all rects can be maximum width
		for (i = 0; i < g->nkids; i++)
			d[i] += p[i];
		x -= g->size.max.x - g->size.min.x;
	} else {
		if (debug) fprint(2, "divvie up: %d < %d - %d", x, g->size.max.x, g->size.min.x);
		// rects can't be max width, divide up the rest
		j = x;
		for (i = 0; i < g->nkids; i++) {
			t = p[i] * x/(g->size.max.x - g->size.min.x);
			d[i] += t;
			j -= t;
		}
		d[0] += j;
		x = 0;
	}
	j = 0;
	g->nseparators = g->nkids-1;
	g->separators = realloc(g->separators, g->nseparators*sizeof(Rectangle));
	rr = r;
	for (i = 0; i < g->nkids; i++) {
		q = g->kids[i];
		if (i < g->nkids - 1){
			g->separators[i].min.y = r.min.y;
			g->separators[i].max.y = r.max.y;
		}
		t = x / (g->nkids - i);
		x -= t;
		j += t/2;
		rr.min.x = r.min.x + j;
		if (i)
			g->separators[i-1].max.x = rr.min.x;
		j += d[i];
		rr.max.x = r.min.x + j;
		if (i < g->nkids - 1)
			g->separators[i].min.x = rr.max.x;
		j += g->separation + t - t/2;
		_ctlprint(q, "rect %R", rr);
		if(debug) fprint(2, "	%d %q: %R (%d×%d)\n", i, q->name, rr, Dx(rr), Dy(rr));
	}
	free(d);
	free(p);
}

static void
stackresize(Group *g, Rectangle r)
{
	int x, y, i;
	Control *q;

	x = Dx(r);
	y = Dy(r);
	if(debug) fprint(2, "stackresize %q %R (%d×%d)\n", g->name, r, Dx(r), Dy(r));
	if (x < g->size.min.x){
		werrstr("resize %s: too narrow: need %d, have %d", g->name, g->size.min.x, x);
		return;
	}
	if (y < g->size.min.y){
		werrstr("resize %s: too short: need %d, have %d", g->name, g->size.min.y, y);
		return;
	}
	if (x > g->size.max.x) {
		x = (x - g->size.max.x)/2;
		r.min.x += x;
		r.max.x -= x;
	}
	if (y > g->size.max.y) {
		y = (y - g->size.max.y)/2;
		r.min.y += y;
		r.max.y -= y;
	}
	for (i = 0; i < g->nkids; i++){
		q = g->kids[i];
		if(debug) fprint(2, "	%d %q: %R (%d×%d)\n", i, q->name, r, Dx(r), Dy(r));
	}
	for (i = 0; i < g->nkids; i++){
		q = g->kids[i];
		_ctlprint(q, "rect %R", r);
	}
}
