/*
 * Here are some conventions about  our Tk widgets.
 *
 * When a widget is packed, its act geom record is
 * set so that act.{x,y} is the vector from the containing
 * widget's origin to the position of this widget.  The position
 * is the place just outside the top-left border.  The origin
 * is the place just inside the top-left border.
 * act.{width,height} gives the allocated dimensions inside
 * the border --- it will be the requested width or height
 * plus ipad{x,y} plus any filling done by the packer.
 *
 * The tkposn function returns the origin of its argument
 * widget, expressed in absolute screen coordinates.
 *
 * The actual drawing contents of the widget should be
 * drawn at an internal origin that is the widget's origin
 * plus the ipad vector.
 */

/*
 * event printing
 */
#pragma	varargck	type	"v"	int
#pragma	varargck	argpos	tkwreq	2

enum
{
	TKframe,		/* Widget type */
	TKlabel,
	TKcheckbutton,
	TKbutton,
	TKmenubutton,
	TKmenu,
	TKseparator,
	TKcascade,
	TKlistbox,
	TKscrollbar,
	TKtext,
	TKcanvas,
	TKentry,
	TKradiobutton,
	TKscale,
	TKpanel,
	TKchoicebutton,
	TKwidgets,

	TKsingle	= 0,	/* Select mode */
	TKbrowse,
	TKmultiple,
	TKextended,

	TKraised,		/* Relief */
	TKsunken,
	TKflat,
	TKgroove,
	TKridge,

	TkArepl		= 0,	/* Bind options */
	TkAadd,
	TkAsub,

	Tkvertical	= 0,	/* Scroll bar orientation */
	Tkhorizontal,

	Tkwrapnone	= 0,	/* Wrap mode */
	Tkwrapword,
	Tkwrapchar,

	TkDdelivered	= 0,	/* Event Delivery results */
	TkDbreak,
	TkDnone,

	TkLt = 0,		/* Comparison operators */
	TkLte,
	TkEq,
	TkGte,
	TkGt,
	TkNeq,

	
	OPTdist		= 0,	/* Distance; aux is offset of TkEnv* */
	OPTstab,		/* String->Constant table; aux is TkStab array */
	OPTtext,		/* Text string */
	OPTwinp,		/* Window Path to Tk ptr */
	OPTflag,		/* Option sets bitmask; aux is TkStab array*/
	OPTbmap,		/* Option specifies bitmap file */
	OPTbool,		/* Set to one if option present */
	OPTfont,		/* Env font */
	OPTfrac,		/* list of fixed point distances (count in aux) */
	OPTnnfrac,	/* non-negative fixed point distance */
	OPTctag,		/* Tag list for canvas item */
	OPTtabs,		/* Tab stops; aux is offset of TkEnv* */
	OPTcolr,		/* Colors; aux is index into colour table */
	OPTimag,		/* Image */
	OPTsize,		/* width/height; aux is offset of TkEnv* */
	OPTnndist,		/* Non-negative distance */
	OPTact,		/* actx or acty */
	OPTignore,	/* ignore this option */
	OPTsticky,	/* sticky (any comb. of chars n, s, e, w) */
	OPTlist,		/* list of text values */
	OPTflags,		/* more than one OPTflag */

	BoolX		= 0,
	BoolT,
	BoolF,

	Tkmaxitem	= 128,
	Tkminitem	= 32,
	Tkcvstextins	= 1024,
	Tkmaxdump	= 1024,
	Tkentryins	= 128,
	TkMaxmsgs	= 100,
	Tkshdelta	= 0x40,	/* color intensity delta for shading */

	Tkfpscalar	= 10000,	/* Fixed point scale factor */

	Tkdpi		= 100,	/* pixels per inch on an average display */

	Tksweep		= 64,	/* binds before a sweep */

	TkColcachesize = 128,	/* colours cached per context (display) */

	TkStatic	= 0,
	TkDynamic	= 1,

	TkRptclick		= 100,	/* granularity */
	TkRptpause	= 300,	/* autorepeat inital delay */
	TkRptinterval	= 100,	/* autorepeat interval */

	TkBlinkinterval	= 500
};

#define TKSTRUCTALIGN	4
#define TKI2F(i)	((i)*Tkfpscalar)
extern	int TKF2I(int);
/*#define TKF2I(f)	(((f) + Tkfpscalar/2)/Tkfpscalar)*/
#define IAUX(i)		((void*)i)
#define AUXI(i)		((int)i)
#define TKKEY(i)	((i)&0xFFFF)

