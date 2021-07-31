/*
 * hysteresis thresholding
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
int low, high;
filter(unsigned char *, unsigned char *, unsigned char *, int, int);
main(int argc, char *argv[]){
	int y;
	unsigned char *l0, *l1, *l2, *t;
	PICFILE *in, *out;
	argc=getflags(argc, argv, "");
	if(argc!=3 && argc!=4) usage("lowthresh highthresh [picture]");
	low=atoi(argv[1]);
	high=atoi(argv[2]);
	in=picopen_r(argc==4?argv[3]:"IN");
	if(in==0){
		picerror(argc==4?argv[3]:"IN");
		exits("open input");
	}
	out=picopen_w("OUT", PIC_SAMEARGS(in));
	if(out==0){
		picerror("OUT");
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
filter(unsigned char *l0, unsigned char *l1, unsigned char *l2, int nchan, int npix){
	unsigned char *op=outline;
	int v;
	npix*=nchan;
	if(npix==0) return;
	do{
		if(l1[0]<low
		|| l1[0]<high
		&& (l0[-nchan]<low || l0[0]<low || l0[nchan]<low
		 || l1[-nchan]<low              || l1[nchan]<low
		 || l2[-nchan]<low || l2[0]<low || l2[nchan]<low))
			*op++=0;
		else
			*op++=255;
		l0++;
		l1++;
		l2++;
	}while(--npix!=0);
}
