#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <mouse.h>
#include <keyboard.h>
#include <control.h>

static int debug = 0;

typedef struct Text Text;

struct Text
{
	Control;
	int		border;
	int		topline;
	int		scroll;
	int		nvis;
	int		lastbut;
	CFont	*font;
	CImage	*image;
	CImage	*textcolor;
	CImage	*bordercolor;
	CImage	*selectcolor;
	CImage	*selectingcolor;
	Rune		**line;
	int		selectmode;	// Selsingle, Selmulti
	int		selectstyle;	// Seldown, Selup (use Selup only with Selsingle)
	uchar	*selected;
	int		nline;
	int		warp;
	int		align;
	int		sel;		// line nr of selection made by last button down
	int		but;		// last button down (still being hold)
	int		offsel;	// we are on selection
};

enum
{
	Selsingle,
	Selmulti,
	Seldown,
	Selup,
};

enum{
	EAccumulate,
	EAdd,
	EAlign,
	EBorder,
	EBordercolor,
	EClear,
	EDelete,
	EFocus,
	EFont,
	EHide,
	EImage,
	ERect,
	EReplace,
	EReveal,
	EScroll,
	ESelect,
	ESelectcolor,
	ESelectingcolor,
	ESelectmode,
	ESelectstyle,
	EShow,
	ESize,
	ETextcolor,
	ETopline,
	EValue,
	EWarp,
};

static char *cmds[] = {
	[EAccumulate] =	"accumulate",
	[EAdd] =			"add",
	[EAlign] =			"align",
	[EBorder] =		"border",
	[EBordercolor] =	"bordercolor",
	[EClear] =			"clear",
	[EDelete] =		"delete",
	[EFocus] = 		"focus",
	[EFont] =			"font",
	[EHide] =			"hide",
	[EImage] =		"image",
	[ERect] =			"rect",
	[EReplace] =		"replace",
	[EReveal] =		"reveal",
	[EScroll] =			"scroll",
	[ESelect] =		"select",
	[ESelectcolor] =	"selectcolor",
	[ESelectingcolor] =	"selectingcolor",
	[ESelectmode] =	"selectmode",
	[ESelectstyle] =		"selectstyle",
	[EShow] =			"show",
	[ESize] =			"size",
	[ETextcolor] =		"textcolor",
	[ETopline] =		"topline",
	[EValue] =			"value",
	[EWarp] =			"warp",
	nil
};

static void	textshow(Text*);
static void	texttogglei(Text*, int);
static int	textline(Text*, Point);
static int	texttoggle(Text*, Point);

static void
textmouse(Control *c, Mouse *m)
{
	Text *t;
	int sel;

	t = (Text*)c;
	if (debug) fprint(2, "textmouse %s t->lastbut %d; m->buttons %d\n", t->name, t->lastbut, m->buttons);
	if (t->warp >= 0)
		return;
	if ((t->selectstyle == Selup) && (m->buttons&7)) {
		sel = textline(t, m->xy);
		if (t->sel >= 0) {
//			if (debug) fprint(2, "textmouse Selup %q sel=%d t->sel=%d t->but=%d\n",
//						t->name, sel, t->sel, t->but);
			t->offsel = (sel == t->sel) ? 0 : 1;
			if ((sel == t->sel &&
				    ((t->selected[t->sel] && !t->but) ||
				     ((!t->selected[t->sel]) && t->but))) ||
			    (sel != t->sel &&
				     ((t->selected[t->sel] && t->but) ||
                                         ((!t->selected[t->sel]) && (!t->but))))) {
				texttogglei(t, t->sel);
			}
		}
	}
	if(t->lastbut != (m->buttons&7)){
		if(m->buttons & 7){
			sel = texttoggle(t, m->xy);
			if(sel >= 0) {
				if (t->selectstyle == Seldown) {
					chanprint(t->event, "%q: select %d %d",
						t->name, sel, t->selected[sel] ? (m->buttons & 7) : 0);
					if (debug) fprint(2, "textmouse Seldown event %q: select %d %d\n",
						t->name, sel, t->selected[sel] ? (m->buttons & 7) : 0);
				} else {
					if (debug) fprint(2, "textmouse Selup no event yet %q: select %d %d\n",
						t->name, sel, t->selected[sel] ? (m->buttons & 7) : 0);
					t->sel = sel;
					t->but =  t->selected[sel] ? (m->buttons & 7) : 0;
				}
			}
		} else if (t->selectstyle == Selup) {
			sel = textline(t, m->xy);
			t->offsel = 0;
			if ((sel >= 0) && (sel == t->sel)) {
				chanprint(t->event, "%q: select %d %d",
					t->name, sel, t->but);
				if (debug) fprint(2, "textmouse Selup event %q: select %d %d\n",
					t->name, sel, t->but);
			} else if (sel != t->sel) {
				if  ((t->selected[t->sel] && t->but) ||
                                         ((!t->selected[t->sel]) && (!t->but))) {
					texttogglei(t, t->sel);
				} else {
					textshow(t);
				}
				if (debug) fprint(2, "textmouse Selup cancel %q: select %d %d\n",
					t->name, sel, t->but);
			}
			t->sel = -1;
			t->but = 0;
		}
		t->lastbut = m->buttons & 7;
	}
}

