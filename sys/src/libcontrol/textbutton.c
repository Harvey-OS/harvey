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

typedef struct Textbutton Textbutton;

struct Textbutton
{
	Control;
	CFont	*font;
	CImage	*image;
	CImage	*mask;
	CImage	*light;
	CImage	*bordercolor;
	CImage	*textcolor;
	CImage	*pressedtextcolor;
	CImage	*paletextcolor;
	int		pressed;
	int		lastbut;
	int		lastshow;
	char		**line;
	int		nline;
	int		align;
	int		border;
	int		off;
	int		showoff;
	int		prepress;
};

enum{
	EAlign,
	EBorder,
	EBordercolor,
	EFocus,
	EFont,
	EFormat,
	EHide,
	EImage,
	ELight,
	EMask,
	EPaletextcolor,
	EPressedtextcolor,
	ERect,
	EReveal,
	EShow,
	ESize,
	EText,
	ETextcolor,
	EValue,
};

static char *cmds[] = {
	[EAlign] =			"align",
	[EBorder] =		"border",
	[EBordercolor] = 	"bordercolor",
	[EFocus] = 		"focus",
	[EFont] =			"font",
	[EFormat] = 		"format",
	[EHide] =			"hide",
	[EImage] =		"image",
	[ELight] =			"light",
	[EMask] =			"mask",
	[EPaletextcolor] ="paletextcolor",
	[EPressedtextcolor] ="pressedtextcolor",
	[ERect] =			"rect",
	[EReveal] =		"reveal",
	[EShow] =			"show",
	[ESize] =			"size",
	[EText] =			"text",
	[ETextcolor] =		"textcolor",
	[EValue] =			"value",
	nil
};

static void	textbuttonshow(Textbutton*);

static void
textbuttonmouse(Control *c, Mouse *m)
{
	Textbutton *t;

	t = (Textbutton *)c;
	if(m->buttons&7) {
		if (ptinrect(m->xy,t->Control.rect)) {
			if (t->off) {
				t->off = 0;
				textbuttonshow(t);
			}
		} else {
			if (!t->off) {
				t->off = 1;
				textbuttonshow(t);
			}
		}
	}
	if((m->buttons&7) != t->lastbut){
		if(m->buttons & 7){
			t->prepress = t->pressed;
			if (t->pressed)
				t->pressed = 0;
			else
				t->pressed = 1;
			textbuttonshow(t);
		}else{	/* generate event on button up */
			if (ptinrect(m->xy,t->Control.rect))
				chanprint(t->Control.event, t->Control.format, t->Control.name, t->pressed);
			else {
				t->off = 0;
				t->pressed = t->prepress;
				textbuttonshow(t);
			}
		}
	}
	t->lastbut = m->buttons & 7;
}

static void
textbuttonfree(Control *c)
{
	int i;
	Textbutton *t;

	t = (Textbutton*)c;
	_putctlfont(t->font);
	_putctlimage(t->image);
	_putctlimage(t->light);
	_putctlimage(t->mask);
	_putctlimage(t->textcolor);
	_putctlimage(t->bordercolor);
	_putctlimage(t->paletextcolor);
	_putctlimage(t->pressedtextcolor);
	for(i=0; i<t->nline; i++)
		free(t->line[i]);
	free(t->line);
}

static void
textbuttonshow(Textbutton *t)
{
	Rectangle r, clipr;
	int i, dx, dy, w;
	Font *f;
	Point p, q;
	Image *im;

	if(t->Control.hidden || (t->lastshow == t->pressed && t->showoff == t->off))
		return;
	f = t->font->font;
	draw(t->Control.screen, t->Control.rect, t->image->image, nil, t->image->image->r.min);
	if(t->border > 0)
		border(t->Control.screen, t->Control.rect, t->border, t->bordercolor->image, ZP);
	/* text goes here */
	dx = 0;
	for(i=0; i<t->nline; i++){
		w = stringwidth(f, t->line[i]);
		if(dx < w)
			dx = w;
	}
	dy = t->nline*f->height;
	clipr = insetrect(t->Control.rect, t->border);
	p = _ctlalignpoint(clipr, dx, dy, t->align);
	im = t->textcolor->image;
	if(t->off)
		im = t->paletextcolor->image;
	else if(t->pressed)
		im = t->pressedtextcolor->image;
	for(i=0; i<t->nline; i++){
		r.min = p;
		r.max.x = p.x+dx;
		r.max.y = p.y+f->height;
		q = _ctlalignpoint(r, stringwidth(f, t->line[i]), f->height, t->align%3);
		_string(t->Control.screen, q, im,
			ZP, f, t->line[i], nil, strlen(t->line[i]),
			clipr, nil, ZP, SoverD);
		p.y += f->height;
	}
	if(t->off || t->pressed)
		draw(t->Control.screen, t->Control.rect, t->light->image, t->mask->image, t->mask->image->r.min);
	t->lastshow = t->pressed;
	t->showoff = t->off;
	flushimage(display, 1);
}

