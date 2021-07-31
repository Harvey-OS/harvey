/*
 * rotate angle [picfile]
 *	Rotate an image by angle (degrees) about its center.
 * Bug:
 *	no antialiasing
 */
#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
#define	SCALE	65536
int c, s;			/* cos and sin of rotation angle, scaled by SCALE */
int X0=0, Y0=0, X1=0, Y1=0;	/* window of output image */
void bound(int x, int y){
	int tx=(c*x+s*y)/SCALE, ty=(-s*x+c*y)/SCALE;
	if(tx<X0) X0=tx; else if(tx>=X1) X1=tx+1;
	if(ty<Y0) Y0=ty; else if(ty>=Y1) Y1=ty+1;
}
main(int argc, char *argv[]){
	PICFILE *in, *out;
	char *fb, *line, *sp, *dp, *edp;
	int y, x, ny, nx, nchan, Y, X;
	char *inname;
	switch(getflags(argc, argv, "")){
	case 2:	inname="IN"; break;
	case 3: inname=argv[2]; break;
	default: usage("angle [in]");
	}
	in=picopen_r(inname);
	if(in==0){
		picerror(inname);
		exits("open input");
	}
	c=cos(atof(argv[1])*PI/180.)*SCALE;
	s=sin(atof(argv[1])*PI/180.)*SCALE;
	nx=PIC_WIDTH(in);
	ny=PIC_HEIGHT(in);
	nchan=PIC_NCHAN(in);
	bound(0, 0);
	bound(nx, 0);
	bound(0, ny-1);
	bound(nx, ny-1);
	out=picopen_w("OUT", in->type,
		PIC_XOFFS(in), PIC_YOFFS(in), X1-X0, Y1-Y0,
		in->chan,
		argv,
		in->cmap);
	if(out==0){
		picerror(argv[0]);
		exits("create output");
	}
	fb=malloc(nx*ny*nchan);
	line=malloc((X1-X0)*nchan);
	if(fb==0 || line==0){
		fprint(2, "%s: no space\n", argv[0]);
		exits("no space");
	}
	for(y=0;y!=ny;y++)
		picread(in, fb+y*nx*nchan);
	for(Y=Y0;Y!=Y1;Y++){
		dp=line;
		for(X=X0;X!=X1;X++){
			x=(c*X-s*Y)/SCALE;
			y=(s*X+c*Y)/SCALE;
			edp=dp+nchan;
			if(0<=x && x<nx && 0<=y && y<ny){
				sp=fb+(y*nx+x)*nchan;
				while(dp!=edp) *dp++ = *sp++;
			}
			else
				while(dp!=edp) *dp++ = 0;
		}
		picwrite(out, line);
	}
	exits("");
}
