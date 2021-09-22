#pragma src "/sys/src/libworldmap"
#pragma lib "libworldmap.a"

enum {
/* Don't interchange the order of 0-3 and 4-7 */
	River		= 0,	/* 0x01 */
	Riverlet	= 1,	/* 0x02 */
	Border		= 2,	/* 0x04 */
	Stateline	= 3,	/* 0x08 */
	Coastline	= 4,	/* 0x10 */
	Sea		= 5,	/* 0x20 */
	Land		= 6,	/* 0x40 */
	Unknown		= 7,	/* 0x80 */
	Nmaptype	= 8,
};

typedef struct Poly Poly;

struct Poly {
	Point	*p;
	int	np;
};

extern	char	*mapbase;

void	mapinit(void);
void	*readtile(int, int, int);
void	drawtile(void*, int, void*);
void	freetile(void*);
void	drawpoly(Poly*, int, void*);