typedef struct Tk Tk;
typedef struct TkCol TkCol;
typedef struct TkCtxt TkCtxt;
typedef struct TkEnv TkEnv;
typedef struct TkName TkName;
typedef struct TkTtabstop TkTtabstop;
typedef struct TkOptab TkOptab;
typedef struct TkOption TkOption;
typedef struct TkStab TkStab;
typedef struct TkTop TkTop;
typedef struct TkGeom TkGeom;
typedef struct TkMethod TkMethod;
typedef struct TkAction TkAction;
typedef struct TkWin TkWin;
typedef struct TkCmdtab TkCmdtab;
typedef struct TkMemimage TkMemimage;
typedef struct TkMouse TkMouse;
typedef struct TkVar TkVar;
typedef struct TkMsg TkMsg;
typedef struct TkEbind TkEbind;
typedef struct TkImg TkImg;
typedef struct TkPanelimage TkPanelimage;
typedef struct TkWinfo TkWinfo;
typedef struct TkCursor TkCursor;
typedef struct TkGrid TkGrid;
typedef struct TkGridbeam TkGridbeam;
typedef struct TkGridcell TkGridcell;

#pragma incomplete TkCol

struct TkImg
{
	TkTop*	top;
	int	ref;
	int	type;
	int	w;
	int	h;
	TkEnv*	env;
	Image*	img;
	TkImg*	link;
	TkName*	name;
	char *cursor;
};

struct TkCursor
{
	int	def;
	Point	p;
	Image*	bit;
	TkImg*	img;
};

struct TkEbind
{
	int	event;
	char*	cmd;
};

struct TkMsg
{
	TkVar*	var;
	TkMsg*	link;
	char	msg[TKSTRUCTALIGN];
};

enum
{
	TkVchan		= 1,
	TkVstring,
};

struct TkVar
{
	int	type;
	TkVar*	link;
	void*	value;
	char	name[TKSTRUCTALIGN];
};

struct TkPanelimage
{
	void*		image;		/* Image paired with Draw_Image: see lookupimage in libinterp/draw.c */
	int			ref;
	TkPanelimage*	link;
};

struct TkMouse
{
	int		x;
	int		y;
	int		b;
};

struct TkMemimage
{
	Rectangle	r;
	ulong	chans;
	uchar*	data;
};

struct TkCmdtab
{
	char*		name;
	char*		(*fn)(Tk*, char*, char**);
};

struct TkWinfo
{
	Tk*		w;
	Rectangle	r;
};

struct TkAction
{
	int		event;
	int		type;
	char*		arg;
	TkAction*	link;
};

struct TkStab
{
	char*		val;
	int		con;
};

struct TkOption
{
	char*		o;
	int		type;
	int		offset;
	void*		aux;
};

struct TkOptab
{
	void*		ptr;
	TkOption*	optab;	
};

enum
{
	/* Widget Control Bits */
	Tktop		= (1<<0),
	Tkbottom	= (1<<1),
	Tkleft		= (1<<2),
	Tkright		= (1<<3),
	Tkside		= Tktop|Tkbottom|Tkleft|Tkright,
	Tkfillx		= (1<<4),
	Tkfilly		= (1<<5),
	Tkfill		= Tkfillx|Tkfilly,
	Tkexpand	= (1<<6),
	Tkrefresh	= (1<<7),
	Tknoprop	= (1<<8),
	Tkbaseline 	= (1<<10),
	Tknumeric	= (1<<11),

	Tknorth		= (1<<10),
	Tkeast		= (1<<11),
	Tksouth		= (1<<12),
	Tkwest		= (1<<13),
	Tkcenter		= 0,
	Tkanchor	= Tknorth|Tkeast|Tksouth|Tkwest,
	Tksuspended	= (1<<14),		/* window suspended pending resize */

