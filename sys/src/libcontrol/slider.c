#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <mouse.h>
#include <keyboard.h>
#include <control.h>

typedef struct Slider Slider;

struct Slider
{
	Control;
	int		border;
	CImage	*image;
	CImage	*textcolor;
	CImage	*bordercolor;
	CImage	*indicatorcolor;
	int		absolute;
	int		max;
	int		vis;
	int		value;
	int		clamphigh;
	int		clamplow;
	int		horizontal;
	int		lastbut;
};

enum{
	EAbsolute,
	EBorder,
	EBordercolor,
	EClamp,
	EFocus,
	EFormat,
	EHide,
	EImage,
	EIndicatorcolor,
	EMax,
	EOrient,
	ERect,
	EReveal,
	EShow,
	ESize,
	EValue,
	EVis,
};

static char *cmds[] = {
	[EAbsolute] =		"absolute",
	[EBorder] =		"border",
	[EBordercolor] =	"bordercolor",
	[EClamp] =		"clamp",
	[EFocus] = 		"focus",
	[EFormat] = 		"format",
	[EHide] =			"hide",
	[EImage] =		"image",
	[EIndicatorcolor] =	"indicatorcolor",
	[EMax] =			"max",
	[EOrient] =		"orient",
	[ERect] =			"rect",
	[EReveal] =		"reveal",
	[EShow] =			"show",
	[ESize] =			"size",
	[EValue] =			"value",
	[EVis] =			"vis",
};

static void
sliderfree(Control *c)
{
	Slider *s;

	s = (Slider*)c;
	_putctlimage(s->image);
	_putctlimage(s->textcolor);
	_putctlimage(s->bordercolor);
	_putctlimage(s->indicatorcolor);
}

static void
slidershow(Slider *s)
{
	Rectangle r, t;
	int l, h, d;

	if (s->hidden)
		return;
	r = s->rect;
	draw(s->screen, r, s->image->image, nil, s->image->image->r.min);
	if(s->border > 0){
		border(s->screen, r, s->border, s->bordercolor->image, s->bordercolor->image->r.min);
		r = insetrect(r, s->border);
	}
	if(s->max <= 0)
		return;
	if(s->horizontal)
		d = Dx(r);
	else
		d = Dy(r);
	l = muldiv(s->value, d, s->max);
	h = muldiv(s->value+s->vis, d, s->max);
	if(s->clamplow && s->clamphigh){
		l = 0;
		h = d;
	}else if(s->clamplow){
		h = l;
		l = 0;
	}else if(s->clamphigh)
		h = d;
	t = r;
	if(s->horizontal){
		r.max.x = r.min.x+h;
		r.min.x += l;
	}else{
		r.max.y = r.min.y+h;
		r.min.y += l;
	}
	if(rectclip(&r, t))
		draw(s->screen, r, s->indicatorcolor->image, nil, s->indicatorcolor->image->r.min);
	flushimage(display, 1);
}

