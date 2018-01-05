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

typedef struct Keyboard Keyboard;

enum{
	SRegular	= 0,
	SShift	= 1,
	SCaps	= 2,
	SMask	= 3,
	Nstate	= 4,
	SControl	= 4,
};

struct Keyboard
{
	Control Control;
	CImage	*image;
	CImage	*mask;
	CImage	*light;
	CImage	*textcolor;
	CImage	*bordercolor;
	CFont	*font;
	CFont	*ctlfont;
	Image	*im[Nstate];
	int		border;
	int		lastbut;
	int		state;
	char		*_key; // Declared in struct Control
};

enum{
	EBorder,
	EBordercolor,
	EFocus,
	EFont,
	EFormat,
	EHide,
	EImage,
	ELight,
	EMask,
	ERect,
	EReveal,
	EShow,
	ESize,
};

static char *cmds[] = {
	[EBorder] =	"border",
	[EBordercolor] = "bordercolor",
	[EFocus] = 	"focus",
	[EFont] =		"font",
	[EFormat] = 	"format",
	[EHide] =		"hide",
	[EImage] =	"image",
	[ELight] =		"light",
	[EMask] =		"mask",
	[ERect] =		"rect",
	[EReveal] =	"reveal",
	[EShow] =		"show",
	[ESize] =		"size",
	nil
};

enum
{
	Nrow = 5
};

static uint8_t wid [Nrow][16] = {
	{16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 30, },
	{24, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 24, },
	{32, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, },
	{40, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, },
	{30, 30, 80, 40, 42, 24, },
};

static char *keyregular[Nrow] = {
	"`\0001\0002\0003\0004\0005\0006\0007\0008\0009\0000\0-\0=\0\\\0<-\0\0",
	"->\0q\0w\0e\0r\0t\0y\0u\0i\0o\0p\0[\0]\0Del\0\0",
	"Caps\0a\0s\0d\0f\0g\0h\0j\0k\0l\0;\0'\0Enter\0\0",
	"Shift\0z\0x\0c\0v\0b\0n\0m\0,\0.\0/\0Shift\0\0",
	"Ctrl\0Alt\0 \0Scrib\0Menu\0Esc\0\0"
};

static char *keyshift[Nrow] = {
	"~\0!\0@\0#\0$\0%\0^\0&\0*\0(\0)\0_\0+\0|\0<-\0\0",
	"->\0Q\0W\0E\0R\0T\0Y\0U\0I\0O\0P\0{\0}\0Del\0\0",
	"Caps\0A\0S\0D\0F\0G\0H\0J\0K\0L\0:\0\"\0Enter\0\0",
	"Shift\0Z\0X\0C\0V\0B\0N\0M\0<\0>\0?\0Shift\0\0",
	"Ctrl\0Alt\0 \0Scrib\0Menu\0Esc\0\0"
};

static char *keycaps[Nrow] = {
	"`\0001\0002\0003\0004\0005\0006\0007\0008\0009\0000\0-\0=\0\\\0<-\0\0",
	"->\0Q\0W\0E\0R\0T\0Y\0U\0I\0O\0P\0[\0]\0Del\0\0",
	"Caps\0A\0S\0D\0F\0G\0H\0J\0K\0L\0;\0'\0Enter\0\0",
	"Shift\0Z\0X\0C\0V\0B\0N\0M\0,\0.\0/\0Shift\0\0",
	"Ctrl\0Alt\0 \0Scrib\0Menu\0Esc\0\0"
};

static char *keycapsshift[Nrow] = {
	"~\0!\0@\0#\0$\0%\0^\0&\0*\0(\0)\0_\0+\0|\0<-\0\0",
	"->\0q\0w\0e\0r\0t\0y\0u\0i\0o\0p\0{\0}\0Del\0\0",
	"Caps\0a\0s\0d\0f\0g\0h\0j\0k\0l\0:\0\"\0Enter\0\0",
	"Shift\0z\0x\0c\0v\0b\0n\0m\0<\0>\0?\0Shift\0\0",
	"Ctrl\0Alt\0 \0Scrib\0Menu\0Esc\0\0"
};

