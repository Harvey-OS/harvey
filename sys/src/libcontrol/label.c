#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <mouse.h>
#include <keyboard.h>
#include <control.h>

typedef struct Label Label;

struct Label
{
	Control;
	int		border;
	CFont	*font;
	CImage	*image;
	CImage	*textcolor;
	CImage	*bordercolor;
	char		*text;
	int		align;
};

enum{
	EAlign,
	EBorder,
	EBordercolor,
	EFocus,
	EFont,
	EImage,
	ERect,
	EShow,
	ETextcolor,
	EValue,
};

static char *cmds[] = {
	[EAlign] =			"align",
	[EBorder] =		"border",
	[EBordercolor] =	"bordercolor",
	[EFocus] = 		"focus",
	[EFont] =			"font",
	[EImage] =		"image",
	[ERect] =			"rect",
	[EShow] =			"show",
	[ETextcolor] =		"textcolor",
	[EValue] =			"value",
	nil
};

static void	labelctl(Control*, char*);
static void	labelshow(Label*);
static void	labelfree(Label*);

static void
labelthread(void *v)
{
	char buf[32];
	Label *l;

	l = v;

	snprint(buf, sizeof buf, "label-%s-0x%p", l->name, l);
	threadsetname(buf);

	l->text = ctlstrdup("");
	l->image = _getctlimage("white");
	l->textcolor = _getctlimage("black");
	l->bordercolor = _getctlimage("black");
	l->font = _getctlfont("font");
	l->border = 0;

	for(;;){
		switch(alt(l->alts)){
		default:
			ctlerror("%q: unknown message", l->name);
		case AKey:
			break;
		case AMouse:
			/* ignore mouse */
			break;
		case ACtl:
			_ctlcontrol(l, l->str, labelctl);
			free(l->str);
			break;
		case AWire:
			_ctlrewire(l);
			break;
		case AExit:
			labelfree(l);
			sendul(l->exit, 0);
			return;
		}
	}
}

Control*
createlabel(Controlset *cs, char *name)
{
	return _createctl(cs, "label", sizeof(Label), name, labelthread, 0);
}

static void
labelfree(Label *l)
{
	_putctlfont(l->font);
	_putctlimage(l->image);
	_putctlimage(l->textcolor);
	_putctlimage(l->bordercolor);
}


static void
labelshow(Label *l)
{
	Rectangle r;
	Point p;

	r = l->rect;
	draw(l->screen, r, l->image->image, nil, l->image->image->r.min);
	if(l->border > 0){
		border(l->screen, r, l->border, l->bordercolor->image, l->bordercolor->image->r.min);
		r = insetrect(r, l->border);
	}
	p = _ctlalignpoint(r,
		stringwidth(l->font->font, l->text),
		l->font->font->height, l->align);
	_string(l->screen, p, l->textcolor->image,
		ZP, l->font->font, l->text, nil, strlen(l->text),
		r, nil, ZP);
	flushimage(display, 1);
}

static void
labelctl(Control *c, char *str)
{
	int cmd;
	CParse cp;
	Rectangle r;
	Label *l;

	l = (Label*)c;
	cmd = _ctlparse(&cp, str, cmds);
	switch(cmd){
	default:
		ctlerror("%q: unrecognized message '%s'", l->name, cp.str);
		break;
	case EAlign:
		_ctlargcount(l, &cp, 2);
		l->align = _ctlalignment(cp.args[1]);
		break;
	case EBorder:
		_ctlargcount(l, &cp, 2);
		if(cp.iargs[1] < 0)
			ctlerror("%q: bad border: %c", l->name, cp.str);
		l->border = cp.iargs[1];
		break;
	case EBordercolor:
		_ctlargcount(l, &cp, 2);
		_setctlimage(l, &l->bordercolor, cp.args[1]);
		break;
	case EFocus:
		/* ignore focus change */
		break;
	case EFont:
		_ctlargcount(l, &cp, 2);
		_setctlfont(l, &l->font, cp.args[1]);
		break;
	case EImage:
		_ctlargcount(l, &cp, 2);
		_setctlimage(l, &l->image, cp.args[1]);
		break;
	case ERect:
		_ctlargcount(l, &cp, 5);
		r.min.x = cp.iargs[1];
		r.min.y = cp.iargs[2];
		r.max.x = cp.iargs[3];
		r.max.y = cp.iargs[4];
		if(Dx(r)<=0 || Dy(r)<=0)
			ctlerror("%q: bad rectangle: %s", l->name, cp.str);
		l->rect = r;
		break;
	case EShow:
		_ctlargcount(l, &cp, 1);
		labelshow(l);
		break;
		_ctlargcount(l, &cp, 2);
		_setctlimage(l, &l->textcolor, cp.args[1]);
		break;
	case ETextcolor:
		_ctlargcount(l, &cp, 2);
		_setctlimage(l, &l->textcolor, cp.args[1]);
		break;
	case EValue:
		_ctlargcount(l, &cp, 2);
		if(strcmp(cp.args[1], l->text) != 0){
			free(l->text);
			l->text = ctlstrdup(cp.args[1]);
			labelshow(l);
		}
		break;
	}
}
