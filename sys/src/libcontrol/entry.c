#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <mouse.h>
#include <keyboard.h>
#include <control.h>

typedef struct Entry Entry;

struct Entry
{
	Control;
	int		border;
	CFont	*font;
	CImage	*image;
	CImage	*textcolor;
	CImage	*bordercolor;
	Rune		*text;
	int		ntext;
	int		cursor;
	int		align;
	int		hasfocus;
	int		lastbut;
};

enum{
	EAlign,
	EBorder,
	EBordercolor,
	EData,
	EFocus,
	EFont,
	EFormat,
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
	[EData] = 			"data",
	[EFocus] = 		"focus",
	[EFont] =			"font",
	[EFormat] = 		"format",
	[EImage] =		"image",
	[ERect] =			"rect",
	[EShow] =			"show",
	[ETextcolor] =		"textcolor",
	[EValue] =			"value",
	nil
};

static void	entryctl(Control*, char*);
static void	entryshow(Entry*);
static void	entrykey(Entry*, Rune);
static void	entryfree(Entry*);
static Point	entrypoint(Entry*, int);
static void	entrysetpoint(Entry*, Point);

static void
entrythread(void *v)
{
	char buf[32];
	Entry *e;
	int i;

	e = v;

	snprint(buf, sizeof buf, "entry-%s-0x%p", e->name, e);
	threadsetname(buf);

	e->text = ctlmalloc(sizeof(Rune));
	e->ntext = 0;
	e->image = _getctlimage("white");
	e->textcolor = _getctlimage("black");
	e->bordercolor = _getctlimage("black");
	e->font = _getctlfont("font");
	e->format = ctlstrdup("%q: value %q");
	e->border = 0;

	for(;;){
		switch(alt(e->alts)){
		default:
			ctlerror("%q: unknown message", e->name);
		case AKey:
			for(i=0; e->kbdr[i]!=L'\0'; i++)
				entrykey(e, e->kbdr[i]);
			entryshow(e);
			break;
		case AMouse:
			if(e->m.buttons==1 && e->lastbut==0)
				entrysetpoint(e, e->m.xy);
			e->lastbut = e->m.buttons;
			break;
		case ACtl:
			_ctlcontrol(e, e->str, entryctl);
			free(e->str);
			break;
		case AWire:
			_ctlrewire(e);
			break;
		case AExit:
			entryfree(e);
			sendul(e->exit, 0);
			return;
		}
	}
}

Control*
createentry(Controlset *cs, char *name)
{
	return _createctl(cs, "entry", sizeof(Entry), name, entrythread, 0);
}

static void
entryfree(Entry *e)
{
	_putctlfont(e->font);
	_putctlimage(e->image);
	_putctlimage(e->textcolor);
	_putctlimage(e->bordercolor);
	free(e->text);
}

static Point
entrypoint(Entry *e, int c)
{
	Point p;
	Rectangle r;

	r = e->rect;
	if(e->border > 0)
		r = insetrect(r, e->border);
	p = _ctlalignpoint(r,
		runestringnwidth(e->font->font, e->text, e->ntext),
		e->font->font->height, e->align);
	if(c > e->ntext)
		c = e->ntext;
	p.x += runestringnwidth(e->font->font, e->text, c);
	return p;
}

static void
entrysetpoint(Entry *e, Point cp)
{
	Point p;
	int i;

	if(!ptinrect(cp, insetrect(e->rect, e->border)))
		return;
	p = entrypoint(e, 0);
	for(i=0; i<e->ntext; i++){
		p.x += runestringnwidth(e->font->font, e->text+i, 1);
		if(p.x > cp.x)
			break;
	}
	e->cursor = i;
	entryshow(e);
}

static void
entryshow(Entry *e)
{
	Rectangle r, dr;
	Point p;

	r = e->rect;
	draw(e->screen, r, e->image->image, nil, e->image->image->r.min);
	if(e->border > 0){
		border(e->screen, r, e->border, e->bordercolor->image, e->bordercolor->image->r.min);
		dr = insetrect(r, e->border);
	}else
		dr = r;
	p = entrypoint(e, 0);
	_string(e->screen, p, e->textcolor->image,
		ZP, e->font->font, nil, e->text, e->ntext,
		dr, nil, ZP);
	if(e->hasfocus){
		p = entrypoint(e, e->cursor);
		r.min = p;
		r.max.x = p.x+1;
		r.max.y = p.y+e->font->font->height;
		if(rectclip(&r, dr))
			draw(e->screen, r, e->textcolor->image, nil, ZP);
	}
	flushimage(display, 1);
}

