/*
 * gradient shading
 */
#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
unsigned char inline[3][8192*8];
unsigned char outline[8192*8];
void input(PICFILE *in, unsigned char *l, unsigned char *m){
	int i;
	if(!picread(in, (char *)l)){
		memcpy((char *)l-in->nchan, (char *)m-in->nchan,
			PIC_NCHAN(in)*(PIC_WIDTH(in)+2));
		return;
	}
	m=l+PIC_NCHAN(in)*PIC_WIDTH(in);
	for(i=0;i!=PIC_NCHAN(in);i++,l++,m++){
		l[-PIC_NCHAN(in)]=l[0];
		m[0]=m[-PIC_NCHAN(in)];
	}
}
double l[3]={1, -1, 1};
void filter(unsigned char *, unsigned char *, unsigned char *, int, int);
main(int argc, char *argv[]){
	int y;
	unsigned char *l0, *l1, *l2, *t;
	PICFILE *in, *out;
	double len;
	argc=getflags(argc, argv, "l:3[x y z]");
	if(argc!=1 && argc!=2) usage("[picture]");
	if(flag['l']){
		l[0]=atof(flag['l'][0]);
		l[1]=atof(flag['l'][1]);
		l[2]=atof(flag['l'][2]);
	}
	len=sqrt(l[0]*l[0]+l[1]*l[1]+l[2]*l[2])/255.;
	if(len==0){
		fprint(2, "%s: null light vector\n", argv[0]);
		exits("bad light");
	}
	l[0]/=len;
	l[1]/=len;
	l[2]/=len;
	in=picopen_r(argc==2?argv[1]:"IN");
	if(in==0){
		perror(argc==2?argv[1]:"IN");
		exits("open input");
	}
	out=picopen_w("OUT", PIC_SAMEARGS(in));
	if(out==0){
		perror("OUT");
		exits("create output");
	}
	l0=inline[0]+in->nchan;
	l1=inline[1]+in->nchan;
	l2=inline[2]+in->nchan;
	input(in, l0, l0);
	memcpy((char *)l1, (char *)l0, PIC_NCHAN(in)*(PIC_WIDTH(in)+2));
	for(y=0;y!=PIC_HEIGHT(in);y++){
		input(in, l2, l1);
		filter(l0, l1, l2, PIC_NCHAN(in), PIC_WIDTH(in));
		picwrite(out, (char *)outline);
		t=l0;
		l0=l1;
		l1=l2;
		l2=t;
	}
}
void filter(unsigned char *l0, unsigned char *l1, unsigned char *l2, int nchan, int npix){
	unsigned char *op=outline;
	int v;
	double nx, ny;
	npix*=nchan;
	if(npix==0) return;
	do{
		nx=.5*(l1[nchan]-l1[-nchan]);
		ny=.5*(l2[0]-l0[0]);
		v=(nx*l[0]+ny*l[1]+l[2])/sqrt(nx*nx+ny*ny+1.);
		l0++;
		l1++;
		l2++;
		*op++=v<0?0:v<256?v:255;
	}while(--npix!=0);
}
