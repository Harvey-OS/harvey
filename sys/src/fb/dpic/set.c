/*
 * procedures that set type, scan converting troff output primitives
 */
#include "ext.h"
#define	FACEDIR	"/sys/lib/troff/contour"
#define	LINEWID	.007		/* in inches, actually half-width */
#define	ERR	.5		/* in pixels, maximum deviation of approximating polygons */
Typeface *face[MAXFONTS+1];
Rgba color={255,255,255,255};
Rgba bgcolor={0,0,0,0};
Rgba str2color(char *c){
	int r, g, b, a;
	if(sscanf(c, "%d%d%d%d", &r, &g, &b, &a)!=4
	|| a<0 || 255<a || r<0 || a<r || g<0 || a<g || b<0 || a<b)
		error("bad color %s", c);
	return (Rgba){r,g,b,a};
}
/*
 * set the color of scan-converted primitives
 */
void setcolor(char *c){
	color=str2color(c);
}
void setbg(char *c){
	bgcolor=str2color(c);
}
/*
 * Clear the screen to the background color
 */
void setclear(void){
	Rgba *p, *ep=&page[pagehgt-1][pagewid];
	for(p=&page[0][0];p!=ep;p++) *p=bgcolor;
}
void setpicfile(char *buf){
	char name[1024];
	int x0, y0, y, nc, i;
	PICFILE *f;
	unsigned char *r0, *g0, *b0, *a0, *line, *r, *g, *b, *a, a255;
	Rgba *pp, *ep;
	if(sscanf(buf, "%s%d%d", name, &x0, &y0)!=3)
		error("bad X picfile %s", buf);
	x0=x0*dpi/res;
	y0=y0*dpi/res;
	f=picopen_r(name);
	if(f==0) error("bad picfile %s", name);
	if(x0<0 || x0+PIC_WIDTH(f)>=pagewid || y0<0 || y0>=pagewid+PIC_HEIGHT(f))
		error("picture clipped (window %d %d %d %d)",
			x0, x0+PIC_WIDTH(f), y0, y0+PIC_HEIGHT(f));
	nc=PIC_NCHAN(f);
	line=emalloc(PIC_WIDTH(f)*nc);
	r0=g0=b0=a0=0;
	for(i=0;f->chan[i];i++) switch(f->chan[i]){
	case 'm': r0=g0=b0=line+i; break;
	case 'r': r0=line+i; break;
	case 'g': g0=line+i; break;
	case 'b': b0=line+i; break;
	case 'a': a0=line+i; break;
	}
	if(r0==0) r0=line;
	if(g0==0) g0=line;
	if(b0==0) b0=line;
	for(y=0;y!=PIC_HEIGHT(f);y++){
		r=r0; g=g0; b=b0;
		pp=&page[y0+y][x0];
		ep=pp+PIC_WIDTH(f);
		picread(f, line);
		if(a0){
			a=a0;
			for(;pp!=ep;pp++){
				a255=255-*a;
				pp->r=a255*pp->r/255+*r; r+=nc;
				pp->g=a255*pp->g/255+*g; g+=nc;
				pp->b=a255*pp->b/255+*b; b+=nc;
				pp->a=a255*pp->a/255+*a; a+=nc;
			}
		}
		else{
			for(;pp!=ep;pp++){
				*pp=(Rgba){*r,*g,*b,255};
				r+=nc;
				g+=nc;
				b+=nc;
			}
		}
	}
	free(line);
	picclose(f);
}
/*
 * Flush the page to a file and get set for the next page
 */
void setpage(int pagenum){
	PICFILE *f;
	char name[100];
	int y;
	if(pagenum>=0){
		sprintf(name, "%s.%d", flag['s']?flag['s'][0]:"page", pagenum);
		f=picopen_w(name, "runcode", xmin, ymin, pagewid, pagehgt, "rgba", 0, 0);
		if(f==0){
			picerror(name);
			exits("open page");
		}
		for(y=0;y!=pagehgt;y++)
			picwrite(f, (char *)page[y]);
		picclose(f);
	}
	setclear();
}
/*
 * scan convert a character
 */