struct{
	char	*name;
	int	val;
}keytab[] = {
	"Shift",	0,
	"Ctrl",	0,
	"Alt",		0,
	"Caps",	0,
	"Del",	'\177',
	"Enter",	'\n',
	"Esc",	'\033',
	"<-",		'\b',
	"->",		'\t',
	"Scrib",	0x10000,
	"Menu",	0x10001,
	nil,		0,
};

static char **keyset[Nstate] = {
	keyregular,
	keyshift,
	keycaps,
	keycapsshift,
};

static void	keyboardshow(Keyboard*);
static void	keyup(Keyboard*, Point);
static void	keydown(Keyboard*, Point);
static void	keyresize(Keyboard*);

static void
keyboardmouse(Control *c, Mouse *m)
{
	Keyboard *k;

	k = (Keyboard *)c;
	if(m->buttons==1)
		keydown(k, m->xy);
	else if(k->lastbut==1 && m->buttons==0)
		keyup(k, m->xy);
	k->lastbut = m->buttons;
}

static void
keyboardfree(Control *c)
{
	int i;
	Keyboard *k;

	k = (Keyboard *)c;
	_putctlimage(k->image);
	_putctlimage(k->mask);
	_putctlimage(k->light);
	_putctlimage(k->textcolor);
	_putctlimage(k->bordercolor);
	_putctlfont(k->font);
	_putctlfont(k->ctlfont);
	for(i=0; i<nelem(k->im); i++)
		freeimage(k->im[i]);
	free(k->Control.format);
}

static int
keyboardy(Keyboard *k, int row)
{
	int dy;

	if(row >= Nrow)
		return k->Control.rect.max.y-k->border;
	dy = Dy(k->Control.rect)-2*k->border;
	return k->Control.rect.min.y+k->border+(row*dy+Nrow-1)/Nrow;
}

static char*
whichkey(Keyboard *k, Point p, int *rowp, int *colp, Rectangle *rp)
{
	uint8_t *wp;
	char *kp;
	int row, col, dx, dy, x, n, maxx;
	Rectangle r;

	r = insetrect(k->Control.rect, k->border);
	if(!ptinrect(p, r))
		return nil;
	maxx = r.max.x;
	dx = Dx(r);
	dy = Dy(r);
	row = (p.y - r.min.y)*Nrow/dy;
	if(row >= Nrow)
		row = Nrow-1;
	r.min.y = keyboardy(k, row);
	r.max.y = keyboardy(k, row+1);
	x = r.min.x;
	kp = keyset[k->state&SMask][row];
	wp = wid[row];
	for(col=0; *kp; col++,kp+=n+1){
		n = strlen(kp);
		r.min.x = x;
		r.max.x = x + (wp[col]*dx+255)/256;
		if(kp[n+1] == '\0')
			r.max.x = maxx;
		if(r.max.x > p.x)
			break;
		x = r.max.x;
	}
	*rp = insetrect(r, 1);
	*rowp = row;
	*colp = col;
	return kp;
}

static Rectangle
keyrect(Keyboard *k, int row, int col)
{
	uint8_t *wp;
	char *kp;
	int i, x, n, dx;
	Rectangle r;
	Point p;

	r = insetrect(k->Control.rect, k->border);
	p = r.min;
	dx = Dx(r);
	r.min.y = keyboardy(k, row);
	r.max.y = keyboardy(k, row+1);
	x = r.min.x;
	kp = keyset[0][row];
	wp = wid[row];
	for(i=0; *kp; i++,kp+=n+1){
		n = strlen(kp);
		r.min.x = x;
		r.max.x = x + (wp[i]*dx+255)/256;
		if(kp[n+1] == '\0')
			r.max.x = p.x+dx;
		if(i >= col)
			break;
		x = r.max.x;
	}
	return insetrect(r, 1);
}