static void
entryctl(Control *c, char *str)
{
	int cmd;
	CParse cp;
	Rectangle r;
	Entry *e;
	Rune *rp;

	e = (Entry*)c;
	cmd = _ctlparse(&cp, str, cmds);
	switch(cmd){
	default:
		ctlerror("%q: unrecognized message '%s'", e->name, cp.str);
		break;
	case EAlign:
		_ctlargcount(e, &cp, 2);
		e->align = _ctlalignment(cp.args[1]);
		break;
	case EBorder:
		_ctlargcount(e, &cp, 2);
		if(cp.iargs[1] < 0)
			ctlerror("%q: bad border: %c", e->name, cp.str);
		e->border = cp.iargs[1];
		break;
	case EBordercolor:
		_ctlargcount(e, &cp, 2);
		_setctlimage(e, &e->bordercolor, cp.args[1]);
		break;
	case EData:
		_ctlargcount(e, &cp, 1);
		printctl(e->data, "%S", e->text);
		break;
	case EFocus:
		_ctlargcount(e, &cp, 2);
		e->hasfocus = cp.iargs[1];
		e->lastbut = 0;
		entryshow(e);
		break;
	case EFont:
		_ctlargcount(e, &cp, 2);
		_setctlfont(e, &e->font, cp.args[1]);
		break;
	case EFormat:
		_ctlargcount(e, &cp, 2);
		e->format = ctlstrdup(cp.args[1]);
		break;
	case EImage:
		_ctlargcount(e, &cp, 2);
		_setctlimage(e, &e->image, cp.args[1]);
		break;
	case ERect:
		_ctlargcount(e, &cp, 5);
		r.min.x = cp.iargs[1];
		r.min.y = cp.iargs[2];
		r.max.x = cp.iargs[3];
		r.max.y = cp.iargs[4];
		if(Dx(r)<=0 || Dy(r)<=0)
			ctlerror("%q: bad rectangle: %s", e->name, cp.str);
		e->rect = r;
		break;
	case EShow:
		_ctlargcount(e, &cp, 1);
		entryshow(e);
		break;
		_ctlargcount(e, &cp, 2);
		_setctlimage(e, &e->textcolor, cp.args[1]);
		break;
	case ETextcolor:
		_ctlargcount(e, &cp, 2);
		_setctlimage(e, &e->textcolor, cp.args[1]);
		break;
	case EValue:
		_ctlargcount(e, &cp, 2);
		rp = _ctlrunestr(cp.args[1]);
		if(runestrcmp(rp, e->text) != 0){
			free(e->text);
			e->text = rp;
			e->ntext = runestrlen(e->text);
			entryshow(e);
		}else
			free(rp);
		break;
	}
}

static void
entrykey(Entry *e, Rune r)
{
	Rune *s;
	int n;
	char *p;

	switch(r){
	default:
		e->text = ctlrealloc(e->text, (e->ntext+1+1)*sizeof(Rune));
		memmove(e->text+e->cursor+1, e->text+e->cursor,
			(e->ntext+1-e->cursor)*sizeof(Rune));
		e->text[e->cursor++] = r;
		e->ntext++;
		break;
	case L'\n':	/* newline: return value */
		p = _ctlstrrune(e->text);
		printctl(e->event, e->format, e->name, p);
		free(p);
		return;
	case L'\b':
		if(e->cursor > 0){
			memmove(e->text+e->cursor-1, e->text+e->cursor,
				(e->ntext+1-e->cursor)*sizeof(Rune));
			e->cursor--;
			e->ntext--;
		}
		break;
	case 0x15:	/* control U: kill line */
		e->cursor = 0;
		e->ntext = 0;
		break;
	case 0x16:	/* control V: paste (append snarf buffer) */
		s = _ctlgetsnarf();
		if(s != nil){
			n = runestrlen(s);
			e->text = ctlrealloc(e->text, (e->ntext+n+1)*sizeof(Rune));
			memmove(e->text+e->cursor+n, e->text+e->cursor,
				(e->ntext+1-e->cursor)*sizeof(Rune));
			memmove(e->text+e->cursor, s, n*sizeof(Rune));
			e->cursor += n;
			e->ntext += n;
		}
		break;
	}
	e->text[e->ntext] = L'\0';
}
