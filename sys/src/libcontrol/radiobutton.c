#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <mouse.h>
#include <keyboard.h>
#include <control.h>

typedef struct Radio Radio;

struct Radio
{
	Control;
	int		value;
	int		lastbut;
	Control	**buttons;
	int		nbuttons;
	char		*eventstr;
};

enum{
	EAdd,
	EFocus,
	EFormat,
	ERect,
	EShow,
	EValue,
};

static char *cmds[] = {
	[EAdd] =		"add",
	[EFocus] = 	"focus",
	[EFormat] = 	"format",
	[ERect] =		"rect",
	[EShow] =		"show",
	[EValue] =		"value",
	nil
};

static void	radioctl(Control*, char*);
static void	radioshow(Radio*);
static void	radiosubevent(Radio*, int, char*);
static void	radiofree(Radio*);

static void
radiothread(void *v)
{
	char buf[32];
	Radio *r;
	int i, a;

	r = v;

	snprint(buf, sizeof buf, "radiobutton-%s-0x%p", r->name, r);
	threadsetname(buf);

	r->format = ctlstrdup("%q: value %d");
	r->value = -1;	/* nobody set at first */

	for(;;){
		switch(a = alt(r->alts)){
		default:
			a -= NALT;
			if(a < 0 || a >= r->nbuttons)
				ctlerror("%q: unknown alt index %d", r->name, a+NALT);
			/* event received on button #a */
			radiosubevent(r, a, r->eventstr);
			break;
		case AKey:
			/* ignore keystrokes */
			break;
		case AMouse:
			for(i=0; i<r->nbuttons; i++)
				if(ptinrect(r->m.xy, r->buttons[i]->rect)){
					send(r->buttons[i]->mouse, &r->m);
					break;
				}
			break;
		case ACtl:
			_ctlcontrol(r, r->str, radioctl);
			free(r->str);
			break;
		case AWire:
			_ctlrewire(r);
			break;
		case AExit:
			radiofree(r);
			sendul(r->exit, 0);
			return;
		}
	}
}

Control*
createradiobutton(Controlset *cs, char *name)
{
	return _createctl(cs, "label", sizeof(Radio), name, radiothread, 0);
}

static void
radiofree(Radio*)
{
}

static void
radioshow(Radio *r)
{
	int i;

	for(i=0; i<r->nbuttons; i++)
		printctl(r->buttons[i]->ctl, "value %d\nshow", (r->value==i));
}

static void
radioctl(Control *c, char *str)
{
	int cmd;
	CParse cp;
	Rectangle rect;
	Radio *r;

	r = (Radio*)c;

	cmd = _ctlparse(&cp, str, cmds);
	switch(cmd){
	default:
		ctlerror("%q: unrecognized message '%s'", r->name, cp.str);
		break;
	case EAdd:
		_ctlargcount(r, &cp, 2);
		c = controlcalled(cp.args[1]);
		if(c == nil)
			ctlerror("%q: can't add %s: %r", r->name, cp.args[1]);
		r->buttons = ctlrealloc(r->buttons, (r->nbuttons+1)*sizeof(Control*));
		r->buttons[r->nbuttons] = c;
		r->alts = ctlrealloc(r->alts, (NALT+r->nbuttons+1+1)*sizeof(Alt));
		r->alts[NALT+r->nbuttons].c = c->event;
		r->alts[NALT+r->nbuttons].v = &r->eventstr;
		r->alts[NALT+r->nbuttons].op = CHANRCV;
		r->nbuttons++;
		r->alts[NALT+r->nbuttons].op = CHANEND;
		r->value = -1;
		radioshow(r);
		break;
	case EFormat:
		_ctlargcount(r, &cp, 2);
		r->format = ctlstrdup(cp.args[1]);
		break;
	case EFocus:
		/* ignore focus change */
		break;
	case ERect:
		_ctlargcount(r, &cp, 5);
		rect.min.x = cp.iargs[1];
		rect.min.y = cp.iargs[2];
		rect.max.x = cp.iargs[3];
		rect.max.y = cp.iargs[4];
		r->rect = rect;
		break;
	case EShow:
		_ctlargcount(r, &cp, 1);
		radioshow(r);
		break;
	case EValue:
		_ctlargcount(r, &cp, 2);
		r->value = cp.iargs[1];
		radioshow(r);
		break;
	}
}

static void
radiosubevent(Radio *r, int b, char *s)
{
	int cmd;
	CParse cp;

	cmd = _ctlparse(&cp, s, cmds);
	switch(cmd){
	default:
		/* ignore */
		break;
	case EValue:
		_ctlargcount(r, &cp, 2);
		if(cp.iargs[1] == 0){
			/* button was turned off; turn it back on */
			printctl(r->buttons[b]->ctl, "value 1");
		}else{
			r->value = b;
			printctl(r->event, r->format, r->name, b);
		}
		radioshow(r);
		break;
	}
	free(s);
}