static void
keydraw(Keyboard *k, int state)
{
	Point p, q;
	int row, col, x, dx, dy, nexty, n;
	uint8_t *wp;
	char *kp;
	Rectangle r;
	Font *f, *f1, *f2;
	Image *im;

	freeimage(k->im[state]);
	k->im[state] = nil;
	if(Dx(k->Control.rect)-2*k->border <= 0)
		return;

	im = allocimage(display, k->Control.rect, screen->chan, 0, ~0);
	if(im == nil)
		return;
	k->im[state] = im;

	r = insetrect(k->Control.rect, k->border);
	border(im, k->Control.rect, k->border, k->bordercolor->image, ZP);
	draw(im, r, k->image->image, nil, ZP);
	dx = Dx(r);
	dy = Dy(r);
	p = r.min;
	f1 = k->font->font;
	f2 = k->ctlfont->font;
	nexty = p.y;
	for(row=0; row<Nrow; row++){
		x = p.x;
		kp = keyset[state][row];
		wp = wid[row];
		r.min.y = nexty;
		nexty = keyboardy(k, row+1);
		r.max.y = nexty;
		for(col=0; *kp; col++,kp+=n+1){
			r.min.x = x;
			r.max.x = x + (wp[col]*dx+255)/256;
			n = strlen(kp);
			if(kp[n+1] == '\0')
				r.max.x = p.x+dx;
			if(row == Nrow-1)
				r.max.y = p.y+dy;
			if(n > 1)
				f = f2;
			else
				f = f1;
			q = _ctlalignpoint(r, stringnwidth(f, kp, n), f->height, Acenter);
			_string(im, q, k->textcolor->image,
				ZP, f, kp, nil, n, r,
				nil, ZP, SoverD);
			x = r.max.x;
			if(kp[n+1])
				draw(im, Rect(x, r.min.y, x+1, r.max.y),
					k->textcolor->image, nil, ZP);
		}
		if(row != Nrow-1)
			draw(im, Rect(p.x, r.max.y, p.x+dx, r.max.y+1),
				k->textcolor->image, nil, ZP);
	}
}

static void
keyresize(Keyboard *k)
{
	int i;

	for(i=0; i<Nstate; i++)
		keydraw(k, i);
}

static void
keyboardshow(Keyboard *k)
{
	Rectangle r;

	if (k->Control.hidden)
		return;
	if(k->im[0]==nil || !eqrect(k->im[0]->r, k->Control.rect))
		keyresize(k);
	if(k->im[k->state&SMask] == nil)
		return;
	draw(k->Control.screen, k->Control.rect, k->im[k->state&SMask], nil, k->Control.rect.min);
	if(k->state & SShift){
		r = keyrect(k, 3, 0);
		draw(k->Control.screen, r, k->light->image, k->mask->image, ZP);
		r = keyrect(k, 3, 11);
		draw(k->Control.screen, r, k->light->image, k->mask->image, ZP);
	}
	if(k->state & SCaps){
		r = keyrect(k, 2, 0);
		draw(k->Control.screen, r, k->light->image, k->mask->image, ZP);
	}
	if(k->state & SControl){
		r = keyrect(k, 4, 0);
		draw(k->Control.screen, r, k->light->image, k->mask->image, ZP);
	}
	flushimage(display, 1);
}

static void
keydown(Keyboard *k, Point p)
{
	int row, col;
	Rectangle r;
	char *s;

	s = whichkey(k, p, &row, &col, &r);
	if(s == k->_key)
		return;
	keyboardshow(k);
	if(s != nil)
		draw(k->Control.screen, r, k->light->image, k->mask->image, ZP);
	flushimage(display, 1);
	k->_key = s;
}

static int
keylookup(char *s)
{
	int i;

	for(i=0; keytab[i].name; i++)
		if(strcmp(s, keytab[i].name) == 0)
			return keytab[i].val;
	return s[0];
}

static void
keyup(Keyboard *k, Point p)
{
	int row, col;
	Rectangle r;
	char *s;
	int val;

	s = whichkey(k, p, &row, &col, &r);
	if(s == nil)
		return;
	val = keylookup(s);
	if(k->state & SControl)
		if(' '<val && val<0177)
			val &= ~0x60;
	if(strcmp(s, "Alt") == 0)
		{;}
	if(strcmp(s, "Ctrl") == 0){
		k->state ^= SControl;
	}else
		k->state &= ~SControl;
	if(strcmp(s, "Shift")==0 || strcmp(s, "Caps")==0){
		if(strcmp(s, "Shift") == 0)
			k->state ^= SShift;
		if(strcmp(s, "Caps") == 0)
			k->state ^= SCaps;
	}else
		k->state &= ~SShift;
	keyboardshow(k);
	if(val)
		chanprint(k->Control.event, k->Control.format, k->Control.name, val);
	k->Control.key = nil;
}

