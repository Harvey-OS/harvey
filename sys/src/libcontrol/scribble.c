#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <mouse.h>
#include <keyboard.h>
#include <control.h>
#include <scribble.h>

typedef struct Scrib Scrib;

struct Scrib
{
	Control;
	int		border;
	CImage	*image;
	CImage	*color;
	CImage	*bordercolor;
	CFont	*font;
	int		align;
	int		lastbut;
	char		lastchar[8];
	Scribble	*scrib;
};

enum{
	EAlign,
	EBorder,
	EBordercolor,
	EFocus,
	EFont,
	EHide,
	EImage,
	ELinecolor,
	ERect,
	EReveal,
	EShow,
	ESize,
};

static char *cmds[] = {
	[EAlign] =		"align",
	[EBorder] =	"border",
	[EBordercolor] ="bordercolor",
	[EFocus] = 	"focus",
	[EFont] =		"font",
	[EHide] =		"hide",
	[EImage] =	"image",
	[ELinecolor] =	"linecolor",
	[ERect] =		"rect",
	[EReveal] =	"reveal",
	[EShow] =		"show",
	[ESize] =		"size",
	nil
};

static void	scribshow(Scrib*);
static void scribchar(Scrib*, Rune);

static void	resetstroke(Scrib *w);
static void	displaystroke(Scrib *w);
static void	displaylast(Scrib *w);
static void	addpoint(Scrib *w, Point p);

static void
scribmouse(Control *c, Mouse *m)
{
	Scrib *b;
	Rune r;

	b = (Scrib*)c;
	if (m->buttons & 0x1) {
		if ((b->lastbut & 0x1) == 0) {
			/* mouse went down */
			resetstroke(b);
		}
		/* mouse is down */
		if (ptinrect(m->xy,b->rect))
			addpoint(b, m->xy);
	} else if (b->lastbut & 0x1) {
		/* mouse went up */
		if (ptinrect(m->xy,b->rect)) {
			r = recognize(b->scrib);
			scribchar(b, r);
			scribshow(b);
			if (r) chanprint(b->event, b->format, b->name, r);
		}
	}
	b->lastbut = m->buttons;
}

static void
scribfree(Control *c)
{
	Scrib *b;

	b = (Scrib*)c;
	_putctlimage(b->image);
	_putctlimage(b->color);
	_putctlimage(b->bordercolor);
	_putctlfont(b->font);
//	scribblefree(b->scrib);
}

static void
scribchar(Scrib *b, Rune r)
{
	if(r == 0)
		b->lastchar[0] = '\0';
	else if(r == ' ')
		strcpy(b->lastchar, "' '");
	else if(r < ' ')
		sprint(b->lastchar, "ctl-%c", r+'@');
	else
		sprint(b->lastchar, "%C", r);
}


static void
scribshow(Scrib *b)
{
	Image *i;
	Rectangle r;
	char *mode;
	Scribble *s = b->scrib;
	char buf[32];

	if (b->hidden)
		return;
	if(b->border > 0){
		r = insetrect(b->rect, b->border);
		border(b->screen, b->rect, b->border, b->bordercolor->image, ZP);
	}else
		r = b->rect;

	i = b->image->image;
	draw(b->screen, r, i, nil, i->r.min);

	if (s->ctrlShift)
		mode = " ^C";
	else if (s->puncShift)
		mode = " #&^";
	else if (s->curCharSet == CS_DIGITS)
		mode = " 123";
	else if (s->capsLock)
		mode = " ABC";
	else if (s->tmpShift)
		mode = " Abc";
	else
		mode = " abc";

	snprint(buf, sizeof buf, "%s %s", mode, b->lastchar);

	string(b->screen, r.min, b->color->image, ZP, b->font->font, buf);
	flushimage(display, 1);
}