static void
textbuttonctl(Control *c, CParse *cp)
{
	int cmd, i;
	Rectangle r;
	Textbutton *t;

	t = (Textbutton*)c;
	cmd = _ctllookup(cp->args[0], cmds, nelem(cmds));
	switch(cmd){
	default:
		ctlerror("%q: unrecognized message '%s'", t->Control.name, cp->str);
		break;
	case EAlign:
		_ctlargcount(&t->Control, cp, 2);
		t->align = _ctlalignment(cp->args[1]);
		t->lastshow = -1;	/* force redraw */
		break;
	case EBorder:
		_ctlargcount(&t->Control, cp, 2);
		t->border = cp->iargs[1];
		t->lastshow = -1;	/* force redraw */
		break;
	case EBordercolor:
		_ctlargcount(&t->Control, cp, 2);
		_setctlimage(&t->Control, &t->bordercolor, cp->args[1]);
		t->lastshow = -1;	/* force redraw */
		break;
	case EFocus:
		break;
	case EFont:
		_ctlargcount(&t->Control, cp, 2);
		_setctlfont(t, &t->font, cp->args[1]);
		t->lastshow = -1;	/* force redraw */
		break;
	case EFormat:
		_ctlargcount(&t->Control, cp, 2);
		t->Control.format = ctlstrdup(cp->args[1]);
		break;
	case EHide:
		_ctlargcount(&t->Control, cp, 1);
		t->Control.hidden = 1;
		break;
	case EImage:
		_ctlargcount(&t->Control, cp, 2);
		_setctlimage(&t->Control, &t->image, cp->args[1]);
		t->lastshow = -1;	/* force redraw */
		break;
	case ELight:
		_ctlargcount(&t->Control, cp, 2);
		_setctlimage(&t->Control, &t->light, cp->args[1]);
		t->lastshow = -1;	/* force redraw */
		break;
	case EMask:
		_ctlargcount(&t->Control, cp, 2);
		_setctlimage(&t->Control, &t->mask, cp->args[1]);
		t->lastshow = -1;	/* force redraw */
		break;
	case EPaletextcolor:
		_ctlargcount(&t->Control, cp, 2);
		_setctlimage(&t->Control, &t->paletextcolor, cp->args[1]);
		t->lastshow = -1;	/* force redraw */
		break;
	case EPressedtextcolor:
		_ctlargcount(&t->Control, cp, 2);
		_setctlimage(&t->Control, &t->pressedtextcolor, cp->args[1]);
		t->lastshow = -1;	/* force redraw */
		break;
	case ERect:
		_ctlargcount(&t->Control, cp, 5);
		r.min.x = cp->iargs[1];
		r.min.y = cp->iargs[2];
		r.max.x = cp->iargs[3];
		r.max.y = cp->iargs[4];
		if(Dx(r)<=0 || Dy(r)<=0)
			ctlerror("%q: bad rectangle: %s", t->Control.name, cp->str);
		t->Control.rect = r;
		t->lastshow = -1;	/* force redraw */
		break;
	case EReveal:
		_ctlargcount(&t->Control, cp, 1);
		t->Control.hidden = 0;
		t->lastshow = -1;	/* force redraw */
		textbuttonshow(t);
		break;
	case EShow:
		_ctlargcount(&t->Control, cp, 1);
		t->lastshow = -1;	/* force redraw */
		textbuttonshow(t);
		break;
	case ESize:
		if (cp->nargs == 3)
			r.max = Pt(0x7fffffff, 0x7fffffff);
		else{
			_ctlargcount(&t->Control, cp, 5);
			r.max.x = cp->iargs[3];
			r.max.y = cp->iargs[4];
		}
		r.min.x = cp->iargs[1];
		r.min.y = cp->iargs[2];
		if(r.min.x<=0 || r.min.y<=0 || r.max.x<=0 || r.max.y<=0 || r.max.x < r.min.x || r.max.y < r.min.y)
			ctlerror("%q: bad sizes: %s", t->Control.name, cp->str);
		t->Control.size.min = r.min;
		t->Control.size.max = r.max;
		break;
	case EText:
		/* free existing text */
		for(i=0; i<t->nline; i++)
			free(t->line[i]);
		t->nline = cp->nargs-1;
		t->line = ctlrealloc(t->line, t->nline*sizeof(char*));
		for(i=0; i<t->nline; i++)
			t->line[i] = ctlstrdup(cp->args[i+1]);
		t->lastshow = -1;	/* force redraw */
		textbuttonshow(t);
		break;
	case ETextcolor:
		_ctlargcount(&t->Control, cp, 2);
		_setctlimage(&t->Control, &t->textcolor, cp->args[1]);
		t->lastshow = -1;	/* force redraw */
		break;
	case EValue:
		_ctlargcount(&t->Control, cp, 2);
		if((cp->iargs[1]!=0) != t->pressed){
			t->pressed ^= 1;
			textbuttonshow(t);
		}
		break;
	}
}

Control*
createtextbutton(Controlset *cs, char *name)
{
	Textbutton *t;

	t = (Textbutton *)_createctl(cs, "textbutton", sizeof(Textbutton), name);
	t->line = ctlmalloc(sizeof(char*));
	t->nline = 0;
	t->image = _getctlimage("white");
	t->light = _getctlimage("yellow");
	t->mask = _getctlimage("opaque");
	t->bordercolor = _getctlimage("black");
	t->textcolor = _getctlimage("black");
	t->pressedtextcolor = _getctlimage("black");
	t->paletextcolor = _getctlimage("paleyellow");
	t->font = _getctlfont("font");
	t->Control.format = ctlstrdup("%q: value %d");
	t->lastshow = -1;
	t->Control.mouse = textbuttonmouse;
	t->Control.ctl = textbuttonctl;
	t->Control.exit = textbuttonfree;
	t->prepress = 0;
	t->off = 0;
	t->showoff = -1;
	return (Control *)t;
}
