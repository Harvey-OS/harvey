#define CACHE_MLIMIT	50
#define CACHE_CLIMIT	190
#ifdef FAX
#define CACHE_SPACE	390000
#define CACHE_BLIMIT	800
#else
#ifdef HIRES
#define CACHE_SPACE	1100000
#define CACHE_BLIMIT	6000
#else
#define CACHE_SPACE	1000000
#define CACHE_BLIMIT	300
#endif
#endif
struct cachefont{
	struct object cfontid;
	struct object cfonts;
	double cmatrix[4];
	struct Bitmap *charbits;
	int cwidth, cheight;
	int charno;
	int sequence;
	int internal;
	struct pspoint origin, upper;
	struct chars {
		Rectangle edges;
		double yadj;		/*adjustment for +90 degree rotation*/
		double texy;		/*adjustment for tex imagemask*/
		double xadj;		/*italic xleft*/
		int lastused;
		int value;
		double gwidth, gheight;
	}cachec[CACHE_CLIMIT];
};
extern int Fonts;
extern int blimit;
extern struct cachefont *currentcache, cachefont[];

void clipchar(double, double, double, double);
int smallpath(double, double);
int findfno(void);
void mcachechar(void);
int putcachech(void);
void putfcache(int, double, double, double, double);
int getbytes(double, double, double, double);
Bitmap *cachealloc(Rectangle);
extern int texfont, fw, fh;
extern struct pspoint forigin;
