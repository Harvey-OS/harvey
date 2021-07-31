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
	int		pressed;
	int		lastbut;
	int		lastshow;
	char		**line;
	int		nline;
	int		align;
	int		border;
};

enum{
	EAlign,
	EBorder,
	EBordercolor,
	EFocus,
	EFont,
	EFormat,
	EImage,
	ELight,
	EMask,
	EPressedtextcolor,
	ERect,
	EShow,
	EText,
	ETextcolor,
	EValue,
};

static char *cmds[] = {
	[EAlign] =			"align",
	[EBorder] =		"border",
	[EBordercolor] = "bordercolor",
	[EFocus] = 		"focus",
	[EFont] =			"font",
	[EFormat] = 		"format",
	[EImage] =		"image",
	[ELight] =			"light",
	[EMask] =			"mask",
	[EPressedtextcolor] ="pressedtextcolor",
	[ERect] =			"rect",
	[EShow] =			"show",
	[EText] =			"text",
	[ETextcolor] =		"textcolor",
	[EValue] =			"value",
	nil
};

static void	textbuttonctl(Control*, char*);
static void	textbuttonshow(Textbutton*);
static void	textbuttonfree(Textbutton*);

static void
textbuttonthread(void *v)
{
	char buf[32];
	Textbutton *t;

	t = v;

	snprint(buf, sizeof buf, "textbutton-%s-0x%p", t->name, t);
	threadsetname(buf);

	t->line = ctlmalloc(sizeof(char*));
	t->nline = 0;
	t->image = _getctlimage("white");
	t->mask = _getctlimage("opaque");
	t->light = _getctlimage("yellow");
	t->bordercolor = _getctlimage("black");
	t->textcolor = _getctlimage("black");
	t->pressedtextcolor = _getctlimage("black");
	t->font = _getctlfont("font");
	t->format = ctlstrdup("%q: value %d");
	t->lastshow = -1;

	for(;;){
		switch(alt(t->alts)){
		default:
			ctlerror("%q: unknown message", t->name);
		case AKey:
			/* ignore keystrokes */
			break;
		case AMouse:
			if((t->m.buttons&1) != t->lastbut){
				if(t->m.buttons & 1){
					t->pressed ^= 1;
					textbuttonshow(t);
					printctl(t->event, t->format, t->name, t->pressed);
				}
			}
			t->lastbut = t->m.buttons & 1;
			break;
		case ACtl:
			_ctlcontrol(t, t->str, textbuttonctl);
			free(t->str);
			break;
		case AWire:
			_ctlrewire(t);
			break;
		case AExit:
			textbuttonfree(t);
			sendul(t->exit, 0);
			return;
		}
	}
}

Control*
createtextbutton(Controlset *cs, char *name)
{
	return _createctl(cs, "textbutton", sizeof(Textbutton), name, textbuttonthread, 0);
}

