#include <u.h>
#include <libc.h>
#include <thread.h>
#include <draw.h>
#include <mouse.h>
#include <keyboard.h>
#include <control.h>
#include "group.h"

typedef struct Tab Tab;

struct Tab {
	Control;
	int		border;
	int		selected;
	int		separation;
	char		*format;
	CImage	*bordercolor;
	CImage	*image;
	Control	*tabrow;
	Control	*tabstack;
	Control	*tabcolumn;
	int		ntabs;
	int		nbuttons;
	Control	**buttons;
};

enum{
	EAdd,
	EBorder,
	EBordercolor,
	EButton,
	EFocus,
	EFormat,
	EHide,
	EImage,
	ERect,
	EReveal,
	ESeparation,
	ESeparatorcolor,
	EShow,
	ESize,
	EValue,
};

static char *cmds[] = {
	[EAdd] =			"add",
	[EBorder] =		"border",
	[EBordercolor] =	"bordercolor",
	[EButton] =		"button",
	[EFocus] = 		"focus",
	[EFormat] = 		"format",
	[EHide] =			"hide",
	[EImage] =		"image",
	[ERect] =			"rect",
	[EReveal] =		"reveal",
	[ESeparation] =		"separation",
	[ESeparatorcolor] =	"separatorcolor",
	[EShow] =			"show",
	[ESize] =			"size",
	[EValue] =			"value",
};

static void
tabshow(Tab *t)
{
	int i;
	Rectangle r;
	Group *g;

	if (t->hidden)
		return;
	for(i=0; i<t->nbuttons; i++){
		_ctlprint(t->buttons[i], "value %d", (t->selected==i));
	}
	_ctlprint(t->tabstack, "reveal %d", t->selected);
	_ctlprint(t->tabcolumn, "show");
	if (t->selected < 0)
		return;
	g = (Group*)t->tabcolumn;
	if (g->nseparators == 0){
		return;
	}
	r = g->separators[0];
	r.min.x = t->buttons[t->selected]->rect.min.x;
	r.max.x = t->buttons[t->selected]->rect.max.x;
	draw(t->screen, r, t->image->image, nil, t->image->image->r.min);
	flushimage(display, 1);
}

static void
tabsize(Control *c)
{
	Tab *t = (Tab*)c;
	if (t->tabcolumn && t->tabcolumn->setsize)
		t->tabcolumn->setsize((Control*)t->tabcolumn);
}