void setchar(int c, int x, int y){
	char name[300];
	Fonthd *font;
	Typeface *f;
	Glyph *g;
	Boundary *b, *eb;
	Point *p, *ep;
	double rps;		/* relative point size */
	double x0, y0, x1, y1;
	font=fmount[curfont];
	if(face[curfont]==0){
		sprintf(name, "%s/%s", FACEDIR, font->name);
		face[curfont]=rdtypeface(name);	/* wrong! should indirect through mount */
	}
	f=face[curfont];
	if(c<=0 || f->nglyph<c){
		fprintf(stderr, "Bad code %d font %d\n", c, curfont);
		return;
	}
	g=&f->glyph[c];
	rps=size/36.;		/* font outlines are for 36 point characters */
	tilestart();
	for(eb=g->boundary+g->nboundary,b=g->boundary;b!=eb;b++){
		ep=&b->pt[b->npt];
		x0=(hpos+ep[-1].x*rps)/res*dpi;
		y0=(vpos-ep[-1].y*rps)/res*dpi;
		for(p=b->pt;p!=ep;p++){
			x1=(hpos+p->x*rps)/res*dpi;
			y1=(vpos-p->y*rps)/res*dpi;
			tileedge(x1, y1, x0, y0);
			x0=x1;
			y0=y1;
		}
	}
	tileend();
}
/*
 * scan convert a line
 */
void setline(int ix0, int iy0, int ix1, int iy1){
	double x0=ix0*dpi/res, y0=iy0*dpi/res;
	double x1=ix1*dpi/res, y1=iy1*dpi/res;
	double ny=x1-x0, nx=y0-y1;
	double l;
	l=nx*nx+ny*ny;
	if(l==0) return;
	l=dpi*LINEWID/sqrt(l);
	nx*=l;
	ny*=l;
	tilestart();
	tileedge(x0+nx, y0+ny, x1+nx, y1+ny);
	tileedge(x1+nx, y1+ny, x1-nx, y1-ny);
	tileedge(x1-nx, y1-ny, x0-nx, y0-ny);
	tileedge(x0-nx, y0-ny, x0+nx, y0+ny);
	tileend();
}
/*
 * scan convert an ellipse
 * Bug:
 *	The edges of offset lines from an ellipse are not quite elliptical.
 *	However the offset lines from a circle are circular, and it's only
 *	for large aspect ratios that we incur significant error.
 */
void setellipse(int xl, int yl, int wid, int hgt){
	double xc=(xl+.5*wid)*dpi/res, yc=(yl+.5*wid)*dpi/res;
	double xr=.5*wid*dpi/res, yr=.5*hgt*dpi/res;
	double w=LINEWID*dpi;
	tilestart();
	elcontour(xc, yc, xr+w, yr+w, -1.);
	elcontour(xc, yc, xr-w, yr-w, 1.);
	tileend();
}
void elcontour(double xc, double yc, double xr, double yr, double sense){
	double x0, y0, x1, y1;
	int i, nseg;
	double err, theta;
	if(xr<=0 || yr<=0) return;
	err=1.-ERR/(.5*(xr+yr));
	if(err<=0 || (nseg=ceil(PI/acos(err)))<8) nseg=8;
	x0=xc+xr;
	y0=yc;
	for(i=1;i<=nseg;i++){
		theta=2*PI*i/nseg*sense;
		x1=xc+xr*cos(theta);
		y1=yc+yr*sin(theta);
		tileedge(x0, y0, x1, y1);
		x0=x1;
		y0=y1;
	}
}
/*
 * scan convert an arc
 */
