/* can't have a balu unless *all* the drawing routines know about it */
#define NOBALU
#ifdef NOBALU
#undef ADD
#undef SUB
#undef SADD
#define ADD	(S^D)
#define SUB	(S^D)
#define SADD	(S|D)
#endif

/* hit codes */
#define LINEHIT		1
#define BOXHIT		2
#define STRHIT		4
#define DOTHIT		010
#define LINEBOX		020
#define BOXBOX		040
#define STRBOX		0100
#define DOTBOX		0200
#define ALLHIT		(LINEHIT|BOXHIT|STRHIT|DOTHIT)
#define ALLBOX		(LINEBOX|BOXBOX|STRBOX|DOTBOX)
#define ALL		(ALLBOX|ALLHIT)

/* string placement displacements */
#define HALFX	1
#define FULLX	2
#define HALFY	4
#define FULLY	8
#define INVIS	16

/* jraw codes for text placement */
#define	BL	01	/* placement is bottom left corner of string */
#define BC	02	/* bottom center */
#define BR	04	/* bottom right */
#define RC	010	/* right center */
#define TR	020	/* top right */
#define TC	040	/* top center */
#define TL	0100	/* top left */
#define LC	0200	/* left center */

#define GRID	8
#define GRIDMASK	(GRID-1)

#define P	r.min
#define Q	r.max
#define X	P.x
#define Y	P.y
#define U	Q.x
#define V	Q.y
#define ul(r)	r.min
#define ur(r)	Pt(r.max.x,r.min.y)
#define lr(r)	r.max
#define ll(r)	Pt(r.min.x,r.max.y)

#define lo(a,b)	(a < b ? a : b)
#define hi(a,b)	(a > b ? a : b)

#define EQ(p,q)	(p.x == q.x && p.y == q.y)
#define IN(p,r)	(p.x > r.min.x && p.y > r.min.y && p.x < r.max.x && p.y < r.max.y)
#define WITHIN(p,r)	(p.x >= r.min.x && p.y >= r.min.y && p.x <= r.max.x && p.y <= r.max.y)
#define MAXBB	Rect(10000,10000,-10000,-10000)

#define GENERIC	\
	struct Line *next;	/* we're part of a list, see */\
	Rectangle r;		/* generally easier to handle than two points */\
	char type;		/* yeah, YOU know, and I know, but.. */\
	char sel;		/* what pieces are selected */\
	char mod		/* and why */

typedef struct Line {
	GENERIC;
} Line;

typedef struct Box {
	GENERIC;
} Box;

typedef struct Dots {
	GENERIC;
} Dots;

typedef struct Macro {
	GENERIC;
} Macro;

typedef struct String {
	GENERIC;
	char len;	/* of string space */
	char code;	/* orientation */
	char *s;
} String;

typedef struct Inst {
	GENERIC;
	char *name;	/* must be interned */
} Inst;

typedef struct Ref {	/* a library reference we can i/o */
	GENERIC;
	char *filename;
} Ref;

typedef struct Master {		/* ahem, we do need to retain these somewhere */
	GENERIC;
	char *name;
	Line *dl;
	Bitmap *b;		/* much faster than drawing */
} Master;

typedef struct Dict {
	int n,used;
	char **key;
	char **val;
} Dict;

typedef struct File {
	char *name;
	int date;
	Dict *d;	/* local, for fast lookup */
	Line *dl;
	char changed;
} File;

typedef enum Thing {
	LINE = 0,
	BOX = 1,
	MACRO = 2,
	DOTS = 3,
	STRING = 4,
	INST = 5,
	REF = 6,
	MASTER = 7,
	SWEEP, CUT, PASTE, SNARF, SLASH, SCROLL

} Thing;

typedef struct Texture {
	uchar a[32];
	Bitmap *b;
} Texture;

typedef struct Blitmap {
	ulong *base;
	long zero;
	short width;
	short ldepth;
	Rectangle r;
	Bitmap *b;
} Blitmap;

char *intern(char *),*lookup(char *,Dict *);
char *assign(void *,void *,Dict *),*getline(char *,char *);
char *addachar(char *,int);
int showgrey,dombb(Rectangle*,Rectangle),selbox(Line *l,Rectangle r);
int selborder(Line *l,int mask,Rectangle r),collinear(Line *,Line *,Rectangle);
Line *newline(Point,Line *),*newbox(Point,Line *);
Line *newmacro(Point,Line *),*newstring(Point,int,Line *);
Line *newref(char *,Line *),*newinst(Point,char *,Line *),*newmaster(char *,Line *);
Line *cut(Line **),*copy(Line *),*newdots(Point,Line *);
File *newfile(char *);
Rectangle minbb,canon(Rectangle),mygetrect(int),gridrect(Rectangle);
Rectangle mbb(Line *,Dict *),mastermbb(Master *,Dict *);
Point and(Point,Point),rclipl(Rectangle,Rectangle),origin(void);
Point gridpt(Point);
Bitmap display,*stage,*maptex(Texture *),*mapblit(Blitmap *);
int getgrid(void);
int draw(Line *,Bitmap *,Fcode,int);
int drawinst(Inst *,Bitmap *,Fcode);
int drawstring(String *,Bitmap *,Fcode);
int drawrect(Rectangle,Bitmap *,Fcode);
int filter(Rectangle),readfile(char *);
int setstring(String *,char *);
int append(Line **,Line *);
int slashbox(Line *,Rectangle);
int drag(int,int);
int unselect(Line **,int);
int panic(char *);
int selpt(Line *,Point);
int snap(Line **);
int textcode(Line *,Point);
int addathing(Thing);
int alphkeys(Dict *,char **);
int addchars(String *,char *);
int removel(Line *);
int addaref(Line **,char *);
int move(Line *,Point);
int allmove(Line *,Point);
int complain(char *,char *);
Dict *rehash(Dict *,int);
int initfile(File *);
int mtime(int);
int cursoron(void),cursoroff(void);
