/*
 * view -- interactive cpr file viewer
 * syntax:
 *	{						push transformation stack
 *	}						pop transformation stack
 *	v fov near far ex ey ez lx ly lz ux uy uz	Set viewing parameters
 *	R angle axis					rotate
 *	Q r i j k					quaternion rotate
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
#include <stdio.h>
#include <libg.h>
#include <geometry.h>
#define	NCOLOR	500
void geode(Point3 *a, Point3 *b, Point3 *c, Point3 cen, double r, int c1, int c2, int n);
void input(FILE *ifd);
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
void view(void);
void polyren(int c, int other);
void normalize(Point3 *);
void standardize(void);
double dist2(Point3, Point3);
#define	EPS	.001
/*
 * Polygon input buffer
 */
#define	NPT	2048
Point3 pt[NPT];
Point3 *ept;
Point3 light;
typedef struct Edge Edge;
typedef struct Vert Vert;
struct Edge{
	int npoly;
	Vert *p, *q;
	Edge *next;
};
struct Vert{
	Point3 world, screen;
	Vert *next;
};
Space *mx;
Edge *edge;
Vert *vert;
int npoly;
void main(int argc, char *argv[]){
	FILE *ifd;
	int i;
	Edge *e, *ne;
	Vert *v, *nv;
	binit(0, 0, "view");
	einit(Emouse|Ekeyboard);
	for(;;){
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
		while(mx!=0) mx=popmat(mx);
		standardize();
		view();
		if(argc==1){
			fprintf(stderr, "%s: can't reread standard input\n", argv[0]);
			exits("reread stdin");
		}
		for(e=edge;e;e=ne){
			ne=e->next;
			free(e);
		}
		for(v=vert;v;v=nv){
			nv=v->next;
			free(v);
		}
		edge=0;
		vert=0;
	}
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
void readcomment(FILE *f){
	register c;
	do
		c=getc(f);
	while(c!='\n' && c!=EOF);
}
void readbg(FILE *f){
	int r, g, b, a;
	if(fscanf(f, "%d%d%d%d", &r, &g, &b, &a)!=4){
		fprintf(stderr, "cpr: bad background\n");
		exits("bad bg");
	}
}
void readcolor(FILE *f){
	int n, r, g, b, a;
	if(fscanf(f, "%d%d%d%d%d", &n, &r, &g, &b, &a)!=5 || n<0 || NCOLOR<=n){
		fprintf(stderr, "cpr: Bad color\n");
		exits("bad color");
	}
}
void readlight(FILE *f){
	if(fscanf(f, "%lf%lf%lf", &light.x, &light.y, &light.z)!=3){
		fprintf(stderr, "cpr: Bad light\n");
		exits("bad light");
	}
}
void readview(FILE *f){
	double fov, near, far, ex, ey, ez, vx, vy, vz, ux, uy, uz;
	if(fscanf(f, "%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf\n",
		&fov, &near, &far,
		&ex, &ey, &ez, &vx, &vy, &vz, &ux, &uy, &uz)!=12){
		fprintf(stderr, "cpr: Bad view spec\n");
		exits("bad view");
	}
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
void readpoly(FILE *f){
	Point3 in;
	int c, c0, c1;
	if(fscanf(f, "%d%d", &c0, &c1)!=2){
	Bad:
		fprintf(stderr, "cpr: bad polygon\n");
		exits("bad polygon");
	}
	in.x=in.y=in.z=0;
	in.w=1.;
	for(ept=pt;ept!=&pt[NPT];ept++){
		do
			c=getc(f);
		while(c==' ' || c=='\t' || c=='\n');
		if(c==';' || c==EOF) break;
		ungetc(c, f);
		if(fscanf(f, "%lf%lf%lf", &in.x, &in.y, &in.z)!=3)
			goto Bad;
		*ept=in;
	}
	if(ept<pt+3)
		return;
	polyren(c0, c1);
}
void readtri(FILE *f){
	Point3 in;
	int c0, c1;
	in.w=1.;
	for(ept=pt;ept!=&pt[3];ept++){
		if(fscanf(f, "%lf%lf%lf", &in.x, &in.y, &in.z)!=3)
			goto Bad;
		*ept=in;
	}
	if(fscanf(f, "%d%d", &c0, &c1)!=2){
	Bad:
		fprintf(stderr, "cpr: bad triangle\n");
		exits("bad triangle");
	}
	polyren(c0, c1);
}
void *emalloc(int n){
	void *v;
	v=malloc(n);
	if(v==0){
		fprintf(stderr, "out of space");
		exits("no mem");
	}
	return v;
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
	save=(float *)emalloc(nx*sizeof(float));
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

			pt[0].x=x0+x*dx;
			pt[0].z=f1;
			pt[0].y=y0+y*dy;
			pt[0].w=1.;

			pt[1].x=x0+(x-1)*dx;
			pt[1].z=f0;
			pt[1].y=y0+y*dy;
			pt[1].w=1.;

			pt[2].x=x0+(x-1)*dx;
			pt[2].z=save[x-1];
			pt[2].y=y0+(y-1)*dy;
			pt[2].w=1.;

			ept=pt+3;
			polyren(c1, c2);

			pt[0].x=x0+x*dx;
			pt[0].z=f1;
			pt[0].y=y0+y*dy;
			pt[0].w=1.;

			pt[1].x=x0+(x-1)*dx;
			pt[1].z=save[x-1];
			pt[1].y=y0+(y-1)*dy;
			pt[1].w=1.;

			pt[2].x=x0+x*dx;
			pt[2].z=save[x];
			pt[2].y=y0+(y-1)*dy;
			pt[2].w=1.;

			ept=pt+3;
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
		pt[0].x=cen.x+a->x*r;
		pt[0].y=cen.y+a->y*r;
		pt[0].z=cen.z+a->z*r;
		pt[0].w=1;

		pt[1].x=cen.x+b->x*r;
		pt[1].y=cen.y+b->y*r;
		pt[1].z=cen.z+b->z*r;
		pt[1].w=1;

		pt[2].x=cen.x+c->x*r;
		pt[2].y=cen.y+c->y*r;
		pt[2].z=cen.z+c->z*r;
		pt[2].w=1;

		ept=&pt[3];
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
Vert *getvert(Point3 p){
	Vert *v;
	for(v=vert;v;v=v->next) if(dist2(p, v->world)<EPS*EPS) return v;
	v=emalloc(sizeof(Vert));
	v->world=p;
	v->next=vert;
	vert=v;
	return v;
}
void mkedge(Point3 pc, Point3 qc){
	Edge **e, *v;
	Vert *p, *q;
	p=getvert(pc);
	q=getvert(qc);
	for(e=&edge;*e;e=&((*e)->next)){
		if((*e)->p==p && (*e)->q==q
		|| (*e)->p==q && (*e)->q==p){
			if((*e)->npoly==npoly){
				v=*e;
				*e=(*e)->next;
				free(v);
			}
			return;
		}
	}
	v=emalloc(sizeof(Edge));
	v->p=p;
	v->q=q;
	v->next=edge;
	v->npoly=npoly;
	edge=v;
}
void polyren(int c, int other){
	Point3 *v, *w;
	if(ept<pt+3) return;
	++npoly;
	for(v=pt;v!=ept;v++) *v=xformpointd(*v, 0, mx);
	w=ept-1;
	for(v=pt;v!=ept;v++){
		mkedge(*v, *w);
		w=v;
	}
}
void normalize(Point3 *p){
	double l;
	l=sqrt(p->x*p->x+p->y*p->y+p->z*p->z);
	if(l>0){
		p->x/=l;
		p->y/=l;
		p->z/=l;
	}
}
double dist2(Point3 p, Point3 q){
	p.x-=q.x;
	p.y-=q.y;
	p.z-=q.z;
	return p.x*p.x+p.y*p.y+p.z*p.z;
}
Point3 cen;
double size;
void standardize(void){
	Vert *v;
	Point3 lo, hi, wid;
	if(vert==0) return;
	lo=hi=vert->world;
	for(v=vert;v;v=v->next){
		if(v->world.x<lo.x) lo.x=v->world.x;
		if(v->world.y<lo.y) lo.y=v->world.y;
		if(v->world.z<lo.z) lo.z=v->world.z;
		if(v->world.x>hi.x) hi.x=v->world.x;
		if(v->world.y>hi.y) hi.y=v->world.y;
		if(v->world.z>hi.z) hi.z=v->world.z;
	}
	cen.x=.5*(lo.x+hi.x);
	cen.y=.5*(lo.y+hi.y);
	cen.z=.5*(lo.z+hi.z);
	wid.x=hi.x-cen.x;
	wid.y=hi.y-cen.y;
	wid.z=hi.z-cen.z;
	size=wid.x;
	if(wid.y>size) size=wid.y;
	if(wid.z>size) size=wid.z;
}
Quaternion q={1,0,0,0};	/* total rotation */
#define	BORDER	4
Rectangle image;
Bitmap *stage;
myborder(Bitmap *l, Rectangle r, int i, Fcode c){
	if(i<0) border(l, inset(r, i), -i, c);
	else border(l, r, i, c);
}
rectf(Bitmap *b, Rectangle r, Fcode f){
	bitblt(b, r.min, b, r, f);
}
void redraw(void){
	Vert *v;
	Edge *e;
	Space *view;
	view=pushmat(0);
	viewport(view, image, 1.);
	persp(view, 30., 8., 12.);
	look(view, (Point3){0., 0., -10., 1.}, (Point3){0., 0., 1., 1.},
		(Point3){0., 1., 1., 1.});
	qrot(view, q);
	scale(view, 1./size, 1./size, 1./size);
	move(view, -cen.x, -cen.y, -cen.z);
	for(v=vert;v;v=v->next) v->screen=xformpointd(v->world, 0, view);
	rectf(stage, stage->r, 0);
	for(e=edge;e;e=e->next)
		segment(stage, Pt(e->p->screen.x, e->p->screen.y),
			       Pt(e->q->screen.x, e->q->screen.y), 3, S);
	bitblt(&screen, image.min, stage, stage->r, S);
	bflush();
	popmat(view);
}
void ereshaped(Rectangle r){
	Point size;
	screen.r=r;
	image=inset(r, 3*BORDER);
	size=sub(image.max, image.min);
	if(size.x>size.y){
		image.min.x+=(size.x-size.y)/2;
		image.max.x=image.min.x+size.y;
	}
	else{
		image.min.y+=(size.y-size.x)/2;
		image.max.y=image.min.y+size.x;
	}
	if(stage) bfree(stage);
	stage=balloc(image, 1);
	rectf(&screen, inset(r, BORDER), 0);
	myborder(&screen, image, -BORDER, F);
	redraw();
	bflush();
}
/*
 * Here follows a more-or-less generic outer loop for a mouse-driven program.
 * To customize, change the button labels, fill the switch cases in differently,
 * and write an ereshaped routine.
 */
char *button2[]={
	"reread",
	0
};
Menu menu2={button2};
char *button3[]={
	"output",
	"view",
	"exit",
	0
};
Menu menu3={button3};
Cursor confirmcursor={
	0, 0,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

	0x00, 0x0E, 0x07, 0x1F, 0x03, 0x17, 0x73, 0x6F,
	0xFB, 0xCE, 0xDB, 0x8C, 0xDB, 0xC0, 0xFB, 0x6C,
	0x77, 0xFC, 0x00, 0x00, 0x00, 0x01, 0x00, 0x03,
	0x94, 0xA6, 0x63, 0x3C, 0x63, 0x18, 0x94, 0x90,
};
confirm(int b){
	Mouse down, up;
	cursorswitch(&confirmcursor);
	do down=emouse(); while(!down.buttons);
	do up=emouse(); while(up.buttons);
	cursorswitch(0);
	return down.buttons==(1<<(b-1));
}
void output(void){
	Vert *v;
	Edge *e;
	Space *view;
	view=pushmat(0);
	viewport(view, Rect(5,5,0,0), 1.);
	persp(view, 30., 8., 12.);
	look(view, (Point3){0., 0., -10., 1.}, (Point3){0., 0., 1., 1.},
		(Point3){0., 1., 1., 1.});
	qrot(view, q);
	for(v=vert;v;v=v->next) v->screen=xformpointd(v->world, 0, view);
	print(".PS\n");
	for(e=edge;e;e=e->next)
		print("line from (%g,%g) to (%g,%g)\n", 5.-e->p->screen.x, e->p->screen.y,
			5.-e->q->screen.x, e->q->screen.y);
	print(".PE\n");
	popmat(view);
}
void printview(void){
	print("v 30 8 12 0 0 -10 0 0 1 0 1 1\n");
	print("Q %g %g %g %g\n", q.r, q.i, q.j, q.k);
	print("S %g %g %g\n", 1./size, 1./size, 1./size);
	print("T %g %g %g\n", -cen.x, -cen.y, -cen.z);
}
void view(void){
	Mouse m;
	ereshaped(screen.r);
	for(;;){
		while(ecanmouse())
			m=emouse();
		switch(m.buttons){
		case 1:
			if(ptinrect(m.xy, image)){
				qball(image, &m, &q, redraw, 0);
				break;
			}
			do
				m=emouse();
			while(m.buttons);
			break;
		case 2:
			if(menuhit(2, &m, &menu2)==0) return;
			break;
		case 4:
			switch(menuhit(3, &m, &menu3)){
			case 0:
				output();
				break;
			case 1:
				printview();
				break;
			case 2:
				if(confirm(3)) exits("");
				break;
			}
		}
	}
}
