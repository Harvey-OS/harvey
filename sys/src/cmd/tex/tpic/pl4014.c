/* replacement for pltroff.c to call Cherry 4014 plot routines

   Ellipses are drawn as circles and splines as straight lines;
   I don't understand the definitions in plot(5).
*/

#include <stdio.h>
#include <math.h>
#include "pic.h"
#include "lib4014/tek.h"

double rangex, rangey;  /* probably already available inside pic somewhere */
double textht;
extern int dbg;
int frameno;

/*-----------copied from old version----------*/

arrow(x0, y0, x1, y1, w, h, ang, nhead) 	/* draw arrow (without shaft) */
	double x0, y0, x1, y1, w, h, ang;	/* head wid w, len h, rotated ang */
	int nhead;				/* and drawn with nhead lines */
{
	double alpha, rot, drot, hyp;
	float dx, dy;
	int i;

	rot = atan2(w / 2, h);
	hyp = sqrt(w/2 * w/2 + h * h);
	alpha = atan2(y1-y0, x1-x0) + ang;
	if (nhead < 2)
		nhead = 2;
	for (i = nhead-1; i >= 0; i--) {
		drot = 2 * rot / (double) (nhead-1) * (double) i;
		dx = hyp * cos(alpha + PI - rot + drot);
		dy = hyp * sin(alpha + PI - rot + drot);
		line(x1+dx, y1+dy, x1, y1);
	}
}


/*-----------new code----------*/

printlf(line,name)
	int line;
	char *name;
{
}

fillstart(v)	/* this works only for postscript, for now */
	double v;
{
}

fillend()
{
}

troff(s)
	char *s;
{
}

space(x0, y0, x1, y1)	/* set limits of page */
	double x0, y0, x1, y1;
{
	double midx = .5*(x0+x1);
	double midy = .5*(y0+y1);
	double r;
	rangex = x1 - x0;
	rangey = y1 - y0;
	r = .6*((rangex>rangey)? rangex: rangey);
	range(midx-r,midy-r,midx+r,midy+r);
	frame(.2,0.,1.2,1.);
	textht = .07*r;
	if(frameno++) ppause();
}

dot()
{
	disc(e1->copyx,e1->copyy,.005/(rangex+rangey));
}

label(s, t, nh)	/* text s of type t nh half-lines up */
	char *s;
	int t, nh;
{
	char *p, *malloc();

	if (t & ABOVE)
		nh++;
	else if (t & BELOW)
		nh--;
	if (nh)
		rmove(0.,.5*nh*textht);
	t &= ~(ABOVE|BELOW);
	rmove(0.,-.2*textht); /* tek 4014 correction */
	if (t & LJUST) {
		text2(s);
	} else {
		p = malloc((3+strlen(s))*(sizeof(char)));
		if (t & RJUST) {
			strcpy(p,"\\R");
			strcat(p,s);
			text2(p);
		} else {	/* CENTER */
			strcpy(p,"\\C");
			strcat(p,s);
			text2(p);
		}
		free(p);
	}
}

spline(x, y, n, p, dashed, ddval)
	double x, y;
	float *p;
	double n;	/* sic */
	int dashed;
	double ddval;
{
	double x2, y2;
	int k, j;
	for(k=0, j=0; k<n; k++, j+=2){
		x2 = x + p[j];
		y2 = y + p[j+1];
		line(x,y,x2,y2);
		x = x2;
		y = y2;
	}
}

ellipse(x, y, r1, r2)
	double x, y, r1, r2;
{
	circle(x,y,(r1+r2)/2);
}

/*-----------calling sequence from pic, code from lib4014----------*/

arc(x, y, x0, y0, x1, y1)	/* draw arc with center x,y */
	double x, y, x0, y0, x1, y1;
{
  devarc(x0, y0, x1, y1, x, y, 1 );   /* radius=1 means counterclockwise */
}