static void
textfree(Control *c)
{
	int i;
	Text *t;

	t = (Text*)c;
	_putctlfont(t->font);
	_putctlimage(t->image);
	_putctlimage(t->textcolor);
	_putctlimage(t->bordercolor);
	_putctlimage(t->selectcolor);
	_putctlimage(t->selectingcolor);
	for(i=0; i<t->nline; i++)
		free(t->line[i]);
	free(t->line);
	free(t->selected);
}

static void
textshow(Text *t)
{
	Rectangle r, tr;
	Point p;
	int i, ntext;
	Font *f;
	Rune *text;

	if (t->hidden)
		return;
	r = t->rect;
	f = t->font->font;
	draw(t->screen, r, t->image->image, nil, t->image->image->r.min);
	if(t->border > 0){
		border(t->screen, r, t->border, t->bordercolor->image, t->bordercolor->image->r.min);
		r = insetrect(r, t->border);
	}
	tr = r;
	t->nvis = Dy(r)/f->height;
	for(i=t->topline; i<t->nline && i<t->topline+t->nvis; i++){
		text = t->line[i];
		ntext = runestrlen(text);
		r.max.y = r.min.y+f->height;
		if(t->sel == i && t->offsel)
			draw(t->screen, r, t->selectingcolor->image, nil, ZP);
		else if(t->selected[i])
			draw(t->screen, r, t->selectcolor->image, nil, ZP);
		p = _ctlalignpoint(r,
			runestringnwidth(f, text, ntext),
			f->height, t->align);
		if(t->warp == i) {
			Point p2;
			 p2.x = p.x + 0.5*runestringnwidth(f, text, ntext);
			 p2.y = p.y + 0.5*f->height;
			moveto(t->controlset->mousectl, p2);
			t->warp = -1;
		}
		_string(t->screen, p, t->textcolor->image,
			ZP, f, nil, text, ntext, tr,
			nil, ZP, SoverD);
		r.min.y += f->height;
	}
	flushimage(display, 1);
}

