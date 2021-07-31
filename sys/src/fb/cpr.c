/*% cyntax % && cc -go # % -lpicfile -lfb
 * cpr -- cheezy polygon renderer.
 * syntax:
 *	{						push transformation stack
 *	}						pop transformation stack
 *	v fov near far ex ey ez lx ly lz ux uy uz	Set viewing parameters
 *	Q r i j k					Quaternion rotate
 *	R angle axis					rotate
 *	T dx dy dz					translate
 *	S sx sy sz					scale
 *	M m00 m01 m02 m03 m10 m11 ... m33		transformation matrix
 *	t x0 y0 z0 x1 y1 z1 x2 y2 z2 c0 c1		Render a triangle
 *	p c0 c1 [x y z]*;				Render a polygon
 *	c num r g b a					Set a color table entry
 *	l x y z						Set light source direction
 *	b r g b a					Clear the background
 *	h c0 c1 nx ny x0 y0 x1 y1 file			Render a height field
 *	s c0 c1 x y z r					Render a sphere
 */
#include <u.h>
#include <libc.h>
#include <libg.h>
#include <stdio.h>
#include <fb.h>
#include <geometry.h>
void depthclip(void);
void geode(Point3 *a, Point3 *b, Point3 *c, Point3 cen, double r, int c1, int c2, int n);
int getnormal(void);
void input(FILE *ifd);
void insert(struct edge *e, struct edge **bp);
double len(Point3 p);
void main(int argc, char *argv[]);
char *mkchan(void);
void normalize(Point3 *v);
void outline(PICFILE *f, unsigned char *p);
void polyren(int c, int other);
void readbg(FILE *f);
void readcolor(FILE *f);
void readcomment(FILE *f);
void readheight(FILE *f);
void readlight(FILE *f);
void readmat(FILE *f);
void readpoly(FILE *f);
void readpop(FILE *f);
void readpush(FILE *f);
void readquat(FILE *f);
void readrot(FILE *f);
void readscale(FILE *f);
void readsphere(FILE *f);
void readtran(FILE *f);
void readtri(FILE *f);
void readview(FILE *f);
void span(int z, double x0, double y0, double x1, double y1);
#define	F_UNCOVERED	HUGE
/*
 * Screen characteristics
 */
