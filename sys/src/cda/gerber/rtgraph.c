#include	<u.h>
#include	<libc.h>
#include	<libg.h>
#include	"rtgraph.h"

static Rectangle world;
static int wpi;
static Bitmap *grey;
static int greyv;
static Point origin;

int cmap[16] = { [Black] ~0, [White] 0, [Grey]2 };

void
G_init(int inch, int mix, int miy, int max, int may)
{
	binit(berror, (char *)0, "hahaha");
	bitblt(&screen, screen.r.min, &screen, screen.r, Zero);
	bflush();
	grey = balloc((Rectangle){(Point){0, 0}, (Point){1, 1}}, 1);
	if(grey == 0){
		fprint(2, "grey balloc failed\n");
		exits("grey balloc");
	}
	point(grey, (Point){0,0}, cmap[Grey], S);
	wpi = inch;
	world.min = (Point){mix, miy};
	world.max = (Point){max, may};
	origin = add(screen.r.min, (Point){100, 0});
}

void
G_exit(void)
{
	bflush();
	bclose();
}

void
G_flush(void)
{
	bflush();
}

static Rectangle
map_rect(int llx, int lly, int urx, int ury)
{
	Rectangle r;

	r.min.x = origin.x + (llx - world.min.x)*DPI/wpi;
	r.max.x = origin.x + (urx - world.min.x)*DPI/wpi;
	r.min.y = origin.y + (world.max.y - ury)*DPI/wpi;
	r.max.y = origin.y + (world.max.y - lly)*DPI/wpi;
	return(r);
}

static double
mapx(double x)
{
	return(origin.x + (x - world.min.x)*DPI/wpi);
}

static double
mapy(double y)
{
	return(origin.y + (world.max.y - y)*DPI/wpi);
}

static double
map(double v)
{
	return(v*DPI/wpi);
}

void
G_rect(int mix, int miy, int max, int may, Shade col)
{
	Rectangle r;

	r = map_rect(mix, miy, max, may);
	switch(col)
	{
	case White:
		bitblt(&screen, r.min, &screen, r, Zero);
		break;
	case Black:
		bitblt(&screen, r.min, &screen, r, DxorS);
		break;
	case Grey:
		texture(&screen, r, grey, DxorS);
		break;
	}
}

void
G_line(int ax, int ay, int bx, int by, Shade col)
{
	segment(&screen, (Point){mapx(ax), mapy(ay)}, (Point){mapx(bx), mapy(by)}, cmap[col], F);
}

void
G_disc(int x, int y, int dia, Shade col)
{
	disc(&screen, (Point){mapx(x), mapy(y)}, map(dia/2)+.5, cmap[col], S);
}

void
G_outline(int mix, int miy, int max, int may)
{
	Rectangle r;

	r = map_rect(mix, miy, max, may);
	border(&screen, r, 1, F);
}

void
G_quad(double a, double b, double c, double d, double e, double f, double g, double h, Shade col)
{
	int cnt[2];
	double vv[8], *v, *vvv[1];

	v = vv;
	*v++ = mapx(a);
	*v++ = mapy(b);
	*v++ = mapx(c);
	*v++ = mapy(d);
	*v++ = mapx(e);
	*v++ = mapy(f);
	*v++ = mapx(g);
	*v = mapy(h);
	cnt[0] = 4;
	cnt[1] = 0;
	vvv[0] = vv;
	polyfill(cnt, vvv, cmap[col]);
}

void
G_poly(double *v, int n, Shade col)
{
	int cnt[2];
	int i;
	double *vv[1];

	for(i = 0; i < n; i++){
		v[i] = mapx(v[i]);
		i++;
		v[i] = mapy(v[i]);
	}
	cnt[0] = n/2;
	cnt[1] = 0;
	vv[0] = v;
	polyfill(cnt, vv, cmap[col]);
}
