/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* use button 3 for a proper function to the application, that is not for plumber
 *  as default control(2) supposes.
 *  codes are mostly from /sys/src/libcontrol/textbutton.c
 */
#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <mouse.h>
#include <keyboard.h>
#include <control.h>

typedef struct Textbutton3 Textbutton3;

struct Textbutton3
{
	Control Control;
	CFont	*font;
	CImage	*image;
	CImage	*mask;
	CImage	*light;
	CImage	*bordercolor;
	CImage	*textcolor;
	CImage	*pressedtextcolor;
	int		pressed;
	int		lastbut;
	int		lastshow;
	char		**line;
	int		nline;
	int		align;
	int		border;
	int		left;
	int		middle;
	int		right;
	int		toggle;
	int		gettextflg;
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
	EPressedtextcolor,
	ERect,
	EReveal,
	EShow,
	ESize,
	EText,
	ETextcolor,
	EEnable,
	EDisable,
	EToggle,
	EGettext,
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
	[EPressedtextcolor] ="pressedtextcolor",
	[ERect] =			"rect",
	[EReveal] =		"reveal",
	[EShow] =			"show",
	[ESize] =			"size",
	[EText] =			"text",
	[ETextcolor] =		"textcolor",
	[EEnable] =		"enable",
	[EDisable] =		"disable",
	[EToggle] =		"toggle",
	[EGettext] =		"gettext",
	[EValue] =			"value",
	nil
};

static void	textbutton3show(Textbutton3 *);

static void
textbutton3mouse(Control *c, Mouse *m)
{
	Textbutton3 *t;

	t = (Textbutton3 *)c;
	if(t->left == 1) {
		if((m->buttons&1) == 1 && (t->lastbut&1) == 0){
			t->pressed ^= 1;
			textbutton3show(t);
			t->lastbut = m->buttons & 1;
		}else if((m->buttons&1) == 0 && (t->lastbut&1) == 1){
			if(t->gettextflg == 0)
				chanprint(t->Control.event, t->Control.format, t->Control.name, t->pressed, m->xy.x, m->xy.y);
			else
				chanprint(t->Control.event, "%q: value %q", t->Control.name, t->line[0]);
			t->pressed ^= 1;
			textbutton3show(t);
			t->lastbut = m->buttons & 1;
		}
	}
	if(t->middle == 1) {
		if((m->buttons&2) == 2 && (t->lastbut&2) == 0){
			t->pressed ^= 2;
			textbutton3show(t);
			t->lastbut = m->buttons & 2;
		}else if((m->buttons&2) == 0 && (t->lastbut&2) == 2){
			if(t->gettextflg == 0)
				chanprint(t->Control.event, t->Control.format, t->Control.name, t->pressed, m->xy.x, m->xy.y);
			else
				chanprint(t->Control.event, "%q: value %q", t->Control.name, t->line[0]);
			t->pressed ^= 2;
			textbutton3show(t);
			t->lastbut = m->buttons & 2;
		}
	}
	if(t->right == 1) {
		if((m->buttons&4) == 4 && (t->lastbut&4) == 0){
			t->pressed ^= 4;
			textbutton3show(t);
			t->lastbut = m->buttons & 4;
		}else if((m->buttons&4) == 0 && (t->lastbut&4) == 4){
			if(t->gettextflg == 0)
				chanprint(t->Control.event, t->Control.format, t->Control.name, t->pressed, m->xy.x, m->xy.y);
			else
				chanprint(t->Control.event, "%q: value %q", t->Control.name, t->line[0]);
			t->pressed ^= 4;
			textbutton3show(t);
			t->lastbut = m->buttons & 4;
		}
	}
}

static void
textbutton3free(Control *c)
{
	int i;
	Textbutton3 *t;

	t = (Textbutton3*)c;
	_putctlfont(t->font);
	_putctlimage(t->image);
	_putctlimage(t->light);
	_putctlimage(t->mask);
	_putctlimage(t->textcolor);
	_putctlimage(t->bordercolor);
	_putctlimage(t->pressedtextcolor);
	for(i=0; i<t->nline; i++)
		free(t->line[i]);
	free(t->line);
}