double aspect;			/* aspect ratio of whole picture */
int nz=480;
int nx=640;
typedef struct color{
	unsigned char r, g, b, a;
}Color;
struct pix{
	Color rgba;
	float y;
}**pix;
Color rgba;
#define	NCOLOR	500
Color color[NCOLOR];
Point3 light={-.6, .48, .64, 1};
Point3 eye;
Point3 yclip;		/* actually a plane */
#define	NEDGE	2048
Point3 vert[NEDGE];
Point3 *evert;
struct edge{
	Point3 p, q;
	double x, y, dx, dy;
	struct edge *next;
}edge[NEDGE];
struct edge *eedge;
Point3 normal;
int npoly=1;
struct edge **buck;
struct edge **b0, **b1;
#define	R	0x01
#define	G	0x02
#define	B	0x04
#define	A	0x08
#define	Z	0xf0
int chan;
Space *mx;		/* modeling space */
Space *wx;		/* world space */
void main(int argc, char *argv[]){
	struct pix *pp;
	int i, z;
	PICFILE *ofd;
	FILE *ifd;
	argc=getflags(argc, argv, "a:1[aspect]c:1[rgbaz]w:2[nx nz]");
	if(argc<=0) usage("file ...");
	if(flag['w']){
		nx=atoi(flag['w'][0]);
		nz=atoi(flag['w'][1]);
	}
	if(flag['a']) aspect=atof(flag['a'][0]);
	else aspect=(double)nz/nx;
	buck=malloc((nz+1)*sizeof(struct edge *));
	b0=&buck[0];
	b1=&buck[nz];
	if(buck==0){
	NoSpace:
		fprint(2, "%s: can't malloc\n", argv[0]);
		exits("no space");
	}
	pix=malloc((nz+1)*sizeof(struct pix *));	/* pix[nz] is end of array */
	if(pix==0) goto NoSpace;
	pix[0]=malloc(nx*nz*sizeof(struct pix));
	if(pix[0]==0) goto NoSpace;
	for(z=1;z<=nz;z++) pix[z]=pix[z-1]+nx;
	if((ofd=picopen_w("OUT", "runcode", 0, 0, nx, nz, mkchan(), 0, 0))==0){
		perror(argv[0]);
		exits("create output");
	}
	for(pp=pix[0];pp!=pix[nz];pp++)
		pp->y=F_UNCOVERED;
	for(z=0;z!=256;z++){
		color[z].r=color[z].g=color[z].b=z;
		color[z].a=255;
	}
	for(z=0;z!=12;z++){
		color[z+256].r=color[z+256].g=color[z+256].b=pow(255., z/11.);
		color[z+256].a=255;
	}
	for(z=0;z!=20;z++){
		color[z+268].r=color[z+268].g=color[z+268].b=pow(255., z/19.);
		color[z+268].a=255;
	}
	mx=pushmat(0);
	if(argc==1) input(stdin);
	else for(i=1;i!=argc;i++){
		ifd=fopen(argv[i], "r");
		if(ifd==0){
			perror(argv[i]);
			exits("open input");
		}
		input(ifd);
		fclose(ifd);
	}
	for(z=0;z!=nz;z++)
		outline(ofd, (unsigned char *)&pix[z][0]);
	exits("");
}
void input(FILE *ifd){
	int c;
	while((c=getc(ifd))!=EOF) switch(c){
	case ' ':
	case '\t':
	case '\n':
		break;
	default:
		fprintf(stderr, "Bad input `%c", c);
		while((c=getc(ifd))!='\n' && c!=EOF)
			putc(c, stderr);
		fprintf(stderr, "'\n");
		break;
	case '{': readpush(ifd); break;
	case '}': readpop(ifd); break;
	case 'v': readview(ifd); break;
	case 'Q': readquat(ifd); break;
	case 'R': readrot(ifd); break;
	case 'T': readtran(ifd); break;
	case 'S': readscale(ifd); break;
	case 'M': readmat(ifd); break;
	case 'c': readcolor(ifd); break;
	case 'l': readlight(ifd); break;
	case 'b': readbg(ifd); break;
	case 't': readtri(ifd); break;
	case 'p': readpoly(ifd); break;
	case 'h': readheight(ifd); break;
	case 's': readsphere(ifd); break;
	case '#': readcomment(ifd); break;
	}
}
char *mkchan(void){
	if(!flag['c'] || strcmp(flag['c'][0], "rgb")==0){
		chan=R|G|B;
		return "rgb";
	}
	if(strcmp(flag['c'][0], "rgba")==0){
		chan=R|G|B|A;
		return "rgba";
	}
	if(strcmp(flag['c'][0], "rgbaz")==0){
		chan=R|G|B|A|Z;
		return sizeof(float)==4?"rgbaz...":"rgbaz.......";
	}
	fprintf(stderr, "Bad channels %s\n", flag['c'][0]);
	exits("bad chan");
	return 0;	/* for cyntax */
}
void readcomment(FILE *f){
	register c;
	do
		c=getc(f);
	while(c!='\n' && c!=EOF);
}
void readbg(FILE *f){
	int r, g, b, a;
	Color c;
	register struct pix *p;
	if(fscanf(f, "%d%d%d%d", &r, &g, &b, &a)!=4){
		fprintf(stderr, "cpr: bad background\n");
		exits("bad bg");
	}
	c.r=r;
	c.g=g;
	c.b=b;
	c.a=a;
	for(p=pix[0];p!=pix[nz];p++) p->rgba=c;
}
void readcolor(FILE *f){
	int n, r, g, b, a;
	if(fscanf(f, "%d%d%d%d%d", &n, &r, &g, &b, &a)!=5 || n<0 || NCOLOR<=n){
		fprintf(stderr, "cpr: Bad color\n");
		exits("bad color");
	}
	color[n].r=r;
	color[n].g=g;
	color[n].b=b;
	color[n].a=a;
}
double len(Point3 p){
	return sqrt(p.x*p.x+p.y*p.y+p.z*p.z);
}
void readlight(FILE *f){
	double l;
	if(fscanf(f, "%lf%lf%lf", &light.x, &light.y, &light.z)!=3){
		fprintf(stderr, "cpr: Bad light\n");
		exits("bad light");
	}
	l=len(light);
	if(l!=0){
		light.x/=l;
		light.y/=l;
		light.z/=l;
	}
}
void readview(FILE *f){
	double fov, near, far;
	Point3 e, v, u;
	double l;
	e.w=1;
	v.w=1;
	u.w=1;
	if(fscanf(f, "%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf\n",
		&fov, &near, &far,
		&e.x, &e.y, &e.z, &v.x, &v.y, &v.z, &u.x, &u.y, &u.z)!=12){
		fprintf(stderr, "cpr: Bad view spec\n");
		exits("bad view");
	}
	/* viewport transformation, then perspective, then viewing direction */
	while(mx) mx=popmat(mx);
	mx=pushmat(0);
	move(mx, .5*nx, 0., .5*nz);
	scale(mx, .5*nx, 1., -.5*aspect*nz);
	persp(mx, fov, near, far);
	look(mx, e, v, u);
	wx=mx;
	mx=pushmat(mx);
	eye=e;
	yclip=sub3(v, e);
	l=near/sqrt(yclip.x*yclip.x+yclip.y*yclip.y+yclip.z*yclip.z);
	yclip.w=-((e.x+l*yclip.x)*yclip.x+(e.y+l*yclip.y)*yclip.y+(e.z+l*yclip.z)*yclip.z);
}
void readpush(FILE *f){
	mx=pushmat(mx);
}
void readpop(FILE *f){
	mx=popmat(mx);
}
void readquat(FILE *f){
	Quaternion q;
	if(fscanf(f, "%lf%lf%lf%lf", &q.r, &q.i, &q.j, &q.k)!=4
	|| q.r==0 && q.i==0 && q.j==0 && q.k==0){
		fprintf(stderr, "bad rotate\n");
		exits("bad rot");
	}
	q=qunit(q);
	qrot(mx, q);
}
void readrot(FILE *f){
	double theta;
	int axis;
	if(fscanf(f, "%lf%d", &theta, &axis)!=2 || axis<0 || 2<axis){
		fprintf(stderr, "bad rotate\n");
		exits("bad rot");
	}
	rot(mx, theta, axis);
}
void readscale(FILE *f){
	double sx, sy, sz;
	if(fscanf(f, "%lf%lf%lf", &sx, &sy, &sz)!=3){
		fprintf(stderr, "bad scale\n");
		exits("bad scale");
	}
	scale(mx, sx, sy, sz);
}
void readtran(FILE *f){
	double tx, ty, tz;
	if(fscanf(f, "%lf%lf%lf", &tx, &ty, &tz)!=3){
		fprintf(stderr, "bad translate\n");
		exits("bad translate");
	}
	move(mx, tx, ty, tz);
}
void readmat(FILE *f){
	Matrix m;
	int i, j;
	for(i=0;i!=4;i++) for(j=0;j!=4;j++) if(fscanf(f, "%lf", &m[i][j])!=1){
		fprintf(stderr, "bad matrix\n");
		exits("bad matrix");
	}
	xform(mx, m);
}
/*
 * Get the polygon's normal vector.  The normal is guaranteed pointing
 * towards the viewer.  The return value is 0 if the polygon passes its
 * vertices clockwise from the viewer's point of view and 1 if counterclockwise.
 */