static void
textbuttonfree(Textbutton *t)
{
	int i;

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
textbuttonshow(Textbutton *t)
{
	Rectangle r, clipr;
	int i, dx, dy, w;
	Font *f;
	Point p, q;
	Image *im;

	if(t->lastshow == t->pressed)
		return;
	f = t->font->font;
	draw(t->screen, t->rect, t->image->image, nil, t->image->image->r.min);
	if(t->border > 0)
		border(t->screen, t->rect, t->border, t->bordercolor->image, ZP);
	/* text goes here */
	dx = 0;
	for(i=0; i<t->nline; i++){
		w = stringwidth(f, t->line[i]);
		if(dx < w)
			dx = w;
	}
	dy = t->nline*f->height;
	clipr = insetrect(t->rect, t->border);
	p = _ctlalignpoint(clipr, dx, dy, t->align);
	im = t->textcolor->image;
	if(t->pressed)
		im = t->pressedtextcolor->image;
	for(i=0; i<t->nline; i++){
		r.min = p;
		r.max.x = p.x+dx;
		r.max.y = p.y+f->height;
		q = _ctlalignpoint(r, stringwidth(f, t->line[i]), f->height, t->align%3);
		_string(t->screen, q, im,
			ZP, f, t->line[i], nil, strlen(t->line[i]),
			clipr, nil, ZP);
		p.y += f->height;
	}
	if(t->pressed)
		draw(t->screen, t->rect, t->light->image, t->mask->image, t->mask->image->r.min);
	t->lastshow = t->pressed;
	flushimage(display, 1);
}

static void
textbuttonctl(Control *c, char *str)
{
	int cmd, i;
	CParse cp;
	Rectangle r;
	Textbutton *t;

	t = (Textbutton*)c;
	cmd = _ctlparse(&cp, str, cmds);
	switch(cmd){
	default:
		ctlerror("%q: unrecognized message '%s'", t->name, cp.str);
		break;
	case EAlign:
		_ctlargcount(t, &cp, 2);
		t->align = _ctlalignment(cp.args[1]);
		t->lastshow = -1;	/* force redraw */
		break;
	case EBorder:
		_ctlargcount(t, &cp, 2);
		t->border = cp.iargs[1];
		t->lastshow = -1;	/* force redraw */
		break;
	case EBordercolor:
		_ctlargcount(t, &cp, 2);
		_setctlimage(t, &t->bordercolor, cp.args[1]);
		t->lastshow = -1;	/* force redraw */
		break;
	case EFocus:
		break;
	case EFont:
		_ctlargcount(t, &cp, 2);
		_setctlfont(t, &t->font, cp.args[1]);
		t->lastshow = -1;	/* force redraw */
		break;
	case EFormat:
		_ctlargcount(t, &cp, 2);
		t->format = ctlstrdup(cp.args[1]);
		break;
	case EImage:
		_ctlargcount(t, &cp, 2);
		_setctlimage(t, &t->image, cp.args[1]);
		t->lastshow = -1;	/* force redraw */
		break;
	case ELight:
		_ctlargcount(t, &cp, 2);
		_setctlimage(t, &t->light, cp.args[1]);
		t->lastshow = -1;	/* force redraw */
		break;
	case EMask:
		_ctlargcount(t, &cp, 2);
		_setctlimage(t, &t->mask, cp.args[1]);
		t->lastshow = -1;	/* force redraw */
		break;
	case EPressedtextcolor:
		_ctlargcount(t, &cp, 2);
		_setctlimage(t, &t->pressedtextcolor, cp.args[1]);
		t->lastshow = -1;	/* force redraw */
		break;
	case ERect:
		_ctlargcount(t, &cp, 5);
		r.min.x = cp.iargs[1];
		r.min.y = cp.iargs[2];
		r.max.x = cp.iargs[3];
		r.max.y = cp.iargs[4];
		if(Dx(r)<=0 || Dy(r)<=0)
			ctlerror("%q: bad rectangle: %s", t->name, cp.str);
		t->rect = r;
		t->lastshow = -1;	/* force redraw */
		break;
	case EShow:
		_ctlargcount(t, &cp, 1);
		t->lastshow = -1;	/* force redraw */
		textbuttonshow(t);
		break;
	case EText:
		/* free existing text */
		for(i=0; i<t->nline; i++)
			free(t->line[i]);
		t->nline = cp.nargs-1;
		t->line = ctlrealloc(t->line, t->nline*sizeof(char*));
		for(i=0; i<t->nline; i++)
			t->line[i] = ctlstrdup(cp.args[i+1]);
		t->lastshow = -1;	/* force redraw */
		textbuttonshow(t);
		break;
	case ETextcolor:
		_ctlargcount(t, &cp, 2);
		_setctlimage(t, &t->textcolor, cp.args[1]);
		t->lastshow = -1;	/* force redraw */
		break;
	case EValue:
		_ctlargcount(t, &cp, 2);
		if((cp.iargs[1]!=0) != t->pressed){
			t->pressed ^= 1;
			textbuttonshow(t);
		}
		break;
	}
}
