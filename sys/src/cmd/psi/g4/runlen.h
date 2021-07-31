typedef struct Scan {	/* run-length-encoded scan line */
	short	*runN;
	short	run[2200];
} Scan;

typedef struct Bitrun {	/* run-length-encoded bitmap */
	Rectangle	r;
	Scan	*l;
} Bitrun;
typedef struct T6state {
	int fd;
	int bufct;
	unsigned long buf;
	int nch;
	uchar *chpt;
	uchar chbuf[4096];
} T6state;

extern Scan *runlen(void *, int, Scan *);

extern T6state	*t6winit(T6state *, int);
extern void	t6write(T6state *, Scan *, Scan *);
extern void	t6wclose(T6state *);

extern Rectangle	window(int);

extern T6state	*t6rinit(int);
extern Scan	*t6read(T6state*, Scan *);

enum { SOL, Pass, Vert, White, Black=White+1, EOL };

extern Bitmap	*blowup(Bitmap *, int);
extern void	printscan(int, int, Scan*);
extern void	t6(int, Scan *, Scan *);

extern Bitmap	*getrlemap(int);
extern Bitmap	*getbinarymap(int);
static uchar *t6flush(T6state *, uchar *);
