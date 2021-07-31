void	_gbitblt(GBitmap*, Point, GBitmap*, Rectangle, Fcode);
void	_gtexture(GBitmap*, Rectangle, GBitmap*, Fcode);
void	_gsegment(GBitmap*, Point, Point, int, Fcode);
void	_gpoint(GBitmap*, Point, int, Fcode);
void	hwscreenwrite(int, int);

/* for devbit.c */

#define	gbitblt		_gbitblt
#define	gtexture	_gtexture
#define	gsegment	_gsegment
#define	gpoint		_gpoint

#define mbbpt(x)
#define mbbrect(x)
#define screenupdate()
#define mousescreenupdate()

extern void setcursor(Cursor*);
extern int hwgcmove(Point);
