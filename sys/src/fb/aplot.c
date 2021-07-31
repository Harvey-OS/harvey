/*
 * anti-aliased isoplot into a file
 * does this really obey window?
 */
#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
#include <stdio.h>
float **data;
int ndata;
typedef struct Pixel{
	unsigned char m, a;
}Pixel;
Pixel **pic;
int wid=640, hgt=512;
typedef struct Acc{
	float accm, acca;
	float m, a;
}Acc;
Acc *col;
int lo, hi;
float *line, *linealpha, *stripe, *stripealpha;
#define	NLIGHT	16
int nlight=1;
struct light{
	float lx, ly, lz;	/* direction to light */
	float lum;		/* brightness of light */
	float sx, sy, sz;	/* direction to hilite */
}light[NLIGHT]={
	2, 3, 9, 1, 0, 0
};
float spec=0;			/* amount of specular reflection */
float bump=80;			/* exponent of specular bump */
float diff=.98;			/* amount of diffuse reflection */
float ambient=.02;		/* amount of ambient light */
double range;
void erread(int f, char *p, int n){
	if(read(f, p, n)!=n){
		fprint(2, "aplot: eof!\n");
		exits("eof");
	}
}
#define	cvt(name, type)\
void name(int f, float *fp, int n)\
{\
	type line[8192];	/* bug, could be too small! */\
	type *lp=line;\
	int i;\
	erread(f, (char *)line, n*sizeof(type));\
	for(i=0;i!=n;i++)\
		*fp++=*lp++;\
}
cvt(cint, int);
cvt(cshort, short);
cvt(clong, long);
cvt(cdouble, double);
cvt(cchar, char);
cvt(cuchar, unsigned char);
void cfloat(int f, float *fp, int n){
	erread(f, (char *)fp, n*sizeof(float));
}
void getlights(char *);
int size(int, int);
void fixlights(void);
void adraw(int);
void *emalloc(long n){
	void *v=malloc(n);
	if(v==0){
		fprint(2, "aplot: no space\n");
		exits("no space");
	}
	return v;
}
main(int argc, char *argv[]){
	float hi=0, scale;
	register i, j, f;
	char *picargv[10];
	char window[1024];
	char *line;
	void (*crack)(int, float *, int);
	int x0=0, y0=0;
	PICFILE *outf;
	/* Crack flags */
	if(getflags(argc, argv, "al:1[lightfile]t:1[type]r:1[range]w:4[x0 y0 x1 y1]")!=2)
		usage("infile");
	if(flag['l']) getlights(flag['l'][0]);
	if(flag['r']) range=atof(flag['r'][0]);
	if(flag['w']){
		x0=atoi(flag['w'][0]);
		y0=atoi(flag['w'][1]);
		wid=atoi(flag['w'][2]-x0);
		hgt=atoi(flag['w'][3]-y0);
	}
	if(wid<=0 || hgt<=0){
		fprint(2, "%s: output window too small (%dx%d)\n", argv[0], wid, hgt);
		exits("window too small");
	}
	pic=emalloc(hgt*sizeof(Pixel *));
	pic[0]=emalloc(wid*hgt*sizeof(Pixel));
	memset(pic[0], 0, wid*hgt*sizeof(Pixel));
	for(i=1;i!=hgt;i++) pic[i]=pic[i-1]+wid;
	line=emalloc(2*wid*sizeof(char));
	col=emalloc(hgt*sizeof(Acc));
	line=emalloc(hgt*sizeof(float));
	linealpha=emalloc(hgt*sizeof(float));
	stripe=emalloc(hgt*sizeof(float));
	stripealpha=emalloc(hgt*sizeof(float));
	lo=hgt;
	hi=0;
	switch(flag['t']?flag['t'][0][0]:'f'){
	case 's': i=sizeof(short);	crack=cshort;	break;
	case 'i': i=sizeof(int);	crack=cint;	break;
	case 'l': i=sizeof(long);	crack=clong;	break;
	case 'f': i=sizeof(float);	crack=cfloat;	break;
	case 'd': i=sizeof(double);	crack=cdouble;	break;
	case 'c': i=sizeof(char);	crack=cchar;	break;
	case 'u': i=sizeof(unsigned char);crack=cuchar;	break;
	default: fprint(2, "bad type %c\n", flag['t'][0][0]); exits("bad type");
	}
	/* Check and read input */
	if((f=open(argv[1], 0))<0){
		perror(argv[1]);
		exits("open input");
	}
	ndata=size(f, i);
	if(ndata<=0){
		fprint(2, "%s: data file too small (ndata=%d)\n", argv[0], ndata);
		exits("too small");
	}
	data=emalloc(ndata*sizeof(float *));
	data[0]=emalloc(ndata*ndata*sizeof(float));
	for(i=1;i!=ndata;i++) data[i]=data[i-1]+ndata;
	fixlights();
	for(i=0;i!=ndata;i++){
		(*crack)(f, data[i], ndata);
		for(j=0;j!=ndata;j++)
			if(fabs(data[i][j])>hi)hi=fabs(data[i][j]);
	}
	fprint(2, "size %dx%d, range %g\n", ndata, ndata, hi);
	/* Create output */
	outf=picopen_w("OUT", "runcode", x0, y0, wid, hgt, flag['a']?"m":"ma", argv, 0);
	if(outf==0){
		perror("OUT");
		exits("creat output");
	}
	/* Rescale input data */
	scale=flag['r']?range:hi;
	if(scale!=0.){
		for(i=0;i!=ndata;i++)for(j=0;j!=ndata;j++)
			data[i][j]*=ndata/scale;
	}
	/* Draw image */
	for(j=0;j!=wid;j++)
		adraw(j);
	/* Write output */
	for(i=0;i!=hgt;i++){
		if(flag['a']){
			for(j=0;j!=wid;j++)
				line[j]=pic[i][j].m;
		}
		else{
			for(j=0;j!=wid;j++){
				line[2*j]=pic[i][j].m;
				line[2*j+1]=pic[i][j].a;
			}
		}
		picwrite(outf, line);
	}
	exits("");
}
int size(int f, int l){
	Dir stbuf;
	register i;
	dirfstat(f, &stbuf);
	for(i=0;i*i*l<stbuf.length;i++);
	if(i*i*l!=stbuf.length)
		fprint(2, "Non-square file padded to %dx%d\n", i, i);
	return i;
}
float sample(int x, int y){
	if(x<0) x=0; else if(ndata<=x) x=ndata-1;
	if(y<0) y=0; else if(ndata<=y) y=ndata-1;
	return data[y][x];
}
double fmin(double a, double b){ return a<b?a:b; }
int min(int a, int b){ return a<b?a:b; }
double fmax(double a, double b){ return a>b?a:b; }
int max(int a, int b){ return a>b?a:b; }
int clamp(double a){
	return a<0?0:a<255?a:255;
}
float intens(int x, int y){
	struct light *l, *el=&light[nlight];
	float nx, ny, nz, v, len;
	nx=sample(x+1, y)-sample(x-1, y);
	ny=sample(x, y+1)-sample(x, y-1);
	nz=2;
	len=sqrt(nx*nx+ny*ny+nz*nz);
	nx/=len;
	ny/=len;
	nz/=len;
	v=ambient;
	for(l=light;l!=el;l++)
		v+=l->lum*(diff*fmax(nx*l->lx+ny*l->ly+nz*l->lz, 0.)
			+spec*pow(fmax(nx*l->sx+ny*l->sy+nz*l->sz, 0.), bump));
	return v;
}
typedef struct{
	int x, y, z;
	float tx, tz, pv;
}Point3d;
int inside(Point3d p){
	return(0<=p.x && p.x<ndata && 0<=p.y && p.y<ndata);
}
Point3d add3(Point3d p, Point3d q){
	p.x+=q.x;
	p.y+=q.y;
	p.z+=q.z;
	return(p);
}
Point3d Pt3(int x, int y, int z){
	Point3d p;
	p.x=x;
	p.y=y;
	p.z=z;
	return(p);
}
/*
 * samples transformed into screen coordinates
 */
