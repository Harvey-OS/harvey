#define VARARGCK 1
typedef struct Bytes	Bytes;
typedef struct Cmd		Cmd;
typedef struct Focus	Focus;
typedef struct URLwin	URLwin;

enum
{
	/* statement types */
	Xnop = 0,
	Xcomma,
	Xblock,
	Xif,
	Xinput,
	Xfind,
	Xload,
	Xnot,
	Xprint,
	Xsubmit,
	Xlist,

	/* html element types (thing) */
	Tnone = 0,
	Tcell,
	Tcheckbox,
	Tform,
	Tpage,
	Tpassword,
	Tradiobutton,
	Trow,
	Tselect,
	Ttable,
	Ttext,
	Ttextarea,
	Ttextbox,
	
	/* search directions (where) */
	Wnone = 0,
	Wfirst,
	Winner,
	Wouter,
	Wprev,
	Wnext,
	
	/* focus types */
	FocusNone = 0,
	FocusAnchor,
	FocusForm,
	FocusFormfield,
	// FocusFrame,
	FocusItem,
	FocusPage,
	FocusTable,
	FocusTablecell,	
	FocusTablerow,
	FocusStart,
	FocusEnd,
	
	/* extra bits we use in Item.state */
	/* if we run out of bits, could use enum here instead */
	IFtable =	0x00010000,
	IFcell =	0x00020000,
	IFrow =		0x00040000,
	IFfloat =	0x00080000,
};

struct Bytes
{
	uchar	*b;
	long	n;
	long	nalloc;
};

struct Cmd
{
	int		xop;
	int		arg;
	int		arg1;
	char	*s;
	Cmd		*cond;
	Cmd		*left;
	Cmd		*right;
};

struct Focus
{
	int		type;
	Item	*ifirst;
	Item	*ilast;

	Anchor		*anchor;
	Form		*form;
	Formfield	*formfield;
	Table		*table;
	Tablecell	*tablecell;
	Tablerow	*tablerow;
};

struct URLwin
{
	int		type;
	char	*url;
	Item	*items;
	Docinfo	*docinfo;
};

extern	int		cencodefmt(Fmt*);
extern	int		charset(char*);
extern	void	error(char*, ...);
extern	char*	eappend(char*, char*, char*);
extern	char*	egrow(char*, char*, char*);
extern	void*	emalloc(ulong);
extern	char*	estrdup(char*);
extern	char*	estrstrdup(char*, char*);
extern	int		find(int, int, char*);
extern	int		flatindex(Item*);
extern	void	focusanchor(Focus*, Anchor*, Item*);
extern	void	focusend(Focus*);
extern	void	focusform(Focus*, Form*);
extern	void	focusformfield(Focus*, Formfield*);
extern	void	focusitem(Focus*, Item*);
extern	void	focusitems(Focus*, Item*, Item*);
extern	void	focuspage(Focus*);
extern	void	focusstart(Focus*);
extern	void	focustable(Focus*, Table*);
extern	void	focustablecell(Focus*, Tablecell*);
extern	void	focustablerow(Focus*, Tablerow*);
extern	int		formfieldfmt(Fmt*);
extern	char*	fullurl(URLwin*, Rune*);
extern	void	freeurlwin(URLwin*);
extern	void	growbytes(Bytes*, char*, long);
extern	Bytes*	httpget(char*, char*);
extern	Cmd*	mkcmd(int, int, char*);
extern	int		inputvalue(Focus*, char*);
extern	void	loadurl(char*, char*);
extern	void	printfocus(Focus*);
extern	char*	readfile(char*, char*, int*);
extern	void	render(URLwin*, Bytes*, Item*, Item*, int);
extern	void	renderbytes(Bytes*, char*, ...);
extern	void	rendertext(URLwin*, Bytes*);
extern	void	rerender(URLwin*);
extern	int		runcmd(Cmd*);
extern	int		runeurlencodefmt(Fmt*);
extern	int		submitform(Form*, Formfield*);
extern	int		thingfmt(Fmt*);
extern	int		urlencodefmt(Fmt*);
extern	int		wherefmt(Fmt*);
extern	int		yyerror(char*);
extern	int		yylex(void);
extern	int		yyparse(void);

#ifdef VARARGCK
#pragma	varargck	argpos	error	1
#pragma	varargck	type	"T"	int
#pragma	varargck	type	"F"	int
#pragma	varargck	type	"W"	int
#pragma	varargck	type "y"	char*
#pragma	varargck	type "z"	char*
#pragma varargck	type	"Z"	Rune*
#endif

extern	int		aflag;			/* print anchor refs? */
extern	Biobuf*	bscript;		/* input */
extern	int		defcharset;		/* default character set */
extern	URLwin*	doc;			/* current document */
extern	char*	filename;		/* name of file being read */
extern	Item**	flatitem;		/* flat item list */
extern	Focus	focus;			/* current selection in document */
extern	int		lexerrors;		/* count of lex errors */
extern	int		lineno;			/* line of file being read */
extern	int		nflatitem;		/* count of flatitem[] */
extern	int		width;			/* line wrap width in characters */
extern	int		verbose;		/* chatty output */
extern	int		xtrace;			/* print commands as they are executed */
extern	int		yylexdebug;		/* debug lexer */