static void
textbutton3show(Textbutton3 *t)
{
	Rectangle r, clipr;
	int i, dx, dy, w;
	Font *f;
	Point p, q;
	Image *im;

	if(t->Control.hidden || t->lastshow == t->pressed)
		return;
	f = t->font->font;
	draw(t->Control.screen, t->Control.rect, t->image->image, nil, t->image->image->r.min);
	if(t->pressed || t->toggle)
		draw(t->Control.screen, t->Control.rect, t->light->image, t->mask->image, t->mask->image->r.min);
	if(t->border > 0)
		border(t->Control.screen, t->Control.rect, t->border, t->bordercolor->image, ZP);
	/* text goes here */
	dx = 0;
	for(i=0; i<t->nline; i++){
		w = stringwidth(f, t->line[i]);		/*****/
		if(dx < w)
			dx = w;
	}
	dy = t->nline*f->height;
	clipr = insetrect(t->Control.rect, t->border);
	p = _ctlalignpoint(clipr, dx, dy, t->align);
	im = t->textcolor->image;
	if(t->pressed)
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
	t->lastshow = t->pressed;
	flushimage(display, 1);
}

static void
textbutton3ctl(Control *c, CParse *cp)
{
	int cmd, i;
	Rectangle r;
	Textbutton3 *t;

	t = (Textbutton3*)c;
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
		_setctlfont(&t->Control, &t->font, cp->args[1]);
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
		textbutton3show(t);
		break;
	case EShow:
		_ctlargcount(&t->Control, cp, 1);
		t->lastshow = -1;	/* force redraw */
		textbutton3show(t);
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
		textbutton3show(t);
		break;
	case ETextcolor:
		_ctlargcount(&t->Control, cp, 2);
		_setctlimage(&t->Control, &t->textcolor, cp->args[1]);
		t->lastshow = -1;	/* force redraw */
		break;
	case EEnable:
		_ctlargcount(&t->Control, cp, 2);
		if(strcmp(cp->args[1], "left") == 0)
				t->left = 1;
		else if(strcmp(cp->args[1], "middle") == 0)
				t->middle = 1;
		else if(strcmp(cp->args[1], "right") == 0)
				t->right = 1;
		break;
	case EDisable:
		_ctlargcount(&t->Control, cp, 2);
		if(strcmp(cp->args[1], "left") == 0)
			t->left = 0;
		else if(strcmp(cp->args[1], "middle") == 0)
			t->middle = 0;
		else if(strcmp(cp->args[1], "right") == 0)
			t->right = 0;
		break;
	case EToggle:
		_ctlargcount(&t->Control, cp, 2);
		if(strcmp(cp->args[1], "on") == 0)
			t->toggle = 1;
		else if(strcmp(cp->args[1], "off") == 0)
			t->toggle = 0;
		t->lastshow = -1;	/* force redraw */
		break;
	case EGettext:
		_ctlargcount(&t->Control, cp, 2);
		if(strcmp(cp->args[1], "on") == 0)
			t->gettextflg = 1;
		else if(strcmp(cp->args[1], "off") == 0)
			t->gettextflg = 0;
		break;
	case EValue:
		_ctlargcount(&t->Control, cp, 2);
		if((cp->iargs[1]!=0) != t->pressed){
			t->pressed ^= 1;
			textbutton3show(t);
		}
		break;
	}
}

Control*
createtextbutton3(Controlset *cs, char *name)
{
	Textbutton3 *t;

	t = (Textbutton3 *)_createctl(cs, "textbutton3", sizeof(Textbutton3), name);
	t->line = ctlmalloc(sizeof(char*));
	t->nline = 0;
	t->image = _getctlimage("white");
	t->light = _getctlimage("yellow");
	t->mask = _getctlimage("opaque");
	t->bordercolor = _getctlimage("black");
	t->textcolor = _getctlimage("black");
	t->pressedtextcolor = _getctlimage("black");
	t->font = _getctlfont("font");
	t->Control.format = ctlstrdup("%q: value %d %d %d");
	t->lastshow = -1;
	t->Control.mouse = textbutton3mouse;
	t->Control.ctl = textbutton3ctl;
	t->Control.exit = textbutton3free;
	t->left = 1;
	t->middle = 1;
	t->right = 1;
	t->toggle = 0;
	t->gettextflg = 0;
	return (Control *)t;
}
