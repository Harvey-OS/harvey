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
	Control;
	CImage	*image;
	CImage	*mask;
	CImage	*light;
	CImage	*bordercolor;
	int		pressed;
	int		lastbut;
	int		lastshow;
	int		border;
	int		align;
};

enum{
	EAlign,
	EBorder,
	EBordercolor,
	EFocus,
	EFormat,
	EImage,
	ELight,
	EMask,
	ERect,
	EShow,
	EValue,
};

static char *cmds[] = {
	[EAlign] = 		"align",
	[EBorder] = 	"border",
	[EBordercolor] = "bordercolor",
	[EFocus] = 	"focus",
	[EFormat] = 	"format",
	[EImage] =	"image",
	[ELight] =		"light",
	[EMask] =		"mask",
	[ERect] =		"rect",
	[EShow] =		"show",
	[EValue] =		"value",
	nil
};

static void	buttonctl(Control*, char*);
static void	buttonshow(Button*);
static void	buttonfree(Button*);

static void
buttonthread(void *v)
{
	char buf[32];
	Button *b;

	b = v;

	snprint(buf, sizeof buf, "button-%s-0x%p", b->name, b);
	threadsetname(buf);

	b->image = _getctlimage("white");
	b->mask = _getctlimage("opaque");
	b->light = _getctlimage("yellow");
	b->bordercolor = _getctlimage("black");
	b->format = ctlstrdup("%q: value %d");
	b->lastshow = -1;
	b->border = 0;
	b->align = Aupperleft;

	for(;;){
		switch(alt(b->alts)){
		default:
			ctlerror("%q: unknown message", b->name);
		case AKey:
			/* ignore keystrokes */
			break;
		case AMouse:
			if((b->m.buttons&1) != b->lastbut){
				if(b->m.buttons & 1){
					b->pressed ^= 1;
					buttonshow(b);
					printctl(b->event, b->format, b->name, b->pressed);
				}
			}
			b->lastbut = b->m.buttons & 1;
			break;
		case ACtl:
			_ctlcontrol(b, b->str, buttonctl);
			free(b->str);
			break;
		case AWire:
			_ctlrewire(b);
			break;
		case AExit:
			buttonfree(b);
			sendul(b->exit, 0);
			return;
		}
	}
}

Control*
createbutton(Controlset *cs, char *name)
{
	return _createctl(cs, "button", sizeof(Button), name, buttonthread, 0);
}

static void
buttonfree(Button *b)
{
	_putctlimage(b->image);
	_putctlimage(b->mask);
	_putctlimage(b->light);
	_putctlimage(b->bordercolor);
}

static void
buttonshow(Button *b)
{
	Rectangle r;

	r = b->rect;
	if(b->border > 0){
		border(b->screen, r, b->border, b->bordercolor->image, ZP);
		r = insetrect(b->rect, b->border);
	}
	if(b->lastshow != b->pressed){
		draw(b->screen, r, b->image->image, nil, b->image->image->r.min);
		if(b->pressed)
			draw(b->screen, r, b->light->image, b->mask->image, b->mask->image->r.min);
		b->lastshow = b->pressed;
		flushimage(display, 1);
	}
}

static void
buttonctl(Control *c, char *str)
{
	int cmd;
	CParse cp;
	Rectangle r;
	Button *b;

	b = (Button*)c;
	cmd = _ctlparse(&cp, str, cmds);
	switch(cmd){
	default:
		ctlerror("%q: unrecognized message '%s'", b->name, cp.str);
		break;
	case EAlign:
		_ctlargcount(b, &cp, 2);
		b->align = cp.iargs[1];
		b->lastshow = -1;	/* force redraw */
		break;
	case EBorder:
		_ctlargcount(b, &cp, 2);
		b->border = cp.iargs[1];
		b->lastshow = -1;	/* force redraw */
		break;
	case EBordercolor:
		_ctlargcount(b, &cp, 2);
		_setctlimage(b, &b->bordercolor, cp.args[1]);
		b->lastshow = -1;	/* force redraw */
		break;
	case EFocus:
		/* ignore focus change */
		break;
	case EFormat:
		_ctlargcount(b, &cp, 2);
		b->format = ctlstrdup(cp.args[1]);
		break;
	case EImage:
		_ctlargcount(b, &cp, 2);
		_setctlimage(b, &b->image, cp.args[1]);
		b->lastshow = -1;	/* force redraw */
		break;
	case ELight:
		_ctlargcount(b, &cp, 2);
		_setctlimage(b, &b->light, cp.args[1]);
		b->lastshow = -1;	/* force redraw */
		break;
	case EMask:
		_ctlargcount(b, &cp, 2);
		_setctlimage(b, &b->mask, cp.args[1]);
		b->lastshow = -1;	/* force redraw */
		break;
	case ERect:
		_ctlargcount(b, &cp, 5);
		r.min.x = cp.iargs[1];
		r.min.y = cp.iargs[2];
		r.max.x = cp.iargs[3];
		r.max.y = cp.iargs[4];
		if(Dx(r)<0 || Dy(r)<0)
			ctlerror("%q: bad rectangle: %s", b->name, cp.str);
		b->rect = r;
		b->lastshow = -1;	/* force redraw */
		break;
	case EShow:
		_ctlargcount(b, &cp, 1);
		buttonshow(b);
		break;
	case EValue:
		_ctlargcount(b, &cp, 2);
		if((cp.iargs[1]!=0) != b->pressed){
			b->pressed ^= 1;
			buttonshow(b);
		}
		break;
	}
}
