/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <mouse.h>
#include <keyboard.h>
#include <control.h>

typedef struct Menu0 Menu0;	/* Menu is taken by mouse.h */

struct Menu0
{
	Control Control;
	CImage	*image;
	CImage	*bordercolor;
	CImage	*textcolor;
	CImage	*selectcolor;
	CImage	*selecttextcolor;
	CFont	*font;
	char		**line;
	int		nline;
	int		border;
	int		align;
	Image	*window;
	int		visible;	/* state of menu */
	int		selection;		/* currently selected line; -1 == none */
	int		prevsel;	/* previous selection */
	int		lastbut;	/* previous state of mouse button */
};

enum{
	EAdd,
	EAlign,
	EBorder,
	EBordercolor,
	EFocus,
	EFont,
	EFormat,
	EHide,
	EImage,
	ERect,
	EReveal,
	ESelectcolor,
	ESelecttextcolor,
	EShow,
	ESize,
	ETextcolor,
	EWindow,
};

static char *cmds[] = {
	[EAdd] = 			"add",
	[EAlign] = 			"align",
	[EBorder] = 		"border",
	[EBordercolor] =	"bordercolor",
	[EFocus] = 		"focus",
	[EFont] =			"font",
	[EFormat] = 		"format",
	[EHide] =			"hide",
	[EImage] =		"image",
	[ERect] =			"rect",
	[EReveal] =		"reveal",
	[ESelectcolor] =	"selectcolor",
	[ESelecttextcolor] =	"selecttextcolor",
	[EShow] =			"show",
	[ESize] =			"size",
	[ETextcolor] =		"textcolor",
	[EWindow] =		"window",
	nil
};

static void	menushow(Menu0*);
static void menuhide(Menu0*);

static void
menufree(Control *c)
{
	Menu0 *m;

	m = (Menu0*)c;
	_putctlfont(m->font);
	_putctlimage(m->image);
	_putctlimage(m->textcolor);
	_putctlimage(m->bordercolor);
	_putctlimage(m->selectcolor);
	_putctlimage(m->selecttextcolor);
}

static void
menushow(Menu0 *m)
{
	Rectangle r, clipr;
	int i, dx, dy, w;
	Font *f;
	Point p, q;
	Image *im, *c;

	if(m->Control.hidden || m->window == nil)
		return;

	m->visible = 1;
	f = m->font->font;
	draw(m->window, m->Control.rect, m->image->image, nil, m->image->image->r.min);
	if(m->border > 0)
		border(m->window, m->Control.rect, m->border, m->bordercolor->image, ZP);
	/* text goes here */
	dx = 0;
	for(i=0; i<m->nline; i++){
		w = stringwidth(f, m->line[i]);
		if(dx < w)
			dx = w;
	}
	dy = m->nline*f->height;
	clipr = insetrect(m->Control.rect, m->border);
	p = _ctlalignpoint(clipr, dx, dy, m->align);
	im = m->textcolor->image;
//	if(m->pressed)
//		im = m->pressedtextcolor->image;
	for(i=0; i<m->nline; i++){
		r.min = p;
		r.max.x = p.x+dx;
		r.max.y = p.y+f->height;
		c = im;
		if(i == m->selection){
			draw(m->window, r, m->selectcolor->image, nil, ZP);
			c = m->selecttextcolor->image;
		}
		q = _ctlalignpoint(r, stringwidth(f, m->line[i]), f->height, m->align%3);
		_string(m->window, q, c,
			ZP, f, m->line[i], nil, strlen(m->line[i]),
			clipr, nil, ZP, SoverD);
		p.y += f->height;
	}
//	if(m->pressed)
//		draw(m->Control.screen, m->Control.rect, m->lighm->image, m->mask->image, m->mask->image->r.min);
	flushimage(display, 1);
}

static Point
menusize(Menu0 *m)
{
	int x, y;
	int i;
	Point p;
	Font *f;

	x = 0;
	y = 0;
	f = m->font->font;
	for(i=0; i<m->nline; i++){
		p = stringsize(f, m->line[i]);
		if(p.x > x)
			x = p.x;
		y += f->height;
	}

	return Pt(x+2*m->border, y+2*m->border);
}

static void
menuhide(Menu0 *m)
{
	freeimage(m->window);
	m->window = nil;
	m->Control.rect.max.y = m->Control.rect.min.y;	/* go to zero size */
	m->lastbut = 0;
	m->visible = 0;
	if(m->selection >= 0)
		m->prevsel = m->selection;
	m->selection = -1;
	_ctlfocus(m, 0);
}

static void
menutrack(Control *c, Mouse *ms)
{
	Rectangle r;
	int s;
	Menu0 *m;

	m = (Menu0*)c;
	if(m->window == nil)
		return;
	if(m->lastbut && ms->buttons==0){	/* menu was released */
		chanprint(m->Control.event, "%q: value %d", m->Control.name, m->selection);
		menuhide(m);
		return;
	}
	m->lastbut = ms->buttons;
	r = insetrect(m->Control.rect, m->border);
	if(!ptinrect(ms->xy, r))
		s = -1;
	else{
		s = (ms->xy.y - r.min.y)/m->font->font->height;
		if(s < 0 || s >= m->nline)
			s = -1;
	}
	if(m->visible== 0 || s!=m->selection){
		m->selection = s;
		menushow(m);
	}
}

