#pragma src "/sys/src/libcontrol"
#pragma lib "libcontrol.a"

#pragma	varargck	argpos	printctl	2
#pragma	varargck	type	"q"	char*
#pragma	varargck	type	"Q"	Rune*

typedef struct Control Control;
typedef struct Controlset Controlset;
typedef struct CParse CParse;
typedef struct CCache CCache;
typedef struct CCache CImage;
typedef struct CCache CFont;
typedef struct CWire CWire;

enum	/* alts */
{
	AKey,
	AMouse,
	ACtl,
	AWire,
	AExit,
	NALT
};

struct CWire
{
	Channel	*chan;	/* chan(char*) */
	Channel	*sig;		/* chan(int) for synch */
	char		*name;
};

struct Controlset
{
	Control		*controls;
	Image		*screen;
	Control		*actives;
	Control		*focus;
	Channel		*kbdc;
	Channel		*kbdexitc;
	Channel		*mousec;
	Channel		*mouseexitc;
	Channel		*resizec;
	Channel		*resizeexitc;
	Keyboardctl	*keyboardctl;	/* will be nil if user supplied keyboard */
	Mousectl		*mousectl;	/* will be nil if user supplied mouse */
	int			clicktotype;	/* flag */
};

struct Control
{
	/* known to client */
	char			*name;
	Rectangle	rect;
	Channel		*event;			/* chan(char*) to client */
	Channel		*ctl;				/* chan(char*) from client */
	Channel		*data;			/* chan(char*) to client */
	/* internal to control set */
	Controlset	*controlset;
	Channel		*kbd;			/* chan(Rune)[20], as in keyboard.h */
	Channel		*mouse;			/* chan(Mouse) as in mouse.h */
	Channel		*wire;			/* chan(CWire) */
	Channel		*exit;			/* chan(int) */
	Image		*screen;			/* where Control appears */
	char			*format;			/* used to generate events */
	char			wevent;			/* event channel rewired */
	char			wctl;				/* ctl channel rewired */
	char			wdata;			/* data channel rewired */
	Alt			*alts;
	/* things alted on */
	Mouse		m;
	char			*str;
	Rune			*kbdr;
	CWire		cwire;
	Control		*nextactive;
	Control		*next;
};

struct CCache
{
	union{
		Image	*image;
		Font		*font;
	};
	char		*name;
	int		index;			/* entry number in cache */
	int		ref;				/* one for client, plus one for each use */
};

struct CParse
{
	char	str[128];
	char	*sender;
	char	*args[10];
	int	iargs[10];
	int nargs;
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

/* Functions used internally */
int		_ctlparse(CParse*, char*, char**);
void		_ctlargcount(Control*, CParse*, int);
void		_ctlcontrol(Control*, char*, void (*)(Control*, char*));
Control*	_createctl(Controlset*, char*, uint, char*, void (*)(void*), int stksize);
Rune*	_ctlrunestr(char*);
char*	_ctlstrrune(Rune*);
void		_ctlputsnarf(Rune*);
Rune*	_ctlgetsnarf(void);
int		_ctlalignment(char*);
Point		_ctlalignpoint(Rectangle, int, int, int);
void		_ctlrewire(Control*);

/* images */
CImage*	_getctlimage(char*);
void		_setctlimage(Control*, CImage**, char*);
void		_putctlimage(CImage*);
CFont*	_getctlfont(char*);
void		_putctlfont(CFont*);

/* fonts */
CImage*	_getctlfont(char*);
void		_setctlfont(Control*, CImage**, char*);
void		_putctlfont(CImage*);
CFont*	_getctlfont(char*);
void		_putctlfont(CFont*);

/* Public functions */

/* images */
int		namectlimage(Image*, char*);
int		freectlimage(char*);
/* fonts */
int		namectlfont(Font*, char*);
int		freectlfont(char*);

/* general */
void			initcontrols(void);
Controlset*	newcontrolset(Image*, Channel*, Channel*, Channel*);
void			closecontrolset(Controlset*);
void			closecontrol(Control*);
int			printctl(Channel*, char*, ...);
void			ctlerror(char*, ...);
Control*		controlcalled(char*);

/* publicly visible error-checking allocation routines */
void*	ctlmalloc(uint);
void*	ctlrealloc(void*, uint);
char*	ctlstrdup(char*);

/* creation */
void		controlwire(Control*, char*, Channel*);
void		activate(Control*);
void		deactivate(Control*);
Control*	createbox(Controlset*, char*);
Control*	createbutton(Controlset*, char*);
Control*	createentry(Controlset*, char*);
Control*	createkeyboard(Controlset*, char*);
Control*	createlabel(Controlset*, char*);
Control*	createradiobutton(Controlset*, char*);
Control*	createscribble(Controlset*, char*);
Control*	createslider(Controlset*, char*);
Control*	createtext(Controlset*, char*);
Control*	createtextbutton(Controlset*, char*);

/* user-supplied */
void		resizecontrolset(Controlset*);

int		_ctlsnarffd;
char		*alignnames[];
int		ctldeletequits;
