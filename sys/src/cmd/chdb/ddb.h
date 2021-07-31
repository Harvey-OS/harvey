/*
 * wish list:
 *	snarf and text select
 */

#include	<u.h>
#include	<libc.h>
#include	<libg.h>
#include	<ctype.h>
#include	<bio.h>
#include	<regexp.h>
#include	"gen.h"

enum
{
	NLINE		= 9,		/* number of print lines in header box */
	INSET		= 5,		/* pixel width of 'tween' space */
	BAR		= 12,		/* pixel width of scroll bar */
	CHT		= 18,		/* pixel height of char */
	CHW		= 9,		/* pixel width of char */
	SIZEV		= 8,		/* button 1,3 max width, vert */
	SIZEH		= 8,		/* button 1,3 max width, horiz */
	STRSIZE		= 200,		/* size of string manipulation buf */
	CMDSIZE		= 200,		/* size of command buf */
	NODESIZE	= 100,		/* size of node freelist */

	End		= -1000,
	Fileoffset	= 2,

	Byorder		= 0,
	Byfile,
	Bywhite,
	Byblack,
	Byevent,
	Byopening,
	Bydate,
	Blast,
	Byresult,

	Nnull		= 0,
	Nrex,
	Nmark,
	Nand,
	Nor,
	Nsub,
	Nnot,
	Ngame,
	Nposit,
	Nall,
};

#define	nelem(x)	(sizeof(x)/sizeof((x)[0]))
#define	offsetof(t,x)	((ulong)&((t*)0)->x)
#define	D		(&screen)
#define	FROM(x)		(((x)>>6)&077)
#define	TO(x)		(((x)>>0)&077)

typedef	struct	Game	Game;
typedef	struct	String	String;
typedef	struct	Str	Str;
typedef	struct	Node	Node;
typedef	struct	Xor	Xor;
typedef	Rectangle	Box;

struct	String
{
	int	beg;
	int	end;
};
struct	Game
{
	char*	file;
	char*	white;
	char*	black;
	char*	event;
	char*	result;
	char*	opening;
	char*	date;
	short*	moves;
	short	nmoves;
	short	position;
};
struct	Str
{
	ushort*	gp;
	ulong	mark;

	short	position;
	char	whiteoffset;
	char	blackoffset;

	char	eventoffset;
	char	resultoffset;
	char	openingoffset;
	char	dateoffset;

};
struct	Node
{
	int	op;
	union
	{
		struct
		{
			/* op = Nrex */
			Reprog*	rex;
			long	field;
		};
		struct
		{
			/* op = Nmark */
			int	mark;
		};
		struct
		{
			/* op = N<op> */
			Node*	left;
			Node*	right;
		};
		struct
		{
			/* op = Ngame */
			ushort*	gp;
		};
		struct
		{
			/* op = Nposit */
			Xor*	xor;
		};
	};
};
struct	Xor
{
	long	cursize;
	long	maxsize;
	long	mask;
	ulong	words[10];
};

struct
{
	Box	screen;
	Box	board;
	Box	vbar;
	Box	hbar;
	Box	header;
	int	side;
} d;

Bitmap*	bitpiece[20];
char	chars[STRSIZE];
char	cmd[CMDSIZE];
int	cmdi;
long	gsizelist[30];
Game	curgame;
Event	ev;
long	gameno;
ushort*	games;
long	gsize;
long	tgsize;
int	hdrsize;
char	lastbd[64];
char	mklist[8];
Menu	men3;
char*	men3l[];
long	ngames;
Node	nodes[NODESIZE];
int	nodi;
int	numcmp(char*, char*);
int	ogameno;
short*	piece[];
int	pipefid[2];
int	printcol;
int	sortby;
Str*	str;
ushort*	lastgame;
long	tgames;
Cursor	thinking;
char	word[CMDSIZE];
ulong	xora[16*64];
ulong	xorb[16*64];
ulong	initpxora;
ulong	initpxorb;
ulong	btmxora;
ulong	btmxorb;
char	xbd[64];
char	xpc[32];
ulong	xposxor;		/* side effect of calling posxor[wb] */
ushort	g0[1000];
int	yystring;		/* looking for a string delim */
int	yyterminal;		/* flag between ';' and $eof */
int	yysyntax;		/* syntax has happened */
int	cinitf1;		/* chessinit arg 1 */
int	cinitf2;		/* chessinit arg 2 */

void	buildgames(void);
int	button(int);
long	countgames(void);
void	decode(Game*, Str*);
void	dodupl(void);
void	doutline(Box);
void	dowrite(int, char*);
Bitmap*	draw(short*, int);
void	forcegame(int);
void	freenodes(void);
void	funh(long);
void	funhd(long);
void	funhu(long);
void	funv(long);
void	funvd(long);
void	funvu(long);
ulong	genmark(int);
void	getposn(int);
long	getshort(void);
Node*	new(int);
int	openfile(char*);
ulong	posxorb(void);
ulong	posxorw(void);
void	prline(int);
void	readit(char*);
long	scroll(Box*, int, long, void (*)(long), int);
void	search(Node*, long, long, long, int);
void	setgame(long);
void	setmark(int);
void	setposn(int);
void	setscroll(int);
void	sortit(int);
void	xorinit(void);
Xor*	xorset(int);
void	yyerror(char*, ...);
int	yylex(void);
int	yyparse(void);
void	moveselect(int);
long	rellast(void);
