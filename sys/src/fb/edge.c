/*
 * edge -- linear edge detection filter with kernel
 * -1 -1 -1
 * -1  8 -1
 * -1 -1 -1
 */
#include "filter.cc"
void filter(unsigned char *l0, unsigned char *l1, unsigned char *l2, int nchan, int npix){
	unsigned char *op=outline;
	int v;
	npix*=nchan;
	if(npix==0) return;
	do{
		v=-l0[-nchan]  -l0[0]-l0[nchan]
		  -l1[-nchan]+8*l1[0]-l1[nchan]
		  -l2[-nchan]  -l2[0]-l2[nchan];
		l0++;
		l1++;
		l2++;
		*op++=v<0?0:v<128?2*v:255;
	}while(--npix!=0);
}
