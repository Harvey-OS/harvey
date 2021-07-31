#ifdef	Rect
#undef	Rect
#endif
#define	Rxy(x1, y1, x2, y2)	((Rectangle){Pt(x1, y1), Pt(x2, y2)})

typedef struct Clump	Clump;
typedef struct FPoint	FPoint;
typedef struct Hershey	Hershey;
typedef struct Hfont	Hfont;
typedef struct Inc	Inc;
typedef struct Path	Path;
typedef struct Rcomp	Rcomp;
typedef struct Rect	Rect;
typedef struct Repeat	Repeat;
typedef struct Ring	Ring;
typedef struct Side	Side;
typedef struct Sym	Sym;
typedef struct Text	Text;

struct FPoint{
	float	x;
	float	y;
};

struct Sym{
	Sym *	next;
	char *	name;
	void *	ptr;
	int	type;
};

enum{
	Keyword = 1,
	Clname,
	Layer,
	Operator
};

enum{
	BLOB,
	CLUMP,
	END,
	ETC,
	INC,
	PATH,
	RCOMP,
	RECT,
	RECTI,
	REPEAT,
	RING,
	SIDE,
	TEXT
};

struct Clump{
	int	type;
	int	n;
	int	size;
	void **	o;
};

struct Inc{
	int	type;
	Sym *	clump;
	Point	pt;
	int	angle;
};

struct Path{
	int	type;
	Sym *	layer;
	int	width;
	int	n;
	int	size;
	Point *	edge;
};

struct Rect{
	int	type;
	Sym *	layer;
	Rectangle;
};

struct Repeat{
	int	type;
	Sym *	clump;
	Point	pt;
	int	count;
	Point	inc;
};

struct Ring{
	int	type;
	Sym *	layer;
	Point	pt;
	int	diam;
};

struct Text{
	int	type;
	Sym *	layer;
	int	size;
	Point	start;
	int	angle;
	Sym *	text;
};

struct Side{
	Side *	next;
	int	sense;
	int	x;
	int	ymin;
	int	ymax;
};

struct Rcomp{
	Rcomp *	next;
	Rectangle;
};

struct Hershey{
	int	gnum;
	uchar	stroke[2];
};

struct Hfont{
	int	addr[96];
	Hershey *h[96];
};

#define	Required	0x80000000

extern Sym *	op_plus;
extern Sym *	op_minus;

extern Biobuf	out;
extern int	strunique;
extern int	debug;
extern char *	infile;
extern int	lineno;
extern Clump *	allclump;
extern Sym *	master;
extern int	rflag;
extern int	colors[];
extern int	colori;