static void
tabctl(Control *c, CParse *cp)
{
	int cmd, i;
	Control *cbut, *cwin;
	Tab *t;
	Rectangle r;

	t = (Tab*)c;
	cmd = _ctllookup(cp->args[0], cmds, nelem(cmds));
	switch(cmd){
	case EAdd:
		if ((cp->nargs & 1) == 0)
			ctlerror("%q: arg count: %s", t->name, cp->args[1]);
		for (i = 1; i < cp->nargs; i += 2){
			cbut = controlcalled(cp->args[i]);
			if (cbut == nil)
				ctlerror("%q: no such control: %s", t->name, cp->args[i]);
			cwin = controlcalled(cp->args[i+1]);
			if (cwin == nil)
				ctlerror("%q: no such control: %s", t->name, cp->args[i+1]);
			_ctladdgroup(t->tabrow, cbut);
			_ctlprint(t->tabstack, "add %q", cp->args[i+1]);
			_ctlprint(cbut, "format '%%s: %q button %%d'", t->name);
			controlwire(cbut, "event", t->controlset->ctl);
			t->buttons = ctlrealloc(t->buttons, (t->nbuttons+1)*sizeof(Control*));
			t->buttons[t->nbuttons] = cbut;
			t->nbuttons++;
			t->selected = -1;
		}
		_ctlprint(t->tabcolumn, "size");
		t->size = t->tabcolumn->size;
		break;
	case EBorder:
		_ctlargcount(t, cp, 2);
		if(cp->iargs[1] < 0)
			ctlerror("%q: bad border: %c", t->name, cp->str);
		t->border = cp->iargs[1];
		break;
	case EBordercolor:
		_ctlargcount(t, cp, 2);
		_setctlimage(t, &t->bordercolor, cp->args[1]);
		break;
	case EButton:
		_ctlargcount(t, cp, 2);
		if (cp->sender == nil)
			ctlerror("%q: senderless buttonevent: %q", t->name, cp->str);
		cbut = controlcalled(cp->sender);
		for(i=0; i<t->nbuttons; i++)
			if (cbut == t->buttons[i])
				break;
		if (i == t->nbuttons)
			ctlerror("%q: not my event: %q", t->name, cp->str);
		if(cp->iargs[1] == 0){
			/* button was turned off; turn it back on */
			_ctlprint(cbut, "value 1");
		}else{
			t->selected = i;
			if (t->format)
				chanprint(t->event, t->format, t->name, i);
			tabshow(t);
		}
		break;
	case EFocus:
		/* ignore focus change */
		break;
	case EFormat:
		_ctlargcount(t, cp, 2);
		t->format = ctlstrdup(cp->args[1]);
		break;
	case EImage:
		_ctlargcount(t, cp, 2);
		_setctlimage(t, &t->image, cp->args[1]);
		break;
	case ESeparation:
		t->tabrow->ctl(t->tabrow, cp);
		t->tabcolumn->ctl(t->tabcolumn, cp);
		break;
	case ERect:
		_ctlargcount(t, cp, 5);
		r.min.x = cp->iargs[1];
		r.min.y = cp->iargs[2];
		r.max.x = cp->iargs[3];
		r.max.y = cp->iargs[4];
		if(Dx(r)<=0 || Dy(r)<=0)
			ctlerror("%q: bad rectangle: %s", t->name, cp->str);
		t->rect = r;
		r = insetrect(r, t->border);
		_ctlprint(t->tabcolumn, "rect %R", r);
		break;
	case EReveal:
		_ctlargcount(t, cp, 1);
	case EHide:
	case ESize:
		t->tabcolumn->ctl(t->tabcolumn, cp);
		break;
	case EShow:
		tabshow(t);
		break;
	case EValue:
		_ctlargcount(t, cp, 2);
		if (cp->iargs[1] < 0 || cp->iargs[1] >= t->nbuttons)
			ctlerror("%q: illegal value '%s'", t->name, cp->str);
		t->selected = cp->iargs[1];
		tabshow(t);
		break;
	default:
		ctlerror("%q: unrecognized message '%s'", t->name, cp->str);
		break;
	}
}

static void
tabfree(Control*c)
{
	Tab *t = (Tab*)c;
	t->nbuttons = 0;
	free(t->buttons);
	t->buttons = 0;
}

static void
activatetab(Control *c, int act)
{
	Tab *t;

	t = (Tab*)c;
	if (act)
		activate(t->tabcolumn);
	else
		deactivate(t->tabcolumn);
}
	
Control *
createtab(Controlset *cs, char *name)
{
	char s[128];

	Tab *t;
	t = (Tab*)_createctl(cs, "tab", sizeof(Tab), name);
	t->selected = -1;
	t->nbuttons = 0;
	t->ctl = tabctl;
	t->mouse = nil;
	t->exit = tabfree;
	snprint(s, sizeof s, "_%s-row", name);
	t->tabrow = createrow(cs, s);
	snprint(s, sizeof s, "_%s-stack", name);
	t->tabstack = createstack(cs, s);
	snprint(s, sizeof s, "_%s-column", name);
	t->tabcolumn = createcolumn(cs, s);
	ctlprint(t->tabcolumn, "add %q %q", t->tabrow->name, t->tabstack->name);
	t->bordercolor = _getctlimage("black");
	t->image = _getctlimage("white");
	t->setsize = tabsize;
	t->activate = activatetab;
	return (Control*)t;
}
