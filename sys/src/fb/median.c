/*% cyntax % -lfb && cc -go # % -lfb
 * 3x3 median filter
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
		memcpy((char *)l-PIC_NCHAN(in), (char *)m-PIC_NCHAN(in),
			PIC_NCHAN(in)*(PIC_WIDTH(in)+2));
		return;
	}
	m=l+PIC_NCHAN(in)*PIC_WIDTH(in);
	for(i=0;i!=PIC_NCHAN(in);i++,l++,m++){
		l[-PIC_NCHAN(in)]=l[0];
		m[0]=m[-PIC_NCHAN(in)];
	}
}
void medians(unsigned char *l0, unsigned char *l1, unsigned char *l2, int nchan, int npix){
	unsigned char xx[9];
	int t, j, k;
	unsigned char *op=outline;
	npix*=nchan;
	if(npix==0) return;
	do{
		xx[0]=l0[-nchan]; xx[1]=l0[0]; xx[2]=l0[nchan];
		xx[3]=l1[-nchan]; xx[4]=l1[0]; xx[5]=l1[nchan];
		xx[6]=l2[-nchan]; xx[7]=l2[0]; xx[8]=l2[nchan];
		for(j=0;j!=5;j++) for(k=j;k!=9;k++)
			if(xx[j]<xx[k]){ t=xx[j]; xx[j]=xx[k]; xx[k]=t; }
		*op++=xx[4];
		l0++;
		l1++;
		l2++;
	}while(--npix!=0);
}
main(int argc, char *argv[]){
	int y;
	unsigned char *l0, *l1, *l2, *t;
	PICFILE *in, *out;
	argc=getflags(argc, argv, "");
	if(argc!=1 && argc!=2) usage("[picture]");
	in=picopen_r(argc==2?argv[1]:"IN");
	if(in==0){
		picerror(argc==2?argv[1]:"IN");
		exits("open input");
	}
	out=picopen_w("OUT", PIC_SAMEARGS(in));
	if(out==0){
		picerror(argv[0]);
		exits("create output");
	}
	l0=inline[0]+PIC_NCHAN(in);
	l1=inline[1]+PIC_NCHAN(in);
	l2=inline[2]+PIC_NCHAN(in);
	input(in, l0, l0);
	memcpy((char *)l1, (char *)l0, PIC_NCHAN(in)*(PIC_WIDTH(in)+2));
	for(y=0;y!=PIC_HEIGHT(in);y++){
		input(in, l2, l1);
		medians(l0, l1, l2, PIC_NCHAN(in), PIC_WIDTH(in));
		picwrite(out, (char *)outline);
		t=l0;
		l0=l1;
		l1=l2;
		l2=t;
	}
	exits("");
}
