#include	<u.h>
#include	<libc.h>
#include	<libg.h>


extern void gerberhseg(int x1, int x2, int y);
static void hseg(int x1, int x2, int y);
static Rectangle canvas;

typedef struct {
	double lo, hi;
	double m, b;	/* edge is x = my+b */
	int horiz;	/* unless horiz in which case its m,lo -> b,lo */
} Edge;

static int
fcmp(double *a, double *b)
{
	if(*a < *b)
		return(-1);
	else if(*a > *b)
		return(1);
	return(0);
}

void gerberfill(double *pts, int n, int width)
{
	Edge *e, *le, *edges;
	double *f, *ep, *cr;
	double lx, ly, x, y;
	double miny, maxy;
	int i, j;

	if((edges = malloc(n/2*sizeof(Edge))) == 0)
		exits("edge malloc");
	if((cr = malloc(n*sizeof(cr[0]))) == 0)
		exits("cross malloc");
	maxy = miny = pts[1];
	lx = pts[n-2];
	ly = pts[n-1];
	e = edges;
	for(f = pts, ep = f+n; f < ep; e++){
		x = *f++;
		y = *f++;
		if(y < miny) miny = y;
		if(y > maxy) maxy = y;
		if(ly < y)
			e->lo = ly, e->hi = y;
		else
			e->lo = y, e->hi = ly;
		if(y == ly){
			e->horiz = 1;
			if(lx > x)
				e->m = x, e->b = lx;
			else
				e->m = lx, e->b = x;
		} else {
			e->horiz = 0;
			e->m = (x-lx)/(double)(y-ly);
			e->b = lx - ly*e->m;
		}
		if(0)
			fprint(2, "e: lo=%.0f hi=%.0f horiz=%d m=%.0f b=%.0f [l=%.0f,%.0f; x=%.0f,%.0f]\n", e->lo, e->hi, e->horiz, e->m, e->b, lx, ly, x, y);
		lx = x;
		ly = y;
	}
	le = e;
	if(0)
		fprint(2, "%d egdes!\n", le-edges);
	for(y = miny+width/2; y < maxy; y += width-2){
		i = 0;
		for(e = edges; e < le; e++){
			if((y < e->lo) || (y > e->hi))
				continue;
			if(e->horiz){
				cr[i++] = e->m;
				cr[i++] = e->b;
			} else
				cr[i++] = e->m*y + e->b;
		}
		qsort(cr, i, sizeof(cr[0]), fcmp);
		if(0){
			fprint(2, "y=%.1f: %d crossings:", y, i);
			for(j = 0; j < i; j++)
				fprint(2, " %.2f", cr[j]);
			fprint(2, "\n");
		}
		for(j = 0; j < i; j += 2){
			x = cr[j] + width/4;
			lx = cr[j+1] - width/4;
			if(x < lx)
				gerberhseg(x, lx, y);
		}
	}
	free(cr);
	free(edges);
}
