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

typedef struct Button Button;

struct Button
{
	Control Control;
	CImage	*image;
	CImage	*mask;
	CImage	*light;
	CImage	*pale;
	CImage	*bordercolor;
	int		pressed;
	int		lastbut;
	int		lastshow;
	int		border;
	int		align;
	int		off;
	int		prepress;
};

enum{
	EAlign,
	EBorder,
	EBordercolor,
	EFocus,
	EFormat,
	EHide,
	EImage,
	ELight,
	EMask,
	EPale,
	ERect,
	EReveal,
	EShow,
	ESize,
	EValue,
};

static char *cmds[] = {
	[EAlign] = 		"align",
	[EBorder] = 	"border",
	[EBordercolor] = "bordercolor",
	[EFocus] = 	"focus",
	[EFormat] = 	"format",
	[EHide] =		"hide",
	[EImage] =	"image",
	[ELight] =		"light",
	[EMask] =		"mask",
	[EPale] =		"pale",
	[ERect] =		"rect",
	[EReveal] =	"reveal",
	[EShow] =		"show",
	[ESize] =		"size",
	[EValue] =		"value",
	nil
};

static void
buttonfree(Control *c)
{
	Button *b;

	b = (Button *)c;
	_putctlimage(b->image);
	_putctlimage(b->mask);
	_putctlimage(b->light);
	_putctlimage(b->pale);
	_putctlimage(b->bordercolor);
}

static void
buttonshow(Button *b)
{
	Rectangle r;

	if (b->Control.hidden)
		return;
	r = b->Control.rect;
	if(b->border > 0){
		border(b->Control.screen, r, b->border, b->bordercolor->image, ZP);
		r = insetrect(b->Control.rect, b->border);
	}
	draw(b->Control.screen, r, b->image->image, nil, b->image->image->r.min);
	if(b->off)
		draw(b->Control.screen, r, b->pale->image, b->mask->image, b->mask->image->r.min);
	else if(b->pressed)
		draw(b->Control.screen, r, b->light->image, b->mask->image, b->mask->image->r.min);
	b->lastshow = b->pressed;
	flushimage(display, 1);
}

static void
buttonmouse(Control *c, Mouse *m)
{
	Button *b;

	b = (Button*)c;

	if(m->buttons&7) {
		if (ptinrect(m->xy,b->Control.rect)) {
			if (b->off) {
				b->off = 0;
				buttonshow(b);
			}
		} else {
			if (!b->off) {
				b->off = 1;
				buttonshow(b);
			}
		}
	}
	if((m->buttons&7) != b->lastbut){
		if(m->buttons & 7){
			b->prepress = b->pressed;
			if (b->pressed)
				b->pressed = 0;
			else
				b->pressed = m->buttons & 7;
			buttonshow(b);
		}else	/* generate event on button up */
			if (ptinrect(m->xy,b->Control.rect))
				chanprint(b->Control.event, b->Control.format, b->Control.name, b->pressed);
			else {
				b->off = 0;
				b->pressed = b->prepress;
				buttonshow(b);
			}
	}
	b->lastbut = m->buttons & 7;
}

static void
buttonctl(Control *c, CParse *cp)
{
	int cmd;
	Rectangle r;
	Button *b;

	b = (Button*)c;
	cmd = _ctllookup(cp->args[0], cmds, nelem(cmds));
	switch(cmd){
	default:
		ctlerror("%q: unrecognized message '%s'", b->Control.name, cp->str);
		break;
	case EAlign:
		_ctlargcount(&b->Control, cp, 2);
		b->align = _ctlalignment(cp->args[1]);
		b->lastshow = -1;	/* force redraw */
		break;
	case EBorder:
		_ctlargcount(&b->Control, cp, 2);
		b->border = cp->iargs[1];
		b->lastshow = -1;	/* force redraw */
		break;
	case EBordercolor:
		_ctlargcount(&b->Control, cp, 2);
		_setctlimage(&b->Control, &b->bordercolor, cp->args[1]);
		b->lastshow = -1;	/* force redraw */
		break;
	case EFocus:
		/* ignore focus change */
		break;
	case EFormat:
		_ctlargcount(&b->Control, cp, 2);
		b->Control.format = ctlstrdup(cp->args[1]);
		break;
	case EHide:
		_ctlargcount(&b->Control, cp, 1);
		b->Control.hidden = 1;
		break;
	case EImage:
		_ctlargcount(&b->Control, cp, 2);
		_setctlimage(&b->Control, &b->image, cp->args[1]);
		b->lastshow = -1;	/* force redraw */
		break;
	case ELight:
		_ctlargcount(&b->Control, cp, 2);
		_setctlimage(&b->Control, &b->light, cp->args[1]);
		b->lastshow = -1;	/* force redraw */
		break;
	case EMask:
		_ctlargcount(&b->Control, cp, 2);
		_setctlimage(&b->Control, &b->mask, cp->args[1]);
		b->lastshow = -1;	/* force redraw */
		break;
	case EPale:
		_ctlargcount(&b->Control, cp, 2);
		_setctlimage(&b->Control, &b->pale, cp->args[1]);
		b->lastshow = -1;	/* force redraw */
		break;
	case ERect:
		_ctlargcount(&b->Control, cp, 5);
		r.min.x = cp->iargs[1];
		r.min.y = cp->iargs[2];
		r.max.x = cp->iargs[3];
		r.max.y = cp->iargs[4];
		if(Dx(r)<0 || Dy(r)<0)
			ctlerror("%q: bad rectangle: %s", b->Control.name, cp->str);
		b->Control.rect = r;
		b->lastshow = -1;	/* force redraw */
		break;
	case EReveal:
		_ctlargcount(&b->Control, cp, 1);
		b->Control.hidden = 0;
		buttonshow(b);
		break;
	case EShow:
		_ctlargcount(&b->Control, cp, 1);
		buttonshow(b);
		break;
	case ESize:
		if (cp->nargs == 3)
			r.max = Pt(0x7fffffff, 0x7fffffff);
		else{
			_ctlargcount(&b->Control, cp, 5);
			r.max.x = cp->iargs[3];
			r.max.y = cp->iargs[4];
		}
		r.min.x = cp->iargs[1];
		r.min.y = cp->iargs[2];
		if(r.min.x<=0 || r.min.y<=0 || r.max.x<=0 || r.max.y<=0 || r.max.x < r.min.x || r.max.y < r.min.y)
			ctlerror("%q: bad sizes: %s", b->Control.name, cp->str);
		b->Control.size.min = r.min;
		b->Control.size.max = r.max;
		break;
	case EValue:
		_ctlargcount(&b->Control, cp, 2);
		if((cp->iargs[1]!=0) != b->pressed){
			b->pressed ^= 1;
			buttonshow(b);
		}
		break;
	}
}

Control*
createbutton(Controlset *cs, char *name)
{
	Button *b;
	b = (Button*)_createctl(cs, "button", sizeof(Button), name);
	b->image = _getctlimage("white");
	b->mask = _getctlimage("opaque");
	b->light = _getctlimage("yellow");
	b->pale = _getctlimage("paleyellow");
	b->bordercolor = _getctlimage("black");
	b->Control.format = ctlstrdup("%q: value %d");
	b->lastshow = -1;
	b->border = 0;
	b->align = Aupperleft;
	b->Control.ctl = buttonctl;
	b->Control.mouse = buttonmouse;
	b->key = nil;
	b->Control.exit = buttonfree;
	b->off = 0;
	b->prepress = 0;
	return (Control*)b;
}
