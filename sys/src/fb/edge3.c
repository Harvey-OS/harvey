/*
 * edge3 -- gradient phase filter
 */
#include "filter.cc"
int phase(int x, int y){
	int t, p=0;
	if(y<0){
		p+=128;
		y=-y;
		x=-x;
	}
	if(x<0){
		p+=64;
		t=x;
		x=y;
		y=-x;
	}
	if(x<y) p+=32*x/y;
	else if(x!=0) p+=64-32*y/x;
	return p;
}
void filter(unsigned char *l0, unsigned char *l1, unsigned char *l2, int nchan, int npix){
	unsigned char *op=outline;
	int v;
	npix*=nchan;
	if(npix==0) return;
	do{
		v=phase((l0[-nchan]+2*l0[0]+l0[nchan])
			-(l2[-nchan]+2*l2[0]+l2[nchan]),
			 (l0[-nchan]+2*l1[-nchan]+l2[-nchan])
			-(l0[ nchan]+2*l1[ nchan]+l2[ nchan]));
		*op++=v<0?0:v<256?v:255;
		l0++;
		l1++;
		l2++;
	}while(--npix!=0);
}
