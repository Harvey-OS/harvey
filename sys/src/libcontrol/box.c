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
	EImage,
	ERect,
	EShow,
};

static char *cmds[] = {
	[EAlign] =		"align",
	[EBorder] =	"border",
	[EBordercolor] ="bordercolor",
	[EFocus] = 	"focus",
	[EImage] =	"image",
	[ERect] =		"rect",
	[EShow] =		"show",
	nil
};

static void	boxctl(Control*, char*);
static void	boxshow(Box*);
static void	boxfree(Box*);

static void
boxthread(void *v)
{
	char buf[32];
	Box *b;

	b = v;

	snprint(buf, sizeof buf, "box-%s-0x%p", b->name, b);
	threadsetname(buf);

	b->image = _getctlimage("white");
	b->bordercolor = _getctlimage("black");
	b->align = Aupperleft;

	for(;;){
		switch(alt(b->alts)){
		default:
			ctlerror("%q: unknown message", b->name);
		case AKey:
			printctl(b->event, "%q: key 0x%x", b->name, b->kbdr[0]);
			break;
		case AMouse:
			printctl(b->event, "%q: mouse %P %d %ld", b->name,
				b->m.xy, b->m.buttons, b->m.msec);
			break;
		case ACtl:
			_ctlcontrol(b, b->str, boxctl);
			free(b->str);
			break;
		case AWire:
			_ctlrewire(b);
			break;
		case AExit:
			boxfree(b);
			sendul(b->exit, 0);
			return;
		}
	}
}

Control*
createbox(Controlset *cs, char *name)
{
	return _createctl(cs, "box", sizeof(Box), name, boxthread, 0);
}

static void
boxfree(Box *b)
{
	_putctlimage(b->image);
}

static void
boxshow(Box *b)
{
	Image *i;
	Rectangle r;

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
boxctl(Control *c, char *str)
{
	int cmd;
	CParse cp;
	Rectangle r;
	Box *b;

	b = (Box*)c;
	cmd = _ctlparse(&cp, str, cmds);
	switch(cmd){
	default:
		ctlerror("%q: unrecognized message '%s'", b->name, cp.str);
		break;
	case EAlign:
		_ctlargcount(b, &cp, 2);
		b->align = _ctlalignment(cp.args[1]);
		break;
	case EBorder:
		_ctlargcount(b, &cp, 2);
		if(cp.iargs[1] < 0)
			ctlerror("%q: bad border: %c", b->name, cp.str);
		b->border = cp.iargs[1];
		break;
	case EBordercolor:
		_ctlargcount(b, &cp, 2);
		_setctlimage(b, &b->bordercolor, cp.args[1]);
		break;
	case EFocus:
		_ctlargcount(b, &cp, 2);
		printctl(b->event, "%q: focus %s", b->name, cp.args[1]);
		break;
	case EImage:
		_ctlargcount(b, &cp, 2);
		_setctlimage(b, &b->image, cp.args[1]);
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
		break;
	case EShow:
		_ctlargcount(b, &cp, 1);
		boxshow(b);
		break;
	}
}