void tsample(Point3d *p){
	p->tz=sample(p->x, p->y)+(p->x+p->y)/2.+hgt/4;
	p->tx=p->y-p->x+wid/2;
	p->pv=intens(p->x, p->y);
}
void flushline(void);
void accumulate(Point3d, Point3d, Point3d);
void flushstripe(int);
/*
 * draw an antialised vertical stripe of the picture in pic[.][j].
 * This stripe maps into the line y=j-wid/2+x in the data array.
 * Points [x, y, f(x, y)] map to [(x+y)/2+f(x, y)+hgt/4, y-x+wid/2]
 * In front to back order, we accumulate sub-stripes into an array that
 * gets composited into the ultimate vertical scanline and zeroed every time the
 * stripe changes direction from up to down or vice versa.
 */
#define	trap(a, b) ((b.tx-a.tx)*(a.tz+b.tz))
void adraw(int j){
	int dir=0, plo;
	Point3d p, q, r;
	p.x=0;
	p.y=j-ndata;
	tsample(&p);
	q=add3(p, Pt3(0, 1, 0));
	if(!inside(p)){
		p=add3(p, Pt3(-p.y, -p.y, 0));
		tsample(&p);
		if(!inside(p))
			return;
	}
	tsample(&q);
	if(!inside(q)){
		q=add3(q, Pt3(-q.y, -q.y, 0));
		tsample(&q);
		if(!inside(q))
			return;
	}
	for(;;){
		plo=p.x+p.y<q.x+q.y;
		if(plo)
			r=add3(p, Pt3(1, 1, 0));
		else
			r=add3(q, Pt3(1, 1, 0));
		if(!inside(r))
			break;
		tsample(&r);
		if((trap(p, q)+trap(q, r)<trap(p, r))!=dir){
			flushline();
			dir=!dir;
		}
		accumulate(p, q, r);
		if(plo)
			p=r;
		else
			q=r;
	}
	flushstripe(j);
}
void flushline(void){
	int i;
	for(i=lo;i<=hi;i++){
		if(stripe[i]<1. && fabs(col[i].a-col[i].acca)>1e-3){
			col[i].m+=col[i].accm*(1-col[i].a);
			col[i].a+=col[i].acca*(1-col[i].a);
		}
		col[i].accm=0;
		col[i].acca=0;
	}
	lo=hgt;
	hi=0;
}
void flushstripe(int j){
	int i;
	flushline();
	for(i=0;i!=hgt;i++){
		pic[hgt-1-i][j].m=clamp(col[i].m*256);
		pic[hgt-1-i][j].a=clamp(col[i].a*256);
		col[i].m=0;
		col[i].a=0;
	}
}
void accumulate(Point3d p, Point3d q, Point3d r){
	Point3d s, t;
	register float a, newa, da, y, z, dz, z0, z1, pv;
	pv=r.pv;
	/* sort so that p.tz<=q.tz<=r.tz */
	if(p.tz>q.tz){ t=p; p=q; q=t; }
	if(q.tz>r.tz){ t=q; q=r; r=t; }
	if(p.tz>q.tz){ t=p; p=q; q=t; }
	if(r.tz==p.tz)
		return;
	s.tz=q.tz;
	s.tx=p.tx+(r.tx-p.tx)*(q.tz-p.tz)/(r.tz-p.tz);
	if(q.tx>s.tx){ t=q; q=s; s=t; }
	/* p.tz<=q.tz==s.tz<=r.tz and q.tx<=s.tx */
	if(p.tz!=q.tz){
		y=(s.tx-q.tx)/(q.tz-p.tz)/2.;
		a=0.;
		z0=ceil(p.tz);
		z1=ceil(q.tz);
		for(z=z0;z<=z1;z+=1){
			dz=fmin(z, q.tz)-p.tz;
			newa=dz*dz*y;
			da=newa-a;
			if(0<z && z<=hgt){
				col[(int)z-1].accm+=da*pv;
				col[(int)z-1].acca+=da;
			}
			a=newa;
		}
	}
	if(r.tz!=q.tz){
		y=(s.tx-q.tx)/(r.tz-q.tz)/2.;
		a=0.;
		z0=floor(r.tz);
		z1=floor(q.tz);
		for(z=z0;z>=z1;z-=1){
			dz=r.tz-fmax(z, q.tz);
			newa=dz*dz*y;
			da=newa-a;
			if(0<=z && z<hgt){
				col[(int)z].accm+=da*pv;
				col[(int)z].acca+=da;
			}
			a=newa;
		}
	}
	lo=max(0, min((int)floor(p.tz), lo));
	hi=min(hgt-1, max((int)ceil(r.tz), hi));
}
void fixlights(void){
	register struct light *lp, *el=&light[nlight];
	float l;
	for(lp=light;lp!=el;lp++){
		l=sqrt(lp->lx*lp->lx+lp->ly*lp->ly+lp->lz*lp->lz);
		if(l!=0.){
			lp->lx/=l;
			lp->ly/=l;
			lp->lz/=l;
		}
		lp->sx=lp->lx+.5;
		lp->sy=lp->ly+.5;
		lp->sz=lp->lz+sqrt(.5);
		l=sqrt(lp->sx*lp->sx+lp->sy*lp->sy+lp->sz*lp->sz);
		if(l!=0.){
			lp->sx/=l;
			lp->sy/=l;
			lp->sz/=l;
		}
	}
}
void getlights(char *name){
	register struct light *lp=light;
	register FILE *fd;
	double x, y, z, lum;
	char line[512];
	if((fd=fopen(name, "r"))==NULL){
		perror(name);
		exits("open lights");
	}
	while(fgets(line, sizeof line, fd)!=NULL){
		if(sscanf(line, "light%lf%lf%lf%lf", &x, &y, &z, &lum)==4){
			if(lp==&light[NLIGHT]){
				fprint(2, "aplot: can only do %d lights\n",
					NLIGHT);
				exits("too many lights");
			}
			lp->lx=x;
			lp->ly=y;
			lp->lz=z;
			lp->lum=lum;
			lp++;
		}else if(sscanf(line, "ambient%lf", &x)==1)
			ambient=x;
		else if(sscanf(line, "spec%lf", &x)==1)
			spec=x;
		else if(sscanf(line, "diff%lf", &x)==1)
			diff=x;
		else if(sscanf(line, "bump%lf", &x)==1)
			bump=x;
		else{
			if(line[0])
				line[strlen(line-1)]='\0';
			fprint(2, "aplot: what means `%s'?\n", line);
		}
	}
	nlight=lp-light;
	fclose(fd);		
}
