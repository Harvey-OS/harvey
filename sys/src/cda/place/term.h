#define button1() (mouse.buttons & 1)
#define button3() (mouse.buttons & 4)
#define rectf(X,Y,Z) texture(X,Y,grey,Z)
#define Drect screen.r
#define origin min
#define corner max
#define F_XOR S^D
#define F_OR S|D
#define F_STORE S
#define F_CLR 0
#define	muldiv(a,b,c)	(((a)*(b))/(c))
#define Max(x,y) ((x>y)?x:y)
#define Min(x,y) ((x<y)?x:y)

struct Screen
{
	Rectangle sr;		/* window into board, scrn coords */
	Rectangle br;		/* sr in board coords */
	Rectangle bmax;		/* max(br, bmaxx) */
	Rectangle bmaxx;	/* board bounds */
	Rectangle bmr;		/* mapped br */
	Rectangle map;		/* map rect */
	Rectangle panel;	/* control panel including bname, coord */
	Rectangle bname;	/* where board name goes */
	Rectangle coord;	/* where mouse coords go */
	Point bd;		/* dimension of bmax */
	short mag;
	short grid;
	Font *font;
	char showo;		/* show overlay */
	char shown;		/* show chip names, not types */
	char showp;		/* show pins */
	char selws;		/* sel comp side */
};
extern struct Screen scrn;

typedef struct Pin 
{
	Point pos;
	short type;
} Pin;


typedef struct Chip
{
	Rectangle br, sr;
	Point pmin;
	short name, type;	/* into chipstr */
	short npins;
	Pin * pins;
	short flags;
	short id;
	Bitmap * b;
} Chip;
#define		PLACE		0x0003		/* must be same as fizz.h */
#define			SOFT	0x0000		/* must be same as fizz.h */
#define			HARD	0x0001		/* must be same as fizz.h */
#define			NOMOVE	0x0002		/* must be same as fizz.h */
#define		UNPLACED	0x0004		/* must be same as fizz.h */
#define		IGRECT		0x0008		/* must be same as fizz.h */
#define		IGPINS		0x0010		/* must be same as fizz.h */
#define		IGNAME		0x0020		/* must be same as fizz.h */
#define		WSIDE		0x0080		/* on wiring side */
#define		IGPIN1		0x0040
#define		SELECTED	0x0100
#define		EOLIST		0x0200
#define		PLACED		0x0400
#define		MAPPED		0x0800

typedef struct Pinhole
{
	Rectangle r;
	Point sp;
} Pinhole;

typedef struct Signal
{
	short name;		/* 0 means end of list */
	short npts;
	short id;
	Point *pts;
} Signal;

typedef struct Plane
{
	short signo;
	Rectangle r;
} Plane;

#define		NCURSIG		512

struct Board
{
	char *name;
	Pinhole *pinholes;
	Chip *chips;
	char *chipstr;
	Signal *sigs;
	short cursig[NCURSIG];
	short ncursig;
	short resig;		/* set to indicate gdrawsig, not drawsig */
	short nplanes;
	Plane *planes;
	short planemap[6];
	short nkeepouts;
	Plane *keepouts;
	short keepoutmap[6];
};
extern Bitmap * grey;
extern struct Board b;
extern Mouse mouse;
extern Bitmap * colour;
extern char * caption;
extern uchar colourbits[];
#define		UNITMAG		10

extern Cursor blank;
Rectangle rstob(Rectangle), rbtos(Rectangle);
Point pstob(Point), pbtos(Point);
Point confine(Point, Rectangle), snap(Point), ident(Point);
Point midpt(Point, Point);
Rectangle rmax(Rectangle, Rectangle);
Rectangle pan_it(int, Rectangle, Rectangle, Point (*)(Point), Point (*)(Point), void (*)(Rectangle));
extern godot;
extern int ncolours;
void draw(void), xdraw(void), odraw(void);
Point Cstring(Bitmap *, Point, Font * , char *, Fcode);
void border(Bitmap *, Rectangle, int, Fcode);
void rot(uchar *, int, uchar *, int, int, int);
int offset(int , int , int);
int getlvl(uchar *, int, int , int ); 
void putlvl(uchar * , int , int , int , int );
void Cbitblt(Bitmap *, Point, Bitmap *, Rectangle, Fcode);
void bepatient(char *);
void bedamned(void);
void expand(char *);
void pan(void);
int button(int);
void getmouse(void);
void select(Chip *);
void Crectf(Bitmap *, Rectangle, Fcode);
int rinr(Rectangle, Rectangle);
void panto(Point );
int newtype(Chip *, Chip *, char *);
void drawmap(void);
void drawcolourmap(void);
void Clip(Rectangle);
void drawplane(int);
void Ctexture(Bitmap *, Rectangle, Bitmap *, Fcode);
void twostring(Point, char *, char *);
void track(void);
int Connect(char *, char *);
void put1(int);
void rectblk(Rectangle);
void puts(char *);
void putn(int);
void hosterr(char *s, int bored);
void draw(void);
int rcv(void);
void drawsig(int);
void Csegment(Bitmap *, Point , Point , Fcode);
void drawoverlay(void);
void putp(Point);
void drawkeepout(int);
int sqbrack(char **, int);
char * getline(char *,char *);
void newbmax(void);
void draw(void);
char * getpath(char *);
int rcvchar(void);
void panic(char *);
void drawchip(Chip *);
void getsigs(short *);
void planecolours(void);
void improveinfo(int);
void drawchip(Chip *);
void gdrawsig(int );
void getmydir(void);
void call_host(char *);
void but1(void);
void menu2(Mouse *);
void menu3(Mouse *);
void sendnchars(int, char *);
void iconinit(void);
/* Rectangle getrect(int); */
