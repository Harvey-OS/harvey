/*was jerq.h*/
#ifdef FAX
#define FXMAX	1728
#define XMAX	1600
#define YMAX	2200
#define DPI	200.
#define XBYTES	200
#else
#ifdef HIRES
#define FXMAX	2592
#define XMAX	2400
#define YMAX	3300
#define DPI	300.
#define XBYTES	300
#else
#define XMAX	800
#define YMAX	1100
#define DPI	100.
#define XBYTES	100
#endif
#endif
typedef int Word;
#define WORDSIZE	32

#ifndef Plan9
typedef struct Point {
	short x,y;
} Point;

typedef struct Rectangle {
	Point min, max;	
} Rectangle;
typedef long Texture[32];
typedef struct Bitmap {
	Rectangle r;
	unsigned width;
	Word *base;
} Bitmap;
typedef
enum	Fcode
{
	Zero	=	0x0,
	S	=	0xc,
	D	=	0xa,
	DandnotS	=	0x2
}Fcode;
Texture ones, zeros, *pgrey[17];
extern Texture grey[];
void bitblt(Bitmap *, Point, Bitmap *, Rectangle, Fcode);
Rectangle Rect(int, int, int, int);
long *addr(register Bitmap *, Point);
Bitmap *balloc(Rectangle, int);
void texture(Bitmap *, Rectangle, void *, Fcode);		/*plan9 Texture*/
Point Pt(int, int);
void point(Bitmap *, Point, int, Fcode);
Bitmap screen;
#define ONES	(~0)

#else
extern Bitmap *ones, *zeros, *pgrey[];
#endif

extern Rectangle cborder, Drect;
extern Bitmap *bp;
#define rectf(b,r,f)	texture(b,r,ones,f)

long *	faddr(Bitmap *, Point );
Bitmap *cachealloc(Rectangle);
void mybinit(void);
void ckbdry(int, int);
void putbitmap(Bitmap *, Rectangle, char *, int);
void putbits(FILE *);
void openblt(char *);
void done(int);
void alarmcatch(void);
void ispsi(void);
void getwindow(void);
short readshort(void);
void nsendchar(unsigned char);
void sendchar(unsigned char);
void sendshort(int);
void sendnchar(unsigned char *, int);
int ckmouse(int);
void putrast(FILE *);