	Tkgridpack	= (1<<17),		/* temporary flag marking addition to grid */
	Tkgridremove	= (1<<18),		/* marked for removal from grid */
	Tktakefocus	= (1<<19),
	Tktransparent		= (1<<20),	/* Doesn't entirely obscure its background */
	Tkwindow	= (1<<21),	/* Top level window */
	Tkactive		= (1<<22),	/* Pointer is over active object */
	Tkactivated	= (1<<23),	/* Button pressed etc.. */
	Tkdisabled	= (1<<24),	/* Button is not accepting events */
	Tkmapped	= (1<<25),	/* Mapped onto display */
	Tknograb		= (1<<26),
	Tkdestroy		= (1<<27),	/* Marked for death */
	Tksetwidth	= (1<<28),
	Tksetheight	= (1<<29),
	Tksubsub	= (1<<30),
	Tkswept		= (1<<31),

	/* Supported Event Types 		*/
	/*
	 * Bits 0-15 are keyboard characters
	 * but duplicated by other events, as we can distinguish
	 * key events from others with the TkKey bit.
	 * N.B. make sure that this distinction *is* made;
	 * i.e.
	 *	if (event & TkButton1P)
	 *   is no longer sufficient to test for button1 down.
	 *
	 * 	if (!(event & TkKey) && (event & TkButton1P))
	 *   would be better.
	 */
	TkButton1P	= (1<<0),
	TkButton1R	= (1<<1),
	TkButton2P	= (1<<2),
	TkButton2R	= (1<<3),
	TkButton3P	= (1<<4),
	TkButton3R	= (1<<5),
	TkButton4P	= (1<<6),
	TkButton4R	= (1<<7),
	TkButton5P	= (1<<8),
	TkButton5R	= (1<<9),
	TkButton6P	= (1<<10),
	TkButton6R	= (1<<11),
	/*
	 * 12-15 reserved for more buttons.
	 */
	TkExtn1		= (1<<16),
	TkExtn2		= (1<<17),
	TkTakefocus	= (1<<18),
	/*
	 * 19-20 currently free
	 */
	TkDestroy		= (1<< 21),
	TkEnter		= (1<<22),
	TkLeave		= (1<<23),
	TkMotion	= (1<<24),
	TkMap		= (1<<25),
	TkUnmap		= (1<<26),
	TkKey		= (1<<27),
	TkFocusin	= (1<<28),
	TkFocusout	= (1<<29),
	TkConfigure	= (1<<30),
	TkDouble	= (1<<31),
	TkEmouse	= TkButton1P|TkButton1R|TkButton2P|TkButton2R|
			TkButton3P|TkButton3R|
			TkButton4P|TkButton4R|
			TkButton5P|TkButton5R|
			TkButton6P|TkButton6R|TkMotion,
	TkEpress	= TkButton1P|TkButton2P|TkButton3P|
			TkButton4P|TkButton5P|TkButton6P,
	TkErelease	= TkButton1R|TkButton2R|TkButton3R|
					TkButton4R|TkButton5R|TkButton6R,
	TkExtns	= TkExtn1|TkExtn2,
	TkAllEvents	= ~0,

	TkCforegnd	= 0,
	TkCbackgnd,			/* group of 3 */
	TkCbackgndlght,
	TkCbackgnddark,
	TkCselect,
	TkCselectbgnd,			/* group of 3 */
	TkCselectbgndlght,
	TkCselectbgnddark,
	TkCselectfgnd,
	TkCactivebgnd,		/* group of 3 */
	TkCactivebgndlght,
	TkCactivebgnddark,
	TkCactivefgnd,
	TkCdisablefgnd,
	TkChighlightfgnd,
	TkCfill,
	TkCtransparent,

	TkNcolor,

	TkSameshade	= 0,		/* relative shade for tkgshade() and tkrgbashade()*/
	TkLightshade,
	TkDarkshade
};


struct TkEnv
{
	int		ref;
	TkTop*		top;			/* Owner */
	ulong		set;
	ulong		colors[TkNcolor];	/* OPTcolr */
	Font*		font;			/* Font description */
	int		wzero;			/* Width of "0" in pixel */
};

struct TkGeom
{
	int		x;
	int		y;
	int		width;
	int		height;
};

struct TkGridbeam
{
	int		equalise;
	int		minsize;
	int		maxsize;
	int		weight;
	int		pad;
	int		act;
	char*	name;
};

struct TkGridcell
{
	Tk*		tk;
	Point		span;
};

struct TkGrid
{
	TkGridcell**	cells;		/* 2D array of cells, y major, x minor */
	Point			dim;		/* dimensions of table */
	TkGridbeam*	rows;
	TkGridbeam*	cols;
	Point			origin;	/* top left point grid was rendered at */
};

