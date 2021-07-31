#pragma	src	"/sys/src/libgnot"
#pragma	lib	"libgnot.a"

extern void	*bbmalloc(int);
extern void	bbfree(void *, int);
extern int	bbonstack(void);
extern void	bbexec(void(*)(void), int, int);

/*
 * Graphics types
 */

typedef	struct	GBitmap		GBitmap;
typedef struct	GFont		GFont;
typedef struct	GSubfont	GSubfont;
typedef struct	GCacheinfo	GCacheinfo;

struct	GBitmap
{
	ulong	*base;		/* pointer to start of data */
	long	zero;		/* base+zero=&word containing (0,0) */
	ulong	width;		/* width in 32 bit words of total data area */
	int	ldepth;		/* log base 2 of number of bits per pixel */
	Rectangle r;		/* rectangle in data area, local coords */
	Rectangle clipr;	/* clipping region */
	GBitmap	*cache;		/* zero; distinguishes bitmap from layer */
};


/*
 * GFont etc. are not used in the library, only in devbit.c.
 * GSubfont is only barely used.
 */
struct	GSubfont
{
	short	n;		/* number of chars in font */
	char	height;		/* height of bitmap */
	char	ascent;		/* top of bitmap to baseline */
	Fontchar *info;		/* n+1 character descriptors */
	GBitmap	*bits;		/* where the characters are */
};
struct GCacheinfo
{
	ulong		xright;	/* right edge of bits */
	Fontchar;
};

struct GFont
{
	uchar		height;	/* max height of bitmap, interline spacing */
	char		ascent;	/* top of bitmap to baseline */
	char		width;	/* widest so far; used in caching only */	
	char		ldepth;	/* of images */
	short		id;	/* of font */
	int		ncache;	/* number of entries in cache */
	GCacheinfo	*cache;	/* cached characters */
	GBitmap		*b;	/* cached images */
};

extern ulong	 *gaddr(GBitmap*, Point);
extern uchar	 *gbaddr(GBitmap*, Point);
extern void	 gbitblt(GBitmap*, Point, GBitmap*, Rectangle, Fcode);
extern void	 gbitbltclip(void*);
extern void	 gtexture(GBitmap*, Rectangle, GBitmap*, Fcode);
extern Point	 gsubfstrsize(GSubfont*, char*);
extern int	 gsubfstrwidth(GSubfont*, char*);
extern Point	 gsubfstring(GBitmap*, Point, GSubfont*, char*, Fcode);
extern Point	 gbitbltstring(GBitmap*, Point, GSubfont*, char*, Fcode);
extern void	 gsegment(GBitmap*, Point, Point, int, Fcode);
extern void	 gpoint(GBitmap*, Point, int, Fcode);
extern void	 gflushcpucache(void);
extern GBitmap*	 gballoc(Rectangle, int);
extern void	 gbfree(GBitmap*);
