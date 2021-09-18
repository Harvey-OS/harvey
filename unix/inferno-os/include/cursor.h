/*
 * This is a separate file because image.h cannot be
 * included in many of the graphics drivers due to
 * name conflicts
 */

typedef struct Drawcursor Drawcursor;
struct Drawcursor
{
	int	hotx;
	int	hoty;
	int	minx;
	int	miny;
	int	maxx;
	int	maxy;
	uchar*	data;
};

void	drawcursor(Drawcursor*);