static void
textctl(Control *c, CParse *cp)
{
	int cmd, i, n;
	Rectangle r;
	Text *t;
	Rune *rp;

	t = (Text*)c;
	cmd = _ctllookup(cp->args[0], cmds, nelem(cmds));
	switch(cmd){
	default:
		ctlerror("%q: unrecognized message '%s'", t->name, cp->str);
		break;
	case EAlign:
		_ctlargcount(t, cp, 2);
		t->align = _ctlalignment(cp->args[1]);
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
	case EClear:
		_ctlargcount(t, cp, 1);
		for(i=0; i<t->nline; i++)
			free(t->line[i]);
		free(t->line);
		free(t->selected);
		t->line = ctlmalloc(sizeof(Rune*));
		t->selected = ctlmalloc(1);
		t->nline = 0;
		textshow(t);
		break;
	case EDelete:
		_ctlargcount(t, cp, 2);
		i = cp->iargs[1];
		if(i<0 || i>=t->nline)
			ctlerror("%q: line number out of range: %s", t->name, cp->str);
		free(t->line[i]);
		memmove(t->line+i, t->line+i+1, (t->nline-(i+1))*sizeof(Rune*));
		memmove(t->selected+i, t->selected+i+1, t->nline-(i+1));
		t->nline--;
		textshow(t);
		break;
	case EFocus:
		break;
	case EFont:
		_ctlargcount(t, cp, 2);
		_setctlfont(t, &t->font, cp->args[1]);
		break;
	case EHide:
		_ctlargcount(t, cp, 1);
		t->hidden = 1;
		break;
	case EImage:
		_ctlargcount(t, cp, 2);
		_setctlimage(t, &t->image, cp->args[1]);
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
		t->nvis = (Dy(r)-2*t->border)/t->font->font->height;
		break;
	case EReplace:
		_ctlargcount(t, cp, 3);
		i = cp->iargs[1];
		if(i<0 || i>=t->nline)
			ctlerror("%q: line number out of range: %s", t->name, cp->str);
		free(t->line[i]);
		t->line[i] = _ctlrunestr(cp->args[2]);
		textshow(t);
		break;
	case EReveal:
		_ctlargcount(t, cp, 1);
		t->hidden = 0;
		textshow(t);
		break;
	case EScroll:
		_ctlargcount(t, cp, 2);
		t->scroll = cp->iargs[1];
		break;
	case ESelect:
		if(cp->nargs!=2 && cp->nargs!=3)
	badselect:
			ctlerror("%q: bad select message: %s", t->name, cp->str);
		if(cp->nargs == 2){
			if(strcmp(cp->args[1], "all") == 0){
				memset(t->selected, 1, t->nline);
				break;
			}
			if(strcmp(cp->args[1], "none") == 0){
				memset(t->selected, 0, t->nline);
				break;
			}
			if(cp->args[1][0]<'0' && '9'<cp->args[1][0])
				goto badselect;
			texttogglei(t, cp->iargs[1]);
			break;
		}
		if(cp->iargs[1]<0 || cp->iargs[1]>=t->nline)
			ctlerror("%q: selection index out of range (nline %d): %s", t->name, t->nline, cp->str);
		if(t->selected[cp->iargs[1]] != (cp->iargs[2]!=0))
			texttogglei(t, cp->iargs[1]);
		break;
	case ESelectcolor:
		_ctlargcount(t, cp, 2);
		_setctlimage(t, &t->selectcolor, cp->args[1]);
		break;
	case ESelectmode:
		_ctlargcount(t, cp, 2);
		if(strcmp(cp->args[1], "single") == 0)
			t->selectmode = Selsingle;
		else if(strncmp(cp->args[1], "multi", 5) == 0)
			t->selectmode = Selmulti;
		break;
	case ESelectstyle:
		_ctlargcount(t, cp, 2);
		 if(strcmp(cp->args[1], "down") == 0)
			t->selectstyle = Seldown;
		else if(strcmp(cp->args[1], "up") == 0)
			t->selectstyle = Selup;
		break;
	case EShow:
		_ctlargcount(t, cp, 1);
		textshow(t);
		break;
	case ESize:
		if (cp->nargs == 3)
			r.max = Pt(10000, 10000);
		else{
			_ctlargcount(t, cp, 5);
			r.max.x = cp->iargs[3];
			r.max.y = cp->iargs[4];
		}
		r.min.x = cp->iargs[1];
		r.min.y = cp->iargs[2];
		if(r.min.x<=0 || r.min.y<=0 || r.max.x<=0 || r.max.y<=0 || r.max.x < r.min.x || r.max.y < r.min.y)
			ctlerror("%q: bad sizes: %s", t->name, cp->str);
		t->size.min = r.min;
		t->size.max = r.max;
		break;
	case ETextcolor:
		_ctlargcount(t, cp, 2);
		_setctlimage(t, &t->textcolor, cp->args[1]);
		break;
	case ETopline:
		_ctlargcount(t, cp, 2);
		i = cp->iargs[1];
		if(i < 0)
			i = 0;
		if(i > t->nline)
			i = t->nline;
		if(t->topline != i){
			t->topline = i;
			textshow(t);
		}
		break;
	case EValue:
		/* set contents to single line */
		/* free existing text and fall through to add */
		for(i=0; i<t->nline; i++){
			free(t->line[i]);
			t->line[i] = nil;
		}
		t->nline = 0;
		t->topline = 0;
		/* fall through */
	case EAccumulate:
	case EAdd:
		switch (cp->nargs) {
		default:
			ctlerror("%q: wrong argument count in '%s'", t->name, cp->str);
		case 2:
			n = t->nline;
			break;
		case 3:
			n = cp->iargs[1];
			if(n<0 || n>t->nline)
				ctlerror("%q: line number out of range: %s", t->name, cp->str);
			break;
		}
		rp = _ctlrunestr(cp->args[cp->nargs-1]);
		t->line = ctlrealloc(t->line, (t->nline+1)*sizeof(Rune*));
		memmove(t->line+n+1, t->line+n, (t->nline-n)*sizeof(Rune*));
		t->line[n] = rp;
		t->selected = ctlrealloc(t->selected, t->nline+1);
		memmove(t->selected+n+1, t->selected+n, t->nline-n);
		t->selected[n] = (t->selectmode==Selmulti && cmd!=EAccumulate);
		t->nline++;
		if(t->scroll) {
			if(n > t->topline + (t->nvis - 1)){
				t->topline = n - (t->nvis - 1);
				if(t->topline < 0)
					t->topline = 0;
			}
			if(n < t->topline)
				t->topline = n;
		}
		if(cmd != EAccumulate)
			if(t->scroll || t->nline<=t->topline+t->nvis)
				textshow(t);
		break;
	case EWarp:
		_ctlargcount(t, cp, 2);
		i = cp->iargs[1];
		if(i <0 || i>=t->nline)
			ctlerror("%q: selection index out of range (nline %d): %s", t->name, t->nline, cp->str);
		if(i < t->topline || i >=  t->topline+t->nvis){
			t->topline = i;
		}
		t->warp = cp->iargs[1];
		textshow(t);
		t->warp = -1;
		break;
	}
}