void setarc(int x0, int y0, int x1, int y1, int ixc, int iyc){
	double dx=(x0-ixc)*dpi/res, dy=(y0-iyc)*dpi/res;
	double t0=atan2(dy, dx), t1=atan2(y1-iyc, x1-ixc);
	double xc=ixc*dpi/res, yc=iyc*dpi/res;
	double w=LINEWID*dpi;
	double r=sqrt(dx*dx+dy*dy);
	double c, s;
	if(t1>=t0) t1-=2*PI;
	tilestart();
	arccontour(xc, yc, r+w, t0, t1);
	c=cos(t1);
	s=sin(t1);
	tileedge(xc+(r+w)*c, yc+(r+w)*s, xc+(r-w)*c, yc+(r-w)*s);
	arccontour(xc, yc, r-w, t1, t0);
	c=cos(t0);
	s=sin(t0);
	tileedge(xc+(r-w)*c, yc+(r-w)*s, xc+(r+w)*c, yc+(r+w)*s);
	tileend();
}
void arccontour(double xc, double yc, double r, double t0, double t1){
	double x0, y0, x1, y1;
	int i, nseg;
	double err, theta;
	if(r<=0) return;
	err=1.-ERR/r;
	if(err<=0 || (nseg=ceil(fabs(t1-t0)/acos(err)))<8) nseg=8;
	x0=xc+r*cos(t0);
	y0=yc+r*sin(t0);
	for(i=1;i<=nseg;i++){
		theta=t0+(t1-t0)*i/nseg;
		x1=xc+r*cos(theta);
		y1=yc+r*sin(theta);
		tileedge(x0, y0, x1, y1);
		x0=x1;
		y0=y1;
	}
}
/*
 * scan convert a spline segment
 *	x=(.5*(ix2-2*ix1+ix0)*t+ix1-ix0)*t+.5*(ix1+ix0)
 *	y=(.5*(iy2-2*iy1+iy0)*t+iy1-iy0)*t+.5*(iy1+iy0)
 * Converting to segments is a bit cheezy
 */
#define	DT	.0625
void setspline(int ix0, int iy0, int ix1, int iy1, int ix2, int iy2){
	double xc2=.5*(ix2-2*ix1+ix0), xc1=ix1-ix0, xc0=.5*(ix1+ix0);
	double yc2=.5*(iy2-2*iy1+iy0), yc1=iy1-iy0, yc0=.5*(iy1+iy0);
	double x0, y0, x1, y1;
	double t;
	x0=xc0;
	y0=yc0;
	for(t=DT;t<=1.;t+=DT){
		x1=(xc2*t+xc1)*t+xc0;
		y1=(yc2*t+yc1)*t+yc0;
		setline(x0, y0, x1, y1);
		y0=y1;
		x0=x1;
	}
}
void setclrwin(char *buf){
	int x0, y0, x1, y1, x, y;
	if(sscanf(buf, "%d%d%d%d", &x0, &y0, &x1, &y1)!=4) error("bad clrwin");
	if(x1<x0){ x=x1; x1=x0; x0=x; }
	if(y1<y0){ y=y1; y1=y0; y0=y; }
	x0=floor(x0*dpi/res);
	y0=floor(y0*dpi/res);
	x1=ceil(x1*dpi/res);
	y1=ceil(y1*dpi/res);
	if(x0<0) x0=0;
	if(y0<0) y0=0;
	if(x1>=pagewid) x1=pagewid-1;
	if(y1>=pagehgt) y1=pagehgt-1;
	for(y=y0;y<=y1;y++) for(x=x0;x<=x1;x++) page[y][x]=bgcolor;
}
void setborder(char *buf){
	int x0, y0, x1, y1, x, y;
	if(sscanf(buf, "%d%d%d%d", &x0, &y0, &x1, &y1)!=4) error("bad clrwin");
	if(x1<x0){ x=x1; x1=x0; x0=x; }
	if(y1<y0){ y=y1; y1=y0; y0=y; }
	x0=floor(x0*dpi/res);
	y0=floor(y0*dpi/res);
	x1=ceil(x1*dpi/res);
	y1=ceil(y1*dpi/res);
	if(x0<0) x0=0;
	if(y0<0) y0=0;
	if(x1>=pagewid) x1=pagewid-1;
	if(y1>=pagehgt) y1=pagehgt-1;
	if(x0>x1 || y0>y1) return;
	for(y=y0;y<=y1;y++){
		page[y][x0]=color;
		page[y][x1]=color;
	}
	for(x=x0;x<=x1;x++){
		page[x][y0]=color;
		page[x][y1]=color;
	}
}
