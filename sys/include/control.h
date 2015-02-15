/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#pragma src "/sys/src/libcontrol"
#pragma lib "libcontrol.a"

#pragma	varargck	argpos	ctlprint	2
#pragma varargck	argpos	_ctlprint	2

typedef struct Control Control;
typedef struct Controlset Controlset;
typedef struct CParse CParse;
typedef struct CCache CCache;
typedef struct CCache CImage;
typedef struct CCache CFont;

enum	/* types */
{
	Ctlunknown,
	Ctlbox,
	Ctlbutton,
	Ctlentry,
	Ctlkeyboard,
	Ctllabel,
	Ctlmenu,
	Ctlradio,
	Ctlscribble,
	Ctlslider,
	Ctltabs,
	Ctltext,
	Ctltextbutton,
	Ctltextbutton3,
	Ctlgroup,		/* divider between controls and metacontrols */
	Ctlboxbox,
	Ctlcolumn,
	Ctlrow,
	Ctlstack,
	Ctltab,
	Ntypes,
};

struct Controlset
{
	Control		*controls;
	Image		*screen;
	Control		*actives;
	Control		*focus;
	Channel		*ctl;
	Channel		*data;		/* currently only for sync */
	Channel		*kbdc;
	Channel		*mousec;
	Channel		*resizec;
	Channel		*resizeexitc;
	Channel		*csexitc;
	Keyboardctl	*keyboardctl;	/* will be nil if user supplied keyboard */
	Mousectl	*mousectl;	/* will be nil if user supplied mouse */
	int		clicktotype;	/* flag */
};

struct Control
{
	/* known to client */
	int8_t		*name;
	Rectangle	rect;
	Rectangle	size;		/* minimum/maximum Dx, Dy (not a rect) */
	Channel		*event;		/* chan(char*) to client */
	Channel		*data;		/* chan(char*) to client */

	/* internal to control set */
	int		type;
	int		hidden;		/* hide hides, show unhides (and redraws) */
	Controlset	*controlset;
	Image		*screen;	/* where Control appears */
	int8_t		*format;	/* used to generate events */
	int8_t		wevent;		/* event channel rewired */
	int8_t		wdata;		/* data channel rewired */

	/* method table */
	void		(*ctl)(Control*, CParse*);
	void		(*mouse)(Control*, Mouse*);
	void		(*key)(Control*, Rune*);
	void		(*exit)(Control*);
	void		(*setsize)(Control*);
	void		(*activate)(Control*, int);
	Control		*nextactive;
	Control		*next;
};

struct CCache
{
	union{
		Image	*image;
		Font	*font;
	};
	int8_t		*name;
	int		index;		/* entry number in cache */
	int		ref;		/* one for client, plus one for each use */
};

struct CParse
{
	int8_t	str[256];
	int8_t	*sender;
	int8_t	*receiver;
	int	cmd;
	int8_t	*pargs[32];
	int	iargs[32];
	int8_t	**args;
	int	nargs;
};

enum	/* alignments */
{
	Aupperleft = 0,
	Auppercenter,
	Aupperright,
	Acenterleft,
	Acenter,
	Acenterright,
	Alowerleft,
	Alowercenter,
	Alowerright,
	Nalignments
};

enum
{
	_Ctlmaxsize = 10000,
};

extern int8_t *ctltypenames[];

/* Functions used internally */
void		_ctladdgroup(Control*, Control*);
void		_ctlargcount(Control*, CParse*, int);
Control*	_createctl(Controlset*, int8_t*, uint, int8_t*);
Rune*		_ctlrunestr(int8_t*);
int8_t*		_ctlstrrune(Rune*);
void		_ctlputsnarf(Rune*);
Rune*		_ctlgetsnarf(void);
int		_ctlalignment(int8_t*);
Point		_ctlalignpoint(Rectangle, int, int, int);
void		_ctlfocus(Control*, int);
void		_activategroup(Control*);
void		_deactivategroup(Control*);
int		_ctllookup(int8_t *s, int8_t *tab[], int ntab);
void		_ctlprint(Control *c, int8_t *fmt, ...);

/* images */
CImage*		_getctlimage(int8_t*);
void		_setctlimage(Control*, CImage**, int8_t*);
void		_putctlimage(CImage*);
CFont*		_getctlfont(int8_t*);
void		_putctlfont(CFont*);

/* fonts */
CImage*		_getctlfont(int8_t*);
void		_setctlfont(Control*, CImage**, int8_t*);
void		_putctlfont(CImage*);
CFont*		_getctlfont(int8_t*);
void		_putctlfont(CFont*);

/* Public functions */

/* images */
int		namectlimage(Image*, int8_t*);
int		freectlimage(int8_t*);

/* fonts */
int		namectlfont(Font*, int8_t*);
int		freectlfont(int8_t*);

/* commands */
int		ctlprint(Control*, int8_t*, ...);

/* general */
void		initcontrols(void);
Controlset*	newcontrolset(Image*, Channel*, Channel*, Channel*);
void		closecontrolset(Controlset*);
void		closecontrol(Control*);
void		ctlerror(int8_t*, ...);
Control*	controlcalled(int8_t*);

/* publicly visible error-checking allocation routines */
void*		ctlmalloc(uint);
void*		ctlrealloc(void*, uint);
int8_t*		ctlstrdup(int8_t*);

/* creation */
void		controlwire(Control*, int8_t*, Channel*);
void		activate(Control*);
void		deactivate(Control*);
Control*	createbox(Controlset*, int8_t*);
Control*	createbutton(Controlset*, int8_t*);
Control*	createcolumn(Controlset*, int8_t*);
Control*	createboxbox(Controlset*, int8_t*);
Control*	createentry(Controlset*, int8_t*);
Control*	createkeyboard(Controlset*, int8_t*);
Control*	createlabel(Controlset*, int8_t*);
Control*	createmenu(Controlset*, int8_t*);
Control*	createradiobutton(Controlset*, int8_t*);
Control*	createrow(Controlset*, int8_t*);
Control*	createscribble(Controlset*, int8_t*);
Control*	createslider(Controlset*, int8_t*);
Control*	createstack(Controlset*, int8_t*);
Control*	createtab(Controlset*, int8_t*);
Control*	createtext(Controlset*, int8_t*);
Control*	createtextbutton(Controlset*, int8_t*);
Control*	createtextbutton3(Controlset*, int8_t*);

/* user-supplied */
void		resizecontrolset(Controlset*);

int		_ctlsnarffd;
int8_t		*alignnames[];
int		ctldeletequits;