circle(x, y, r)
	double x, y, r;
{
	if(r<.001*(rangex+rangey)){
		line(x,y,x,y);
	}else{
		devarc(x+r,y,x+r,y,x,y,-r);
	}
}

/******* old code, to be discarded after new version soaks *******/
arc2(x1, y1, x2, y2, xc, yc, rr)
double	x1, x2, y1, y2, xc, yc, rr;
{
	register double dx, dy, a, b;
	double	ph, dph, rd, xnext;
	double quantum = .2/e1->scalex;
	register int	n;
	dx = x1 - xc;
	dy = y1 - yc;
	rd = sqrt(dx * dx + dy * dy);
	dprintf("\narc x2 y2: %g %g\n",x2,y2);
	if (rd < quantum) { 
		move(xc, yc); 
		vec(xc, yc); 
		return(0);
	}
	dph = acos(1.0 - (quantum / rd));
	if (dph > PI/4) 
		dph = PI/4;
	ph=atan2((y2-yc),(x2 - xc)) - atan2(dy, dx);
	if (ph < 0) 
		ph += 2*PI; 
	if (rr < 0) 
		ph = 2*PI - ph;
	if (ph < dph) 
		line(x1, y1, x2, y2);
	else {
		n = ph / dph;
		if(n<2) n = 2;
		dph = ph / n;
		a = cos(dph); 
		b = sin(dph); 
		if (rr < 0) 
			b = -b;
		move(x1, y1);
		while ((n--) > 0) {
			xnext = dx * a - dy * b; 
			dy = dx * b + dy * a; 
			dx = xnext;
			vec(dx + xc, dy + yc);
			dprintf("\narc vec to: %g %g\n", dx + xc, dy + yc);
		}
		vec(x2,y2);
	}
}
/******** end of code to be discarded *******/


#define N 0104
#define E 0101
#define NE 0105
#define S 0110
#define W 0102
#define SW 0112
/*	arrange by incremental plotting that an initial
 *	character such as +, X, *, etc will fall
 *	right on the point, and undo it so that further
 *	labels will fall properly in place
 */
#define POINTPL	036
#define BACKSP	''
char	lbl_mv[] = {
	POINTPL,040,S,S,S,S,S,S,SW,SW,SW,SW,SW,SW,SW,SW,SW,SW,PLOTOUT,0
};
char	lbl_umv[] = {
	POINTPL,040,N,N,N,N,N,N,NE,NE,NE,NE,NE,NE,NE,NE,NE,NE,PLOTOUT,0
};
char	plotout[] = {
	PLOTOUT,0
};
text2(s)
char	*s;
{
	register int	n;
	register char *p;
	int centered, right, newline, more, tweek;
	double y;

	while(1){
	n = centered = right = newline = more = tweek = 0;
	for(p=s; *p != '\0'; p++){
		if(*p == '\\'){
			switch(*(++p)){
			case 'C': centered++;
				s = p+1;
				continue;
			case 'R': right++;
				s = p+1;
				continue;
			case 'n': newline++;
				*(p-1) = '\0';
				if(*(p+1) != '\0')more++;
				goto output;
			case 'L': s=p+1;
				continue;
			}
		}
		else if( (n==0) && (*p=='+' || *p == '*' || *p == 'x' ||
			*p == 'X' || *p == '.' || *p == 'o'))tweek++;
		else n = 1;
	}
output:
	if(tweek)
		printf("%s",lbl_mv);
	else printf("%s",plotout);
	n = 0;
	if(centered) n = (p - s)/2 + 1;
	else if(right)n = p - s +1;
	if(n > 0)
		while(--n)
			putchar(BACKSP);
	*p = '\0';
	printf("%s", s);
	if(tweek)
		printf("%s",lbl_umv);
	if(newline){
		y = SCY(e1->copyy) ;
		y -= 34.;
		e1->copyy = (y - e1->bottom)/e1->scaley + e1->ymin;
	}
	move(e1->copyx, e1->copyy);
	if(!more)break;
	s = p+1;
	}
}