int getnormal(void){
	register struct edge *p;
	Point3 a, b;
	double l;
	normal.x=normal.y=normal.z=0;
	normal.w=1;
	a.x=edge[1].p.x-edge[0].p.x;
	a.y=edge[1].p.y-edge[0].p.y;
	a.z=edge[1].p.z-edge[0].p.z;
	for(p=edge+2;p!=eedge;p++){
		b.x=p->p.x-edge[0].p.x;
		b.y=p->p.y-edge[0].p.y;
		b.z=p->p.z-edge[0].p.z;
		normal.x+=a.y*b.z-b.y*a.z;
		normal.y+=b.x*a.z-a.x*b.z;
		normal.z+=a.x*b.y-a.y*b.x;
		a.x=b.x;
		a.y=b.y;
		a.z=b.z;
	}
	l=len(normal);
	if(l==0) return 0;	/* what should we do here? */
	normal.x/=l;
	normal.y/=l;
	normal.z/=l;
	if(normal.x*(eye.x-edge[0].p.x)
	  +normal.y*(eye.y-edge[0].p.y)
	  +normal.z*(eye.z-edge[0].p.z)>=0){
		normal.x=-normal.x;
		normal.y=-normal.y;
		normal.z=-normal.z;
		return 1;
	}
	return 0;
}
void readpoly(FILE *f){
	Point3 in;
	int c, c0, c1;
	if(fscanf(f, "%d%d", &c0, &c1)!=2){
	Bad:
		fprintf(stderr, "cpr: bad polygon %d\n", npoly);
		exits("bad polygon");
	}
	in.x=in.y=in.z=0;
	in.w=1.;
	for(evert=vert;evert!=&vert[NEDGE];evert++){
		do
			c=getc(f);
		while(c==' ' || c=='\t' || c=='\n');
		if(c==';' || c==EOF) break;
		ungetc(c, f);
		if(fscanf(f, "%lf%lf%lf", &in.x, &in.y, &in.z)!=3)
			goto Bad;
		*evert=in;
	}
	if(evert<vert+3)
		return;
	polyren(c0, c1);
}
void readtri(FILE *f){
	Point3 in;
	int c0, c1;
	in.w=1.;
	for(evert=vert;evert!=&vert[3];evert++){
		if(fscanf(f, "%lf%lf%lf", &in.x, &in.y, &in.z)!=3)
			goto Bad;
		*evert=in;
	}
	if(fscanf(f, "%d%d", &c0, &c1)!=2){
	Bad:
		fprintf(stderr, "cpr: bad triangle\n");
		exits("bad triangle");
	}
	polyren(c0, c1);
}
void readheight(FILE *f){
	char name[512];
	FILE *data;
	float flt, f0, f1, *save;
	int nx, ny, x, y, c1, c2;
	double x0, y0, x1, y1, dx, dy;
	if(fscanf(f, "%d%d%d%d%lf%lf%lf%lf%s", &c1, &c2, &nx, &ny, &x0, &y0, &x1, &y1, name)!=9){
		fprintf(stderr, "ncpr: bad sampled surface\n");
		exits("bad height-field");
	}
	if(nx<=0 || ny<=0){
		fprintf(stderr, "ncpr: non-positive sample dimensions\n");
		exits("bad height-field");
	}
	dx=(x1-x0)/nx;
	dy=(y1-y0)/ny;
	data=fopen(name, "r");
	if(data==0){
		perror(name);
		exits("no data file");
	}
	save=(float *)malloc(nx*sizeof(float));
	if(save==0){
		fprintf(stderr, "Out of space\n");
		exits("no space");
	}
	for(x=0;x!=nx;x++){
		if(fread((char *)&flt, sizeof flt, 1, data)!=1){
		Eof:
			fclose(data);
			return;
		}
		save[x]=flt;
	}
	for(y=1;y!=ny;y++){
		if(fread((char *)&flt, sizeof flt, 1, data)!=1) goto Eof;
		f0=flt;
		for(x=1;x!=nx;x++){
			if(fread((char *)&flt, sizeof flt, 1, data)!=1) goto Eof;
			f1=flt;

			vert[0].x=x0+x*dx;
			vert[0].z=f1;
			vert[0].y=y0+y*dy;
			vert[0].w=1.;

			vert[1].x=x0+(x-1)*dx;
			vert[1].z=f0;
			vert[1].y=y0+y*dy;
			vert[1].w=1.;

			vert[2].x=x0+(x-1)*dx;
			vert[2].z=save[x-1];
			vert[2].y=y0+(y-1)*dy;
			vert[2].w=1.;

			evert=vert+3;
			polyren(c1, c2);

			vert[0].x=x0+x*dx;
			vert[0].z=f1;
			vert[0].y=y0+y*dy;
			vert[0].w=1.;

			vert[1].x=x0+(x-1)*dx;
			vert[1].z=save[x-1];
			vert[1].y=y0+(y-1)*dy;
			vert[1].w=1.;

			vert[2].x=x0+x*dx;
			vert[2].z=save[x];
			vert[2].y=y0+(y-1)*dy;
			vert[2].w=1.;

			evert=vert+3;
			polyren(c1, c2);
			save[x-1]=f0;
			f0=f1;
		}
		save[nx-1]=f0;
	}
	fclose(data);
	free((char *)save);
}
/*
 * sphere primitive -- really a period 8 geodesic dome
 */