struct Tk
{
	int		type;		/* Widget type */
	TkName*		name;		/* Hierarchy name */
	Tk*		siblings;	/* Link to descendents */
	Tk*		master;		/* Pack owner */
	Tk*		slave;		/* Packer slaves */
	Tk*		next;		/* Link for packer slaves */
	Tk*		parent;		/* Window is sub of this canvas or text */
	Tk*		depth;		/* Window depth when mapped */
	void		(*geom)(Tk*, int, int, int, int);		/* Geometry change notify function */
	void		(*destroyed)(Tk*);		/* Destroy notify function */
	int		flag;		/* Misc flags. */
	TkEnv*		env;		/* Colors & fonts */
	TkAction*	binds;		/* Binding of current events */

	TkGeom		req;		/* Requested size and position */
	TkGeom		act;		/* Actual size and position */
	int		relief;		/* 3D border type */
	int		borderwidth;	/* 3D border size */
	int		highlightwidth;
	Point		pad;		/* outside frame padding */
	Point		ipad;		/* inside frame padding */
	Rectangle	dirty;	/* dirty rectangle, relative to widget */
	TkGrid*	grid;		/* children are packed in a grid */

	/* followed by widget-dependent data */
};

struct TkWin
{
	Image*	image;
	Tk*		next;
	Point		act;
	Point		req;
	void*	di;		/* !=H if it's been set externally */
	int		changed;	/* requested rect has changed since request sent */
	int		reqid;	/* id of request most recently sent out; replies to earlier requests are ignored */

	/* menu specific members */
	Tk*		slave;
	char*		postcmd;
	char*		cascade;
	int			freeonunmap;
	char			*cbname;		/* name of choicebutton that posted this */

	Point			delta;
	int			speed;
	int			waiting;
};

struct TkMethod
{
	char*		name;
	TkCmdtab*	cmd;
	void		(*free)(Tk*);
	char*	(*draw)(Tk*, Point);
	void		(*geom)(Tk*);
	void		(*getimgs)(Tk*, Image**, Image**);

	void		(*focusorder)(Tk*);	/* add any focusable descendants to the focus order */
	void		(*dirtychild)(Tk*);		/* Child notifies parent to redraw */
	Point		(*relpos)(Tk*);
	Tk*		(*deliver)(Tk*, int, void*);
	void		(*see)(Tk*, Rectangle*, Point*);
	Tk*		(*inwindow)(Tk*, Point*);
	void		(*varchanged)(Tk*, char*, char*);
	void		(*forgetsub)(Tk*, Tk*);
	int		ncmd;
};

struct TkName
{
	TkName*		link;
	void*		obj;		/* Name for ... */
	union {
		TkAction*	binds;
	}prop;				/* Properties for ... */
	int		ref;
	char		name[TKSTRUCTALIGN];
};

struct TkTtabstop
{
	int		pos;
	int		justify;
	TkTtabstop*	next;
};

struct TkCtxt
{
	void*	lock;
	Display*	display;
	int		ncol;
	TkCol*	chead;
	TkCol*	ctail;
	Image*	i;			/* temporary image for drawing buttons, etc */
	Image*	ia;			/* as above, but with an alpha channel */
	Tk*		tkkeygrab;

	int		focused;
	Tk*		mfocus;
	Tk*		mgrab;
	Tk*		entered;
	TkMouse	mstate;

	Tk*		tkmenu;
	void*	extn;
};

struct TkTop
{
	void*	dd;	/* really Draw_Display */
	void*	wreq;	/* really chan of string */
	void*	di;		/* really Draw_Image* */
	void*	wmctxt;	/* really Draw_Wmcontext */
	Rectangle	screenr;	/* XXX sleazy equiv to Draw_Rect, but what else? */

	/* Private from here on */
	TkCtxt*		ctxt;
	Display*	display;
	Tk*		root;			/* list of all objects, linked by siblings; first object is "." */
	Tk*		windows;
	Tk**		focusorder;
	int		nfocus;
	TkEnv*		env;
	TkTop*		link;
	TkVar*		vars;
	TkImg*		imgs;
	TkPanelimage*	panelimages;
	TkAction*	binds[TKwidgets];
	int		debug;
	int		execdepth;
	char		focused;
	char		dirty;
	char		noupdate;
	char*	err;
	char		errcmd[32];
	char		errx[32];
};