static void
sliderctl(Control *c, CParse *cp)
{
	int cmd, prev;
	Rectangle r;
	Slider *s;

	s = (Slider*)c;
	cmd = _ctllookup(cp->args[0], cmds, nelem(cmds));
	switch(cmd){
	default:
		ctlerror("%q: unrecognized message '%s'", s->name, cp->str);
		break;
	case EAbsolute:
		_ctlargcount(s, cp, 2);
		s->absolute = cp->iargs[1];
		break;
	case EBorder:
		_ctlargcount(s, cp, 2);
		if(cp->iargs[1] < 0)
			ctlerror("%q: bad border: %c", s->name, cp->str);
		s->border = cp->iargs[1];
		break;
	case EBordercolor:
		_ctlargcount(s, cp, 2);
		_setctlimage(s, &s->bordercolor, cp->args[1]);
		break;
	case EClamp:
		_ctlargcount(s, cp, 3);
		if(strcmp(cp->args[1], "high") == 0)
			s->clamphigh = cp->iargs[2];
		else if(strcmp(cp->args[1], "low") == 0)
			s->clamplow = cp->iargs[2];
		else
			ctlerror("%q: unrecognized clamp: %s", s->name, cp->str);
		break;
	case EFocus:
		/* ignore focus change */
		break;
	case EFormat:
		_ctlargcount(s, cp, 2);
		s->format = ctlstrdup(cp->args[1]);
		break;
	case EHide:
		_ctlargcount(s, cp, 1);
		s->hidden = 1;
		break;
	case EImage:
		_ctlargcount(s, cp, 2);
		_setctlimage(s, &s->image, cp->args[1]);
		break;
	case EIndicatorcolor:
		_ctlargcount(s, cp, 2);
		_setctlimage(s, &s->indicatorcolor, cp->args[1]);
		break;
	case EMax:
		_ctlargcount(s, cp, 2);
		if(cp->iargs[1] < 0)
			ctlerror("%q: negative max value: %s", s->name, cp->str);
		if(s->max != cp->iargs[1]){
			s->max = cp->iargs[1];
			slidershow(s);
		}
		break;
	case EOrient:
		_ctlargcount(s, cp, 2);
		prev = s->horizontal;
		if(strncmp(cp->args[1], "hor", 3) == 0)
			s->horizontal = 1;
		else if(strncmp(cp->args[1], "ver", 3) == 0)
			s->horizontal = 0;
		else
			ctlerror("%q: unrecognized orientation: %s", s->name, cp->str);
		if(s->horizontal != prev)
			slidershow(s);
		break;
	case ERect:
		_ctlargcount(s, cp, 5);
		r.min.x = cp->iargs[1];
		r.min.y = cp->iargs[2];
		r.max.x = cp->iargs[3];
		r.max.y = cp->iargs[4];
		if(Dx(r)<=0 || Dy(r)<=0)
			ctlerror("%q: bad rectangle: %s", s->name, cp->str);
		s->rect = r;
		break;
	case EReveal:
		_ctlargcount(s, cp, 1);
		s->hidden = 0;
		slidershow(s);
		break;
	case EShow:
		_ctlargcount(s, cp, 1);
		slidershow(s);
		break;
	case ESize:
		if (cp->nargs == 3)
			r.max = Pt(0x7fffffff, 0x7fffffff);
		else{
			_ctlargcount(s, cp, 5);
			r.max.x = cp->iargs[3];
			r.max.y = cp->iargs[4];
		}
		r.min.x = cp->iargs[1];
		r.min.y = cp->iargs[2];
		if(r.min.x<=0 || r.min.y<=0 || r.max.x<=0 || r.max.y<=0 || r.max.x < r.min.x || r.max.y < r.min.y)
			ctlerror("%q: bad sizes: %s", s->name, cp->str);
		s->size.min = r.min;
		s->size.max = r.max;
		break;
	case EValue:
		_ctlargcount(s, cp, 2);
		if(s->value != cp->iargs[1]){
			s->value = cp->iargs[1];
			slidershow(s);
		}
		break;
	case EVis:
		_ctlargcount(s, cp, 2);
		if(s->vis != cp->iargs[1]){
			s->vis = cp->iargs[1];
			slidershow(s);
		}
		break;
	}
}

static void
slidermouse(Control *c, Mouse *m)
{
	Rectangle r;
	int v, l, d, b;
	Slider *s;

	s =(Slider*)c;
	if(m->buttons == 0){
		/* buttons now up */
		s->lastbut = 0;
		return;
	}
	if(!s->absolute && s->lastbut==m->buttons && s->lastbut!=2){
		/* clicks only on buttons 1 & 3; continuous motion on 2 (or when absolute) */
		return;
	}
	if(s->lastbut!=0 && m->buttons!=s->lastbut){
		/* buttons down have changed; wait for button up */
		return;
	}
	s->lastbut = m->buttons;

	r = insetrect(s->rect, s->border);
	if(s->horizontal){
		v = m->xy.x - r.min.x;
		d = Dx(r);
	}else{
		v = m->xy.y - r.min.y;
		d = Dy(r);
	}
	if(s->absolute)
		b = 2;
	else
		b = m->buttons;
	switch(b){
	default:
		return;
	case 1:
		l = s->value - muldiv(v, s->vis, d);
		break;
	case 2:
		l = muldiv(v, s->max, d);
		break;
	case 4:
		l = s->value + muldiv(v, s->vis, d);
		break;
	}
	if(l < 0)
		l = 0;
	if(l > s->max)
		l = s->max;
	if(l != s->value){
		s->value = l;
		chanprint(s->event, s->format, s->name, s->value);
		slidershow(s);
	}
}

Control*
createslider(Controlset *cs, char *name)
{
	Slider *s;

	s = (Slider*)_createctl(cs, "slider", sizeof(Slider), name);
	s->image = _getctlimage("white");
	s->textcolor = _getctlimage("black");
	s->bordercolor = _getctlimage("black");
	s->indicatorcolor = _getctlimage("black");
	s->format = ctlstrdup("%q: value %d");
	s->border = 0;
	s->mouse = slidermouse;
	s->ctl = sliderctl;
	s->exit = sliderfree;
	return (Control*)s;
}