Point3 sphvert[]={	/* icosahedron vertices */
 0.0000, 0.0000, 1.0, 1.0,  0.0000, 0.8660, 0.5, 1.0,  0.5090, 0.7006,-0.5, 1.0,
 0.8236, 0.2676, 0.5, 1.0,  0.8236,-0.2676,-0.5, 1.0,  0.5090,-0.7006, 0.5, 1.0,
 0.0000,-0.8660,-0.5, 1.0, -0.5090,-0.7006, 0.5, 1.0, -0.8236,-0.2676,-0.5, 1.0,
-0.8236, 0.2676, 0.5, 1.0, -0.5090, 0.7006,-0.5, 1.0,  0.0000, 0.0000,-1.0, 1.0
};
Point3 *sphface[][3]={	/* icosahedron faces */
&sphvert[ 0],&sphvert[1],&sphvert[ 3], &sphvert[ 0],&sphvert[ 3],&sphvert[5],
&sphvert[ 0],&sphvert[5],&sphvert[ 7], &sphvert[ 0],&sphvert[ 7],&sphvert[9],
&sphvert[ 0],&sphvert[9],&sphvert[ 1], &sphvert[ 1],&sphvert[ 2],&sphvert[3],
&sphvert[ 2],&sphvert[3],&sphvert[ 4], &sphvert[ 3],&sphvert[ 4],&sphvert[5],
&sphvert[ 4],&sphvert[5],&sphvert[ 6], &sphvert[ 5],&sphvert[ 6],&sphvert[7],
&sphvert[ 6],&sphvert[7],&sphvert[ 8], &sphvert[ 7],&sphvert[ 8],&sphvert[9],
&sphvert[ 8],&sphvert[9],&sphvert[10], &sphvert[ 9],&sphvert[10],&sphvert[1],
&sphvert[10],&sphvert[1],&sphvert[ 2], &sphvert[11],&sphvert[ 2],&sphvert[4],
&sphvert[11],&sphvert[4],&sphvert[ 6], &sphvert[11],&sphvert[ 6],&sphvert[8],
&sphvert[11],&sphvert[8],&sphvert[10], &sphvert[11],&sphvert[10],&sphvert[2],
};
void readsphere(FILE *f){
	Point3 cen;
	double rad;
	int c1, c2, i;
	if(fscanf(f, "%d%d%lf%lf%lf%lf", &c1, &c2, &cen.x, &cen.y, &cen.z, &rad)!=6){
		fprintf(stderr, "ncpr: bad sphere\n");
		exits("bad sphere");
	}
	for(i=0;i!=sizeof sphface/sizeof sphface[0];i++)
		geode(sphface[i][0], sphface[i][1], sphface[i][2], cen, rad, c1, c2, 3);
}
void geode(Point3 *a, Point3 *b, Point3 *c, Point3 cen, double r, int c1, int c2, int n){
	Point3 pa, pb, pc, p;
	if(n==0){
		vert[0].x=cen.x+a->x*r;
		vert[0].y=cen.y+a->y*r;
		vert[0].z=cen.z+a->z*r;
		vert[0].w=1;

		vert[1].x=cen.x+b->x*r;
		vert[1].y=cen.y+b->y*r;
		vert[1].z=cen.z+b->z*r;
		vert[1].w=1;

		vert[2].x=cen.x+c->x*r;
		vert[2].y=cen.y+c->y*r;
		vert[2].z=cen.z+c->z*r;
		vert[2].w=1;

		evert=&vert[3];
		polyren(c1, c2);
		return;
	}
	pa.x=b->x+c->x;
	pa.y=b->y+c->y;
	pa.z=b->z+c->z;
	normalize(&pa);
	pb.x=a->x+c->x;
	pb.y=a->y+c->y;
	pb.z=a->z+c->z;
	normalize(&pb);
	pc.x=b->x+a->x;
	pc.y=b->y+a->y;
	pc.z=b->z+a->z;
	normalize(&pc);
	geode(&pc, &pb, a, cen, r, c1, c2, n-1);
	geode(c, &pb, &pa, cen, r, c1, c2, n-1);
	geode(&pc, b, &pa, cen, r, c1, c2, n-1);
	geode(&pa, &pb, &pc, cen, r, c1, c2, n-1);
}
void normalize(Point3 *v){
	double l=sqrt(v->x*v->x+v->y*v->y+v->z*v->z);
	if(l!=0.){
		v->x/=l;
		v->y/=l;
		v->z/=l;
	}
}
void insert(struct edge *e, struct edge **bp){
	while(*bp && (*bp)->x<e->x)
		bp=&((*bp)->next);
	e->next=*bp;
	*bp=e;
}
void depthclip(void){
	Point3 q, *v;
	struct edge *e;
	int pin, qin;
	double pd, qd, alpha;
	q=evert[-1];
	qd=q.x*yclip.x+q.y*yclip.y+q.z*yclip.z+yclip.w;
	qin=qd>=0;
	eedge=edge;
	for(v=vert;v!=evert;v++){
		pd=v->x*yclip.x+v->y*yclip.y+v->z*yclip.z+yclip.w;
		pin=pd>=0;
		if(pin^qin){
			alpha=pd/(pd-qd);
			eedge->p.x=v->x+alpha*(q.x-v->x);
			eedge->p.y=v->y+alpha*(q.y-v->y);
			eedge->p.z=v->z+alpha*(q.z-v->z);
			eedge->p.w=1;
			eedge++;
		}
		if(pin) eedge++->p=*v;
		q=*v;
		qd=pd;
		qin=pin;
	}
}
void polyren(int c, int other){
	register struct edge *e, *f, *g, **b;
	register z;
	Point3 p, in, xin, n, *v;
	double t;
	Color col;
	++npoly;
	for(v=vert;v!=evert;v++) *v=xformpoint(*v, wx, mx);
	depthclip();
	if(eedge<edge+3) return;
	if(getnormal()) c=other;
	for(e=edge;e!=eedge;e++){
		e->p=xformpointd(e->p, 0, wx);
		if(e==edge)
			eedge[-1].q=e->p;
		else
			e[-1].q=e->p;
	}
	for(b=b0;b<=b1;b++)
		*b=0;
	for(e=edge;e!=eedge;e++){
		if(e->p.z>e->q.z){ p=e->p; e->p=e->q; e->q=p; }
		if(e->p.z==e->q.z || nz<=e->p.z || e->q.z<=0)
			continue;
		if(e->p.z<0)
			z=0;
		else
			z=ceil(e->p.z);
		if(z<e->q.z){
			t=(z-e->p.z)/(e->q.z-e->p.z);
			e->x=e->p.x+t*(e->q.x-e->p.x);
			e->y=e->p.y+t*(e->q.y-e->p.y);
			t=1/(e->q.z-e->p.z);
			e->dx=t*(e->q.x-e->p.x);
			e->dy=t*(e->q.y-e->p.y);
			insert(e, &buck[z]);
		}
	}
	if(c<0){
		c=-c;
		t=(normal.x*light.x+normal.y*light.y+normal.z*light.z+1)/2;
		rgba.r=(color[c].r&255)*t;
		rgba.g=(color[c].g&255)*t;
		rgba.b=(color[c].b&255)*t;
		rgba.a=color[c].a;
	}
	else if(c>=300){
		c-=300;
		rgba.r=c&0xFF;
		rgba.g=(c>>8)&0xFF;
		rgba.b=(c>>16)&0xFF;
		rgba.a=255;
	}
	else
		rgba=color[c];
	for(b=b0,z=0;b!=b1;b++,z++) if(*b){
		for(e=*b;e && (f=e->next);e=g){
			g=f->next;
			span(z, e->x, e->y, f->x, f->y);
			if(z+1<e->q.z){
				e->x+=e->dx;
				e->y+=e->dy;
				insert(e, b+1);
			}
			if(z+1<f->q.z){
				f->x+=f->dx;
				f->y+=f->dy;
				insert(f, b+1);
			}
		}
	}
			
}
void span(int z, double x0, double y0, double x1, double y1){
	register struct pix *pp;
	register x, ix0, ix1;
	double dy, y;
	if(x0==x1)
		return;
	ix0=ceil(x0);
	if(ix0<0)
		ix0=0;
	ix1=floor(x1);
	if(nx<=ix1)
		ix1=nx-1;
	dy=(y1-y0)/(x1-x0);
	for(x=ix0,y=y0+(ix0-x0)*dy,pp=&pix[z][ix0];x<=ix1;x++,y+=dy,pp++){
		if(y<pp->y){
			pp->rgba=rgba;
			pp->y=y;
		}
	}
}
void outline(PICFILE *f, unsigned char *p){
	register unsigned char *q;
	static unsigned char *obuf=0;
	register x;
	if(obuf==0){
		obuf=malloc(nx*8);
		if(obuf==0){
			fprint(2, "cpr: out of space\n");
			exits("no mem");
		}
	}
	if(chan==(R|G|B|A|Z) && sizeof(float)==4){
		picwrite(f, (char *)p);
		return;
	}
	q=obuf;
	for(x=0;x!=nx;x++){
		if(chan&R) *q++=*p++; else p++;
		if(chan&G) *q++=*p++; else p++;
		if(chan&B) *q++=*p++; else p++;
		if(chan&A) *q++=*p++; else p++;
		if(chan&Z){
			switch(sizeof(float)){
			default:
				fprintf(stderr,
				     "cpr: sizeof(float) is %d! rewrite outline!\n",
					sizeof(float));
				exits("bad size!");
			case 8: *q++=*p++; *q++=*p++; *q++=*p++; *q++=*p++;
			case 4: *q++=*p++; *q++=*p++; *q++=*p++; *q++=*p++;
			}
		}
		else
			p+=sizeof(float);
	}
	picwrite(f, (char *)obuf);
}