#define TKobj(t, p)	((t*)((p)+1))
//#define TKobj(t, p)	((t*)(p->obj))
#define OPTION(p, t, o)	(*(t*)((char*)p + o))

/* Command entry points */
extern	char*	tkbind(TkTop*, char*, char**);
extern	char*	tkbutton(TkTop*, char*, char**);
extern	char*	tkcanvas(TkTop*, char*, char**);
extern	char*	tkcheckbutton(TkTop*, char*, char**);
extern	char*	tkchoicebutton(TkTop*, char*, char**);
extern	char*	tkcursorcmd(TkTop*, char*, char**);
extern	char*	tkdestroy(TkTop*, char*, char**);
extern	char*	tkentry(TkTop*, char*, char**);
extern	char*	tkfocus(TkTop*, char*, char**);
extern	char*	tkframe(TkTop*, char*, char**);
extern	char*	tkgrab(TkTop*, char*, char**);
extern	char*	tkgrid(TkTop*, char*, char**);
extern	char*	tkimage(TkTop*, char*, char**);
extern	char*	tklabel(TkTop*, char*, char**);
extern	char*	tklistbox(TkTop*, char*, char**);
extern	char*	tklower(TkTop*, char*, char**);
extern	char*	tkmenu(TkTop*, char*, char**);
extern	char*	tkmenubutton(TkTop*, char*, char**);
extern	char*	tkpack(TkTop*, char*, char**);
extern	char*	tkpanel(TkTop*, char*, char**);
extern	char*	tkputs(TkTop*, char*, char**);
extern	char*	tkradiobutton(TkTop*, char*, char**);
extern	char*	tkraise(TkTop*, char*, char**);
extern	char*	tkscale(TkTop*, char*, char**);
extern	char*	tkscrollbar(TkTop*, char*, char**);
extern	char*	tksend(TkTop*, char*, char**);
extern	char*	tktext(TkTop*, char*, char**);
extern	char*	tkupdatecmd(TkTop*, char*, char**);
extern	char*	tkvariable(TkTop*, char*, char**);
extern	char*	tkwinfo(TkTop*, char*, char**);

extern	TkMethod	*tkmethod[];

/* Errors - xdata.c*/
extern	char	TkNomem[];
extern	char 	TkBadop[];
extern	char 	TkOparg[];
extern	char 	TkBadvl[];
extern	char 	TkBadwp[];
extern	char 	TkNotop[];
extern	char	TkDupli[];
extern	char	TkNotpk[];
extern	char	TkBadcm[];
extern	char	TkIstop[];
extern	char	TkBadbm[];
extern	char	TkBadft[];
extern	char	TkBadit[];
extern	char	TkBadtg[];
extern	char	TkFewpt[];
extern	char	TkBadsq[];
extern	char	TkBadix[];
extern	char	TkNotwm[];
extern	char	TkWpack[];
extern	char	TkBadvr[];
extern	char	TkNotvt[];
extern	char	TkMovfw[];
extern	char	TkBadsl[];
extern	char	TkSyntx[];
extern	char TkRecur[];
extern	char TkDepth[];
extern	char TkNomaster[];
extern	char TkNotgrid[];
extern	char TkIsgrid[];
extern	char TkBadgridcell[];
extern	char TkBadspan[];
extern	char TkBadcursor[];

/* Option tables - xdata.c */
extern	TkStab	tkanchor[];
extern	TkStab	tkbool[];
extern	TkStab	tkcolortab[];
extern	TkStab	tkjustify[];
extern	TkStab	tkorient[];
extern	TkStab	tkrelief[];
extern	TkStab	tktabjust[];
extern	TkStab	tkwrap[];
extern	TkOption	tkgeneric[];
extern	TkOption	tktop[];
extern	TkOption	tktopdbg[];

/* panel.c */
extern	void		tkgetpanelimage(Tk*, Image**, Image**);
extern	void		tksetpanelimage(Tk *tk, Image*, Image*);

/* General - colrs.c */
extern	void		tksetenvcolours(TkEnv*);

