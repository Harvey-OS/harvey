/*
 * $Header: jerq.h,v 1.3 84/05/11 09:18:39 bart Exp $
 *	a re-creation of the jerq world
 */
typedef int	Word;

typedef struct Point {
	short x,y;
} Point;
typedef struct Rectangle {
	Point origin, corner;	
} Rectangle;
typedef long Texture[32];	/* I still don't believe it! */
typedef struct Bitmap {
	Word *base;
	unsigned width;
	Rectangle rect;
	char	*_null;
} Bitmap;

#define F_STORE	0
#define F_OR	1
#define F_CLR	2
#define F_XOR	3

#define WORDSIZE	32
#define WORDMASK	(WORDSIZE-1)
#ifdef FAX
#define XMAX	1600
#define YMAX	2200
#define DPI	200.
#define XBYTES	200
#else
#define XMAX	800
#define YMAX	1100
#define DPI	100.
#define XBYTES	100
#endif