static void
menuctl(Control *c, CParse *cp)
{
	int up, cmd, h;
	Rectangle r;
	Menu0 *m;
	Point diag;

	m = (Menu0*)c;
	cmd = _ctllookup(cp->args[0], cmds, nelem(cmds));
	switch(cmd){
	default:
		ctlerror("%q: unrecognized message '%s'", m->Control.name, cp->str);
		break;
	case EAdd:
		_ctlargcount(&m->Control, cp, 2);
		m->line = ctlrealloc(m->line, (m->nline+1)*sizeof(char*));
		m->line[m->nline++] = ctlstrdup(cp->args[1]);
		menushow(m);
		break;
	case EAlign:
		_ctlargcount(&m->Control, cp, 2);
		m->align = _ctlalignment(cp->args[1]);
		menushow(m);
		break;
	case EBorder:
		_ctlargcount(&m->Control, cp, 2);
		m->border = cp->iargs[1];
		menushow(m);
		break;
	case EBordercolor:
		_ctlargcount(&m->Control, cp, 2);
		_setctlimage(_setctlimage(mm->Control, &m->bordercolor, cp->args[1]);
		menushow(m);
		break;
	case EFocus:
		_ctlargcount(&m->Control, cp, 2);
		if(atoi(cp->args[1]) == 0)
			menuhide(m);
		break;
	case EFont:
		_ctlargcount(&m->Control, cp, 2);
		_setctlfont(_setctlfont(_setctlfont(mm->Controlm->Control, &m->font, cp->args[1]);
		break;
	case EFormat:
		_ctlargcount(&m->Control, cp, 2);
		m->Control.format = ctlstrdup(cp->args[1]);
		break;
	case EHide:
		_ctlargcount(&m->Control, cp, 1);
		m->Control.hidden = 1;
		break;
	case EImage:
		_ctlargcount(&m->Control, cp, 2);
		_setctlimage(_setctlimage(mm->Control, &m->image, cp->args[1]);
		menushow(m);
		break;
	case ERect:
		_ctlargcount(&m->Control, cp, 5);
		r.min.x = cp->iargs[1];
		r.min.y = cp->iargs[2];
		r.max.x = cp->iargs[3];
		r.max.y = cp->iargs[4];
		if(Dx(r)<0 || Dy(r)<0)
			ctlerror("%q: bad rectangle: %s", m->Control.name, cp->str);
		m->Control.rect = r;
		menushow(m);
		break;
	case EReveal:
		_ctlargcount(&m->Control, cp, 1);
		m->Control.hidden = 0;
		menushow(m);
		break;
	case ESelectcolor:
		_ctlargcount(&m->Control, cp, 2);
		_setctlimage(_setctlimage(mm->Control, &m->selectcolor, cp->args[1]);
		menushow(m);
		break;
	case ESelecttextcolor:
		_ctlargcount(&m->Control, cp, 2);
		_setctlimage(_setctlimage(mm->Control, &m->selecttextcolor, cp->args[1]);
		menushow(m);
		break;
	case EShow:
		_ctlargcount(&m->Control, cp, 1);
		menushow(m);
		break;
	case ESize:
		if (cp->nargs == 3)
			r.max = Pt(0x7fffffff, 0x7fffffff);
		else{
			_ctlargcount(&m->Control, cp, 5);
			r.max.x = cp->iargs[3];
			r.max.y = cp->iargs[4];
		}
		r.min.x = cp->iargs[1];
		r.min.y = cp->iargs[2];
		if(r.min.x<=0 || r.min.y<=0 || r.max.x<=0 || r.max.y<=0 || r.max.x < r.min.x || r.max.y < r.min.y)
			ctlerror("%q: bad sizes: %s", m->Control.name, cp->str);
		m->Control.size.min = r.min;
		m->Control.size.max = r.max;
		break;
	case ETextcolor:
			     _ctlargcount(&m->Control, cp, 2);
			     _setctlimage(_setctlimage(mm->Control, &m->textcolor, cp->args[1]);
		menushow(m);
		break;
	case EWindow:
		/* no args == toggle; otherwise 0 or 1 for state of window */
		if(cp->nargs >= 2)
			up = cp->iargs[1];
		else
			up = (m->window == nil);
		if(!up){	/* take window down */
			if(m->window)
				menuhide(m);
			break;
		}
		if(m->window != nil)
			break;
		diag = menusize(m);
		m->Control.rect.max.x = m->Control.rect.min.x + diag.x;
		m->Control.rect.max.y = m->Control.rect.min.y + diag.y;
		m->window = allocwindow(_screen, m->Control.rect, Refbackup, DWhite);
		if(m->window == nil)
			m->window = m->Control.screen;
		up = m->prevsel;
		if(up<0 || up>=m->nline)
			up = 0;
		m->selection = up;
		menushow(m);
		h = m->font->font->height;
		moveto(m->controlset->mousectl,
			Pt(m->Control.rect.min.x+Dx(m->Control.rect)/2, m->Control.rect.min.y+up*h+h/2));
//		_ctlfocus(m, 1);
		break;
	}
}

Control*
createmenu(Controlset *cs, char *name)
{
	Menu0 *m;

	m = (Menu0*)_createctl(cs, "menu", sizeof(Menu0), name);
	m->font = _getctlfont("font");
	m->image = _getctlimage("white");
	m->textcolor = _getctlimage("black");
	m->selectcolor = _getctlimage("yellow");
	m->selecttextcolor = _getctlimage("black");
	m->bordercolor = _getctlimage("black");
	m->Control.format = ctlstrdup("%q: value %d");
	m->border = 0;
	m->align = Aupperleft;
	m->visible = 0;
	m->window = nil;
	m->lastbut = 0;
	m->selection = -1;
	m->Control.mouse = menutrack;
	m->Control.ctl = menuctl;
	m->Control.exit = menufree;
	return (Control *)m;
}