/* General - ebind.c */
extern	void		tkcmdbind(Tk*, int, char*, void*);
extern	TkCtxt*		tkdeldepth(Tk*);
extern	char*		tkdestroy(TkTop*, char*, char**);
extern	char*		tkbindings(TkTop*, Tk*, TkEbind*, int);
extern	int		tkseqparse(char*);
extern	void		tksetkeyfocus(TkTop*, Tk*, int);
extern	void		tksetglobalfocus(TkTop*, int);

/* General - image.c */
extern	TkImg*		tkname2img(TkTop*, char*);
extern	void		tkimgput(TkImg*);
extern	void		tksizeimage(Tk*, TkImg*);
extern	TkImg*		tkauximage(TkTop*, char*, TkMemimage*, int);

/* choicebuttons - menus.c */
extern	Tk*		tkfindchoicemenu(Tk*);

/* General - packr.c */
extern	void		tkdelpack(Tk*);
extern	void		tkappendpack(Tk*, Tk*, int);
extern	void		tkpackqit(Tk*);
extern	void		tkrunpack(TkTop*);
extern	void		tksetslavereq(Tk*, TkGeom);
extern	int		tkisslave(Tk*, Tk*);

/* General - grids.c */
extern	int		tkgridder(Tk*);
extern	void		tkgriddelslave(Tk*);
extern	char*	tkpropagate(TkTop*, char*);
extern	void		tkfreegrid(TkGrid*);

/* General - parse.c */
extern	char*		tkconflist(TkOptab*, char**);
extern	char*		tkskip(char*, char*);
extern	char*		tkword(TkTop*, char*, char*, char*, int*);
extern	char*		tkxyparse(Tk*, char**, Point*);
extern	char*		tkparse(TkTop*, char*, TkOptab*, TkName**);
extern	TkName*		tkmkname(char*);
extern	char*		tkgencget(TkOptab*, char*, char**, TkTop*);
extern	char*		tkparsecolor(char*, ulong*);

/* General - utils.c */
extern	Image*		tkgc(TkEnv*, int);
extern	Image*		tkgshade(TkEnv*, int, int);
extern	Image*		tkcolor(TkCtxt*, ulong);
extern	Image*		tkcolormix(TkCtxt*, ulong, ulong);
extern	Image*		tkgradient(TkCtxt*, Rectangle, int, ulong, ulong);
extern	void			tkclear(Image*, Rectangle);
extern	TkEnv*		tknewenv(TkTop*);
extern	TkEnv*		tkdefaultenv(TkTop*);
extern	void		tkputenv(TkEnv*);
extern	TkEnv*		tkdupenv(TkEnv**);
extern	Tk*		tknewobj(TkTop*, int, int);
extern	Tk*		tkfindsub(Tk*);
extern	void		tkfreebind(TkAction*);
extern	void		tkfreename(TkName*);
extern	void		tkfreeobj(Tk*);
extern	char*		tkaddchild(TkTop*, Tk*, TkName**);
extern	Tk*		tklook(TkTop*, char*, int);
extern	void		tktextsdraw(Image*, Rectangle, TkEnv*, int);
extern	void		tkbox(Image*, Rectangle, int, Image*);
extern	void		tkbevel(Image*, Point, int, int, int, Image*, Image*);
extern	void		tkdrawrelief(Image*, Tk*, Point, int, int);
extern	Point		tkstringsize(Tk*, char*);
extern	void		tkdrawstring(Tk*, Image*, Point, char*, int, Image *, int);
extern	int		tkeventfmt(Fmt*);
extern	Tk*		tkdeliver(Tk*, int, void*);
extern	int		tksubdeliver(Tk*, TkAction*, int, void*, int);
extern	void		tkcancel(TkAction**, int);
extern	char*		tkaction(TkAction**, int, int, char*, int);
extern	char*		tkitem(char*, char*);
extern	Point		tkposn(Tk*);
extern	Point		tkscrn2local(Tk*, Point);
extern	int		tkvisiblerect(Tk *tk, Rectangle *rr);
extern	Point		tkanchorpoint(Rectangle, Point, int);
extern	char*		tkfrac(char**, int*, TkEnv*);
extern	char*		tkfracword(TkTop*, char**, int*, TkEnv*);
extern	char*		tkfprint(char*, int);
extern	char*		tkvalue(char**, char*, ...);
extern	void		tkdirty(Tk *);
extern	void		tksorttable(void);
extern	char*		tkexec(TkTop*, char*, char**);
extern	int		tkeventfmt(Fmt*);
extern	void		tkerr(TkTop*, char*);
extern	char*	tkerrstr(TkTop*, char*);
extern	char*	tkcursorswitch(TkTop*, Image*, TkImg*);
extern	void		tkcursorset(TkTop*, Point);
extern	char*	tksetmgrab(TkTop*, Tk*);
extern	int 		tkinsidepoly(Point*, int, int, Point);
extern	int		tklinehit(Point*, int, int, Point);
extern	int		tkiswordchar(int);
extern	int		tkhaskeyfocus(Tk*);
extern	Rectangle	tkrect(Tk*, int);
extern	int		tkchanhastype(ulong, int);
extern	int		tkhasalpha(TkEnv*, int);
extern	void		tksettransparent(Tk*, int);
extern	ulong	tkrgba(int, int, int, int);
extern	ulong	tkrgbashade(ulong, int);
extern	void		tkrgbavals(ulong, int*, int*, int*, int*);
extern	void		tkrepeat(Tk*, void(*)(Tk*, void*, int), void*, int, int);
extern	void		tkcancelrepeat(Tk*);
extern	void		tkblink(Tk*, void(*)(Tk*, int));
extern	void		tkblinkreset(Tk*);
extern	char*	tkname(Tk*);

