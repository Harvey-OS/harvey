enum{
	DBGEDIT		= 0xFFFF,
	DBGCUT		= 0x0001,
	DBGLAYOUT	= 0x0002,
	DBGSELECT	= 0x0004,
	DBGSHOW	= 0x0008,
	DBGCLEAN	= 0x0010,
	DBGCTL		= 0x0020,
	DBGTICK		= 0x0040,
	DBGMOUSE	= 0x0080,
	DBGPASTE	= 0x0100,
	DBGPREP		= 0x0200,
	DBGPERFORM	= 0x0400,
	DBGPOSITION	= 0x0800,
	DBGSTRING	= 0x1000,
	DBGUSER		= 0x2000,
};
extern int debug;

typedef struct Edit Edit;
typedef struct Selection Selection;
typedef struct Line Line;
typedef struct Mpos Mpos;
typedef struct Frag Frag;
typedef struct Context Context;

struct Context
{
	char		*name;
	CFont	*f;
	int		wordbreak;
	int		tabs[16];
	Context	*next;
};

enum				// Mpos states
{
	Outside	= 1,		// nowhere
	Eol		= 2,		// after end of line
	Bof		= 4,		// below last line
};

struct Mpos
{
	Point		p;		// location for cursor
	Position	pos;		// position before cursor
	Frag		*f;		// frag
	int		flags;	// see enum above
};

enum				// Selection state
{
	Drag1	= 0x01,	// which end being dragged
	Dragging	= 0x02,	// selection being made
	Set		= 0x04,	// selection made
	Enabled	= 0x08,	// Enabled for selecting
	Ticked	= 0x10,	// tick displayed
};

struct Selection
{
	int		state;
	Mpos	mpl, mpr;
	CImage	*color;	// If non-nill, button i selections allowed in this color
	Image	*tick;
	Image	*tickback;
	int		tickh;	// height of current tick image
};

struct Frag			// subset of a string that does not contain line break (or tab)
{
	Position	spos;	// start position of frag
	Position	epos;	// end position of frag
	Line		*l;		// line # Frag belongs to
	int		minx;	// left side of frag
	int		maxx;	// right side of frag
	Frag		*next;
};

struct Line
{
	Position	pos;		// position of first char (still) beyond end of line
	Frag	*	f;		// linked list of Frags in Line
	Rectangle	r;		// rect: Dx(r) = Dx(rplay) + Σ (f->maxx-f->minx)
	Rectangle rplay;	// rect to right of frags
	Line *	next;
};

struct Edit
{
	Control;
	int		border;
	Position	top;
	Position	bot;
	int		dirty;
	int		scroll;
	int		spacing;
	int		lastbut;
	Context *	curcontext;
	CImage *	image;
	CImage *	textcolor;
	CImage *	bordercolor;
	Line	*	lredraw;
	Selection	sel[3];		// Button i selection
	Stringset	*ss;			// Strings
	Line *	l;			// Lines on screen
};

extern Context *context;

#define fragr(f) Rect(f->minx, f->l->r.min.y, f->maxx, f->l->r.max.y)

/* string.c */
		/* internal */
int 		_posbefore(Position p1, Position p2);
int 		_posequal(Position p1, Position p2);
int 		_posdec(Stringset *ss, Position *pp);
int 		_posinc(Stringset *ss, Position *pp);
int 		_stringfindrune(Stringset *ss, Position *pp, Rune r);
int 		_stringfindrrune(Stringset *ss, Position *pp, Rune r);
void 		_stringspace(Stringset *s, int n);
void 		_posadjust(Undo* c, Position *p);
void 		_ssperform(Stringset *ss, Undo* c);
void 		_ssdelrange(Stringset *ss, Position p1, Position p2);
Position 	_insertrunes(Stringset *ss, Position p, Rune *rp, int nr);
Position 	_addonerune(Stringset *ss, Position pos, Rune rune);
Position 	_delonerune(Stringset *ss, Position pos);
String * 	_allocstring(char *c, Rune *r, int n);
void		_stringadd(Stringset *ss, String *s, int n);
Context * 	_newcontext(char *name);
Context * 	_contextnamed(char *s);

/* edit.c */
void		adjustselections(Edit *e, Undo *c);
void		adjustlines(Edit *e, Undo *c);

/* editdebug.c */
int		debugprint(int mask, char *fmt, ...);
void		dumpundo(Stringset *ss);
void		dumpselections(Edit *e);
void		dumpstrings(Stringset *ss);
void		dumplines(Edit *e);

/* editlayout.c */
void		layoutprep(Edit *e, Position p1, Position p2);
void		layoutrect(Edit *e);
Line *	allocline(void);
void		freefrag(Frag *frag);
void		freeline(Line *l);
Frag *	findfrag(Edit *e, Position p);
void		showfrag(Edit *e, Frag *f);
