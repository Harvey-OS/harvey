#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <mouse.h>
#include <keyboard.h>
#include <control.h>

typedef struct Box Box;

struct Box
{
	Control;
	int		border;
	CImage	*bordercolor;
	CImage	*image;
	int		align;
};

enum{
	EAlign,
	EBorder,
	EBordercolor,
	EFocus,
	EHide,
	EImage,
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
	[EHide] =		"hide",
	[EImage] =	"image",
	[ERect] =		"rect",
	[EReveal] =	"reveal",
	[EShow] =		"show",
	[ESize] =		"size",
	nil
};

static void
boxkey(Control *c, Rune *rp)
{
	Box *b;

	b = (Box*)c;
	chanprint(b->event, "%q: key 0x%x", b->name, rp[0]);
}

static void
boxmouse(Control *c, Mouse *m)
{
	Box *b;

	b = (Box*)c;
	if (ptinrect(m->xy,b->rect))
		chanprint(b->event, "%q: mouse %P %d %ld", b->name,
			m->xy, m->buttons, m->msec);
}

static void
boxfree(Control *c)
{
	_putctlimage(((Box*)c)->image);
}

static void
boxshow(Box *b)
{
	Image *i;
	Rectangle r;

	if(b->hidden)
		return;
	if(b->border > 0){
		border(b->screen, b->rect, b->border, b->bordercolor->image, ZP);
		r = insetrect(b->rect, b->border);
	}else
		r = b->rect;
	i = b->image->image;
	/* BUG: ALIGNMENT AND CLIPPING */
	draw(b->screen, r, i, nil, ZP);
}

static void
boxctl(Control *c, CParse *cp)
{
	int cmd;
	Rectangle r;
	Box *b;

	b = (Box*)c;
	cmd = _ctllookup(cp->args[0], cmds, nelem(cmds));
	switch(cmd){
	default:
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
		_ctlargcount(b, cp, 2);
		chanprint(b->event, "%q: focus %s", b->name, cp->args[1]);
		break;
	case EHide:
		_ctlargcount(b, cp, 1);
		b->hidden = 1;
		break;
	case EImage:
		_ctlargcount(b, cp, 2);
		_setctlimage(b, &b->image, cp->args[1]);
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
		boxshow(b);
		break;
	case EShow:
		_ctlargcount(b, cp, 1);
		boxshow(b);
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

Control*
createbox(Controlset *cs, char *name)
{
	Box *b;

	b = (Box *)_createctl(cs, "box", sizeof(Box), name);
	b->image = _getctlimage("white");
	b->bordercolor = _getctlimage("black");
	b->align = Aupperleft;
	b->key = boxkey;
	b->mouse = boxmouse;
	b->ctl = boxctl;
	b->exit = boxfree;
	return (Control *)b;
}