/* General - windw.c */
extern	TkCtxt*		tknewctxt(Display*);
extern	void		tkfreectxt(TkCtxt*);
extern	void		tkfreecolcache(TkCtxt*);
extern	Image*		tkitmp(TkEnv*, Point, int);
extern	void		tkgeomchg(Tk*, TkGeom*, int);
extern	Tk*		tkinwindow(Tk*, Point, int);
extern	Tk*		tkfindfocus(TkTop*, int, int, int);
extern	char*		tkdrawslaves(Tk*, Point, int*);
extern	char*		tkupdate(TkTop*);
extern	int		tkischild(Tk*, Tk*);
extern	void		tksetbits(Tk*, int);
extern	char*		tkmap(Tk*);
extern	void		tkunmap(Tk*);
extern	void		tkmoveresize(Tk*, int, int, int, int);
extern	int		tkupdatewinsize(Tk*);
extern	void		tkmovewin(Tk*, Point);
extern	Image*		tkimageof(Tk*);
extern	void		tktopopt(Tk*, char*);
extern	void		tkdirtyfocusorder(TkTop*);
extern	void		tkbuildfocusorder(TkTop*);
extern	void		tksortfocusorder(TkWinfo*, int);
extern	void		tkappendfocusorder(Tk*);
extern	void		tksee(Tk*, Rectangle, Point);
extern	char*	tkseecmd(TkTop*, char*, char**);

/* Style specific extensions - extns.c */
extern	int		tkextndeliver(Tk*, TkAction*, int, void*);
extern	void		tkextnfreeobj(Tk*);
extern	int		tkextnnewctxt(TkCtxt*);
extern	void		tkextnfreectxt(TkCtxt*);
extern	char*	tkextnparseseq(char*, char*, int*);

/* Debugging */
extern	void		tkdump(Tk*);
extern	void		tktopdump(Tk*);
extern	void		tkcvsdump(Tk*);

/* External to libtk */
extern	void		tkenterleave(TkTop*);
extern	void		tksetwinimage(Tk*, Image*);
extern	void		tkdestroywinimage(Tk*);
extern	void		tkfreevar(TkTop*, char*, int);
extern	TkVar*		tkmkvar(TkTop*, char*, int);
extern	int		tktolimbo(void*, char*);
extern	void		tkwreq(TkTop*, char*, ...);
extern	void		tkdelpanelimage(TkTop*, Image*);

extern	TkMethod		framemethod;
extern	TkMethod		labelmethod;
extern	TkMethod		checkbuttonmethod;
extern	TkMethod		buttonmethod;
extern	TkMethod		menubuttonmethod;
extern	TkMethod		menumethod;
extern	TkMethod		separatormethod;
extern	TkMethod		cascademethod;
extern	TkMethod		listboxmethod;
extern	TkMethod		scrollbarmethod;
extern	TkMethod		textmethod;
extern	TkMethod		canvasmethod;
extern	TkMethod		entrymethod;
extern	TkMethod		radiobuttonmethod;
extern	TkMethod		scalemethod;
extern	TkMethod		panelmethod;
extern	TkMethod		choicebuttonmethod;