static void
scribctl(Control *c, CParse *cp)
{
	int cmd;
	Rectangle r;
	Scrib *b;

	b = (Scrib*)c;
	cmd = _ctllookup(cp->args[0], cmds, nelem(cmds));
	switch(cmd){
	default:
		abort();
		ctlerror("%q: unrecognized message '%s'", b->name, cp->str);
		break;
	case EAlign:
		_ctlargcount(b, cp, 2);
		b->align = _ctlalignment(cp->args[1]);
		break;
	case EBorder:
		_ctlargcount(b, cp, 2);
		if(cp->iargs[1] < 0)
			ctlerror("%q: bad border: %c", b->name, cp->str);
		b->border = cp->iargs[1];
		break;
	case EBordercolor:
		_ctlargcount(b, cp, 2);
		_setctlimage(b, &b->bordercolor, cp->args[1]);
		break;
	case EFocus:
		break;
	case EImage:
		_ctlargcount(b, cp, 2);
		_setctlimage(b, &b->image, cp->args[1]);
		break;
	case ELinecolor:
		_ctlargcount(b, cp, 2);
		_setctlimage(b, &b->bordercolor, cp->args[1]);
		break;
	case ERect:
		_ctlargcount(b, cp, 5);
		r.min.x = cp->iargs[1];
		r.min.y = cp->iargs[2];
		r.max.x = cp->iargs[3];
		r.max.y = cp->iargs[4];
		if(Dx(r)<0 || Dy(r)<0)
			ctlerror("%q: bad rectangle: %s", b->name, cp->str);
		b->rect = r;
		break;
	case EReveal:
		_ctlargcount(b, cp, 1);
		b->hidden = 0;
		scribshow(b);
		break;
	case EShow:
		_ctlargcount(b, cp, 1);
		scribshow(b);
		break;
	case EFont:
		_ctlargcount(b, cp, 2);
		_setctlfont(b, &b->font, cp->args[1]);
		break;
	case EHide:
		_ctlargcount(b, cp, 1);
		b->hidden = 1;
		break;
	case ESize:
		if (cp->nargs == 3)
			r.max = Pt(0x7fffffff, 0x7fffffff);
		else{
			_ctlargcount(b, cp, 5);
			r.max.x = cp->iargs[3];
			r.max.y = cp->iargs[4];
		}
		r.min.x = cp->iargs[1];
		r.min.y = cp->iargs[2];
		if(r.min.x<=0 || r.min.y<=0 || r.max.x<=0 || r.max.y<=0 || r.max.x < r.min.x || r.max.y < r.min.y)
			ctlerror("%q: bad sizes: %s", b->name, cp->str);
		b->size.min = r.min;
		b->size.max = r.max;
		break;
	}
}

static void
resetstroke(Scrib *w)
{
	Scribble *s = w->scrib;

	s->ps.npts = 0;
	scribshow(w);
}

static void
displaystroke(Scrib *b)
{
	Scribble *s = b->scrib;

	poly(b->screen, s->pt, s->ps.npts, Endsquare, Endsquare, 0, b->color->image, ZP);
	flushimage(display, 1);
}

static void
displaylast(Scrib *w)
{
	int	    npt;
	Scribble *s = w->scrib;

	npt = s->ps.npts;
	if (npt > 2)
		npt = 2;
	poly(w->screen, s->pt + (s->ps.npts - npt), npt, Endsquare, Endsquare,
		0, w->color->image, ZP);
	flushimage(display, 1);
}

static void
addpoint(Scrib *w, Point p)
{
	pen_point	*ppa;
	Point	*pt;
	int		ppasize;
	Scribble	*s = w->scrib;

	if (s->ps.npts == s->ppasize) {
		ppasize = s->ppasize + 100;
		ppa = malloc ((sizeof (pen_point) + sizeof (Point)) * ppasize);
		if (!ppa)
			return;
		pt = (Point *) (ppa + ppasize);
		memmove(ppa, s->ps.pts, s->ppasize * sizeof (pen_point));
		memmove(pt, s->pt, s->ppasize * sizeof (Point));
		free (s->ps.pts);
		s->ps.pts = ppa;
		s->pt = pt;
		s->ppasize = ppasize;
	}
	ppa = &s->ps.pts[s->ps.npts];
	ppa->Point = p;

	pt = &s->pt[s->ps.npts];
	*pt = p;

	s->ps.npts++;
	
	displaylast(w);
}

Control*
createscribble(Controlset *cs, char *name)
{
	Scrib *b;

	b = (Scrib*)_createctl(cs, "scribble", sizeof(Scrib), name);
	b->image = _getctlimage("white");
	b->color = _getctlimage("black");
	b->bordercolor = _getctlimage("black");
	b->align = Aupperleft;
	b->format = ctlstrdup("%q: value 0x%x");
	b->font = _getctlfont("font");
	b->scrib = scribblealloc();
	b->lastbut = 0;
	b->bordercolor = _getctlimage("black");
	b->border = 0;
	b->ctl = scribctl;
	b->mouse = scribmouse;
	b->exit = scribfree;
	return (Control*)b;
}
