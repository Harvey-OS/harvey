typedef struct Edgept Edgept;
typedef struct Polys Polys;
typedef struct Maptile Maptile;

struct Edgept {
	int	side;	/* 0x1 N, 0x2 E, 0x4 S, 0x8 W */
	int	dist;
	int	flags;
	int	polynr;
	Poly	*pol;
	Edgept	*peer;	/* other end of poly */		
	Edgept	*next;
	Edgept	*prev;
};

struct Polys {
	Poly	**pol;
	int		npol;
};

struct Maptile {
	int		lat;		/* southernmost latitude in ° */
	int		lon;		/* westernmost longitude in ° */
	int		res;		/* sides of tile in ° */
	Rectangle	fr;		/* lat/lon square, in µ° */
	int		background;	/* Sea or Land */
	Edgept		*edgepts;	/* circular list of edgepoints */
	Polys		cp;		/* circular polys, islands and lakes */
	Polys		features[4];	/* River, Riverlet, Border, Stateline */
};

extern	uchar		minimap[180*360];
extern	long		debug;

void	finishpoly(Maptile *m, Poly *pol);
void	drawriver(Maptile *, int, void *arg);
void	drawcoast(Maptile *, int flags, void *arg);
void	drawcoastline(Maptile *, void *arg);
int 	clockwise(Poly *pol, Rectangle *rr);
void	freeedge(Edgept *);
