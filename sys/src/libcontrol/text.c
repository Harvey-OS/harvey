#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <mouse.h>
#include <keyboard.h>
#include <control.h>

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
	Rune		**line;
	int		selectmode;
	uchar	*selected;
	int		nline;
	int		align;
};

enum
{
	Selsingle,
	Selmulti,
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
	ESelectmode,
	EShow,
	ESize,
	ETextcolor,
	ETopline,
	EValue,
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
	[ESelectmode] =	"selectmode",
	[EShow] =			"show",
	[ESize] =			"size",
	[ETextcolor] =		"textcolor",
	[ETopline] =		"topline",
	[EValue] =			"value",
	nil
};

static void	textshow(Text*);
static void	texttogglei(Text*, int);
static int	texttoggle(Text*, Point);

static void
textmouse(Control *c, Mouse *m)
{
	Text *t;
	int sel;

	t = (Text*)c;
	if(t->lastbut != (m->buttons&7)){
		if(m->buttons & 7){
			sel = texttoggle(t, m->xy);
			if(sel >= 0)
				chanprint(t->event, "%q: select %d %d",
					t->name, sel, t->selected[sel] ? (m->buttons & 7) : 0);
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
		if(t->selected[i])
			draw(t->screen, r, t->selectcolor->image, nil, ZP);
		p = _ctlalignpoint(r,
			runestringnwidth(f, text, ntext),
			f->height, t->align);
		_string(t->screen, p, t->textcolor->image,
			ZP, f, nil, text, ntext, tr,
			nil, ZP);
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
texttoggle(Text *t, Point p)
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
	t->font = _getctlfont("font");
	t->selectmode = Selsingle;
	t->lastbut = 0;
	t->mouse = textmouse;
	t->ctl = textctl;
	t->exit = textfree;
	return (Control *)t;
}