static void
keyboardctl(Control *c, CParse *cp)
{
	int cmd;
	Rectangle r;
	Keyboard *k;

	k = (Keyboard*)c;
	cmd = _ctllookup(cp->args[0], cmds, nelem(cmds));
	switch(cmd){
	default:
		ctlerror("%q: unrecognized message '%s'", k->Control.name, cp->str);
		break;
	case EBorder:
		_ctlargcount(&k->Control, cp, 2);
		if(cp->iargs[1] < 0)
			ctlerror("%q: bad border: %c", k->Control.name, cp->str);
		k->border = cp->iargs[1];
		break;
	case EBordercolor:
		_ctlargcount(&k->Control, cp, 2);
		_setctlimage(&k->Control, &k->bordercolor, cp->args[1]);
		break;
	case EFocus:
		/* ignore focus change */
		break;
	case EFont:
		if(cp->nargs!=2 && cp->nargs!=3)
			ctlerror("%q: bad font message '%s'", k->Control.name, cp->str);
		_setctlfont(&k->Control, &k->font, cp->args[1]);
		if(cp->nargs == 3)
			_setctlfont(&k->Control, &k->ctlfont, cp->args[2]);
		else
			_setctlfont(&k->Control, &k->ctlfont, cp->args[1]);
		break;
	case EFormat:
		_ctlargcount(&k->Control, cp, 2);
		k->Control.format = ctlstrdup(cp->args[1]);
		break;
	case EHide:
		_ctlargcount(&k->Control, cp, 1);
		k->Control.hidden = 1;
		break;
	case EImage:
		_ctlargcount(&k->Control, cp, 2);
		_setctlimage(&k->Control, &k->image, cp->args[1]);
		break;
	case ELight:
		_ctlargcount(&k->Control, cp, 2);
		_setctlimage(&k->Control, &k->light, cp->args[1]);
		break;
	case EMask:
		_ctlargcount(&k->Control, cp, 2);
		_setctlimage(&k->Control, &k->mask, cp->args[1]);
		break;
	case ERect:
		_ctlargcount(&k->Control, cp, 5);
		r.min.x = cp->iargs[1];
		r.min.y = cp->iargs[2];
		r.max.x = cp->iargs[3];
		r.max.y = cp->iargs[4];
		if(Dx(r)<0 || Dy(r)<0)
			ctlerror("%q: bad rectangle: %s", k->Control.name, cp->str);
		k->Control.rect = r;
		keyboardshow(k);
		break;
	case EReveal:
		_ctlargcount(&k->Control, cp, 1);
		k->Control.hidden = 0;
		keyboardshow(k);
		break;
	case EShow:
		_ctlargcount(&k->Control, cp, 1);
		keyboardshow(k);
		break;
	case ESize:
		if (cp->nargs == 3)
			r.max = Pt(0x7fffffff, 0x7fffffff);
		else{
			_ctlargcount(&k->Control, cp, 5);
			r.max.x = cp->iargs[3];
			r.max.y = cp->iargs[4];
		}
		r.min.x = cp->iargs[1];
		r.min.y = cp->iargs[2];
		if(r.min.x<=0 || r.min.y<=0 || r.max.x<=0 || r.max.y<=0 || r.max.x < r.min.x || r.max.y < r.min.y)
			ctlerror("%q: bad sizes: %s", k->Control.name, cp->str);
		k->Control.size.min = r.min;
		k->Control.size.max = r.max;
		break;
	}
}

Control*
createkeyboard(Controlset *cs, char *name)
{
	Keyboard *k;

	k = (Keyboard *)_createctl(cs, "keyboard", sizeof(Keyboard), name);
	k->image = _getctlimage("white");
	k->mask = _getctlimage("opaque");
	k->light = _getctlimage("yellow");
	k->bordercolor = _getctlimage("black");
	k->textcolor = _getctlimage("black");
	k->font = _getctlfont("font");
	k->ctlfont = _getctlfont("font");
	k->Control.format = ctlstrdup("%q: value 0x%x");
	k->border = 0;
	k->lastbut = 0;
	k->Controlkey = nil;
	k->state = SRegular;
	k->Control.ctl = keyboardctl;
	k->Control.mouse = keyboardmouse;
	k->Control.exit = keyboardfree;
	k->Control.size = Rect(246, 2 + 5 * (k->font->font->height + 1), 512, 256);
	return &k->Control;
}