static void
texttogglei(Text *t, int i)
{
	int prev;

	if(t->selectmode == Selsingle){
		/* clear the others */
		prev = t->selected[i];
		memset(t->selected, 0, t->nline);
		t->selected[i] = prev;
	}
	t->selected[i] ^= 1;
	textshow(t);
}

static int
textline(Text *t, Point p)
{
	Rectangle r;
	int i;

	r = t->rect;
	if(t->border > 0)
		r = insetrect(r, t->border);
	if(!ptinrect(p, r))
		return -1;
	i = (p.y-r.min.y)/t->font->font->height;
	i += t->topline;
	if(i >= t->nline)
		return -1;
	return i;
}

static int
texttoggle(Text *t, Point p)
{
	int i;

	i = textline(t, p);
	if (i >= 0)
		texttogglei(t, i);
	return i;
}

Control*
createtext(Controlset *cs, char *name)
{
	Text *t;

	t = (Text*)_createctl(cs, "text", sizeof(Text), name);
	t->line = ctlmalloc(sizeof(Rune*));
	t->selected = ctlmalloc(1);
	t->nline = 0;
	t->image = _getctlimage("white");
	t->textcolor = _getctlimage("black");
	t->bordercolor = _getctlimage("black");
	t->selectcolor = _getctlimage("yellow");
	t->selectingcolor = _getctlimage("paleyellow");
	t->font = _getctlfont("font");
	t->selectmode = Selsingle;
	t->selectstyle = Selup; // Seldown;
	t->lastbut = 0;
	t->mouse = textmouse;
	t->ctl = textctl;
	t->exit = textfree;
	t->warp = -1;
	t->sel = -1;
	t->offsel = 0;
	t->but = 0;
	return (Control *)t;
}
